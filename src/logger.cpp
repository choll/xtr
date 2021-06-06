// Copyright 2019, 2020, 2021 Chris E. Holloway
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

#include "xtr/logger.hpp"
#include "xtr/detail/commands/command_dispatcher.hpp"
#include "xtr/detail/commands/matcher.hpp"
#include "xtr/detail/commands/requests.hpp"
#include "xtr/detail/commands/responses.hpp"
#include "xtr/detail/config.hpp"
#include "xtr/detail/strzcpy.hpp"
#include "xtr/detail/tsc.hpp"

#include <fmt/chrono.h>

#include <algorithm>
#include <climits>
#include <cstring>
#include <condition_variable>

XTR_FUNC
xtr::logger::~logger()
{
    control_.close();
}

XTR_FUNC
xtr::logger::producer xtr::logger::get_producer(std::string name)
{
    return producer(*this, std::move(name));
}

XTR_FUNC
void xtr::logger::register_producer(
    producer& p,
    const std::string& name) noexcept
{
    post(
        [&p, name](consumer& c, auto&)
        {
            c.add_producer(p, name);
        });
}

XTR_FUNC
void xtr::logger::set_output_stream(FILE* stream) noexcept
{
    set_output_function(detail::make_output_func(stream));
}

XTR_FUNC
void xtr::logger::set_error_stream(FILE* stream) noexcept
{
    set_error_function(detail::make_error_func(stream));
}

XTR_FUNC
void xtr::logger::set_command_path(std::string path) noexcept
{
    post([p = std::move(path)](consumer& c, auto&) { c.set_command_path(std::move(p)); });
    control_.sync();
}

XTR_FUNC
void xtr::logger::consumer::run(std::function<::timespec()> clock) noexcept
{
    char ts[32] = {};
    bool ts_stale = true;
    std::size_t flush_count = 0;
    fmt::memory_buffer mbuf;

    for (std::size_t i = 0; !producers_.empty(); ++i)
    {
        ring_buffer::span span;
        // The inner loop below can modify producers so a reference cannot be taken
        const std::size_t n = i % producers_.size();

        if (n == 0)
        {
            // Read the clock and commands once per loop over producers
            ts_stale |= true;
            if (cmds_)
                cmds_->process_commands(/* timeout= */0);
        }

        if ((span = producers_[n]->buf_.read_span()).empty())
        {
            // flush if no further data available (all producers empty)
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
        // producer always begins log records in the first mapping, so we do not
        // read a record beginning in the second mapping). This is done to
        // avoid undefined behaviour---reading an object from a different
        // address than it was written to will work on Intel and probably many
        // other CPUs but is outside of what is permitted by the C++ memory
        // model.
        std::byte* pos = span.begin();
        std::byte* end = std::min(span.end(), producers_[n]->buf_.end());
        do
        {
            assert(std::uintptr_t(pos) % alignof(fptr_t) == 0);
            fptr_t fptr = *reinterpret_cast<const fptr_t*>(pos);
            pos = fptr(mbuf, pos, *this, ts, producers_[n].name);
        } while (pos < end && !destroy);

        producers_[n]->buf_.reduce_readable(
            ring_buffer::size_type(pos - span.begin()));

        if (destroy)
        {
            using std::swap;
            swap(producers_[n], producers_.back()); // possible self-swap, ok
            producers_.pop_back();
            continue;
        }

        std::size_t n_dropped;
        if (producers_[n]->buf_.read_span().empty() &&
            (n_dropped = producers_[n]->buf_.dropped_count()) > 0)
        {
            detail::print(
                mbuf,
                out,
                err,
                "W {} {}: {} messages dropped\n",
                ts,
                producers_[n].name,
                n_dropped);
            producers_[n].dropped_count += n_dropped;
        }

        // flushing may be late/early if producers is modified, doesn't matter
        flush_count = producers_.size();
    }

    close();
}

XTR_FUNC
void xtr::logger::consumer::add_producer(producer& p, const std::string& name)
{
    producers_.push_back(producer_handle{&p, name});
}

XTR_FUNC
void xtr::logger::consumer::set_command_path(std::string path) noexcept
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
    cmds_->register_callback<detail::status>(
        std::bind_front(&consumer::status_handler, this));

    cmds_->register_callback<detail::set_level>(
        std::bind_front(&consumer::set_level_handler, this));

    cmds_->register_callback<detail::reopen>(
        std::bind_front(&consumer::reopen_handler, this));
}

