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

#include "xtr/log_level.hpp"
#include "xtr/detail/throw.hpp"

#define XTR_LOG_LEVELS \
    X(none)            \
    X(fatal)           \
    X(error)           \
    X(warning)         \
    X(info)            \
    X(debug)

XTR_FUNC
xtr::log_level_t xtr::log_level_from_string(std::string_view str)
{
#define X(LEVEL)       \
    if (str == #LEVEL) \
        return log_level_t::LEVEL;
    XTR_LOG_LEVELS
#undef X
    detail::throw_invalid_argument("Invalid log level");
}

XTR_FUNC
const char* xtr::default_log_level_style(log_level_t level)
{
    switch (level)
    {
    case log_level_t::fatal:
        return "F ";
    case log_level_t::error:
        return "E ";
    case log_level_t::warning:
        return "W ";
    case log_level_t::info:
        return "I ";
    case log_level_t::debug:
        return "D ";
    default:
        return "";
    }
}

XTR_FUNC
const char* xtr::systemd_log_level_style(log_level_t level)
{
    switch (level)
    {
    case log_level_t::fatal:
        return "<0>"; // SD_EMERG
    case log_level_t::error:
        return "<3>"; // SD_ERR
    case log_level_t::warning:
        return "<4>"; // SD_WARNING
    case log_level_t::info:
        return "<6>"; // SD_INFO
    case log_level_t::debug:
        return "<7>"; // SD_DEBUG
    default:
        return "";
    }
}
