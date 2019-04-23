//
// Created by Martin Blicha on 2019-04-20.
//

#ifndef SALLY_FRAME_LEMMA_H
#define SALLY_FRAME_LEMMA_H

#include "command.h"

namespace sally {
namespace cmd {


class frame_lemma : public command {
  size_t level;
  expr::term_ref lemma;
  const expr::term_manager& tm;
public:
  frame_lemma(size_t level, expr::term_ref lemma, const expr::term_manager& tm);

  ~frame_lemma() {}

  void run(system::context *ctx, engine *e);

  void to_stream(std::ostream &out) const;

};
}}

#endif //SALLY_FRAME_LEMMA_H
