// Copyright 2021 Chris E. Holloway
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef XTR_DETAIL_COMMANDS_MATCHER_HPP
#define XTR_DETAIL_COMMANDS_MATCHER_HPP

#include "pattern.hpp"

#include <cstddef>
#include <memory>

namespace xtr::detail
{
    class matcher;

    std::unique_ptr<matcher> make_matcher(
        pattern_type_t pattern_type, const char* pattern, bool ignore_case);
}

class xtr::detail::matcher
{
public:
    virtual bool operator()(const char*) const
    {
        return true;
    }

    virtual bool valid() const
    {
        return true;
    }

    virtual void error_reason(char*, std::size_t) const // LCOV_EXCL_LINE
    {
    }

    virtual ~matcher() {};
};

#endif
