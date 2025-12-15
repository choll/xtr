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

#ifndef XTR_DETAIL_COMMANDS_IOS_HPP
#define XTR_DETAIL_COMMANDS_IOS_HPP

#include "responses.hpp"

#include <ostream>

namespace xtr::detail
{
    inline std::ostream& operator<<(std::ostream& os, log_level_t level)
    {
        switch (level)
        {
        case log_level_t::none:
            return os << "none";
        case log_level_t::fatal:
            return os << "fatal";
        case log_level_t::error:
            return os << "error";
        case log_level_t::warning:
            return os << "warning";
        case log_level_t::info:
            return os << "info";
        case log_level_t::debug:
            return os << "debug";
        }
        return os << "<invalid>";
    }

    inline std::ostream& operator<<(std::ostream& os, const sink_info& si)
    {
        return os << si.name << " (" << si.level << ") " << si.buf_capacity / 1024
                  << "K capacity, " << si.buf_nbytes / 1024 << "K used, "
                  << si.dropped_count << " dropped";
    }
}

#endif
