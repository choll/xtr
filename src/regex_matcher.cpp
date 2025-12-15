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

#include "xtr/detail/commands/regex_matcher.hpp"

#include <cassert>

XTR_FUNC
xtr::detail::regex_matcher::regex_matcher(
    const char* pattern, bool ignore_case, bool extended)
{
    const int flags = REG_NOSUB | (ignore_case ? REG_ICASE : 0) |
                      (extended ? REG_EXTENDED : 0);
    errnum_ = ::regcomp(&regex_, pattern, flags);
}

XTR_FUNC
xtr::detail::regex_matcher::~regex_matcher()
{
    if (valid())
        ::regfree(&regex_);
}

XTR_FUNC
bool xtr::detail::regex_matcher::valid() const
{
    return errnum_ == 0;
}

XTR_FUNC
void xtr::detail::regex_matcher::error_reason(char* buf, std::size_t bufsz) const
{
    assert(errnum_ != 0);
    ::regerror(errnum_, &regex_, buf, bufsz);
}

XTR_FUNC
bool xtr::detail::regex_matcher::operator()(const char* str) const
{
    return ::regexec(&regex_, str, 0, nullptr, 0) == 0;
}