XTR_FUNC
void xtr::logger::consumer::status_handler(int fd, detail::status& st)
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

    for (std::size_t i = 1; i < producers_.size(); ++i)
    {
        auto& p = producers_[i];

        if (!(*matcher)(p.name.c_str()))
            continue;

        detail::frame<detail::sink_info> sif;

        sif->level = p->level();
        sif->buf_capacity = p->buf_.capacity();
        sif->buf_nbytes = p->buf_.read_span().size();
        sif->dropped_count = p.dropped_count;
        detail::strzcpy(sif->name, p.name);

        cmds_->send(fd, sif);
    }
}

XTR_FUNC
void xtr::logger::consumer::set_level_handler(int fd, detail::set_level& sl)
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

    for (std::size_t i = 1; i < producers_.size(); ++i)
    {
        auto& p = producers_[i];

        if (!(*matcher)(p.name.c_str()))
            continue;

        p->set_level(sl.level);
    }

    cmds_->send(fd, detail::frame<detail::success>());
}

XTR_FUNC
void xtr::logger::consumer::reopen_handler(int fd, detail::reopen&)
{
    if (!reopen())
        cmds_->send_error(fd, std::strerror(errno));
    else
        cmds_->send(fd, detail::frame<detail::success>());
}

XTR_FUNC
xtr::logger::producer::producer(const producer& other)
{
    *this = other;
}

XTR_FUNC
xtr::logger::producer& xtr::logger::producer::operator=(const producer& other)
{
    level_ = other.level_.load(std::memory_order_relaxed);
    if (!std::exchange(open_, other.open_)) // if previously closed, register
    {
        const_cast<producer&>(other).post(
            [this](consumer& c, const auto& name) { c.add_producer(*this, name); });
    }
    return *this;
}

XTR_FUNC
xtr::logger::producer::producer(logger& owner, std::string name)
:
    open_(true)
{
    owner.register_producer(*this, name);
}

XTR_FUNC
void xtr::logger::producer::close()
{
    if (open_)
    {
        sync(/*destruct=*/true);
        open_ = false;
#if defined(XTR_THREAD_SANITIZER_ENABLED)
    // This is done to prevent thread sanitizer complaining that the last
    // release operation performed by the consumer was not synchronized with a
    // corresponding acquire operation. This will happen if the memory used
    // for synchronized_ring_buffer's atomic variables are accessed
    // non-atomically, e.g. by free(3) or when stack addresses are reused.
    // As the last operation the consumer calls is reduce_readable, we must
    // call write_span to 'balance' the atomic operations.

    // Wait for consumer to perform the offending release
    while (buf_.read_span().size() > 0)
        ;
    // Perform matching acquire operation
    buf_.write_span();
#endif
    }
}

XTR_FUNC
void xtr::logger::producer::sync(bool destroy)
{
    std::condition_variable cv;
    std::mutex m;
    bool notified = false; // protected by m

    post(
        [&cv, &m, &notified, destroy](consumer& c, auto&)
        {
            c.destroy = destroy;

            c.flush();
            c.sync();

            std::scoped_lock lock{m};
            notified = true;
            // Do not move this notify outside of the protection of m. The
            // standard guarantees that a mutex may be destructed while
            // another thread is still inside unlock (but does not hold the
            // lock). From the mutex requirements:
            //
            // ``Note: After a thread A has called unlock(), releasing a
            // mutex, it is possible for another thread B to lock the same
            // mutex, observe that it is no longer in use, unlock it, and
            // destroy it, before thread A appears to have returned from
            // its unlock call. Implementations are required to handle such
            // scenarios correctly, as long as thread A doesn't access the
            // mutex after the unlock call returns.''
            //
            // No such requirement exists for condition_variable and notify,
            // which may access memory (e.g. an internal mutex in pthreads) in
            // the signalling thread after the waiting thread has woken up---so
            // if the lock is not held, the condition_variable could already
            // have been destructed at this time (due to the stack being
            // unwound).
            cv.notify_one();
            // Do not access any captured variables after notifying because if
            // the producer is destructing then the underlying storage may have
            // been freed already.
        });

    std::unique_lock lock{m};
    while (!notified)
        cv.wait(lock);
}

XTR_FUNC
void xtr::logger::producer::set_name(std::string name)
{
    post(
        [name = std::move(name)](auto&, auto& oldname)
        {
            oldname = std::move(name);
        });
    sync();
}

XTR_FUNC
xtr::logger::producer::~producer()
{
    close();
}
