/**
 * This file is part of sally.
 * Copyright (C) 2015 SRI International.
 *
 * Sally is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Sally is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with sally.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "chc_system.h"

#include "expr/term_manager.h"

#include "command/define_transition_system.h"
#include "command/declare_state_type.h"
#include "command/query.h"
#include "command/sequence.h"

#include <iostream>
#include <algorithm>

using namespace sally::expr;

namespace sally {
namespace parser {

void chc_system::add_rule(term_ref head, term_ref tail) {

  // All the head variables should be distinct
  auto additional_equalities = ensure_distinct(head);

  // Normalize head
  auto subst = normalize_head(head);
  // Normalize tail
  tail = ctx.tm().mk_and(tail, ctx.tm().mk_and(additional_equalities));
  normalize_tail(tail, subst);

  d_rules[head].push_back(tail);
}

cmd::command* chc_system::to_commands() {
  if(is_transition_system()){
    return to_transition_system();
  }
  return 0;
}

bool chc_system::is_transition_system() const {
  size_t predicates = get_number_of_predicates();
  assert(predicates > 0);
  if (predicates > 1) { return false; }
  auto & tm = this->ctx.tm();
  bool single_init = false;
  bool single_update = false;
  bool single_query = false;
  for (auto it = d_rules.begin(); it != d_rules.end(); ++it) {
    const term_ref head = it->first;
    if (head == tm.mk_boolean_constant(false)) {
      // query
      single_query = it->second.size() == 1;
    }
    else { // the unique uninterpreted predicate
      auto predicate = get_predicate(head);
      auto& bodies = it->second;
      // There has to be exactly 2 rules: The init and the transition relation
      if (bodies.size() != 2) { return false;}
      for ( auto& body : bodies ) {
        term_vec subterms;
        tm.get_subterms(body, subterms);
        auto it = std::find_if(subterms.begin(), subterms.end(),
                               [=](term_ref sub) { return sub == predicate; });
        if (it == subterms.end()) {
          single_init = true;
        }
        else {
          single_update = true;
        }
      }
    }
  }
  return single_init && single_query && single_update;
}

size_t chc_system::get_number_of_predicates() const {
  // TODO: write properly; this is a quick workaround
  return d_rules.size() - 1;
}

expr::term_ref chc_system::get_fresh_variable_of_type(expr::term_ref type) {
  auto& tm = ctx.tm();
  std::string fresh_name = tm.get_fresh_variable_name();
  return tm.mk_variable(fresh_name, type);
}

substituition chc_system::normalize_head(term_ref &head) {
  substituition sub;
  auto& tm = ctx.tm();
  auto predicate = get_predicate(head);
  auto vars = get_arguments(head);

  auto it = d_normalized.find(predicate);
  if ( it == d_normalized.end()) {
    term_vec fresh_vars;
    for (size_t i = 0; i < vars.size(); ++i) {
      term_ref type = tm.term_of(vars[i])[0];
      fresh_vars.push_back(get_fresh_variable_of_type(type));
    }
    auto res = d_normalized.insert(std::make_pair(predicate, fresh_vars));
    it = res.first;
  }

  // create substituition
  assert(vars.size() == it->second.size());
  for (size_t i = 0; i < vars.size(); ++i) {
    sub.add(vars[i], it->second[i]);
//    std::cout << vars[i] << ' ' << it->second[i] << std::endl;
  }
//  std::cout << "Before: " << head << '\n';
  head = ctx.tm().substitute(head, sub.mapping);
//  std::cout << "After: " << head << std::endl;
  return sub;
}

void chc_system::normalize_tail(term_ref &tail, const substituition &sub) {
  tail = ctx.tm().substitute(tail, sub.mapping);
}

expr::term_ref chc_system::get_predicate(expr::term_ref head) const {
  std::vector<term_ref> args;
  return *this->ctx.tm().term_of(head).begin();

}

chc_system::term_vec chc_system::get_arguments(expr::term_ref head) const {
  std::vector<term_ref> args;
  const term& term = this->ctx.tm().term_of(head);
  for(size_t i = 1; i < term.size(); ++i) {
    args.push_back(term.child(i));
  }
  return args;
}

chc_system::term_vec chc_system::ensure_distinct(expr::term_ref& head) {
  chc_system::term_vec additional_equalities;
  substituition sub;
  std::set<term_ref> seen;
  auto& tm = ctx.tm();
  auto args = get_arguments(head);
  auto new_args = args;
  for (size_t i = 0; i < args.size(); ++i) {
    term_ref var = args[i];
    auto it = seen.find(var);
    if (it == seen.end()) {
      seen.insert(var);
    }
    else {
      auto new_var = get_fresh_variable_of_type(tm.term_of(var)[0]);
      new_args[i] = new_var;
      additional_equalities.push_back(tm.mk_term(TERM_EQ, var, new_var));
    }
  }
  if (args != new_args) {
    head = tm.mk_function_application(tm.term_of(head)[0], new_args);
  }
  return additional_equalities;
}

cmd::command *chc_system::to_transition_system() {
  std::pair<term_ref, term_ref> init_rule;
  std::pair<term_ref, term_ref> transition_rule;
  std::pair<term_ref, term_ref> query_rule;
  term_vec vars;
  auto& tm = ctx.tm();
  for (auto it = this->d_rules.begin(); it != d_rules.end(); ++it) {
    if (it->first == tm.mk_boolean_constant(false)) {
      // query
      query_rule = std::make_pair(it->first, it->second[0]);
    }
    else {
      // the invariant predicate
      auto predicate = get_predicate(it->first);
      auto& bodies = it->second;
      assert(bodies.size() == 2);
      for (auto body : bodies) {
        term_vec subterms;
        tm.get_subterms(body, subterms);
        auto body_predicate = std::find_if(subterms.begin(), subterms.end(),
          [predicate](term_ref sub) { return sub == predicate; });
        bool is_init = body_predicate == subterms.end();
        if (is_init) {
          vars = get_arguments(it->first);
          init_rule = std::make_pair(it->first, body);
//          std::cout << "Init rule:\n";
//          std::cout << it->first << '\n';
//          std::cout << body << std::endl;
        }
        else {
          transition_rule = std::make_pair(it->first, body);
//          std::cout << "Transition rule:\n";
//          std::cout << it->first << '\n';
//          std::cout << body << std::endl;
        }
      }
    }
  }

  cmd::sequence* cmd_seq = new cmd::sequence();

  /*
  The steps for the translation:
  1. Define state variables: Variables of the predicate, plus additional variables in the body of transition fla
  2. Renaming:
    a) Init formula to state variables
    b) Input vars of transition to state variables
    c) Output vars of transition to next-state vars
    d) Additional variables of transition to next-state vars
   */

  // setup
  term_ref transition_fla = transition_rule.second;
  auto predicate = get_predicate(transition_rule.first);

  // Identify the various classes of variables
  // "vars" already contains the variables from predicate of Init and from output predicate of Trans
  // get the input variables of transition
  auto p_term = extract_predicate_from_tail(transition_fla, predicate);
  {
    auto body_equalities = ensure_distinct(p_term);
    transition_fla = tm.mk_and(transition_fla, tm.mk_and(body_equalities));
  }
  auto trans_input_vars = get_arguments(p_term);
  // now the transition formula does not contain the predicate anymore
  // get the additional variables
  std::set<term_ref> additional_vars;
  {

    tm.get_variables(transition_fla, additional_vars);
    std::for_each(trans_input_vars.begin(), trans_input_vars.end(),
      [&](term_ref var){ additional_vars.erase(var); });
    std::for_each(vars.begin(), vars.end(),
                  [&](term_ref var){ additional_vars.erase(var); });
  }

  // make state type
  std::vector<std::string> state_vars;
  term_vec state_types;
  // first the vars of the predicate
  for (auto var : vars) {
    state_vars.push_back(tm.get_variable_name(var));
    state_types.push_back(tm.term_of(var)[0]);
  }
  // then the additional vars
  for (auto var : additional_vars) {
    state_vars.push_back(tm.get_variable_name(var));
    state_types.push_back(tm.term_of(var)[0]);
  }
  expr::term_ref state_type = tm.mk_struct_type(state_vars, state_types);
  expr::term_ref input_type = tm.mk_struct_type({}, {});
  std::string state_type_id = "CHC";
  system::state_type* st = new system::state_type(state_type_id, tm, state_type, input_type);
  cmd::declare_state_type* declare_state_type = new cmd::declare_state_type(state_type_id, st);
  cmd_seq->push_back(declare_state_type);

  // make initial states
  term_ref init_fla = init_rule.second;
  auto & state_current = st->get_variables(system::state_type::STATE_CURRENT);
  substituition sub;
  assert(vars.size() + additional_vars.size() == state_current.size());
  for (size_t i = 0; i < vars.size(); ++i ) {
    sub.add(vars[i], state_current[i]);
  }
  init_fla = tm.substitute(init_fla, sub.mapping);
  assert(st->is_state_formula(init_fla));
