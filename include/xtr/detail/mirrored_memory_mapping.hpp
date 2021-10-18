// Copyright 2014, 2015, 2016, 2019 Chris E. Holloway
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

#ifndef XTR_DETAIL_MEMORY_MIRRORED_MEMORY_MAPPING_HPP
#define XTR_DETAIL_MEMORY_MIRRORED_MEMORY_MAPPING_HPP

#include "memory_mapping.hpp"

#include <cstddef>

// Creates two adjacent memory mappings which map to the same underlying memory.
// This is useful for implementing a ring buffer where the producer/consumer do
// not need to be aware that the underlying memory is a ring buffer. This allows
// for example read(2) and write(2) to use the buffer without an intermediate copy.
//
// This is done via mmap(2). Apparently VirtualAlloc2 can acheive this on
// Windows via MEM_RESERVE_PLACEHOLDER and MEM_REPLACE_PLACEHOLDER:
// https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc2

namespace xtr::detail
{
    class mirrored_memory_mapping;
}

class xtr::detail::mirrored_memory_mapping
{
public:
    mirrored_memory_mapping() = default;
    mirrored_memory_mapping(mirrored_memory_mapping&&) = default;
    mirrored_memory_mapping& operator=(mirrored_memory_mapping&&) = default;

    explicit mirrored_memory_mapping(
        std::size_t length, // must be multiple of page size
        int fd = -1,
        std::size_t offset = 0, // must be multiple of page size
        int flags = 0);

    ~mirrored_memory_mapping();

    void* get()
    {
        return m_.get();
    }

    const void* get() const
    {
        return m_.get();
    }

    std::size_t length() const
    {
        return m_.length();
    }

    explicit operator bool() const
    {
        return !!m_;
    }

private:
    memory_mapping m_;
};

#endif
