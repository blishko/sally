//
// Created by Martin Blicha on 2019-04-20.
//

#include "induction_lemma.h"

sally::cmd::induction_lemma::induction_lemma(size_t level, expr::term_ref lemma, expr::term_ref cex, size_t cex_depth,
  const expr::term_manager& tm)
: command(INDUCTION_LEMMA)
, level(level)
, lemma(lemma)
, cex(cex)
, cex_depth(cex_depth)
, tm(tm)
{}

void sally::cmd::induction_lemma::run(sally::system::context *ctx, sally::engine *e) {
  e->add_induction_lemma(level, lemma, cex, cex_depth);
}

void sally::cmd::induction_lemma::to_stream(std::ostream &out) const {
  out << "ilemma (" + std::to_string(level) + ' ' + tm.to_string(lemma) + ' ' + tm.to_string(cex) + ' ' + std::to_string(cex_depth) + " )";
}
