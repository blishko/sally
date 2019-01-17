//
// Created by Martin Blicha on 2019-01-17.
//

#include "parse_options.h"
#include "engine/factory.h"
#include "ai/factory.h"
#include "smt/factory.h"

#include <iostream>

#include <boost/program_options/variables_map.hpp>
#include <boost/program_options.hpp>

using namespace boost::program_options;
using namespace std;

namespace sally {

std::string get_engines_list() {
  std::vector<string> engines;
  engine_factory::get_engines(engines);
  std::stringstream out;
  out << "The engine to use: ";
  for (size_t i = 0; i < engines.size(); ++i) {
    if (i) { out << ", "; }
    out << engines[i];
  }
  return out.str();
}

std::string get_ai_list() {
  std::vector<string> engines;
  ai::factory::get_interpreters(engines);
  std::stringstream out;
  out << "The abstract interpreter to use: ";
  for (size_t i = 0; i < engines.size(); ++ i) {
    if (i) { out << ", "; }
    out << engines[i];
  }
  return out.str();
}

std::string get_solver_list() {
  std::vector<string> solvers;
  smt::factory::get_solvers(solvers);
  std::stringstream out;
  out << "The SMT solver to use: ";
  for (size_t i = 0; i < solvers.size(); ++ i) {
    if (i) { out << ", "; }
    out << solvers[i];
  }
  return out.str();
}

std::string get_output_languages_list() {
  std::stringstream out;
  out << "Output language to use: ";
  for (int i = 0; i < output::UNKNOWN; ++ i) {
    if (i) { out << ", "; }
    out << output::language_to_string(output::language(i));
  }
  return out.str();
}

void parse_options(int argc, char *argv[], variables_map &variables) {
  // Define the main options
  options_description description("General options");
  description.add_options()
    ("help,h", "Prints this help message.")
    ("verbosity,v", value<unsigned>()->default_value(0), "Set the verbosity of the output.")
    ("input,i", value<vector<string> >()->required(), "A problem to solve.")
#ifndef NDEBUG
    ("debug,d", value<vector<string> >(), "Any tags to trace (only for debug builds).")
#endif
    ("show-trace", "Show the counterexample trace if found.")
    ("show-invariant", "Show the invariant if property is proved.")
    ("parse-only", "Just parse, don't solve.")
    ("engine", value<string>(), get_engines_list().c_str())
    ("ai", value<string>(), get_ai_list().c_str())
    ("solver", value<string>()->default_value(smt::factory::get_default_solver_id()), get_solver_list().c_str())
    ("solver-logic", value<string>(), "Optional smt2 logic to set to the solver (e.g. QF_LRA, QF_LIA, ...).")
    ("output-language", value<string>()->default_value("mcmt"), get_output_languages_list().c_str())
    ("lsal-extensions", "Use lsal extensions to the MCMT language")
    ("no-input-namespace", "Don't use input namespace in the the MCMT language")
    ("live-stats", value<string>(), "Output live statistic to the given file (- for stdout).")
    ("live-stats-time", value<unsigned>()->default_value(100), "Time period for statistics output (in miliseconds)")
    ("smt2-output", value<string>(), "Generate smt2 logs of solver queries with given prefix.")
    ("no-lets", "Don't use let expressions in printouts.");;

  // Get the individual engine options
  engine_factory::setup_options(description);

  // Get the individual solver options
  smt::factory::setup_options(description);

  // Get the abstract interpreter options
  ai::factory::setup_options(description);

  // The input files can be positional
  positional_options_description positional;
  positional.add("input", -1);

  // Parse the options
  bool parseError = false;
  try {
    store(command_line_parser(argc, argv).options(description).positional(positional).run(), variables);
  } catch (...) {
    parseError = true;
  }

  // If help needed, print it out
  if (parseError || variables.count("help") > 0 || variables.count("input") == 0) {
    if (parseError) {
      cout << "Error parsing command line!" << endl;
    }
    cout << "Usage: " << argv[0] << " [options] input ..." << endl;
    cout << description << endl;
    if (parseError) {
      exit(1);
    } else {
      exit(0);
    }
  }
}
}