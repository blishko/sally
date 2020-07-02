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

#ifdef WITH_OPENSMT2

#include "smt/opensmt2/opensmt2_nonitp.h"
#include "smt/opensmt2/opensmt2_internal.h"

#include "utils/trace.h"


#define unused_var(x) { (void) x; }

namespace sally {
namespace smt {

opensmt2_nonitp::opensmt2_nonitp(expr::term_manager& tm, const options& opts, utils::statistics& stats)
  : solver("opensmt2_nonitp", tm, opts, stats)
{
  d_internal = new opensmt2_internal(tm, opts, false);
}

opensmt2_nonitp::~opensmt2_nonitp() {
  delete d_internal;
}

void opensmt2_nonitp::add(expr::term_ref f, formula_class f_class) {
  TRACE("opensmt2_nonitp") << "opensmt2_nonitp[" << d_internal->instance() << "]: adding " << f << std::endl;
  d_internal->add(f, f_class);
}

void opensmt2_nonitp::add_variable(expr::term_ref var, variable_class f_class) {
  solver::add_variable(var, f_class);
  d_internal->add_variable(var, f_class);
}

solver::result opensmt2_nonitp::check() {
  TRACE("opensmt2_nonitp")<< "opensmt2_nonitp[" << d_internal->instance() << "]: check()" << std::endl;
  return d_internal->check();
}

void opensmt2_nonitp::check_model() {
  TRACE("opensmt2_nonitp")<< "opensmt2_nonitp[" << d_internal->instance() << "]: check_model()" << std::endl;
  throw exception("Unsupported");
}

expr::model::ref opensmt2_nonitp::get_model() const {
  TRACE("opensmt2_nonitp")<< "opensmt2_nonitp[" << d_internal->instance() << "]: get_model()" << std::endl;
  return d_internal->get_model();
}

void opensmt2_nonitp::push() {
  TRACE("opensmt2_nonitp")<< "opensmt2_nonitp[" << d_internal->instance() << "]: push()" << std::endl;
  d_internal->push();
}

void opensmt2_nonitp::pop() {
  TRACE("opensmt2_nonitp")<< "opensmt2_nonitp[" << d_internal->instance() << "]: pop()" << std::endl;
  d_internal->pop();
}

bool opensmt2_nonitp::supports(solver::feature f) const {
  switch (f) {
    case solver::feature::GENERALIZATION:
      return true;
    default:
      return false;
  }
  return false; // Compile error otherwise
}

void opensmt2_nonitp::gc_collect(const expr::gc_relocator & gc_reloc) {
  solver::gc_collect(gc_reloc);
}

void opensmt2_nonitp::gc() {
  solver::gc();
}

void opensmt2_nonitp::generalize(solver::generalization_type type, vector<expr::term_ref> &projection_out) {
  TRACE("opensmt2_nonitp") << "opensmt2_nonitp[" << d_internal->instance() << "]: generalizing" << std::endl;
  assert(!d_B_variables.empty());
  d_internal->generalize(type, projection_out);
}

void
opensmt2_nonitp::generalize(solver::generalization_type type, expr::model::ref m, vector<expr::term_ref> &projection_out) {
  TRACE("opensmt2_nonitp") << "opensmt2_nonitp[" << d_internal->instance() << "]: generalizing" << std::endl;
  assert(!d_B_variables.empty());
  d_internal->generalize(type, m, projection_out);
}

}
}

#endif // WITH_OPENSMT2
