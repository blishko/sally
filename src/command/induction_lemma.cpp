//
// Created by Martin Blicha on 2019-04-20.
//

#include "induction_lemma.h"

sally::cmd::induction_lemma::induction_lemma(size_t level, expr::term_ref lemma, expr::term_ref cex,
  const expr::term_manager& tm)
: command(INDUCTION_LEMMA)
, level(level)
, lemma(lemma)
, cex(cex)
, tm(tm)
{}

void sally::cmd::induction_lemma::run(sally::system::context *ctx, sally::engine *e) {
  e->add_reachability_lemma(level, lemma);
}

void sally::cmd::induction_lemma::to_stream(std::ostream &out) const {
  out << "lemma (" + std::to_string(level) + ' ' + tm.to_string(lemma) + " )";

}
