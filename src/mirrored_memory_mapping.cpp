// Copyright 2014, 2015, 2016, 2019, 2020 Chris E. Holloway
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

#include "xtr/detail/mirrored_memory_mapping.hpp"
#include "xtr/detail/file_descriptor.hpp"
#include "xtr/detail/pagesize.hpp"
#include "xtr/detail/retry.hpp"
#include "xtr/detail/throw.hpp"

#include <cassert>
#include <cstdlib>
#include <random>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#if !defined(__linux__)
namespace xtr::detail
{
    XTR_FUNC
    file_descriptor shm_open_anon(int oflag, mode_t mode)
    {
        int fd;

#if defined(SHM_ANON) // FreeBSD extension
        fd = XTR_TEMP_FAILURE_RETRY(::shm_open(SHM_ANON, oflag, mode));
#else
        // Some platforms don't allow slashes in shm object names except
        // for the first character, hence not using the usual base64 table
        const char ctable[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789~-";
        char name[] = "/xtr.XXXXXXXXXXXXXXXX";

        std::random_device rd;
        std::uniform_int_distribution<> udist(0, sizeof(ctable) - 2);

        // As there is no way to create an anonymous shm object we generate
        // random names, retrying up to 64 times if the name already exists.
        std::size_t retries = 64;
        do
        {
            for (char* pos = name + 5; *pos != '\0'; ++pos)
                *pos = ctable[udist(rd)];
            fd = XTR_TEMP_FAILURE_RETRY(::shm_open(name, oflag|O_EXCL|O_CREAT, mode));
        }
        while (--retries > 0 && fd == -1 && errno == EEXIST);

        if (fd != -1)
            ::shm_unlink(name);
#endif

        return file_descriptor(fd);
    }
}
#endif

XTR_FUNC
xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping(
    std::size_t length,
    int fd,
    std::size_t offset,
    int flags)
{
    assert(!(flags & MAP_ANONYMOUS) || fd == -1);
    assert((flags & MAP_FIXED) == 0); // Not implemented (would be easy though)
    assert((flags & MAP_PRIVATE) == 0); // Can't be private, must be shared for mirroring to work

    // length is not automatically rounded up because it would make the class
    // error prone---mirroring would not take place where the user expects.
    if (length != align_to_page_size(length))
    {
        throw_invalid_argument(
            "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping: "
            "Length argument is not page-aligned");
    }

    // To create two adjacent mappings without race conditions:
    // 1.) Create a mapping A of twice the requested size, L*2
    // 2.) Create a mapping of size L at A+L using MAP_FIXED
    // 3.) Create a mapping at of size L at A using MAP_FIXED, this will
    //     also destroy the mapping created in (1).

    const int prot = PROT_READ|PROT_WRITE;

    memory_mapping reserve(
        nullptr,
        length * 2,
        prot,
        MAP_PRIVATE|MAP_ANONYMOUS);

#if !defined(__linux__)
    file_descriptor temp_fd;
#endif

    if (fd == -1)
    {
#if defined(__linux__)
        // Note that it is possible to just do a single mmap of length * 2 for
        // m_ then an mremap of length with target address m_.get() + length
        // for mirror. I have not done this just to make the rest of the class
        // cleaner, as with the below method we don't have to deal with the
        // first mapping's length being length * 2.
        memory_mapping mirror(
            static_cast<std::byte*>(reserve.get()) + length,
            length,
            prot,
            MAP_FIXED|MAP_SHARED|MAP_ANONYMOUS|flags);

        m_.reset(
            ::mremap(
                mirror.get(),
                0,
                length,
                MREMAP_FIXED|MREMAP_MAYMOVE,
                reserve.get()),
            length);

        if (!m_)
        {
            throw_system_error(
                "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping: "
                "mremap failed");
        }

        reserve.release(); // mapping was destroyed by mremap
        mirror.release(); // mirror will be recreated in ~mirrored_memory_mapping
        return;
#else
        if (!(temp_fd = shm_open_anon(O_RDWR, S_IRUSR|S_IWUSR)))
        {
            throw_system_error(
                "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping: "
                "Failed to shm_open backing file");
        }

        fd = temp_fd.get();

        if (::ftruncate(fd, ::off_t(length)) == -1)
        {
            throw_system_error(
                "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping: "
                "Failed to ftruncate backing file");
        }
#endif
    }

    flags &= ~MAP_ANONYMOUS; // Mappings must be shared
    flags |= MAP_FIXED | MAP_SHARED;

    memory_mapping mirror(
        static_cast<std::byte*>(reserve.get()) + length,
        length,
        prot,
        flags,
        fd,
        offset);

    m_ = memory_mapping(reserve.get(), length, prot, flags, fd, offset);

    reserve.release(); // mapping was destroyed when m_ was created
    mirror.release(); // mirror will be recreated in ~mirrored_memory_mapping
}

XTR_FUNC
xtr::detail::mirrored_memory_mapping::~mirrored_memory_mapping()
{
    if (m_)
    {
        memory_mapping{}.reset(
            static_cast<std::byte*>(m_.get()) + m_.length(),
            m_.length());
    }
}

