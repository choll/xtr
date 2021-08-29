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

#ifndef XTR_LOGGER_HPP
#define XTR_LOGGER_HPP

#include "command_path.hpp"
#include "detail/align.hpp"
#include "detail/clock_ids.hpp"
#include "detail/consumer.hpp"
#include "detail/is_c_string.hpp"
#include "detail/string_ref.hpp"
#include "detail/throw.hpp"
#include "log_macros.hpp"
#include "log_level.hpp"
#include "sink.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <atomic>
#include <concepts>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <version>

#include <unistd.h>

namespace xtr
{
    class logger;

    // This can be replaced with a template alias once clang supports it:
    // template<typename T> using nocopy = detail::string_ref<T>;
    template<typename T>
    inline auto nocopy(const T& arg)
    {
        return detail::string_ref(arg);
    }
}

namespace xtr::detail
{
    inline auto make_output_func(FILE* stream)
    {
        return
            [stream](const char* buf, std::size_t size)
            {
                return std::fwrite(buf, 1, size, stream);
            };
    }

    inline auto make_error_func(FILE* stream)
    {
        return
            [stream](const char* buf, std::size_t size)
            {
                (void)std::fwrite(buf, 1, size, stream);
            };
    }

    inline auto make_flush_func(FILE* stream, FILE* err_stream)
    {
        return
            [stream, err_stream]()
            {
                std::fflush(stream);
                std::fflush(err_stream);
            };
    }

    inline auto make_sync_func(FILE* stream, FILE* err_stream)
    {
        return
            [stream, err_stream]()
            {
                ::fsync(::fileno(stream));
                ::fsync(::fileno(err_stream));
            };
    }

    inline auto make_reopen_func(std::string path, FILE* stream)
    {
        return
            [path = std::move(path), stream]()
            {
                return std::freopen(path.c_str(), "a", stream) != nullptr;
            };
    }

    inline FILE* open_path(const char* path)
    {
        FILE* const fp = std::fopen(path, "a");
        if (fp == nullptr)
            detail::throw_system_error_fmt("Failed to open `%s'", path);
        return fp;
    }

#if defined(__cpp_lib_invocable)
    using invocable = std::invocable;
#else
    // This can be removed when libc++ supports invocable
    template<typename F, typename... Args>
    concept invocable = requires(F&& f, Args&&... args)
    {
        std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    };
#endif
}

// TODO: Specify buffer size via template parameter, or as dynamic,
// dynamic_capacity will need to become part of the interface.
// XXX SHOULD GET_SINK SUPPORT SPECIFYING THE SIZE? CONSUMER CAN
// READ VIA TYPE ERASED FUNC

/**
 * The main logger class. When constructed a background thread will be created
 * which is used for formatting log messages and performing I/O. To write to the
 * logger call @ref logger::sink then pass the sink to a macro such as @ref
 * XTR_LOG.
 */
class xtr::logger
{
private:
#if defined(__cpp_lib_jthread)
    using jthread = std::jthread;
#else
    // This can be removed when libc++ supports jthread
    struct jthread : std::thread
    {
        using std::thread::thread;

        jthread& operator=(jthread&&) noexcept = default;

        ~jthread()
        {
            if (joinable())
                join();
        }
    };
#endif

public:
    /**
     * Path only constructor, the first argument is the path to a file which
     * should be opened and logged to. The file will be opened in append mode,
     * and will be created if it does not exist. Errors will be written to
     * stdout.
     *
     * @arg path: The path of a file to write log statements to.
     * @arg clock: @anchor clock_arg
     *             A function returning the current time of day as a
     *             std::timespec. This function will be invoked when creating
     *             timestamps for log statements produced by the basic log
     *             macros\--- please see the
     *             <a href="guide.html#basic-time-source">basic time source</a>
     *             section of the user guide for details. The default clock is
     *             std::chrono::system_clock.
     * @arg command_path: @anchor command_path_arg
     *                    The path where the local domain socket used to
     *                    communicate with <a href="xtrctl.html">xtrctl</a>
     *                    should be created. The default path is
     *                    /run/user/<uid>/xtrctl.<pid>.<N>, where N begins at 0
     *                    and increases for each logger object created by the
     *                    process. If the file cannot be created in
     *                    /run/user/<uid> then /tmp is used instead.
     */
    template<typename Clock = std::chrono::system_clock>
    logger(
        const char* path,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path())
    :
        logger(
            path,
            detail::open_path(path),
            stderr,
            std::forward<Clock>(clock),
            std::move(command_path))
    {
    }

