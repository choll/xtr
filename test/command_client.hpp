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

#ifndef XTR_DETAIL_COMMAND_CLIENT_HPP
#define XTR_DETAIL_COMMAND_CLIENT_HPP

#include "xtr/detail/commands/frame.hpp"
#include "xtr/detail/commands/send.hpp"
#include "xtr/detail/commands/recv.hpp"
#include "xtr/detail/file_descriptor.hpp"
#include "xtr/detail/throw.hpp"

#include <string>
#include <vector>

#include <sys/types.h>

namespace xtr::detail
{
    struct command_client
    {
        void connect(const std::string& path);

        void reconnect();

        template<typename Frame>
        void send_frame(const Frame& frame)
        {
            const ::ssize_t nwritten = command_send(fd_.get(), &frame, sizeof(frame));
            if (nwritten != sizeof(frame))
                throw_system_error_fmt("Error writing to socket");
        }

        template<typename Reply>
        auto read_reply()
        {
            frame_buf buf;
            ::ssize_t nread;
            std::vector<Reply> replies;

            while ((nread = command_recv(fd_.get(), buf)) == sizeof(frame<Reply>))
            {
                if (buf.hdr.frame_id != frame_id_t(Reply::frame_id))
                {
                    throw_runtime_error_fmt(
                        "Unexpected frame id, expected %u, received %u",
                        Reply::frame_id, buf.hdr.frame_id);
                }
                replies.push_back(reinterpret_cast<frame<Reply>&>(buf).payload);
            }

            if (nread < 0)
                throw_system_error_fmt("Error reading from socket");

            if (nread != 0)
            {
                throw_runtime_error_fmt(
                    "Error reading from socket: Expected %zu bytes, received %zu",
                    sizeof(frame<Reply>), nread);
            }

            return replies;
        }

        template<typename Reply, typename Frame>
        auto send_frame(const Frame& frame)
        {
            send_frame(frame);
            return read_reply<Reply>();
        }

        std::string cmd_path_;
        file_descriptor fd_;
    };
}

#endif
