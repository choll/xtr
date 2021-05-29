// Copyright 2020 Chris E. Holloway
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

#ifndef XTR_DETAIL_TRAMPOLINES_HPP
#define XTR_DETAIL_TRAMPOLINES_HPP

#include "align.hpp"

#include <fmt/format.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>

namespace xtr::detail
{
    // trampoline_no_capture
    // trampoline_fixed_size_capture
    // trampoline_variable_size_capture

    template<auto Format, typename State>
    std::byte* trampoline0(
        fmt::memory_buffer& mbuf,
        std::byte* buf,
        State& state,
        const char* ts,
        std::string& name) noexcept
    {
        print(mbuf, state.out, state.err, *Format, ts, name);
        return buf + sizeof(void(*)());
    }

    template<auto Format, typename State, typename Func>
    std::byte* trampolineN(
        fmt::memory_buffer& mbuf,
        std::byte* buf,
        State& state,
        [[maybe_unused]] const char* ts,
        std::string& name) noexcept
    {
        typedef void(*fptr_t)();

        auto func_pos = buf + sizeof(fptr_t);
        if constexpr (alignof(Func) > alignof(fptr_t))
            func_pos = align<alignof(Func)>(func_pos);

        assert(std::uintptr_t(func_pos) % alignof(Func) == 0);

        // Invoke lambda, the first call is for commands sent to the consumer
        // thread such as adding a new producer or modifying the output stream.
        auto& func = *reinterpret_cast<Func*>(func_pos);
        if constexpr (std::is_same_v<decltype(Format), std::nullptr_t>)
            func(state, name);
        else
            func(mbuf, state.out, state.err, *Format, ts, name);

        static_assert(noexcept(func.~Func()));
        std::destroy_at(std::addressof(func));

        return func_pos + align(sizeof(Func), alignof(fptr_t));
    }

    template<auto Format, typename State, typename Func>
    std::byte* trampolineS(
        fmt::memory_buffer& mbuf,
        std::byte* buf,
        State& state,
        const char* ts,
        std::string& name) noexcept
    {
        typedef void(*fptr_t)();

        auto size_pos = buf + sizeof(fptr_t);
        assert(std::uintptr_t(size_pos) % alignof(std::size_t) == 0);

        auto func_pos = size_pos + sizeof(std::size_t);
        if constexpr (alignof(Func) > alignof(std::size_t))
            func_pos = align<alignof(Func)>(func_pos);
        assert(std::uintptr_t(func_pos) % alignof(Func) == 0);

        auto& func = *reinterpret_cast<Func*>(func_pos);
        func(mbuf, state.out, state.err, *Format, ts, name);

        static_assert(noexcept(func.~Func()));
        std::destroy_at(std::addressof(func));

        return buf + *reinterpret_cast<const std::size_t*>(size_pos);
    }
}

#endif
