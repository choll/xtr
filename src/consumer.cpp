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
#include "xtr/sink.hpp"
#include "xtr/timespec.hpp"

#include <fmt/chrono.h>

#include <algorithm>
#include <climits>
#include <cstring>
#include <version>

XTR_FUNC
void xtr::detail::consumer::run(std::function<::timespec()> clock) noexcept
{
    char ts[32] = {};
    bool ts_stale = true;
    std::size_t flush_count = 0;
    fmt::memory_buffer mbuf;

    for (std::size_t i = 0; !sinks_.empty(); ++i)
    {
        sink::ring_buffer::span span;
        // The inner loop below can modify sinks so a reference cannot be taken
        const std::size_t n = i % sinks_.size();

        if (n == 0)
        {
            // Read the clock and commands once per loop over sinks
            ts_stale |= true;
            if (cmds_)
                cmds_->process_commands(/* timeout= */0);
        }

        if ((span = sinks_[n]->buf_.read_span()).empty())
        {
            // flush if no further data available (all sinks empty)
            if (flush_count != 0 && flush_count-- == 1)
                flush();
            continue;
        }

        destroy = false;

        if (ts_stale)
        {
            fmt::format_to(ts, "{}", xtr::timespec{clock()});
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
        std::byte* end = std::min(span.end(), sinks_[n]->buf_.end());
        do
        {
            assert(std::uintptr_t(pos) % alignof(sink::fptr_t) == 0);
            assert(!destroy);
            const sink::fptr_t fptr = *reinterpret_cast<const sink::fptr_t*>(pos);
            pos = fptr(mbuf, pos, *this, ts, sinks_[n].name);
        } while (pos < end);

        if (destroy)
        {
            using std::swap;
            swap(sinks_[n], sinks_.back()); // possible self-swap, ok
            sinks_.pop_back();
            continue;
        }

        sinks_[n]->buf_.reduce_readable(
            sink::ring_buffer::size_type(pos - span.begin()));

        std::size_t n_dropped;
        if (sinks_[n]->buf_.read_span().empty() &&
            (n_dropped = sinks_[n]->buf_.dropped_count()) > 0)
        {
            detail::print(
                mbuf,
                out,
                err,
                "W {} {}: {} messages dropped\n",
                ts,
                sinks_[n].name,
                n_dropped);
            sinks_[n].dropped_count += n_dropped;
        }

        // flushing may be late/early if sinks is modified, doesn't matter
        flush_count = sinks_.size();
    }

    close();
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
        return;

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
void xtr::detail::consumer::reopen_handler(int fd, detail::reopen&)
{
    if (!reopen())
        cmds_->send_error(fd, std::strerror(errno));
    else
        cmds_->send(fd, detail::frame<detail::success>());
}