//  std::cout << "Init formula: " << init_fla << std::endl;
  system::state_formula* init_states = new system::state_formula(tm, st, init_fla);


  // make the transition relation
  auto & state_next = st->get_variables(system::state_type::STATE_NEXT);
  sub.clear();
  assert(vars.size() + additional_vars.size() == state_next.size());
  for (size_t i = 0; i < vars.size(); ++i ) {
    sub.add(vars[i], state_next[i]);
  }
  {
    int i = vars.size();
    auto it = additional_vars.begin();
    for (; it != additional_vars.end(); ++it, ++i) {
      sub.add(*it, state_next[i]);
    }
  }
  transition_fla = tm.substitute(transition_fla, sub.mapping);
//  std::cout << "After first substituition" << transition_fla << std::endl;
  // remove the predicate, and substitute its variable
  // if any input variable is already an output variable, simply add the equality same(var) (that is var == var')
  // TODO: check that all variables are different
  std::vector<term_ref> additional_equalities;
  sub.clear();
  assert(trans_input_vars.size() + additional_vars.size() == state_current.size());
  for (size_t i = 0; i < trans_input_vars.size(); ++i ) {
    // check that input var is not also output var -> otherwise we need to add equality in the tansition
    auto var = trans_input_vars[i];
    auto it = std::find(vars.begin(), vars.end(), var);
    if (it != vars.end()) {
      auto out_index = it - vars.begin();
      additional_equalities.push_back(tm.mk_term(TERM_EQ, state_current[i], state_next[out_index]));
#ifndef NDEBUG
      // transition formula should not contain the input variable at this point anymore
      std::set<term_ref> tmp;
      tm.get_variables(transition_fla, tmp);
      auto tmp_it = std::find(tmp.begin(), tmp.end(), var);
      assert(tmp_it == tmp.end());
#endif // NDEBUG
    }
//    std::cout << "Adding substituition: " << trans_input_vars[i] << " -> " << state_current[i] << std::endl;
    sub.add(trans_input_vars[i], state_current[i]);
  }
  transition_fla = tm.substitute(transition_fla, sub.mapping);
  if (!additional_equalities.empty()) {
    transition_fla = tm.mk_and(transition_fla, tm.mk_and(additional_equalities));
  }
