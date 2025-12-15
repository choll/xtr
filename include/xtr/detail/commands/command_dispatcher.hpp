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

#ifndef XTR_DETAIL_COMMANDS_COMMAND_DISPATCHER_HPP
#define XTR_DETAIL_COMMANDS_COMMAND_DISPATCHER_HPP

#include "frame.hpp"
#include "xtr/detail/file_descriptor.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <poll.h>

namespace xtr::detail
{
    class command_dispatcher;
}

class xtr::detail::command_dispatcher
{
private:
    struct pollfd
    {
        detail::file_descriptor fd;
        short events;
        short revents;
    };

    static_assert(sizeof(pollfd) == sizeof(::pollfd));
    static_assert(alignof(pollfd) == alignof(::pollfd));

    struct buffer
    {
        buffer(const void* srcbuf, std::size_t srcsize);

        std::unique_ptr<char[]> buf;
        std::size_t size;
    };

    struct callback_result
    {
        std::vector<buffer> bufs;
        std::size_t pos = 0;
    };

    struct callback
    {
        std::function<void(int, void*)> func;
        std::size_t size;
    };

public:
    explicit command_dispatcher(std::string path);

    ~command_dispatcher();

    template<typename Payload, typename Callback>
    void register_callback(Callback&& c)
    {
        using frame_type = detail::frame<Payload>;

        static_assert(sizeof(frame_type) <= max_frame_size);
        static_assert(alignof(frame_type) <= max_frame_alignment);

        callbacks_[Payload::frame_id] = callback{
            [c = std::forward<Callback>(c)](int fd, void* buf)
            { c(fd, static_cast<frame_type*>(buf)->payload); },
            sizeof(frame_type)};
    }

    void send(int fd, const void* buf, std::size_t nbytes);

    template<typename FrameType>
    void send(int fd, const FrameType& frame)
    {
        send(fd, &frame, sizeof(frame));
    }

    void send_error(int fd, std::string_view reason);

    void process_commands(int timeout) noexcept;

    bool is_open() const noexcept
    {
        return !pollfds_.empty();
    }

private:
    void process_socket_read(pollfd& pfd) noexcept;
    void process_socket_write(pollfd& pfd) noexcept;
    void disconnect(pollfd& pfd) noexcept;

    std::unordered_map<frame_id_t, callback> callbacks_;
    std::vector<pollfd> pollfds_;
    std::unordered_map<int, callback_result> results_;
    std::string path_;
};

#endif
