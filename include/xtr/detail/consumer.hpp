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

#ifndef XTR_DETAIL_CONSUMER_HPP
#define XTR_DETAIL_CONSUMER_HPP

#include "buffer.hpp"
#include "commands/command_dispatcher_fwd.hpp"
#include "commands/requests_fwd.hpp"

#include <ctime>
#include <cstddef>
#include <functional>
#include <latch>
#include <memory>
#include <string>
#include <vector>

#include <unistd.h>

namespace xtr
{
    class sink;

    namespace detail
    {
        class consumer;
    }
}

class xtr::detail::consumer
{
private:
    struct sink_handle
    {
        sink* operator->()
        {
            return p;
        }

        sink* p;
        std::string name;
        std::size_t dropped_count = 0;
    };

public:
    void run() noexcept;
    bool run_once() noexcept;
    void set_command_path(std::string path) noexcept;

    consumer(
        buffer&& bf,
        sink* control,
        std::string command_path,
        std::function<std::timespec()> clock)
    :
        buf(std::move(bf)),
        clock_(std::move(clock)),
        sinks_({{control, "control", 0}})
    {
        set_command_path(std::move(command_path));
    }

    ~consumer();

    consumer(const consumer&) = delete;
    consumer(consumer&&) = delete;

    consumer& operator=(const consumer&) = delete;
    consumer& operator=(consumer&&) = delete;

    void add_sink(sink& s, const std::string& name);

    buffer buf;
    bool destroy = false;

private:
    void status_handler(int fd, detail::status&);
    void set_level_handler(int fd, detail::set_level&);
    void reopen_handler(int fd, detail::reopen&);

    std::function<std::timespec()> clock_;
    std::vector<sink_handle> sinks_;
    std::unique_ptr<
        detail::command_dispatcher,
        detail::command_dispatcher_deleter> cmds_;
    std::size_t flush_count_ = 0;
    std::latch destruct_latch_{1};
};

#endif
