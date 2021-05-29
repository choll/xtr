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

#ifndef XTR_DETAIL_COMMANDS_FRAME_HPP
#define XTR_DETAIL_COMMANDS_FRAME_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace xtr::detail
{
    using frame_id_t = std::uint32_t;

    struct frame_header
    {
        frame_id_t frame_id;
    };

    inline constexpr std::size_t max_frame_alignment = alignof(std::max_align_t);
    inline constexpr std::size_t max_frame_size = 512;

    union alignas(max_frame_alignment) frame_buf
    {
        frame_header hdr;
        char buf[max_frame_size];
    };

    template<typename Payload>
    struct frame : frame_header
    {
        static_assert(std::is_standard_layout_v<Payload>);
        static_assert(std::is_trivially_destructible_v<Payload>);
        static_assert(std::is_trivially_copyable_v<Payload>);

        using payload_type = Payload;

        frame()
        {
            static_assert(alignof(frame<Payload>) <= max_frame_alignment);
            static_assert(sizeof(frame<Payload>) <= max_frame_size);

            frame_id = Payload::frame_id;
        }

        Payload* operator->()
        {
            return &payload;
        }

        [[no_unique_address]] Payload payload{};
    };
}

#endif
