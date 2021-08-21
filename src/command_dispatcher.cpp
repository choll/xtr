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

#include "xtr/detail/commands/command_dispatcher.hpp"
#include "xtr/detail/commands/command_dispatcher_fwd.hpp"
#include "xtr/detail/commands/frame.hpp"
#include "xtr/detail/commands/recv.hpp"
#include "xtr/detail/commands/responses.hpp"
#include "xtr/detail/commands/send.hpp"
#include "xtr/detail/strzcpy.hpp"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <exception>
#include <iostream>
#include <string_view>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace xtr::detail
{
    template<typename... Args>
    void errx(Args&&... args)
    {
        (std::cerr << ... << args) << "\n";
    }

    template<typename... Args>
    void err(Args&&... args)
    {
        const int errnum = errno;
        errx(std::forward<Args>(args)..., ": ", std::strerror(errnum));
    }
}

XTR_FUNC
xtr::detail::command_dispatcher::command_dispatcher(std::string path)
{
    sockaddr_un addr;

    // Exceptions aren't thrown here for errors relating to the user supplied
    // path to avoid blowing up if exceptions are disabled (as the throw
    // functions will abort in that case). For example if there is a
    // permissions error on opening the socket then aborting would be quite
    // excessive.

    if (path.size() > sizeof(addr.sun_path) - 1)
    {
        errx("Error: Command path '", path, "' is too long");
        return;
    }

    detail::file_descriptor fd(
        ::socket(AF_LOCAL, SOCK_SEQPACKET|SOCK_NONBLOCK, 0));

    if (!fd)
    {
        err("Error: Failed to create command socket");
        return;
    }

    addr.sun_family = AF_LOCAL;
    std::memcpy(addr.sun_path, path.c_str(), path.size() + 1);

#if defined(__linux__)
    if (addr.sun_path[0] == '\0') // abstract socket
    {
        std::memset(
            addr.sun_path + path.size(),
            '\0',
            sizeof(addr.sun_path) - path.size());
    }
#endif

    if (::bind(fd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
    {
        err("Error: Failed to bind command socket to path '", path, "'");
        return;
    }

    path_ = std::move(path);

    if (::listen(fd.get(), 64) == -1)
    {
        err("Error: Failed to listen on command socket");
        return;
    }

    pollfds_.push_back(pollfd{std::move(fd), POLLIN, 0});
}

XTR_FUNC
xtr::detail::command_dispatcher::~command_dispatcher()
{
    if (!path_.empty())
        ::unlink(path_.c_str());
}

XTR_FUNC
xtr::detail::command_dispatcher::buffer::buffer(
    const void* srcbuf, std::size_t srcsize)
:
    buf(new char[srcsize]),
    size(srcsize)
{
    std::memcpy(buf.get(), srcbuf, size);
}

XTR_FUNC
void xtr::detail::command_dispatcher::send(
    int fd,
    const void* buf,
    std::size_t nbytes)
{
    results_[fd].bufs.emplace_back(buf, nbytes);
}

XTR_FUNC
void xtr::detail::command_dispatcher::process_commands(int timeout) noexcept
{
    int nfds =
        ::poll(
            reinterpret_cast<::pollfd*>(&pollfds_[0]),
            ::nfds_t(pollfds_.size()),
            timeout);

    if (pollfds_[0].revents & POLLIN)
    {
        detail::file_descriptor fd(
            ::accept4(pollfds_[0].fd.get(), nullptr, nullptr, SOCK_NONBLOCK));
        if (fd)
            pollfds_.push_back(pollfd{std::move(fd), POLLIN, 0});
        else
            err("Error: Failed to accept connection on command socket");
        --nfds;
    }

    for (std::size_t i = 1; i < pollfds_.size() && nfds > 0; ++i)
    {
        const int fd = pollfds_[i].fd.get();
        if (pollfds_[i].revents & POLLOUT)
        {
            process_socket_write(pollfds_[i]);
            --nfds;
        }
        else if (pollfds_[i].revents & (POLLHUP|POLLIN))
        {
            process_socket_read(pollfds_[i]);
            --nfds;
        }
        if (fd != pollfds_[i].fd.get())
            --i; // Adjust for erased item
    }
}

XTR_FUNC
void xtr::detail::command_dispatcher::process_socket_read(pollfd& pfd) noexcept
{
    const int fd = pfd.fd.get();
    frame_buf buf;

    const ::ssize_t nbytes = command_recv(fd, buf);

    // Set events to POLLOUT in order to (1) process any replies generated by
    // the callback (via send()) which will be sent by process_socket_write,
    // and (2) to disconnect the socket, which will also be done by
    // process_socket_write, once all replies have been sent (if any).
    pfd.events = POLLOUT;

    if (nbytes == -1)
    {
        err("Error: Failed to read command");
        return;
    }

    if (nbytes == 0) // EOF
        return;

    if (nbytes < ::ssize_t(sizeof(frame_header)))
    {
        send_error(fd, "Incomplete frame header");
        return;
    }

    const auto cpos = callbacks_.find(buf.hdr.frame_id);

    if (cpos == callbacks_.end())
    {
        send_error(fd, "Invalid frame id");
        return;
    }

    if (nbytes != ::ssize_t(cpos->second.size))
    {
        send_error(fd, "Invalid frame length");
        return;
    }

#if __cpp_exceptions
    try
    {
#endif
        cpos->second.func(fd, &buf);
#if __cpp_exceptions
    }
    catch (const std::exception& e)
    {
        send_error(fd, e.what());
    }
#endif
}

XTR_FUNC
void xtr::detail::command_dispatcher::process_socket_write(pollfd& pfd) noexcept
{
    const int fd = pfd.fd.get();

    callback_result& cr = results_[fd];

    ::ssize_t nwritten = 0;

    for (; cr.pos < cr.bufs.size(); ++cr.pos)
    {
        nwritten =
            command_send(fd, cr.bufs[cr.pos].buf.get(), cr.bufs[cr.pos].size);
        if (nwritten != ::ssize_t(cr.bufs[cr.pos].size))
            break;
    }

    if ((nwritten == -1 && errno != EAGAIN) || cr.pos == cr.bufs.size())
    {
        results_.erase(fd);
        disconnect(pfd);
    }
}

XTR_FUNC
void xtr::detail::command_dispatcher::disconnect(pollfd& pfd) noexcept
{
    assert(results_.count(pfd.fd.get()) == 0);
    std::swap(pfd, pollfds_.back());
    pollfds_.pop_back();
}

XTR_FUNC
void xtr::detail::command_dispatcher::send_error(int fd, std::string_view reason)
{
    frame<error> ef;
    strzcpy(ef->reason, reason);
    send(fd, ef);
}

XTR_FUNC
void xtr::detail::command_dispatcher_deleter::operator()(command_dispatcher* d)
{
    delete d;
}
