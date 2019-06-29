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

#pragma once

#include <set>
#include <vector>

#include "../../system/trace_helper.h"
#include "smt/solver.h"
#include "expr/term_manager.h"
#include "expr/gc_relocator.h"
#include "system/transition_system.h"
#include "system/context.h"

#include "induction_obligation.h"

namespace sally {
namespace pdkind {

/**
 * Class to manage solvers in IC3.
 *
 * There are three dedicated solver available:
 *   [Reachability] Solver(s) to check one-step reachability from frame to
 *                  next frame. These can be a single solver that uses boolean
 *                  variables to select the frame in question. Checks here should
 *                  only be done through query_at method.
 *   [Induction]    Solver to check induction from last frame. Checks here should
 *                  only be done through the check_inductive method.
 *   [Counterexample] Solver for BMC checks.
 */
class solvers {

  typedef std::set<expr::term_ref> formula_set;

  /** Context */
  const system::context& d_ctx;

  /** Term manager */
  expr::term_manager& d_tm;

  /** Transition system for these solvers */
  const system::transition_system* d_transition_system;

  /** Number of frames */
  size_t d_size;

  /** The trace to get the state variables for unrolling */
  system::trace_helper* d_trace;

  /** A solver per frame with transition relation info */
  std::vector<smt::solver*> d_reachability_solvers;

  /** Solver for reachability queries when in single-solver mode */
  smt::solver* d_reachability_solver;

  /** Solver for initial state queries */
  smt::solver* d_initial_solver;

  /** Solver for induction queries */
  smt::solver* d_induction_solver;

  /** Solver for induction generalization */
  smt::solver* d_induction_generalizer;

  /** Solver for minimization */
  smt::solver* d_minimization_solver;

  /** Depth of the induction solver */
  size_t d_induction_solver_depth;

  /** Relation used in the induction solver */
  expr::term_ref d_transition_relation;

  /** Returns the induction solver */
  smt::solver* get_initial_solver();

  /** Initialize the reachability solver for frame k */
  void init_reachability_solver(size_t k);

  /** Generalize the SAT result */
  expr::term_ref generalize_sat(smt::solver* solver);

  /** Generalize the given model that satisfies assertions */
  expr::term_ref generalize_sat(smt::solver* solver, expr::model::ref m);

  /** Returns the unique reachability solver */
  smt::solver* get_reachability_solver();

  /** Returns the k-th reachability solver */
  smt::solver* get_reachability_solver(size_t k);

  /** Returns the minimization solver */
  smt::solver* get_minimization_solver();

  /** Whether to generate models for queries */
  bool d_generate_models_for_queries;

  /** Use quickxplain to minimize the interpolant */
  void quickxplain_interpolant(bool negate, smt::solver* I_solver, smt::solver* T_solver, const std::vector<expr::term_ref>& formulas, size_t begin, size_t end, std::vector<expr::term_ref>& out);

  /** Use quickxplain to minimize the generalization */
  void quickxplain_generalization(smt::solver* solver, const std::vector<expr::term_ref>& disjuncts, size_t begin, size_t end, std::vector<expr::term_ref>& out);

  /** Use quickxplain to minimize the frame */
  void quickxplain_frame(smt::solver* solver, const std::vector<induction_obligation>& frame, size_t begin, size_t end, std::vector<induction_obligation>& out);

public:

  /** Create solvers for the given transition system */
  solvers(const system::context& ctx, const system::transition_system* transition_system, system::trace_helper* trace);

  /** Delete the solvers */
  ~solvers();

  /** Mark a new frame */
  void new_reachability_frame();

  /** Get number of frames */
  size_t size();

  /** Reset all the solvers. */
  void reset(const std::vector<formula_set>& frames);

  /** Add a formula to frame k */
  void add_to_reachability_solver(size_t k, expr::term_ref f);

  struct query_result {
    /** Result of the query */
    smt::solver::result result;
    /** Model, if sat */
    expr::model::ref model;
    /** The generalization, if sat */
    expr::term_ref generalization;

    query_result();
  };

  /** Checks formula f for satisfiability at frame k using the reachability solvers and returns the generalization. */
  query_result query_with_transition_at(size_t k, expr::term_ref f, smt::solver::formula_class f_class);

  /** Checks formula f for satisfiability at initial frame. */
  smt::solver::result query_at_init(expr::term_ref f);

  /**
   * Reset induction solver so that it has given depth. Depth is the number of
   * transitions. So, if you'd like to try k-induction, you need to do depth k + 1.
   * The solver will have depth frames and all formulas will be added to frames
   * < depth.
   */
  void reset_induction_solver(size_t depth);

  enum induction_assertion_type {
    // First frame
    INDUCTION_FIRST,
    // Intermediate frames
    INDUCTION_INTERMEDIATE
  };

  /** Minimize the frame */
  void minimize_frame(std::vector<induction_obligation>& frame);

  /**
   * Add a formula to induction solver. Formulas will be added to frames < depth.
   */
  void add_to_induction_solver(expr::term_ref f, induction_assertion_type type);

  /**
   * Check if f is inductive, i.e. !f is added at frame depth and check for
   * satisfiability.
   */
  query_result check_inductive(expr::term_ref f);

  /**
   * Check if the given model from induction check satisfies f at frame depth.
   * If yes, returns generalization.
   */
  query_result check_inductive_model(expr::model::ref m, expr::term_ref f);

  smt::solver::result check_inductive_if_added(expr::term_ref f);

  /** Learn forward to refute G at k from k-1 and initial state using reachability solvers */
  expr::term_ref learn_forward(size_t k, expr::term_ref G);

  /** Whether to return models with queries */
  void generate_models_for_queries(bool flag);

  /** Collect solver garbage */
  void gc();

  /** Collect term manager garbage */
  void gc_collect(const expr::gc_relocator& gc_reloc);

  /** Rewrite equalitites to inequalities */
  expr::term_ref eq_to_ineq(expr::term_ref G);

  /** Output EFSMT problem */
  void output_efsmt(expr::term_ref f, expr::term_ref g) const;

  /** Print formulas */
  template<typename container>
  void print_formulas(const container& set, std::ostream& out) const;

};

template <typename container>
void solvers::print_formulas(const container& set, std::ostream& out) const {
  typename container::const_iterator it = set.begin();
  for (; it != set.end(); ++ it) {
    out << *it << std::endl;
  }
}

}
}
