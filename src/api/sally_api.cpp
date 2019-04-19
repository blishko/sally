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
  std::unique_ptr<engine> engine;
  std::unique_ptr<boost::program_options::variables_map> boost_options;
};

namespace{
std::vector<std::string> map_to_cmdline(std::map<std::string, std::string> const & opts) {
  std::vector<std::string> res;
  // add auxiliary first argument -> name of the script
  res.push_back("phony");
  for (const auto & entry : opts) {
    res.push_back("--" + entry.first);
    res.push_back(entry.second);
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
      ctx->engine = std::unique_ptr<engine>(engine_factory::mk_engine(ctx->opts->get_string("engine"), *ctx->context));
  }
  return ctx;
}

void delete_context(sally_context ctx) {
  delete ctx;
}

void run_on_file(std::string file, sally_context ctx) {
  assert(ctx->context);
  assert(ctx->engine);

  auto & context = ctx->context;
  auto & engine = ctx->engine;
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
  try {
    auto &context = ctx->context;
    auto &engine = ctx->engine;
    // Create the parser
    parser::input_language language = parser::INPUT_MCMT;
    parser::parser p(*context, language, content);

    // Parse an process each command
    for (cmd::command *cmd = p.parse_command(); cmd != 0; delete cmd, cmd = p.parse_command()) {
      // Run the command
      cmd->run(context.get(), engine.get());
    }
  }catch (sally::exception &ex){
    throw std::logic_error("Sally exception: " + ex.get_message());
  }
}

std::string term_to_string(sally_context ctx, sally::expr::term_ref const & term_ref) {
  auto &context = ctx->context;
  auto& term = context->tm().term_of(term_ref);
  std::stringstream ss;
  ss << expr::set_tm(context->tm());
  ss << term;
  return ss.str();
}

void set_new_reachability_lemma_eh(sally_context ctx, sally_new_lemma_eh lemma_eh) {
  auto* engine = dynamic_cast<pdkind::pdkind_engine*>(ctx->engine.get());
  if (!engine) { std::cerr << "Error setting hook!" << std::endl; return; }
  engine->set_new_reachability_lemma_eh(ctx, lemma_eh);
}

void add_next_frame_eh(sally_context ctx, sally_general_eh eh) {
  auto* engine = dynamic_cast<pdkind::pdkind_engine*>(ctx->engine.get());
  if (!engine) { std::cerr << "Error setting hook!" << std::endl; return; }
  engine->add_next_frame_eh(ctx, eh);
}

}

