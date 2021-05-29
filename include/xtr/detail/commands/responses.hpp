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

#ifndef XTR_DETAIL_COMMANDS_RESPONSES_HPP
#define XTR_DETAIL_COMMANDS_RESPONSES_HPP

#include "message_id.hpp"
#include "xtr/log_level.hpp"

#include <ostream>

namespace xtr::detail
{
    struct sink_info
    {
        static constexpr auto frame_id = frame_id_t(message_id::sink_info);

        log_level_t level;
        std::size_t buf_capacity;
        std::size_t buf_nbytes;
        std::size_t dropped_count;
        char name[128];
    };

    struct success
    {
        static constexpr auto frame_id = frame_id_t(message_id::success);
    };

    struct error
    {
        static constexpr auto frame_id = frame_id_t(message_id::error);

        char reason[256];
    };
}

#endif
