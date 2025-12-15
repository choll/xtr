// Copyright 2021 Chris E. Holloway
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

#ifndef XTR_DETAIL_COMMANDS_RECV_HPP
#define XTR_DETAIL_COMMANDS_RECV_HPP

#include "xtr/detail/commands/frame.hpp"
#include "xtr/detail/retry.hpp"

#include <sys/socket.h>
#include <sys/types.h>

namespace xtr::detail
{
    [[nodiscard]] ::ssize_t command_recv(int fd, frame_buf& buf);
}

inline ::ssize_t xtr::detail::command_recv(int fd, frame_buf& buf)
{
    ::msghdr hdr{};
    ::iovec iov;

    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;

    iov.iov_base = &buf;
    iov.iov_len = sizeof(buf);

    return XTR_TEMP_FAILURE_RETRY(::recvmsg(fd, &hdr, 0));
}

#endif
