// Copyright 2022 Chris E. Holloway
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

#ifndef XTR_IO_STORAGE_INTERFACE_HPP
#define XTR_IO_STORAGE_INTERFACE_HPP

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>

namespace xtr
{
    struct storage_interface;

    using storage_interface_ptr = std::unique_ptr<storage_interface>;

    inline constexpr auto null_reopen_path = "";
}

struct xtr::storage_interface
{
    virtual std::span<char> allocate_buffer() = 0;

    virtual void submit_buffer(char* buf, std::size_t size) = 0;

    virtual void flush() = 0;

    virtual void sync() noexcept = 0;

    virtual int reopen() noexcept = 0;

    virtual ~storage_interface() = default;
};

#endif
