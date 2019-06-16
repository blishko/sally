//
// Created by Martin Blicha on 2019-01-17.
//

#include "sally_api.h"
#include <utils/statistics.h>
#include <expr/term_manager.h>
#include <system/context.h>
#include <engine/factory.h>
#include <smt/factory.h>
#include <parser/parser.h>
#include <parser/antlr_parser.h>
#include "parse_options.h"
#include <engine/pdkind/pdkind_engine.h>
#include <iostream>

#include <memory>

#include <boost/program_options/variables_map.hpp>

namespace sally {

struct api_context {
  std::unique_ptr<utils::statistics> stats;
  std::unique_ptr<options> opts;
  std::unique_ptr<expr::term_manager> term_manager;
  std::unique_ptr<system::context> context;
  std::unique_ptr<engine> d_engine;
  std::unique_ptr<boost::program_options::variables_map> boost_options;
};

namespace{
std::vector<std::string> map_to_cmdline(std::map<std::string, std::string> const & opts) {
  std::vector<std::string> res;
  // add auxiliary first argument -> name of the script
  res.push_back("phony");
  for (const auto & entry : opts) {
    if (entry.second == "false") { continue; } // flag set to false, do not put it in the command line
    res.push_back("--" + entry.first);
    if (entry.second == " true") { } // flag set to true, only the name of the flag is added to command line
    else { res.push_back(entry.second); }
  }
  // add auxiliary last argument -> file to run
  res.push_back("phony2");
  return res;
}
}

sally_context create_context(std::map<std::string, std::string> const & option_map ){
  sally_context ctx = new api_context();
  ctx->boost_options = std::unique_ptr<boost::program_options::variables_map>(new boost::program_options::variables_map());
  // create options
  std::vector<std::string> parsed_cmdline = map_to_cmdline(option_map);
  std::vector<char*> parsed_cmdline_old;
  for (auto const & s : parsed_cmdline) {
    parsed_cmdline_old.push_back(const_cast<char*>(s.c_str()));
  }
  parse_options(parsed_cmdline.size(), &parsed_cmdline_old[0], *ctx->boost_options);
  ctx->stats = std::unique_ptr<utils::statistics>(new utils::statistics());
  ctx->term_manager = std::unique_ptr<expr::term_manager>(new expr::term_manager(*ctx->stats));
  ctx->opts = std::unique_ptr<options>( new options(*ctx->boost_options));
  ctx->context = std::unique_ptr<system::context>( new system::context(*ctx->term_manager, *ctx->opts, *ctx->stats));
  assert(ctx->opts->has_option("engine"));
  assert(ctx->opts->has_option("solver"));
  smt::factory::set_default_solver(ctx->opts->get_string("solver"));
  if (ctx->opts->has_option("engine")) {
      ctx->d_engine = std::unique_ptr<engine>(engine_factory::mk_engine(ctx->opts->get_string("engine"), *ctx->context));
  }
  return ctx;
}

void delete_context(sally_context ctx) {
  delete ctx;
}

namespace {
  void run_on_string(std::string const& content, parser::input_language language, sally_context ctx) {
    try {
      assert(ctx->context);
      assert(ctx->d_engine);
      auto &context = ctx->context;
      auto &engine = ctx->d_engine;
      // Create the parser
      parser::parser p(*context, language, content);

      // Parse an process each command
      for (cmd::command *cmd = p.parse_command(); cmd != 0; delete cmd, cmd = p.parse_command()) {
        // Run the command
        cmd->run(context.get(), engine.get());
      }
    } catch (sally::exception &ex){
      throw std::logic_error("Sally exception: " + ex.get_message());
    } catch (sally::parser::parser_exception & ex) {
      throw std::logic_error("Sally parser exception: " + ex.get_message());
    }
  }
}

void run_on_file(std::string file, sally_context ctx) {
  assert(ctx->context);
  assert(ctx->d_engine);
  auto & context = ctx->context;
  auto & engine = ctx->d_engine;
  // Create the parser
  parser::input_language language = parser::parser::guess_language(file);
  parser::parser p(*context, language, file.c_str());

  // Parse an process each command
  for (cmd::command* cmd = p.parse_command(); cmd != 0; delete cmd, cmd = p.parse_command()) {
    // Run the command
    cmd->run(context.get(), engine.get());
  }
}

void run_on_mcmt_string(std::string const & content, sally_context ctx) {
  run_on_string(content, parser::INPUT_MCMT, ctx);
}

void run_on_chc_string(std::string const & content, sally_context ctx) {
  run_on_string(content, parser::INPUT_CHC, ctx);
}

std::string term_to_string(sally_context ctx, sally::expr::term_ref const & term_ref) {
//  auto &context = ctx->context;
//  auto& term = context->tm().term_of(term_ref);
//  std::stringstream ss;
//  ss << expr::set_tm(context->tm());
//  ss << term;
//  return ss.str();
  return ctx->term_manager->to_string(term_ref);
}

void add_lemma(sally_context ctx, std::string const &lemma_str) {
  auto &context = ctx->context;
  parser::parser p(*context, parser::INPUT_MCMT, lemma_str);
  cmd::command* c = p.parse_command();
  c->run(&(*ctx->context), &(*ctx->d_engine));
}

void set_new_reachability_lemma_eh(sally_context ctx, sally_new_lemma_eh lemma_eh) {
  auto* engine = dynamic_cast<pdkind::pdkind_engine*>(ctx->d_engine.get());
  if (!engine) { std::cerr << "Error setting hook!" << std::endl; return; }
  engine->set_new_reachability_lemma_eh(ctx, lemma_eh);
}

void add_next_frame_eh(sally_context ctx, sally_general_eh eh, void* state) {
  auto* engine = dynamic_cast<pdkind::pdkind_engine*>(ctx->d_engine.get());
  if (!engine) { std::cerr << "Error setting hook!" << std::endl; return; }
  engine->add_next_frame_eh(state, eh);
}

std::string reachability_lemma_to_command(sally_context ctx, size_t level, const sally::expr::term_ref &lemma_ref) {
  auto tm = ctx->term_manager.get();
  auto state_type = ctx->d_engine->get_current_transition_system()->get_state_type();
  state_type->use_namespace();
  state_type->use_namespace(system::state_type::STATE_CURRENT);
  std::string lemma_str = ctx->term_manager->to_string(lemma_ref);
  tm->pop_namespace();
  tm->pop_namespace();
  assert(ctx->d_engine->get_current_transition_system());
  std::string system_id = ctx->d_engine->get_current_transition_system()->get_id();
  std::string command = "( lemma " + system_id + ' ' + std::to_string(level) + ' ' + lemma_str + " )";
  return command;
}

std::string induction_lemma_to_command(sally_context ctx, size_t level,
  const sally::expr::term_ref &lemma_ref, const sally::expr::term_ref &cex_ref, size_t cex_depth) {

  auto tm = ctx->term_manager.get();
  auto state_type = ctx->d_engine->get_current_transition_system()->get_state_type();
  state_type->use_namespace();
  state_type->use_namespace(system::state_type::STATE_CURRENT);
  std::string lemma_str = ctx->term_manager->to_string(lemma_ref);
  std::string cex_str = ctx->term_manager->to_string(cex_ref);
  tm->pop_namespace();
  tm->pop_namespace();
  assert(ctx->d_engine->get_current_transition_system());
  std::string system_id = ctx->d_engine->get_current_transition_system()->get_id();
  std::string command = "( ilemma " + system_id + ' ' + std::to_string(level) + ' ' + lemma_str
    + ' ' + cex_str + ' ' + std::to_string(cex_depth) + " )";
  return command;
}

void set_obligation_pushed_eh(sally_context ctx, sally_obligation_pushed_eh eh) {
  auto* engine = dynamic_cast<pdkind::pdkind_engine*>(ctx->d_engine.get());
  if (!engine) { std::cerr << "Error setting hook!" << std::endl; return; }
  engine->set_obligation_pushed_eh(ctx, eh);
}

std::vector<std::pair<std::string, std::string>> stats::get_stats() const {

  std::vector<std::pair<std::string, std::string>> ret;
  std::stringstream ss_keys;
  std::stringstream ss_vals;
  this->ctx->context->get_statistics().headers_to_stream(ss_keys);
  this->ctx->context->get_statistics().values_to_stream(ss_vals);
  std::string key;
  std::string val;
  char del = '\t';
  while (std::getline(ss_keys, key, del)) {
    std::getline(ss_vals, val, del);
    ret.emplace_back(key, val);
  }
  return ret;
}
}

