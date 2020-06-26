#ifdef WITH_OPENSMT2

#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include "expr/term.h"
#include "expr/term_manager.h"

#include "smt/factory.h"

#include "utils/options.h"
#include "utils/statistics.h"

#include <iostream>


using namespace std;
using namespace sally;
using namespace expr;
using namespace smt;

struct term_manager_with_opensmt_test_fixture {

  utils::statistics stats;
  term_manager tm;
  solver* osmt2;

public:

  term_manager_with_opensmt_test_fixture()
    : tm(stats)
  {
    boost::program_options::variables_map boost_opts;
    boost_opts.insert(std::make_pair("solver-logic", boost::program_options::variable_value(std::string("QF_LRA"), false)));
    options opts(boost_opts);
    osmt2 = factory::mk_solver("opensmt2", tm, opts, stats);
    cout << set_tm(tm);
    cerr << set_tm(tm);
    output::trace_tag_enable("opensmt2");
  }

  ~term_manager_with_opensmt_test_fixture() {
    output::trace_tag_disable("opensmt2");
    delete osmt2;
  }
};

BOOST_FIXTURE_TEST_SUITE(smt_tests, term_manager_with_opensmt_test_fixture)

BOOST_AUTO_TEST_CASE(osmt2_basic_asserts) {

  // Assert something to yices
  term_ref x = tm.mk_variable("x", tm.real_type());
  term_ref y = tm.mk_variable("y", tm.real_type());
  term_ref b = tm.mk_variable("b", tm.boolean_type());
  term_ref zero = tm.mk_rational_constant(rational());

  term_ref sum = tm.mk_term(TERM_ADD, x, y);

  // x + y <= 0
  term_ref leq = tm.mk_term(TERM_LEQ, sum, zero);

  term_ref f = tm.mk_term(TERM_AND, b, leq);

  cout << "Adding: " << f << endl;
  osmt2->add(f, smt::solver::CLASS_A);

  solver::result result = osmt2->check();
  cout << "Check result: " << result << endl;

  BOOST_CHECK_EQUAL(result, solver::SAT);

  term_ref g = tm.mk_term(TERM_NOT, b);

  cout << "Adding: " << g << endl;
  osmt2->add(g, smt::solver::CLASS_A);

  result = osmt2->check();
  cout << "Check result: " << result << endl;

  BOOST_CHECK_EQUAL(result, solver::UNSAT);

}

BOOST_AUTO_TEST_CASE(osmt2_generalization) {

  osmt2->push();

  // Assert something to yices
  term_ref x = tm.mk_variable("x", tm.real_type());
  term_ref y = tm.mk_variable("y", tm.real_type());
  term_ref z = tm.mk_variable("z", tm.real_type());
  term_ref zero = tm.mk_rational_constant(rational());

  // Mark the variables
  osmt2->add_variable(x, smt::solver::CLASS_A);
  osmt2->add_variable(y, smt::solver::CLASS_A);
  osmt2->add_variable(z, smt::solver::CLASS_B);

  expr::term_ref G;

  term_ref sum_x_y = tm.mk_term(TERM_ADD, x, y);
  term_ref sum_x_y_z = tm.mk_term(TERM_ADD, sum_x_y, z);

  // x + y <= 0
  term_ref leq_x_y = tm.mk_term(TERM_LEQ, sum_x_y, zero);
  // 0 <= x + y
  term_ref geq_x_y = tm.mk_term(TERM_LEQ, zero, sum_x_y);

  // Add and check
  osmt2->add(leq_x_y, smt::solver::CLASS_A);
  osmt2->add(geq_x_y, smt::solver::CLASS_A);

  // Check and generalize
  solver::result result = osmt2->check();
  cout << "Check result: " << result << endl;

  // Generalize
  G = osmt2->generalize(smt::solver::GENERALIZE_BACKWARD);
  cout << "G: " << G << endl;

  // 0 <= x + y + z
  term_ref geq_x_y_z = tm.mk_term(TERM_LEQ, zero, sum_x_y_z);

  // Add some more
  osmt2->add(geq_x_y_z, smt::solver::CLASS_B);

  // Check and generalize
  result = osmt2->check();
  cout << "Check result: " << result << endl;
  G = osmt2->generalize(smt::solver::GENERALIZE_BACKWARD);
  cout << "G: " << G << endl;

  osmt2->pop();

  term_ref b1 = tm.mk_variable("b1", tm.boolean_type());
  term_ref b2 = tm.mk_variable("b2", tm.boolean_type());

  osmt2->add_variable(b2, smt::solver::CLASS_A);
  osmt2->add_variable(b1, smt::solver::CLASS_B);


  term_ref imp1 = tm.mk_term(expr::TERM_IMPLIES, b1, geq_x_y_z);
  term_ref imp2 = tm.mk_term(expr::TERM_IMPLIES, b2, leq_x_y);

  osmt2->add(imp1, smt::solver::CLASS_B);
  osmt2->add(imp2, smt::solver::CLASS_B);

  result = osmt2->check();
  cout << "Check result: " << result << endl;

  G = osmt2->generalize(smt::solver::GENERALIZE_BACKWARD);
  cout << "G: " << G << endl;

}