//  std::cout << "After second substituition" << transition_fla << std::endl;
  assert(st->is_transition_formula(transition_fla));
  system::transition_formula* transition_relation = nullptr;
  transition_relation = new system::transition_formula(tm, st, transition_fla);
  std::string system_id = "CHC";
  system::transition_system* system = new system::transition_system(system_id, st, init_states, transition_relation);
  cmd_seq->push_back(new cmd::define_transition_system(system_id, system));
//  std::cout << *system;

  // create query formula;
  expr::term_ref query_fla = query_rule.second;
//  std::cout << query_fla << std::endl;
  auto extracted_vars = remove_predicate_and_extract_vars(query_fla, predicate);
//  std::cout << query_fla << std::endl;
  // TODO: check that all variables are different
  sub.clear();
  assert(extracted_vars.size() + additional_vars.size() == state_current.size());
  for (size_t i = 0; i < extracted_vars.size(); ++i ) {
    sub.add(extracted_vars[i], state_current[i]);
  }
  query_fla = tm.substitute(query_fla, sub.mapping);
//  std::cout << query_fla << std::endl;
  query_fla = tm.mk_not(query_fla);
//  std::cout << query_fla << std::endl;
  assert(st->is_state_formula(query_fla));
  system::state_formula* query = new system::state_formula(tm, st, query_fla);
  cmd_seq->push_back(new cmd::query(ctx, system_id, query));
  return cmd_seq;
}

chc_system::term_vec chc_system::remove_predicate_and_extract_vars(expr::term_ref &tail, expr::term_ref predicate) const {
  auto p_term = extract_predicate_from_tail(tail, predicate);
  return get_arguments(p_term);
}

expr::term_ref chc_system::extract_predicate_from_tail(expr::term_ref &tail, expr::term_ref predicate_symbol) const {
  auto& tm = ctx.tm();
  term_vec args;
  tm.get_conjuncts(tail, args);
  auto it = std::find_if(args.begin(), args.end(),
                         [&,predicate_symbol](term_ref ref)
                         { return tm.term_of(ref).op() == TERM_FUN_APP && tm.term_of(ref).child(0) == predicate_symbol; });
  assert(it != args.end());
  auto ret = *it;
  args.erase(it);
  tail = tm.mk_and(args);
  return ret;
}

}
}

