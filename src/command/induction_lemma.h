//
// Created by Martin Blicha on 2019-06-14.
//

#ifndef SALLY_INDUCTION_LEMMA_H
#define SALLY_INDUCTION_LEMMA_H

#include "command.h"

namespace sally {
namespace cmd {


class induction_lemma : public command {
  size_t level;
  expr::term_ref lemma;
  expr::term_ref cex;
  size_t cex_depth;
  const expr::term_manager& tm;
public:
  induction_lemma(size_t level, expr::term_ref lemma, expr::term_ref cex, size_t cex_depth, const expr::term_manager& tm);

  ~induction_lemma() {}

  void run(system::context *ctx, engine *e);

  void to_stream(std::ostream &out) const;

};
}}

#endif //SALLY_INDUCTION_LEMMA_H
