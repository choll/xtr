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

#include "xtr/detail/consumer.hpp"
#include "xtr/detail/commands/command_dispatcher.hpp"
#include "xtr/detail/commands/matcher.hpp"
#include "xtr/detail/commands/requests.hpp"
#include "xtr/detail/commands/responses.hpp"
#include "xtr/detail/strzcpy.hpp"
#include "xtr/command_path.hpp"
#include "xtr/log_level.hpp"
#include "xtr/sink.hpp"
#include "xtr/timespec.hpp"

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <exception>
#include <version>

XTR_FUNC
xtr::detail::consumer::~consumer()
{
#if __cpp_exceptions
    try
    {
#endif
        // Wait for run_once to signal that it has finished (all its sinks have
        // closed).
        destruct_latch_.wait();
#if __cpp_exceptions
    }
    catch(const std::exception& e)
    {
        fmt::print(
            stderr,
            FMT_COMPILE("Error destructing consumer: {}\n"),
            e.what());
    }
#endif
}

XTR_FUNC
void xtr::detail::consumer::run() noexcept
{
    while (run_once())
        ;
}

XTR_FUNC
bool xtr::detail::consumer::run_once(pump_io_stats* stats) noexcept
{
    char ts[32] = {};
    bool ts_stale = true;

    // Read commands once per loop over sinks
    if (cmds_ && cmds_->is_open())
        cmds_->process_commands(/* timeout= */0);

    std::size_t n_events = 0;

    // The inner do/while loop below can modify sinks_ so references to sinks_
    // cannot be taken here (i.e. no range-based for).
    for (std::size_t i = 0; i != sinks_.size(); ++i)
    {
        sink::ring_buffer::span span;

        if ((span = sinks_[i]->buf_.read_span()).empty())
        {
            // flush if no further data available (all sinks empty)
            if (flush_count_ != 0 && --flush_count_ == 0)
                buf.flush();
            continue;
        }

        destroy = false;

        // Read the clock once per loop over sinks
        if (ts_stale)
        {
            fmt::format_to(ts, FMT_COMPILE("{}"), xtr::timespec{clock_()});
            ts_stale = false;
        }

        // span.end is capped to the end of the first mapping to guarantee that
        // data is only read from the same address that it was written to (the
        // sink always begins log records in the first mapping, so we do not
        // read a record beginning in the second mapping). This is done to
        // avoid undefined behaviour---reading an object from a different
        // address than it was written to will work on Intel and probably many
        // other CPUs but is outside of what is permitted by the C++ memory
        // model.
        std::byte* pos = span.begin();
        std::byte* end = std::min(span.end(), sinks_[i]->buf_.end());
        do
        {
            assert(std::uintptr_t(pos) % alignof(sink::fptr_t) == 0);
            assert(!destroy);
            const sink::fptr_t fptr = *reinterpret_cast<const sink::fptr_t*>(pos);
            pos = fptr(buf, pos, *this, ts, sinks_[i].name);
            ++n_events;
        } while (pos < end);

        if (destroy)
        {
            using std::swap;
            swap(sinks_[i], sinks_.back()); // possible self-swap, ok
            sinks_.pop_back();
            --i;
            continue;
        }

        sinks_[i]->buf_.reduce_readable(
            sink::ring_buffer::size_type(pos - span.begin()));

        std::size_t n_dropped;
        if (sinks_[i]->buf_.read_span().empty() &&
            (n_dropped = sinks_[i]->buf_.dropped_count()) > 0)
        {
            detail::print(
                buf,
                FMT_COMPILE("{}{} {}: {} messages dropped\n"),
                log_level_t::warning,
                ts,
                sinks_[i].name,
                n_dropped);
            sinks_[i].dropped_count += n_dropped;
        }

        // flushing may be late/early if sinks is modified, doesn't matter
        flush_count_ = sinks_.size();
    }

    // Signal to ~consumer that it can safely destruct (run_once will not be
    // called again after it returns false).
    if (sinks_.empty())
        destruct_latch_.count_down();

    if (stats != nullptr)
        stats->n_events = n_events;

    return !sinks_.empty();
}

