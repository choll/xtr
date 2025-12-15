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

#include "xtr/detail/commands/matcher.hpp"
#include "xtr/detail/commands/regex_matcher.hpp"
#include "xtr/detail/commands/wildcard_matcher.hpp"

XTR_FUNC
std::unique_ptr<xtr::detail::matcher> xtr::detail::make_matcher(
    pattern_type_t pattern_type, const char* pattern, bool ignore_case)
{
    switch (pattern_type)
    {
    case pattern_type_t::wildcard:
        return std::make_unique<wildcard_matcher>(pattern, ignore_case);
    case pattern_type_t::extended_regex:
        return std::make_unique<regex_matcher>(pattern, ignore_case, true);
    case pattern_type_t::basic_regex:
        return std::make_unique<regex_matcher>(pattern, ignore_case, false);
    case pattern_type_t::none:
        break;
    }
    return std::make_unique<matcher>();
}
