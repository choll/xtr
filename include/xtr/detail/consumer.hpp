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

#include "xtr/log_level.hpp"
#include "commands/command_dispatcher_fwd.hpp"
#include "commands/requests_fwd.hpp"

#include <ctime>
#include <cstddef>
#include <functional>
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
    void run(std::function<std::timespec()> clock) noexcept;
    void set_command_path(std::string path) noexcept;

    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename FlushFunction,
        typename SyncFunction,
        typename ReopenFunction,
        typename CloseFunction>
    consumer(
        OutputFunction&& of,
        ErrorFunction&& ef,
        FlushFunction&& ff,
        SyncFunction&& sf,
        ReopenFunction&& rf,
        CloseFunction&& cf,
        log_level_style_t ls,
        sink* control)
    :
        out(std::forward<OutputFunction>(of)),
        err(std::forward<ErrorFunction>(ef)),
        flush(std::forward<FlushFunction>(ff)),
        sync(std::forward<SyncFunction>(sf)),
        reopen(std::forward<ReopenFunction>(rf)),
        close(std::forward<CloseFunction>(cf)),
        lstyle(ls),
        sinks_({{control, "control", 0}})
    {
    }

    void add_sink(sink& p, const std::string& name);

    std::function<::ssize_t(log_level_t level, const char* buf, std::size_t size)> out;
    std::function<void(const char* buf, std::size_t size)> err;
    std::function<void()> flush;
    std::function<void()> sync;
    std::function<bool()> reopen;
    std::function<void()> close;
    log_level_style_t lstyle;
    bool destroy = false;

private:
    void status_handler(int fd, detail::status&);
    void set_level_handler(int fd, detail::set_level&);
    void reopen_handler(int fd, detail::reopen&);

    std::vector<sink_handle> sinks_;
    std::unique_ptr<
        detail::command_dispatcher,
        detail::command_dispatcher_deleter> cmds_;
};

#endif
