// Copyright 2019, 2020 Chris E. Holloway
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

#include <fmt/chrono.h>

#include <algorithm>
#include <condition_variable>
#include <cstring>

xtr::logger::producer xtr::logger::get_producer(std::string name)
{
    return producer{*this, std::move(name)};
}

void xtr::logger::register_producer(producer& p) noexcept
{
    post(
        [&p](state& st)
        {
            st.producers.push_back(&p);
        });
}

void xtr::logger::set_output_stream(FILE* stream) noexcept
{
    set_output_function(detail::make_output_func(stream));
}

void xtr::logger::set_error_stream(FILE* stream) noexcept
{
    set_error_function(detail::make_error_func(stream));
}

// XXX make this a plain function and move the file descriptor in? this
// way you cannot accidentally share data between threads
//
// Yes, just put everything inside a struct, and pass only the struct.
void xtr::logger::consumer(state st, std::function<::timespec()> clock) noexcept
{
    char ts[32] = {};
    bool ts_stale = true;
    std::size_t flush_count = 0;
    fmt::memory_buffer mbuf;

    for (std::size_t i = 0; !st.producers.empty(); ++i)
    {
        ring_buffer::span span;
        // The inner loop below can modify producers so a reference cannot be taken
        const std::size_t n = i % st.producers.size();
        // Read the clock once per loop over producers
        ts_stale |= n == 0;

        if ((span = st.producers[n]->buf_.read_span()).empty())
        {
            // flush if no further data available (all producers empty)
            if (flush_count != 0 && flush_count-- == 1)
                st.flush();
            continue;
        }

        st.destroy = false;

        if (ts_stale)
        {
            fmt::format_to(ts, "{}", xtr::timespec{clock()});
            ts_stale = false;
        }

        // span.end is capped to the end of the first mapping to guarantee that
        // data is only read from the same address that it was written to (the
        // producer always begins log records in the first mapping, so do not
        // read a record beginning in the second mapping). This is done to
        // avoid undefined behaviour---reading an object from a different
        // address than it was written to will work on Intel and probably many
        // other CPUs but is outside of what is permitted by the C++ memory
        // model.
        std::byte* pos = span.begin();
        std::byte* end = std::min(span.end(), st.producers[n]->buf_.end());
        do
        {
            assert(std::uintptr_t(pos) % alignof(fptr_t) == 0);
            fptr_t fptr = *reinterpret_cast<const fptr_t*>(pos);
            pos = fptr(mbuf, pos, st, ts, st.producers[n]->name_);
            if (st.destroy)
            {
                using std::swap;
                swap(st.producers[n], st.producers.back()); // possible self-swap, ok
                st.producers.pop_back();
                goto next;
            }
        } while (pos < end);

        st.producers[n]->buf_.reduce_readable(
            ring_buffer::size_type(pos - span.begin()));

        std::size_t n_dropped;
        if (st.producers[n]->buf_.read_span().empty() &&
            (n_dropped = st.producers[n]->buf_.dropped_count()) > 0)
        {
            detail::print(
                mbuf,
                st.out,
                st.err,
                "{}: {}: {} messages dropped\n",
                ts,
                st.producers[n]->name_,
                n_dropped);
        }

        // flushing may be late/early if producers is modified, doesn't matter
        flush_count = st.producers.size();

        next:;
    }
}

xtr::logger::producer::producer(logger& owner, std::string name)
:
    name_(std::move(name))
{
    owner.register_producer(*this);
}

void xtr::logger::producer::sync(bool destructing)
{
    std::condition_variable cv;
    std::mutex m;
    bool notified = false;

    post(
        [&cv, &m, &notified, destructing](state& st)
        {
            st.destroy = destructing;

            st.flush();
            st.sync();

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

void xtr::logger::producer::set_name(std::string name)
{
    post(
        [this, name{std::move(name)}](state&)
        {
            this->name_ = std::move(name);
        });
    sync();
}

xtr::logger::producer::~producer()
{
    sync(true);
}

