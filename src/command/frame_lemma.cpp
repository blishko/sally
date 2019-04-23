//
// Created by Martin Blicha on 2019-04-20.
//

#include "frame_lemma.h"

sally::cmd::frame_lemma::frame_lemma(size_t level, expr::term_ref lemma, const expr::term_manager& tm)
: command(FRAME_LEMMA)
, level(level)
, lemma(lemma)
, tm(tm)
{}

void sally::cmd::frame_lemma::run(sally::system::context *ctx, sally::engine *e) {
  e->add_reachability_lemma(level, lemma);
}

void sally::cmd::frame_lemma::to_stream(std::ostream &out) const {
  out << "lemma (" + std::to_string(level) + ' ' + tm.to_string(lemma) + " )";

}
