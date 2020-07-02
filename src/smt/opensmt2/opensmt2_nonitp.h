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

#ifdef WITH_OPENSMT2

#include "smt/solver.h"

namespace sally {
namespace smt {

class opensmt2_internal;

class opensmt2_nonitp: public solver {

  /** Internal opensmt data */
  opensmt2_internal* d_internal;

public:

  /** Constructor */
  opensmt2_nonitp(expr::term_manager& tm, const options& opts,
           utils::statistics& stats);

  /** Destructor */
  virtual ~opensmt2_nonitp();

  /** Features */
  bool supports(feature f) const;

  /** Add an assertion f to the solver */
  void add(expr::term_ref f, formula_class f_class);

  void add_variable(expr::term_ref var, variable_class f_class);

  /** Check the assertions for satisfiability */
  result check();

  /** Check the model (debug) */
  void check_model();

  /** Get the model */
  expr::model::ref get_model() const;

  /** Push the solving context */
  void push();

  /** Pop the solving context */
  void pop();

  /** Collect terms */
  void gc_collect(const expr::gc_relocator& gc_reloc);

  /** Collect garbage */
  void gc();

  /**
   * Generalize the last call to check assuming the result was SAT.
   */
  void generalize(generalization_type type, std::vector<expr::term_ref> &projection_out);

  /**
   * Generalize the given model.
   */
  void generalize(generalization_type type, expr::model::ref m, std::vector<expr::term_ref> &projection_out);
};

}
}

#endif // WITH_OPENSMT2