BOOST_AUTO_TEST_CASE(osmt2_generalization_experiment) {

  osmt2->push();

  // Assert something to yices
  term_ref x = tm.mk_variable("x", tm.real_type());
  term_ref x_next = tm.mk_variable("x'", tm.real_type());
  term_ref y = tm.mk_variable("y", tm.real_type());
  term_ref y_next = tm.mk_variable("y'", tm.real_type());

  term_ref one = tm.mk_rational_constant(rational(1, 1));

  // Mark the variables
  osmt2->add_variable(x, smt::solver::CLASS_A);
  osmt2->add_variable(y, smt::solver::CLASS_A);
  osmt2->add_variable(x_next, smt::solver::CLASS_B);
  osmt2->add_variable(y_next, smt::solver::CLASS_B);

  expr::term_ref G;

  // A1: x' = x + 1
  // A2: y' = y + 1
  // A3: y' = 1
  term_ref x_inc = tm.mk_term(TERM_ADD, x, one);
  term_ref y_inc = tm.mk_term(TERM_ADD, y, one);
  term_ref A1 = tm.mk_term(TERM_EQ, x_next, x_inc);
  term_ref A2 = tm.mk_term(TERM_EQ, y_next, y_inc);
  term_ref A3 = tm.mk_term(TERM_EQ, y_next, one);

  // Add and check
  osmt2->add(A1, smt::solver::CLASS_T);
  osmt2->add(A2, smt::solver::CLASS_T);
  osmt2->add(A3, smt::solver::CLASS_B);

  // Check and generalize
  solver::result result = osmt2->check();
  cout << "Check result: " << result << endl;

  // Generalize
  G = osmt2->generalize(smt::solver::GENERALIZE_BACKWARD);
  cout << "G: " << G << endl;

  osmt2->pop();

  osmt2->push();

  // A5:
  // it b then
  //    A2: y' = y + 1
  // else
  //    A4: y' = y + 2
  term_ref b = tm.mk_variable("b", tm.boolean_type());
  term_ref y_inc2 = tm.mk_term(TERM_ADD, y_inc, one);
  term_ref A4 = tm.mk_term(TERM_EQ, y_next, y_inc2);
  term_ref A5 = tm.mk_term(TERM_ITE, b, A2, A4);
  term_ref A6 = tm.mk_term(TERM_GEQ, y_next, one);

  // Add and check
  osmt2->add(A5, smt::solver::CLASS_T);
  osmt2->add(A6, smt::solver::CLASS_B);

  // Check and generalize
  result = osmt2->check();
  cout << "Check result: " << result << endl;

  // Generalize
  G = osmt2->generalize(smt::solver::GENERALIZE_BACKWARD);
  cout << "G: " << G << endl;

  osmt2->pop();
}


BOOST_AUTO_TEST_SUITE_END()

#endif
