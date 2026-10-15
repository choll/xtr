// Copyright 2026 Chris E. Holloway
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

#include "xtr/detail/prefault.hpp"
#include "xtr/detail/pagesize.hpp"

#include <sys/mman.h>

XTR_FUNC
void xtr::detail::prefault_rw(void* addr, std::size_t length)
{
#if defined(MADV_POPULATE_WRITE)
    if (::madvise(addr, length, MADV_POPULATE_WRITE) == 0)
        return;
#endif
    // Perform a non-destructive read/write access on each page.
    volatile std::byte* const p = static_cast<volatile std::byte*>(addr);
    const std::size_t page_size = align_to_page_size(1);
    for (std::size_t i = 0; i < length; i += page_size)
        p[i] = p[i];
}
