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
    /**
     * nocopy is used to specify that a log argument should be passed by
     * reference instead of by value, so that `arg` becomes `nocopy(arg)`.
     * Note that by default, all strings including C strings and
     * std::string_view are copied. In order to pass strings by reference
     * they must be wrapped in a call to nocopy.
     * Please see the <a href="guide.html#passing-arguments-by-value-or-reference">
     * passing arguments by value or reference</a> and
     * <a href="guide.html#string-arguments">string arguments</a> sections of
     * the user guide for further details.
     */
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
            [stream](log_level_t, const char* buf, std::size_t size)
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
 * logger call @ref logger::get_sink to create a sink, then pass the sink
 * to a macro such as @ref XTR_LOG
 * (see the <a href="guide.html#creating-and-writing-to-sinks">creating and
 * writing to sinks</a> section of the user guide for details).
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
     * Path constructor. The first argument is the path to a file which
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
     *                    should be created. The default behaviour is to create
     *                    sockets in $XDG_RUNTIME_DIR (if set, otherwise
     *                    "/run/user/<uid>"). If that directory does not exist
     *                    or is inaccessible then $TMPDIR (if set, otherwise
     *                    "/tmp") will be used instead. See @ref default_command_path for
     *                    further details. To prevent a socket from being created, pass
     *                    @ref null_command_path.
     * @arg level_style: The log level style that will be used to prefix each log
     *                   statement\---please refer to the @ref log_level_style_t
     *                   documentation for details.
     */
    template<typename Clock = std::chrono::system_clock>
    logger(
        const char* path,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style)
    :
        logger(
            path,
            detail::open_path(path),
            stderr,
            std::forward<Clock>(clock),
            std::move(command_path),
            level_style)
    {
    }

    /**
     * Stream constructor.
     *
     * It is expected that this constructor will be used with streams such as
     * stdout or stderr. If a stream that has been opened by the user is to
     * be passed to the logger then the
     * @ref stream-with-reopen "stream constructor with reopen path"
     * constructor is recommended instead.
     *
     * @note The logger will not take ownership of the stream\---i.e. it will
     *       not be closed when the logger destructs.
     *
     * @note Reopening the log file via the
     *       <a href="xtrctl.html#rotating-log-files">xtrctl</a> tool is *not*
     *       supported if this constructor is used.
     *
     * @arg stream: The stream to write log statements to.
     * @arg err_stream: A stream to write error messages to.
     * @arg clock: Please refer to the @ref clock_arg "description"
     *             above.
     * @arg command_path: Please refer to the @ref command_path_arg
     *                    "description" above.
     * @arg level_style: The log level style that will be used to prefix each log
     *                   statement\---please refer to the @ref log_level_style_t
     *                   documentation for details.
     */
    template<typename Clock = std::chrono::system_clock>
    logger(
        FILE* stream = stderr,
        FILE* err_stream = stderr,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style)
    :
        logger(
            detail::make_output_func(stream),
            detail::make_error_func(err_stream),
            detail::make_flush_func(stream, err_stream),
            detail::make_sync_func(stream, err_stream),
            [](){ return true; }, // reopen
            [](){}, // close
            std::forward<Clock>(clock),
            std::move(command_path),
            level_style)
    {
    }

    /**
     * @anchor stream-with-reopen
     *
     * Stream constructor with reopen path.
     *
     * @note The logger will take ownership of
     *       the stream, closing it when the logger destructs.
     *
     * @note Reopening the log file via the
     *       <a href="xtrctl.html#rotating-log-files">xtrctl</a> tool is supported,
     *       with the reopen_path argument specifying the path to reopen.
     *
     * @arg reopen_path: The path of the file associated with the stream argument.
     *                   This path will be used to reopen the stream if requested via
     *                   the <a href="xtrctl.html#rotating-log-files">xtrctl</a> tool.
     * @arg stream: The stream to write log statements to.
     * @arg err_stream: A stream to write error messages to.
     * @arg clock: Please refer to the @ref clock_arg "description"
     *             above.
     * @arg command_path: Please refer to the @ref command_path_arg
     *                    "description" above.
     * @arg level_style: The log level style that will be used to prefix each log
     *                   statement\---please refer to the @ref log_level_style_t
     *                   documentation for details.
     */
    template<typename Clock = std::chrono::system_clock>
    logger(
        const char* reopen_path,
        FILE* stream,
        FILE* err_stream = stderr,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style)
    :
        logger(
            detail::make_output_func(stream),
            detail::make_error_func(err_stream),
            detail::make_flush_func(stream, err_stream),
            detail::make_sync_func(stream, err_stream),
            detail::make_reopen_func(reopen_path, stream),
            [stream](){ std::fclose(stream); }, // close
            std::forward<Clock>(clock),
            std::move(command_path),
            level_style)
    {
    }

    /**
     * Basic custom back-end constructor.
     *
     * @arg out: @anchor out_arg
     *           A function accepting a const char* buffer of formatted log
     *           data and a std::size_t argument specifying the length of the
     *           buffer in bytes. The logger will invoke this function from the
     *           background thread in order to output log data. The return type
     *           should be ssize_t and return value should be -1 if an error
     *           occurred, otherwise the number of bytes successfully written
     *           should be returned. Note that returning anything less than the
     *           number of bytes given by the length argument is considered an
     *           error, resulting in the 'err' function being invoked with a
     *           "Short write" error string.
     * @arg err: @anchor err_arg
     *           A function accepting a const char* buffer of formatted log
     *           data and a std::size_t argument specifying the length of the
     *           buffer in bytes. The logger will invoke this function from the
     *           background thread if an error occurs. The return type should
     *           be void.
     * @arg clock: Please refer to the @ref clock_arg "description"
     *             above.
     * @arg command_path: Please refer to the @ref command_path_arg
     *                    "description" above.
     * @arg level_style: The log level style that will be used to prefix each log
     *                   statement\---please refer to the @ref log_level_style_t
     *                   documentation for details.
     */
    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename Clock = std::chrono::system_clock>
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    requires
        detail::invocable<OutputFunction, log_level_t, const char*, std::size_t> &&
        detail::invocable<ErrorFunction, const char*, std::size_t>
