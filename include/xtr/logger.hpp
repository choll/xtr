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
#include "detail/consumer.hpp"
#include "detail/string_ref.hpp"
#include "io/fd_storage.hpp"
#include "io/storage_interface.hpp"
#include "log_level.hpp"
#include "log_macros.hpp"
#include "pump_io_stats.hpp"
#include "sink.hpp"
#include "xtr/detail/tsc.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include <stdio.h>

namespace xtr
{
    class logger;

    /**
     * Passed to @ref logger::logger to control logger behaviour.
     */
    enum class option_flags_t
    {
        none,
        /**
         * Disables the background worker thread. Users must call @ref
         * logger::pump_io to process log messages.
         *
         * @warning If @ref logger::pump_io is not regularly invoked by a worker
         * thread then @ref logger operations may block until @ref
         * logger::pump_io is called. In particular @ref
         * logger::set_command_path and @ref logger::set_log_level_style will
         * block as they do not return until the worker thread has fully
         * processed the request. It is recommended to construct the logger and
         * then immediately begin invoking @ref logger::pump_io in a separate
         * thread.
         *
         * @warning See notes attached to @ref logger::pump_io on shutting the
         * logger down when this option is enabled.
         */
        disable_worker_thread
    };

    /**
     * nocopy is used to specify that a string argument should be passed by
     * reference instead of by value, so that `arg` becomes `nocopy(arg)`. Note
     * that by default, all strings including C strings and std::string_view are
     * copied. In order to pass strings by reference they must be wrapped in a
     * call to nocopy. Please see the <a
     * href="guide.html#passing-arguments-by-value-or-reference"> passing
     * arguments by value or reference</a> and <a href="guide.html#string-arguments">string
     * arguments</a> sections of the user guide for further details.
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
 * logger call @ref logger::get_sink to create a sink, then pass the sink to a
 * macro such as @ref XTR_LOG (see the <a
 * href="guide.html#creating-and-writing-to-sinks">creating and writing to
 * sinks</a> section of the user guide for details).
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
     * Path constructor. The first argument is the path to a file which should
     * be opened and logged to. The file will be opened in append mode, and will
     * be created if it does not exist. Errors will be written to stdout.
     *
     * @param path: The path of a file to write log statements to.
     *
     * @anchor clock_arg
     * @param clock:  A function returning the current time of day as a
     * std::timespec. This function will be invoked when creating timestamps for
     * log statements produced by the basic log macros\--- please see the <a
     * href="guide.html#basic-time-source">basic time source</a> section of the
     * user guide for details. The default clock is std::chrono::system_clock.
     *
     * @anchor command_path_arg
     * @param command_path: The path where the local domain socket used to
     * communicate with <a href="xtrctl.html">xtrctl</a> should be created. The
     * default behaviour is to create sockets in $XDG_RUNTIME_DIR (if set,
     * otherwise "/run/user/<uid>"). If that directory does not exist or is
     * inaccessible then $TMPDIR (if set, otherwise "/tmp") will be used
     * instead. See @ref default_command_path for further details. To prevent a
     * socket from being created, pass @ref null_command_path.
     *
     * @param level_style: The log level style that will be used to prefix each
     * log statement\---please refer to the @ref log_level_style_t documentation
     * for details.
     *
     * @param options: Logger options, see @ref option_flags_t.
     */
    template<typename Clock = std::chrono::system_clock>
    explicit logger(
        const char* path,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style,
        option_flags_t options = option_flags_t::none) :
        logger(
            make_fd_storage(path),
            std::forward<Clock>(clock),
            std::move(command_path),
            level_style,
            options)
    {
    }

