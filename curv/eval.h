// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_EVAL_H
#define CURV_EVAL_H

#include <curv/value.h>
#include <curv/meaning.h>

namespace curv {

inline Value eval(const Expression& e) { return e.eval(); }

} // namespace curv
#endif // header guard
