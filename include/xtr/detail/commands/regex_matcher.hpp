// Copyright 2021 Chris E. Holloway
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef XTR_DETAIL_COMMANDS_REGEX_MATCHER_HPP
#define XTR_DETAIL_COMMANDS_REGEX_MATCHER_HPP

#include "matcher.hpp"

#include <cstddef>

#include <regex.h>

namespace xtr::detail
{
    class regex_matcher;
}

class xtr::detail::regex_matcher final : public matcher
{
public:
    regex_matcher(const char* pattern, bool ignore_case, bool extended);

    ~regex_matcher() override;

    regex_matcher(const regex_matcher&) = delete;
    regex_matcher& operator=(const regex_matcher&) = delete;

    bool valid() const override;

    void error_reason(char* buf, std::size_t bufsz) const override;

    bool operator()(const char* str) const override;

private:
    // std::regex isn't used because it isn't possible to use it without
    // exceptions (it throws if an expression is invalid).
    ::regex_t regex_;
    int errnum_;
};

#endif