    /**
     * Path and stream constructor.
     *
     * @arg path: The path of a file to write log statements to.
     * @arg clock: Please refer to the @ref clock_arg "description"
     *             above.
     * @arg command_path: Please refer to the @ref command_path_arg
     *                    "description" above.
     */
    template<typename Clock = std::chrono::system_clock>
    logger(
        const char* path,
        FILE* stream,
        FILE* err_stream = stderr,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path())
    :
        logger(
            detail::make_output_func(stream),
            detail::make_error_func(err_stream),
            detail::make_flush_func(stream, err_stream),
            detail::make_sync_func(stream, err_stream),
            detail::make_reopen_func(path, stream),
            [stream](){ std::fclose(stream); }, // close
            std::forward<Clock>(clock),
            std::move(command_path))
    {
    }

    /**
     * Stream only constructor.
     *
     * @arg clock: Please refer to the @ref clock_arg "description"
     *             above.
     * @arg command_path: Please refer to the @ref command_path_arg
     *                    "description" above.
     */
    template<typename Clock = std::chrono::system_clock>
    logger(
        FILE* stream = stderr,
        FILE* err_stream = stderr,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path())
    :
        logger(
            detail::make_output_func(stream),
            detail::make_error_func(err_stream),
            detail::make_flush_func(stream, err_stream),
            detail::make_sync_func(stream, err_stream),
            [](){ return true; }, // reopen
            [](){}, // close
            std::forward<Clock>(clock),
            std::move(command_path))
    {
    }

    /**
     * Simplified custom back-end constructor.
     *
     * @arg out:
     * @arg err:
     * @arg clock: Please refer to the @ref clock_arg "description"
     *             above.
     * @arg command_path: Please refer to the @ref command_path_arg
     *                    "description" above.
     */
    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename Clock = std::chrono::system_clock>
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    requires
        detail::invocable<OutputFunction, const char*, std::size_t> &&
        detail::invocable<ErrorFunction, const char*, std::size_t>
#endif
    logger(
        OutputFunction&& out,
        ErrorFunction&& err,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path())
    :
        logger(
            std::forward<OutputFunction>(out),
            std::forward<ErrorFunction>(err),
            [](){}, // flush
            [](){}, // sync
            [](){ return true; }, // reopen
            [](){}, // close
            std::forward<Clock>(clock),
            std::move(command_path))
    {
    }

    /**
     * Custom back-end constructor.
     *
     * @arg out:
     * @arg err:
     * @arg flush:
     * @arg sync:
     * @arg reopen:
     * @arg close:
     * @arg clock: Please refer to the @ref clock_arg "description"
     *             above.
     * @arg command_path: Please refer to the @ref command_path_arg
     *                    "description" above.
     */
    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename FlushFunction,
        typename SyncFunction,
        typename ReopenFunction,
        typename CloseFunction,
        typename Clock = std::chrono::system_clock>
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    requires
        detail::invocable<OutputFunction, const char*, std::size_t> &&
        detail::invocable<ErrorFunction, const char*, std::size_t> &&
        detail::invocable<FlushFunction> &&
        detail::invocable<SyncFunction> &&
        detail::invocable<ReopenFunction> &&
        detail::invocable<CloseFunction>
#endif
    logger(
        OutputFunction&& out,
        ErrorFunction&& err,
        FlushFunction&& flush,
        SyncFunction&& sync,
        ReopenFunction&& reopen,
        CloseFunction&& close,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path())
    {
        // The consumer thread must be started after control_ has been
        // constructed
        consumer_ =
            jthread(
                &detail::consumer::run,
                detail::consumer(
                    std::forward<OutputFunction>(out),
                    std::forward<ErrorFunction>(err),
                    std::forward<FlushFunction>(flush),
                    std::forward<SyncFunction>(sync),
                    std::forward<ReopenFunction>(reopen),
                    std::forward<CloseFunction>(close),
                    &control_),
                make_clock(std::forward<Clock>(clock)));
        // Passing control_ to the consumer is equivalent to calling
        // register_sink, so mark it as open.
        control_.open_ = true;
        set_command_path(std::move(command_path));
    }