    /**
     * Stream constructor.
     *
     * It is expected that this constructor will be used with streams such as
     * stdout or stderr. If a stream that has been opened by the user is to be
     * passed to the logger then the @ref stream-with-reopen-ctor "stream
     * constructor with reopen path" constructor is recommended instead, as this
     * will mean that the log file can be rotated\---please refer to the xtrctl
     * documentation for the <a href="xtrctl.html#reopening-log-files">reopening
     * log files</a> command for details.
     *
     * @note Reopening the log file via the <a href="xtrctl.html#rotating-log-files">xtrctl</a>
     * tool is *not* supported if this constructor is used.
     *
     * @param stream: The stream to write log statements to.
     *
     * @param clock: Please refer to the @ref clock_arg "description" above.
     *
     * @param command_path: Please refer to the @ref command_path_arg "description" above.
     *
     * @param level_style: The log level style that will be used to prefix each
     * log statement\---please refer to the @ref log_level_style_t documentation for details.
     *
     * @param options: Logger options, see @ref option_flags_t.
     */
    template<typename Clock = std::chrono::system_clock>
    explicit logger(
        FILE* stream = stderr,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style,
        option_flags_t options = option_flags_t::none) :
        logger(
            make_fd_storage(stream, null_reopen_path),
            std::forward<Clock>(clock),
            std::move(command_path),
            level_style,
            options)
    {
    }

    /**
     * @anchor stream-with-reopen-ctor
     *
     * Stream constructor with reopen path.
     *
     * @note Reopening the log file via the
     * <a href="xtrctl.html#rotating-log-files">xtrctl</a> tool is supported,
     * with the reopen_path argument specifying the path to reopen.
     *
     * @param reopen_path: The path of the file associated with the stream
     * argument. This path will be used to reopen the stream if requested via
     * the xtrctl <a href="xtrctl.html#reopening-log-files">reopen command</a>.
     * Pass @ref null_reopen_path if no filename is associated with the stream.
     *
     * @param stream: The stream to write log statements to.
     *
     * @param clock: Please refer to the @ref clock_arg "description" above.
     *
     * @param command_path: Please refer to the @ref command_path_arg
     * "description" above.
     *
     * @param level_style: The log level style that will be used to prefix each
     * log statement\---please refer to the @ref log_level_style_t documentation
     * for details.
     *
     * @param options: Logger options, see @ref option_flags_t.
     */
    template<typename Clock = std::chrono::system_clock>
    logger(
        std::string reopen_path,
        FILE* stream,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style,
        option_flags_t options = option_flags_t::none) :
        logger(
            make_fd_storage(stream, std::move(reopen_path)),
            std::forward<Clock>(clock),
            std::move(command_path),
            level_style,
            options)
    {
    }

    /**
     * @anchor back-end-ctor
     *
     * Custom back-end constructor (please refer to the
     * <a href="guide.html#custom-back-ends">custom back-ends</a> section of the
     * user guide for further details on implementing a custom back-end).
     *
     * @param storage: Unique pointer to an object implementing the @ref storage_interface
     * interface. The logger will invoke methods on this object from the background
     * thread in order to write log data to whatever underlying storage medium
     * is implemented by the object, such as disk, network, dot-matrix printer etc.
     *
     * @param clock: Please refer to the @ref clock_arg "description" above.
     *
     * @param command_path: Please refer to the @ref command_path_arg "description" above.
     *
     * @param level_style: The log level style that will be used to prefix each
     * log statement\---please refer to the @ref log_level_style_t documentation for details.
     *
     * @param options: Logger options, see @ref option_flags_t.
     */
    template<typename Clock = std::chrono::system_clock>
    explicit logger(
        storage_interface_ptr storage,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style,
        option_flags_t options = option_flags_t::none) :
        consumer_(
            detail::buffer(std::move(storage), level_style),
            &control_,
            std::move(command_path),
            make_clock(std::forward<Clock>(clock)))
    {
        if (options != option_flags_t::disable_worker_thread)
        {
            // The consumer thread must be started after control_ has been constructed
            consumer_thread_ = jthread(&detail::consumer::run, &consumer_);
        }
        // Passing control_ to the consumer is equivalent to calling
        // register_sink, so mark it as open.
        control_.open_ = true;
        // On some CPUs the TSC frequency is obtained by estimation. get_tsc_hz
        // is called here to force the estimation to run here rather than on the
        // consumer thread, in order to prevent the consumer thread from
        // stalling while the estimation runs.
        (void)detail::get_tsc_hz();
    }

