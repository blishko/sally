//
// Created by Martin Blicha on 2019-01-17.
//

#ifndef SALLY_SALLY_API_H
#define SALLY_SALLY_API_H

#include <string>
#include <vector>
#include <map>

namespace sally{
// FORWARD DECLARATIONS
namespace system{
class context;
}
namespace utils{
class statistics;
}
namespace expr{
class term_manager;
}
class options;
class engine;
// END OF FORWARD DECLARATIONS

typedef struct api_context* sally_context;

sally_context create_context(std::map<std::string, std::string> const & options );

void delete_context(sally_context);

void run_on_file(std::string file, sally_context ctx);

void run_on_mcmt_string(std::string const & content, sally_context ctx);

}

#endif //SALLY_SALLY_API_H