    /**
     * Logger destructor. This function will join the consumer thread. If
     * sinks are still connected to the logger then the consumer thread
     * will not terminate until the sinks disconnect, i.e. the destructor
     * will block until all connected sinks disconnect from the logger.
     */
    ~logger();

    /**
     *  Returns the native handle for the logger's consumer thread. This
     *  may be used for setting thread affinities or other thread attributes.
     */
    std::thread::native_handle_type consumer_thread_native_handle()
    {
        return consumer_.native_handle();
    }

    /**
     *  Creates a sink with the specified name. Note that each call to this
     *  function creates a new sink; if repeated calls are made with the
     *  same name, separate sinks with the name name are created.
     *
     *  @param name: The name for the given sink.
     */
    [[nodiscard]] sink get_sink(std::string name);

    /**
     *  Registers the sink with the logger. Note that the sink name does not
     *  need to be unique; if repeated calls are made with the same name,
     *  separate sinks with the same name are registered.
     *
     *  @param s: The sink to register.
     *  @param name: The name for the given sink.
     *
     *  @pre The sink must be closed.
     */
    void register_sink(sink& s, std::string name) noexcept;

    /**
     * TODO
     */
    void set_output_stream(FILE* stream) noexcept;

    /**
     * TODO
     */
    void set_error_stream(FILE* stream) noexcept;

    /**
     * TODO
     */
    template<typename Func>
    void set_output_function(Func&& f) noexcept
    {
        static_assert(
            std::is_convertible_v<
                std::invoke_result_t<Func, const char*, std::size_t>,
                ::ssize_t>,
            "Output function type must be of type ssize_t(const char*, size_t) "
            "(returning the number of bytes written or -1 on error)");
        post([f = std::forward<Func>(f)](auto& c, auto&) { c.out = std::move(f); });
        control_.sync();
    }

    /**
     * TODO
     */
    template<typename Func>
    void set_error_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<
                std::invoke_result_t<Func, const char*, std::size_t>,
                void>,
            "Error function must be of type void(const char*, size_t)");
        post([f = std::forward<Func>(f)](detail::consumer& c, auto&) { c.err = std::move(f); });
        control_.sync();
    }

    /**
     * TODO
     */
    template<typename Func>
    void set_flush_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<std::invoke_result_t<Func>, void>,
            "Flush function must be of type void()");
        post([f = std::forward<Func>(f)](detail::consumer& c, auto&) { c.flush = std::move(f); });
        control_.sync();
    }

    /**
     * TODO
     */
    template<typename Func>
    void set_sync_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<std::invoke_result_t<Func>, void>,
            "Sync function must be of type void()");
        post([f = std::forward<Func>(f)](detail::consumer& c, auto&) { c.sync = std::move(f); });
        control_.sync();
    }

    /**
     * TODO
     */
    template<typename Func>
    void set_reopen_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<std::invoke_result_t<Func>, bool>,
            "Reopen function must be of type bool()");
        post([f = std::forward<Func>(f)](detail::consumer& c, auto&) { c.reopen = std::move(f); });
        control_.sync();
    }

    /**
     * TODO
     */
    template<typename Func>
    void set_close_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<std::invoke_result_t<Func>, void>,
            "Close function must be of type void()");
        post([f = std::forward<Func>(f)](detail::consumer& c, auto&) { c.close = std::move(f); });
        control_.close();
    }

    /**
     * TODO
     */
    void set_command_path(std::string path) noexcept;

private:
    template<typename Func>
    void post(Func&& f)
    {
        std::scoped_lock lock{control_mutex_};
        control_.post(std::forward<Func>(f));
    }

    template<typename Clock>
    std::function<std::timespec()> make_clock(Clock&& clock)
    {
        return
            [clock_{std::forward<Clock>(clock)}]() -> std::timespec
            {
                // Note: to_time_t would be useful here except it is unspecified
                // whether time_t rounds up or truncates if time_t has a lower
                // precision than the input time_point.
                using namespace std::chrono;
                const auto now = clock_.now();
                auto sec = time_point_cast<seconds>(now);
                if (sec > now)
                    sec - seconds{1};
                return std::timespec{
                    .tv_sec=sec.time_since_epoch().count(),
                    .tv_nsec=duration_cast<nanoseconds>(now - sec).count()};
            };
    }

    sink control_; // aligned to cache line so first to avoid extra padding
    jthread consumer_;
    std::mutex control_mutex_;

    friend sink;
};

#endif
