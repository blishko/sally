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
class term_ref;
}
class options;
class engine;

// END OF FORWARD DECLARATIONS

typedef struct api_context* sally_context;
typedef void (*sally_new_lemma_eh)(void *, size_t, const sally::expr::term_ref&);
typedef void (*sally_general_eh)(void *);
typedef void (*sally_obligation_pushed_eh)(void *, size_t, const sally::expr::term_ref&,
  const sally::expr::term_ref&, size_t);



sally_context create_context(std::map<std::string, std::string> const & options );

void delete_context(sally_context);

void run_on_file(std::string file, sally_context ctx);

void run_on_mcmt_string(std::string const & content, sally_context ctx);

void run_on_chc_string(std::string const & content, sally_context ctx);

void set_new_reachability_lemma_eh(sally_context ctx, sally_new_lemma_eh, void*);

void set_obligation_pushed_eh(sally_context ctx, sally_obligation_pushed_eh, void*);

void add_next_frame_eh(sally_context ctx, sally_general_eh, void*);

void add_next_obligation_eh(sally_context ctx, sally_general_eh eh, void* state);

std::string term_to_string(sally_context ctx, const sally::expr::term_ref& );

void add_lemma(sally_context ctx, std::string const &lemma_str);

std::string reachability_lemma_to_command(sally_context ctx, size_t level, const sally::expr::term_ref& lemma_ref);

std::string induction_lemma_to_command(sally_context ctx, size_t level,
  const sally::expr::term_ref& lemma_ref, const sally::expr::term_ref& ex_ref, size_t cex_depth);



class stats {
  sally_context ctx;
public:
  stats(sally_context context) : ctx(context) {}

  std::vector<std::pair<std::string, std::string>> get_stats() const;

};

}

#endif //SALLY_SALLY_API_H
