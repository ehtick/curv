// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#include <curv/range.h>

namespace curv {

std::ostream&
operator<<(std::ostream& out, const Range<const char*>& r)
{
    out.write(r.begin(), r.size());
    return out;
}

}
