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

#ifndef XTR_DETAIL_COMMANDS_CONNECT_HPP
#define XTR_DETAIL_COMMANDS_CONNECT_HPP

#include "xtr/detail/file_descriptor.hpp"
#include "xtr/detail/strzcpy.hpp"

#include <string_view>

#include <sys/socket.h>
#include <sys/un.h>

namespace xtr::detail
{
    [[nodiscard]] file_descriptor command_connect(std::string_view path);
}

inline xtr::detail::file_descriptor xtr::detail::command_connect(std::string_view path)
{
    file_descriptor fd(::socket(AF_LOCAL, SOCK_SEQPACKET, 0));

    if (!fd)
        return {};

    sockaddr_un addr;
    addr.sun_family = AF_LOCAL;

    if (path.size() >= sizeof(addr.sun_path))
    {
        errno = ENAMETOOLONG;
        return {};
    }

    strzcpy(addr.sun_path, path);

#if defined(__linux__)
    if (addr.sun_path[0] == '\0') // abstract socket
    {
        std::memset(
            addr.sun_path + path.size(),
            '\0',
            sizeof(addr.sun_path) - path.size());
    }
#endif

    if (::connect(fd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
        return {};

    return fd;
}

#endif