XTR_FUNC
void xtr::detail::consumer::add_sink(sink& s, const std::string& name)
{
    sinks_.push_back(sink_handle{&s, name});
}

XTR_FUNC
void xtr::detail::consumer::set_command_path(std::string path) noexcept
{
    if (path == null_command_path)
    {
        cmds_.reset();
        return;
    }

    cmds_.reset(new detail::command_dispatcher(std::move(path)));

    // Reset if failed to open
    if (!cmds_->is_open())
    {
        cmds_.reset();
        return;
    }

    // Set commands
#if defined(__cpp_lib_bind_front)
    cmds_->register_callback<detail::status>(
        std::bind_front(&consumer::status_handler, this));

    cmds_->register_callback<detail::set_level>(
        std::bind_front(&consumer::set_level_handler, this));

    cmds_->register_callback<detail::reopen>(
        std::bind_front(&consumer::reopen_handler, this));
#else
    // This can be removed when libc++ supports bind_front
    cmds_->register_callback<detail::status>(
        [this](auto&&... args)
        {
            status_handler(std::forward<decltype(args)>(args)...);
        });

    cmds_->register_callback<detail::set_level>(
        [this](auto&&... args)
        {
            set_level_handler(std::forward<decltype(args)>(args)...);
        });

    cmds_->register_callback<detail::reopen>(
        [this](auto&&... args)
        {
            reopen_handler(std::forward<decltype(args)>(args)...);
        });
#endif
}

XTR_FUNC
void xtr::detail::consumer::status_handler(int fd, detail::status& st)
{
    st.pattern.text[sizeof(st.pattern.text) - 1] = '\0';

    const auto matcher =
        detail::make_matcher(
            st.pattern.type, st.pattern.text, st.pattern.ignore_case);

    if (!matcher->valid())
    {
        detail::frame<detail::error> ef;
        matcher->error_reason(ef->reason, sizeof(ef->reason));
        cmds_->send(fd, ef);
        return;
    }

    for (std::size_t i = 1; i < sinks_.size(); ++i)
    {
        auto& s = sinks_[i];

        if (!(*matcher)(s.name.c_str()))
            continue;

        detail::frame<detail::sink_info> sif;

        sif->level = s->level();
        sif->buf_capacity = s->buf_.capacity();
        sif->buf_nbytes = s->buf_.read_span().size();
        sif->dropped_count = s.dropped_count;
        detail::strzcpy(sif->name, s.name);

        cmds_->send(fd, sif);
    }
}

XTR_FUNC
void xtr::detail::consumer::set_level_handler(int fd, detail::set_level& sl)
{
    sl.pattern.text[sizeof(sl.pattern.text) - 1] = '\0';

    if (sl.level > xtr::log_level_t::debug)
    {
        cmds_->send_error(fd, "Invalid level");
        return;
    }

    const auto matcher =
        detail::make_matcher(
            sl.pattern.type, sl.pattern.text, sl.pattern.ignore_case);

    if (!matcher->valid())
    {
        detail::frame<detail::error> ef;
        matcher->error_reason(ef->reason, sizeof(ef->reason));
        cmds_->send(fd, ef);
        return;
    }

    for (std::size_t i = 1; i < sinks_.size(); ++i)
    {
        auto& s = sinks_[i];

        if (!(*matcher)(s.name.c_str()))
            continue;

        s->set_level(sl.level);
    }

    cmds_->send(fd, detail::frame<detail::success>());
}

XTR_FUNC
void xtr::detail::consumer::reopen_handler(int fd, detail::reopen& /* unused */)
{
    buf.flush();
    if (const int errnum = buf.storage().reopen())
        cmds_->send_error(fd, std::strerror(errnum));
    else
        cmds_->send(fd, detail::frame<detail::success>());
}
