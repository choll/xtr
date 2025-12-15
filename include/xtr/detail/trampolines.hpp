// Copyright 2020 Chris E. Holloway
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

#ifndef XTR_DETAIL_TRAMPOLINES_HPP
#define XTR_DETAIL_TRAMPOLINES_HPP

#include "align.hpp"
#include "buffer.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>

namespace xtr::detail
{
    // Zero-capture trampoline---no log arguments so only a single function
    // pointer needs to be written to the queue:
    //
    //    +---------------------------+
    //    | function pointer (fptr_t) |---> trampoline0<Format, ...>
    //    +---------------------------+
    template<auto Format, auto Level, typename State>
    std::byte* trampoline0(
        buffer& buf,
        std::byte* record,
        State&,
        const char* timestamp,
        std::string& name) noexcept
    {
        print(buf, Format, Level, timestamp, name);
        return record + sizeof(void (*)());
    }

    // Fixed-size capture trampoline---log has arguments but the sizes of all
    // arguments are known at compile time. A function pointer and lambda
    // are written to the queue, with the lambda knowing the size.
    //
    //    +---------------------------+
    //    | function pointer (fptr_t) |---> trampolineN<Format, ...>
    //    +---------------------------|          |
    //    | lambda:                   | <--------+ [ trampoline invokes
    //    /    variable size          /              lambda ]
    //    /    known at compile       /
    //    |    time                   |
    //    +---------------------------+
    template<auto Format, auto Level, typename State, typename Func>
    std::byte* trampolineN(
        buffer& buf,
        std::byte* record,
        State& st,
        [[maybe_unused]] const char* timestamp,
        std::string& name) noexcept
    {
        using fptr_t = void (*)();

        auto func_pos = record + sizeof(fptr_t);
        if constexpr (alignof(Func) > alignof(fptr_t))
            func_pos = align<alignof(Func)>(func_pos);

        assert(std::uintptr_t(func_pos) % alignof(Func) == 0);

        // Invoke lambda, the first call is for commands sent to the consumer
        // thread such as adding a new producer or modifying the output stream.
        auto& func = *reinterpret_cast<Func*>(func_pos);
        if constexpr (std::is_same_v<decltype(Format), std::nullptr_t>)
            func(st, name);
        else
            func(buf, record, Format, Level, timestamp, name);

        static_assert(noexcept(func.~Func()));
        std::destroy_at(std::addressof(func));

        return func_pos + align(sizeof(Func), alignof(fptr_t));
    }

    // String capture---log has arguments, some of which are strings whose
    // length is only known at run time. A function pointer, lambda, record
    // size and string table are written to the queue:
    //
    //                   +---------------------------+
    //                   | function pointer (fptr_t) |---> trampolineS<Format, ...>
    //                   +---------------------------+          |
    //                   | lambda:                   |          | [ trampoline
    //             +-----/    variable size          /          |   invokes lambda ]
    //    string   | +---/    known at compile       / <--------+
    //    table    | |   |    time                   |
    //    entries  | |   +---------------------------+
    //             | |   | string table:             |
    //             | +-> /   variable size           /
    //             +---> /   known at run time       /
    //                   |                           |
    //                   +---------------------------+
    template<auto Format, auto Level, typename State, typename Func>
    std::byte* trampolineV(
        buffer& buf,
        std::byte* record,
        State&,
        const char* timestamp,
        std::string& name) noexcept
    {
        using fptr_t = void (*)();

        auto func_pos = record + sizeof(fptr_t);
        if constexpr (alignof(Func) > alignof(fptr_t))
            func_pos = align<alignof(Func)>(func_pos);
        assert(std::uintptr_t(func_pos) % alignof(Func) == 0);

        auto& func = *reinterpret_cast<Func*>(func_pos);
        record = func_pos + sizeof(Func);
        // record is modified by the lambda to point to the end of the string table
        func(buf, record, Format, Level, timestamp, name);

        static_assert(noexcept(func.~Func()));
        std::destroy_at(std::addressof(func));

        return align<alignof(fptr_t)>(record);
    }
}

#endif
