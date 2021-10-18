// Copyright 2014, 2015, 2019, 2020 Chris E. Holloway
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

#include "xtr/detail/memory_mapping.hpp"
#include "xtr/detail/throw.hpp"

#include <cassert>

XTR_FUNC
xtr::detail::memory_mapping::memory_mapping(
    void* addr,
    std::size_t length,
    int prot,
    int flags,
    int fd,
    std::size_t offset)
:
    mem_(::mmap(addr, length, prot, flags, fd, ::off_t(offset))),
    length_(length)
{
    if (mem_ == MAP_FAILED)
    {
        throw_system_error(
            "xtr::detail::memory_mapping::memory_mapping: mmap failed");
    }
}

XTR_FUNC
xtr::detail::memory_mapping::memory_mapping(memory_mapping&& other) noexcept
:
    mem_(other.mem_),
    length_(other.length_)
{
    other.release();
}

XTR_FUNC
xtr::detail::memory_mapping& xtr::detail::memory_mapping::operator=(
    memory_mapping&& other) noexcept
{
    swap(*this, other);
    return *this;
}

XTR_FUNC xtr::detail::memory_mapping::~memory_mapping()
{
    reset();
}

XTR_FUNC
void xtr::detail::memory_mapping::reset(void* addr, std::size_t length) noexcept
{
    if (mem_ != MAP_FAILED)
    {
#ifndef NDEBUG
        const int result =
#endif
            ::munmap(mem_, length_);
        assert(result == 0);
    }
    mem_ = addr;
    length_ = length;
}

