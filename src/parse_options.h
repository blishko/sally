//
// Created by Martin Blicha on 2019-01-17.
//

#ifndef SALLY_PARSE_OPTIONS_H
#define SALLY_PARSE_OPTIONS_H

#endif //SALLY_PARSE_OPTIONS_H

// Forward declare
namespace boost { namespace program_options {
class variables_map;
}}

namespace sally {
/** Parses the program arguments. */
void parse_options(int argc, char *argv[], boost::program_options::variables_map &variables);

}