#endif
    logger(
        OutputFunction&& out,
        ErrorFunction&& err,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style)
    :
        logger(
            std::forward<OutputFunction>(out),
            std::forward<ErrorFunction>(err),
            [](){}, // flush
            [](){}, // sync
            [](){ return true; }, // reopen
            [](){}, // close
            std::forward<Clock>(clock),
            std::move(command_path),
            level_style)
    {
    }

    /**
     * Custom back-end constructor.
     *
     * @arg out: Please refer to the @ref out_arg "description" above.
     * @arg err: Please refer to the @ref err_arg "description" above.
     * @arg flush: @anchor flush_arg
     *             A function that the logger will invoke from
     *             the background thread to indicate that the back-end
     *             should write any buffered data to its associated
     *             backing store.
     * @arg sync: @anchor sync_arg
     *            A function that the logger will invoke from
     *            the background thread to indicate that the back-end
     *            should ensure that all data written to the associated
     *            backing store has reached permanent storage.
     * @arg reopen: @anchor reopen_arg
     *              A function that the logger will invoke from
     *              the background thread to indicate that if the back-end
     *              has a file opened for writing log data then the
     *              file should be reopened (in order to rotate it).
     * @arg close: @anchor close_arg
     *             A function that the logger will invoke from
     *             the background thread to indicate that the back-end
     *             should close any associated backing store.
     * @arg clock: Please refer to the @ref clock_arg "description"
     *             above.
     * @arg command_path: Please refer to the @ref command_path_arg
     *                    "description" above.
     * @arg level_style: The log level style that will be used to prefix each log
     *                   statement\---please refer to the @ref log_level_style_t
     *                   documentation for details.
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
        detail::invocable<OutputFunction, log_level_t, const char*, std::size_t> &&
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
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style)
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
                    level_style,
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
     * Sets the logger output to the specified stream. The existing output
     * will be flushed and closed.
     */
    void set_output_stream(FILE* stream) noexcept;

    /**
     * Sets the logger error output to the specified stream.
     */
    void set_error_stream(FILE* stream) noexcept;

    /**
     * Sets the logger output to the specified function. The existing output
     * will be flushed and closed. Please refer to the 'out' argument @ref
     * out_arg "description" above for details.
     */
    template<typename Func>
    void set_output_function(Func&& f) noexcept
    {
        static_assert(
            std::is_convertible_v<
                std::invoke_result_t<Func, log_level_t, const char*, std::size_t>,
                ::ssize_t>,
            "Output function type must be of type ssize_t(const char*, size_t) "
            "(returning the number of bytes written or -1 on error)");
        post(
            [f = std::forward<Func>(f)](auto& c, auto&)
            {
                c.flush();
                c.close();
                c.out = std::move(f);
            });
        control_.sync();
    }

    /**
     * Sets the logger error output to the specified function. Please refer to
     * the 'err' argument @ref err_arg "description" above for details.
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
     * Sets the logger flush function\---please refer to the 'flush' argument
     * @ref flush_arg "description" above for details.
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
     * Sets the logger sync function\---please refer to the 'sync' argument
     * @ref sync_arg "description" above for details.
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
     * Sets the logger reopen function\---please refer to the 'reopen' argument
     * @ref reopen_arg "description" above for details.
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
     * Sets the logger close function\---please refer to the 'close' argument
     * @ref close_arg "description" above for details.
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
     * Sets the logger command path\---please refer to the 'command_path' argument
     * @ref command_path_arg "description" above for details.
     */
    void set_command_path(std::string path) noexcept;

    /**
     * Sets the logger log level style\---please refer to the @ref log_level_style_t
     * documentation for details.
     */
    void set_log_level_style(log_level_style_t level_style) noexcept;

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