    /**
     * Logger destructor. This function will join the consumer thread if it is
     * in use. If sinks are still connected to the logger then the consumer thread
     * will not terminate until the sinks disconnect, i.e. the destructor will
     * block until all connected sinks disconnect from the logger. If the consumer
     * thread has been disabled via @ref option_flags_t::disable_worker_thread
     * then the destructor will similarly block until @ref logger::pump_io returns false.
     */
    ~logger() = default;

    /**
     * Returns the native handle for the logger's consumer thread. This may be
     * used for setting thread affinities or other thread attributes.
     */
    std::thread::native_handle_type consumer_thread_native_handle()
    {
        return consumer_thread_.native_handle();
    }

    /**
     * Creates a sink with the specified name. Note that each call to this
     * function creates a new sink; if repeated calls are made with the same
     * name, separate sinks with the name name are created.
     *
     * @param name: The name for the given sink.
     */
    [[nodiscard]] sink get_sink(std::string name);

    /**
     * Registers the sink with the logger. Note that the sink name does not need
     * to be unique; if repeated calls are made with the same name, separate
     * sinks with the same name are registered.
     *
     * @param s: The sink to register.
     *
     * @param name: The name for the given sink.
     *
     * @pre The sink must be closed.
     */
    void register_sink(sink& s, std::string name) noexcept;

    /**
     * Sets the logger command path\---please refer to the 'command_path'
     * argument @ref command_path_arg "description" above for details.
     */
    void set_command_path(std::string path) noexcept;

    /**
     * Sets the logger log level style\---please refer to the @ref
     * log_level_style_t documentation for details.
     */
    void set_log_level_style(log_level_style_t level_style) noexcept;

    /**
     * Sets the default log level. Sinks created via future calls to @ref
     * get_sink will be created with the given log level.
     */
    void set_default_log_level(log_level_t level);

    /**
     * If the @ref option_flags_t::disable_worker_thread option has been passed
     * to @ref logger::logger then this function must be called in order to
     * process messages written to the logger. Users may call this function from
     * a thread of their choosing and may interleave calls with other background
     * tasks that their program needs to perform.
     *
     * For best results when interleaving with other tasks ensure that io_uring
     * mode is enabled (so that I/O will be performed asynchronously leaving
     * more time for other tasks to run). See @ref XTR_USE_IO_URING for details
     * on enabling io_uring.
     *
     * @return True if any sinks or the logger are still active, false if no
     * sinks are active and the logger has shut down. Once false has been
     * returned pump_io should not be called again.
     *
     * @warning If the @ref option_flags_t::disable_worker thread option is
     * enabled then in order to shut down the logger pump_io must be called
     * until it returns false. If pump_io has not yet returned false then @ref
     * logger::~logger will block until it returns false. Do not call pump_io
     * again after it has returned false.
     */
    bool pump_io(pump_io_stats* stats = nullptr);

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
        return [clock_{std::forward<Clock>(clock)}]() -> std::timespec
        {
            using namespace std::chrono;
            const auto now = clock_.now();
            const auto sec = floor<seconds>(now);
            return std::timespec{
                .tv_sec = sec.time_since_epoch().count(),
                .tv_nsec = duration_cast<nanoseconds>(now - sec).count()};
        };
    }

    detail::consumer consumer_;
    jthread consumer_thread_;
    sink control_;
    std::mutex control_mutex_;
    std::atomic<log_level_t> default_log_level_ = log_level_t::info;

    friend sink;
};

#endif
