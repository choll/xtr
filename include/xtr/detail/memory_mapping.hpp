// Copyright 2014, 2015, 2019 Chris E. Holloway
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

#ifndef XTR_DETAIL_MEMORY_MEMORY_MAPPING_HPP
#define XTR_DETAIL_MEMORY_MEMORY_MAPPING_HPP

#include <cstddef>
#include <utility>

#include <sys/mman.h>

namespace xtr::detail
{
    class memory_mapping;
}

class xtr::detail::memory_mapping
{
public:
    memory_mapping() = default;

    memory_mapping(
        void* addr,
        std::size_t length,
        int prot,
        int flags,
        int fd = - 1,
        std::size_t offset = 0);

    memory_mapping(const memory_mapping&) = delete;

    memory_mapping(memory_mapping&& other) noexcept;

    memory_mapping& operator=(const memory_mapping&) = delete;

    memory_mapping& operator=(memory_mapping&& other) noexcept;

    ~memory_mapping();

    void reset(void* addr = MAP_FAILED, std::size_t length = 0) noexcept;

    void release() noexcept
    {
        mem_ = MAP_FAILED;
    }

    void* get()
    {
        return mem_;
    }

    const void* get() const
    {
        return mem_;
    }

    std::size_t length() const
    {
        return length_;
    }

    explicit operator bool() const
    {
        return mem_ != MAP_FAILED;
    }

private:
    void* mem_ = MAP_FAILED;
    std::size_t length_{};

    friend void swap(memory_mapping& a, memory_mapping& b) noexcept
    {
        using std::swap;
        swap(a.mem_, b.mem_);
        swap(a.length_, b.length_);
    }
};

#endif
