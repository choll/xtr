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
#include "io/fd_storage.hpp"
#include "io/storage_interface.hpp"
#include "detail/consumer.hpp"
#include "detail/string_ref.hpp"
#include "xtr/detail/tsc.hpp"
#include "log_macros.hpp"
#include "log_level.hpp"
#include "sink.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#include <stdio.h>

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
    explicit logger(
        const char* path,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style)
    :
        logger(
            make_fd_storage(path),
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
     * @ref stream-with-reopen-ctor "stream constructor with reopen path"
     * constructor is recommended instead, as this will mean that the log file
     * can be rotated\---please refer to the xtrctl documentation for the
     * <a href="xtrctl.html#reopening-log-files">reopening log files</a> command
     * for details.
     *
     * @note Reopening the log file via the
     *       <a href="xtrctl.html#rotating-log-files">xtrctl</a> tool is *not*
     *       supported if this constructor is used.
     *
     * @arg stream: The stream to write log statements to.
     * @arg clock: Please refer to the @ref clock_arg "description"
     *             above.
     * @arg command_path: Please refer to the @ref command_path_arg
     *                    "description" above.
     * @arg level_style: The log level style that will be used to prefix each log
     *                   statement\---please refer to the @ref log_level_style_t
     *                   documentation for details.
     */
    template<typename Clock = std::chrono::system_clock>
    explicit logger(
        FILE* stream = stderr,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style)
    :
        logger(
            make_fd_storage(stream, null_reopen_path),
            std::forward<Clock>(clock),
            std::move(command_path),
            level_style)
    {
    }

    /**
     * @anchor stream-with-reopen-ctor
     *
     * Stream constructor with reopen path.
     *
     * @note Reopening the log file via the
     *       <a href="xtrctl.html#rotating-log-files">xtrctl</a> tool is supported,
     *       with the reopen_path argument specifying the path to reopen.
     *
     * @arg reopen_path: The path of the file associated with the stream argument.
     *                   This path will be used to reopen the stream if requested via
     *                   the xtrctl <a href="xtrctl.html#reopening-log-files">reopen command</a>.
     *                   Pass @ref null_reopen_path if no filename is associated with the stream.
     * @arg stream: The stream to write log statements to.
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
        std::string reopen_path,
        FILE* stream,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style)
    :
        logger(
            make_fd_storage(stream, std::move(reopen_path)),
            std::forward<Clock>(clock),
            std::move(command_path),
            level_style)
    {
    }

    /**
     * @anchor back-end-ctor
     *
     * Custom back-end constructor (please refer to the
     * <a href="guide.html#custom-back-ends">custom back-ends</a> section of
     * the user guide for further details on implementing a custom back-end).
     *
     * @arg storage: Unique pointer to an object implementing the
     *               @ref storage_interface interface. The logger will invoke
     *               methods on this object from the background thread in
     *               order to write log data to whatever underlying storage
     *               medium is implemented by the object, such as disk,
     *               network, dot-matrix printer etc.
     * @arg clock: Please refer to the @ref clock_arg "description"
     *             above.
     * @arg command_path: Please refer to the @ref command_path_arg
     *                    "description" above.
     * @arg level_style: The log level style that will be used to prefix each log
     *                   statement\---please refer to the @ref log_level_style_t
     *                   documentation for details.
     */
    template<typename Clock = std::chrono::system_clock>
    explicit logger(
        storage_interface_ptr storage,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style)
    {
        // The consumer thread must be started after control_ has been
        // constructed
        consumer_ =
            jthread(
                &detail::consumer::run,
                std::make_unique<detail::consumer>(
                    detail::buffer(std::move(storage), level_style),
                    &control_,
                    std::move(command_path)),
                make_clock(std::forward<Clock>(clock)));
        // Passing control_ to the consumer is equivalent to calling
        // register_sink, so mark it as open.
        control_.open_ = true;
        // On some CPUs the TSC frequency is obtained by estimation. get_tsc_hz
        // is called here to force the estimation to run here rather than on
        // the consumer thread, in order to prevent the consumer thread from
        // stalling while the estimation runs.
        (void)detail::get_tsc_hz();
    }

    /**
     * Logger destructor. This function will join the consumer thread. If
     * sinks are still connected to the logger then the consumer thread
     * will not terminate until the sinks disconnect, i.e. the destructor
     * will block until all connected sinks disconnect from the logger.
     */
    ~logger() = default;

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
     * Sets the logger command path\---please refer to the 'command_path' argument
     * @ref command_path_arg "description" above for details.
     */
    void set_command_path(std::string path) noexcept;

    /**
     * Sets the logger log level style\---please refer to the @ref log_level_style_t
     * documentation for details.
     */
    void set_log_level_style(log_level_style_t level_style) noexcept;

    /**
     * Sets the default log level. Sinks created via future calls to @ref get_sink
     * will be created with the given log level.
     */
    void set_default_log_level(log_level_t level);

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

    jthread consumer_;
    sink control_;
    std::mutex control_mutex_;
    log_level_t default_log_level_ = log_level_t::info;

    friend sink;
};

#endif
