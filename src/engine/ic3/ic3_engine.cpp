/*
 * ic3_engine.cpp
 *
 *  Created on: Nov 23, 2014
 *      Author: dejan
 */

#include "engine/ic3/ic3_engine.h"

#include "smt/factory.h"
#include "system/state_trace.h"
#include "utils/trace.h"

#include <stack>
#include <cassert>
#include <sstream>
#include <iostream>

namespace sal2 {
namespace ic3 {

/** Lowest weight first, then, lowest frame */
bool obligation_compare_induction::operator () (const obligation& o1, const obligation& o2) const {
  if (o1.weight() != o2.weight()) {
    return o1.weight() > o2.weight();
  }
  if (o1.frame() != o2.frame()) {
    return o1.frame() > o2.frame();
  }
  return o1.formula() > o2.formula();
}

ic3_engine::ic3_engine(const system::context& ctx)
: engine(ctx)
, d_transition_system(0)
, d_property(0)
, d_max_frames(0)
, d_max_frame_size(0)
{}

std::ostream& operator << (std::ostream& out, const ic3_engine& ic3) {
  ic3.to_stream(out);
  return out;
}

ic3_engine::~ic3_engine() {
  for (size_t k = 0; k < d_solvers.size(); ++ k) {
    delete d_solvers[k];
  }
}

expr::term_ref ic3_engine::check_one_step_reachable(size_t k, expr::term_ref F) {
  assert(k > 0);

  // The state type
  const system::state_type* state_type = d_transition_system->get_state_type();

  // Get the solver of the previous frame
  smt::solver* solver = get_solver(k-1);
  smt::solver_scope scope(solver);

  // Add the formula (moving current -> next)
  scope.push();
  expr::term_ref F_next = state_type->change_formula_vars(system::state_type::STATE_CURRENT, system::state_type::STATE_NEXT, F);
  solver->add(F_next);

  // Figure out the result
  smt::solver::result r = solver->check();
  switch (r) {
  case smt::solver::SAT: {
    const std::vector<expr::term_ref>& state_vars = state_type->get_variables(system::state_type::STATE_NEXT);
    return solver->generalize(state_vars);
  }
  case smt::solver::UNSAT:
    // Unsat, we return NULL
    return expr::term_ref();
  default:
    throw exception("SMT unknown result.");
  }

  return expr::term_ref();
}

expr::term_ref ic3_engine::check_inductive_at(size_t k, expr::term_ref F) {
  ensure_frame(k);
  ensure_frame(k+1);
  assert(!F.is_null());
  assert(d_frame_content[k].find(F) != d_frame_content[k].end());
  assert(d_frame_content[k+1].find(F) != d_frame_content[k].end());

  TRACE("ic3") << "ic3: Checking inductive at " << k << " for " << F << std::endl;

  expr::term_ref F_not = tm().mk_term(expr::TERM_NOT, F);
  expr::term_ref result = check_one_step_reachable(k+1, F_not);

  TRACE("ic3") << "ic3: " << (result.is_null() ? "inductive" : "not inductive") << std::endl;

  return result;
}

void ic3_engine::add_inductive_at(size_t k, expr::term_ref F, int weight) {
  // Ensure frame is setup
  ensure_frame(k);
  // The state type
  const system::state_type* state_type = d_transition_system->get_state_type();
  // Add to all frames from 0..k
  int i = k;
  for(; i >= 0; -- i) {
    if (d_frame_content[i].find(F) != d_frame_content[i].end()) {
      break;
    }
    d_frame_content[i].insert(F);
    get_solver(i)->add(F);
    if (i > 0) {
      expr::term_ref F_next = state_type->change_formula_vars(system::state_type::STATE_CURRENT, system::state_type::STATE_NEXT, F);
      get_solver(i-1)->add(F_next);
    }
  }
  // Add to induction obligations
  d_induction_obligations.push(obligation(k, F, weight));
}

void ic3_engine::ensure_frame(size_t k) {
  if (d_solvers.size() <= k && output::get_verbosity(std::cout) > 0) {
    std::cout << "ic3: Extending trace to " << k << std::endl;
  }
  while (d_solvers.size() <= k) {
    // Make the solver
    smt::solver* solver = smt::factory::mk_default_solver(tm(), ctx().get_options());
    d_solvers.push_back(solver);
    // Add the transition relation
    solver->add(d_transition_system->get_transition_relation());
    // Add the frame content
    d_frame_content.push_back(formula_set());
  }
  assert(d_solvers.size() == d_frame_content.size());
}

bool ic3_engine::frame_contains(size_t k, expr::term_ref f) {
  ensure_frame(k);
  return d_frame_content[k].find(f) != d_frame_content[k].end();
}

bool ic3_engine::check_reachable(size_t k, expr::term_ref f) {

  TRACE("ic3") << "ic3: checking reachability at " << k << std::endl;

  // Push the solvers
  push_solvers();

  // Queue of reachability obligations
  std::stack<obligation> reachability_obligations;
  reachability_obligations.push(obligation(k, f, 0));

  // We're not reachable, if we hit 0 we set it to true
  bool reachable = false;

  // The induction not valid, try to extend to full counter-example
  while (!reachability_obligations.empty()) {
    // Get the next reachability obligations
    obligation reach = reachability_obligations.top();
    // If we're at 0 frame, we're reachable anything passed in is consistent
    // part of the abstraction
    if (reach.frame() == 0) {
      reachable = true;
      break;
    }
    // Check if the obligation is reachable
    expr::term_ref G = check_one_step_reachable(reach.frame(), reach.formula());
    if (G.is_null()) {
      // Proven, remove from obligations
      reachability_obligations.pop();
      // Add the negation of the obligation to known facts
      expr::term_ref learnt = tm().mk_term(expr::TERM_NOT, reach.formula());
      get_solver(reach.frame())->add(learnt);
    } else {
      // New obligation to reach the counterexample
      reachability_obligations.push(obligation(reach.frame()-1, G, 0));
    }
  }

  // Pop the solvers
  pop_solvers();

  TRACE("ic3") << "ic3: " << (reachable ? "reachable" : "not reachable") << std::endl;

  // All discharged, so it's not reachable
  return reachable;
}

bool ic3_engine::check_valid_and_add(size_t k, expr::term_ref f, int weight) {

  if (output::get_verbosity(std::cout) > 0) {
     std::cout << "ic3: checking validity" << std::endl;
  }

  ensure_frame(k);

  expr::term_ref f_not = tm().mk_term(expr::TERM_NOT, f);
  smt::solver* solver = get_solver(k);
  solver->push();
  solver->add(f_not);
  smt::solver::result r = solver->check();
  solver->pop();
  switch (r) {
  case smt::solver::SAT:
    // Invalid, property is not valid
    return false;
  case smt::solver::UNSAT:
    // Valid, we continue with P
    add_inductive_at(0, f, weight);
    return true;
  default:
    throw exception("Unknown SMT result.");
  }
}

bool ic3_engine::push_if_inductive(size_t k, expr::term_ref f, int weight) {

  ensure_frame(k);
  ensure_frame(k+1);

  std::vector<obligation> induction_assumptions;

  // Push the solvers
  push_solvers();

  bool inductive = false;
  for (;;) {

    // Check if inductive
    expr::term_ref G = check_inductive_at(k, f);

    // If inductive
    if (G.is_null()) {
      inductive = true;
      break;
    }

    // Check if G is reachable
    bool reachable = check_reachable(k, G);

    // If we discharged all the obligations, let's re-check the induction
    if (reachable) {
      inductive = false;
      break;
    } else {
      expr::term_ref learnt = tm().mk_term(expr::TERM_NOT, G);
      get_solver(k)->add(learnt);
      induction_assumptions.push_back(obligation(k, learnt, weight + 1));
    }
  }

  // Pop the solvers
  pop_solvers();

  // If inductive, add the learnt and all the needed formulas
  if (inductive) {
    // Add the thing we learnt
    add_inductive_at(k+1, f, weight+1);
    for (size_t i = 0; i < induction_assumptions.size(); ++ i) {
      obligation assumption = induction_assumptions[i];
      add_inductive_at(assumption.frame(), assumption.formula(), assumption.weight());
    }
  }

  return inductive;
}

engine::result ic3_engine::query(const system::transition_system* ts, const system::state_formula* sf) {

  // Remember the input
  d_transition_system = ts;
  d_property = sf;

  // Options
  d_max_frames = ctx().get_options().get_unsigned("ic3-max-frames");
  d_max_frame_size = ctx().get_options().get_unsigned("ic3-max-frame-size");

  // Add the initial state
  expr::term_ref I = d_transition_system->get_initial_states();
  add_inductive_at(0, I, 0);

  // Add the property we're trying to prove
  expr::term_ref P = d_property->get_formula();
  bool P_valid = check_valid_and_add(0, P, 0);
  if (!P_valid) {
    return engine::INVALID;
  }

  // Search while we have something to do
  while (!d_induction_obligations.empty() && d_frame_content[0].size() <= d_max_frame_size) {

    // Pick a formula to try and prove inductive, i.e. that F_k & P & T => P'
    obligation ind = d_induction_obligations.top();
    d_induction_obligations.pop();

    // Check if already shown in the next frame
    if (frame_contains(ind.frame()+1, ind.formula())) {
      continue;
    }

    /** Push the formula forward if it's inductive at the fram */
    bool is_inductive = push_if_inductive(ind.frame(), ind.formula(), ind.weight());
    if (!is_inductive) {
      // Not inductive, if P then we have a conterexample
      if (ind.formula() == P) {
        return engine::INVALID;
      }
    } else {
      // Inductive, if frames equal, we have a proofs
      if (d_frame_content[ind.frame()].size() == d_frame_content[ind.frame()+1].size()) {
        return engine::VALID;
      }
    }
  }

  // Didn't prove or disprove, so unknown
  return engine::UNKNOWN;
}

const system::state_trace* ic3_engine::get_trace() {
  return 0;
}

void ic3_engine::to_stream(std::ostream& out) const  {
  for (size_t k = 0; k < d_frame_content.size(); ++ k) {
    out << "Frame " << k << ":" << std::endl;
    formula_set::const_iterator it = d_frame_content[k].begin();
    for (; it != d_frame_content[k].end(); ++ it) {
      out << *it << std::endl;
    }
  }
}

smt::solver* ic3_engine::get_solver(size_t k) {
  ensure_frame(k);
  if (d_solvers_modified_per_push.size() > 0) {
    if (d_solvers_modified_per_push.back().find(k) == d_solvers_modified_per_push.back().end()) {
      d_solvers_modified_per_push.back().insert(k);
      d_solvers[k]->push();
    }
  }
  return d_solvers[k];
}

void ic3_engine::push_solvers() {
  d_solvers_modified_per_push.push_back(std::set<size_t>());
}

void ic3_engine::pop_solvers() {
  assert(!d_solvers_modified_per_push.empty());
  std::set<size_t>::iterator it = d_solvers_modified_per_push.back().begin();
  for (; it != d_solvers_modified_per_push.back().end(); ++ it) {
    get_solver(*it)->pop();
  }
  d_solvers_modified_per_push.pop_back();
}


}
}
