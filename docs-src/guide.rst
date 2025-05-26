.. title:: XTR Logger User Guide

User Guide
==========

Overview
--------

The XTR API contains two classes; :cpp:class:`xtr::logger` and
:cpp:class:`xtr::sink`. Sinks each contain a queue [#queue]_, and
pass log messages to the associated logger via these queues. Each logger
has a background consumer thread which reads from the sinks that were created from the
logger. The background thread then formats the log message and either writes it
to disk or passes it to a custom back-end if one is in use.

::

    +------+
    | Sink |---[queue]---+
    +------+             |
                         |     +--------+
    +------+             +---> |        |     +----------------------------+
    | Sink |---[queue]-------> | Logger |---> | Back-end; file/network/etc |
    +------+             +---> |        |     +----------------------------+
                         |     +--------+
    +------+             |
    | Sink |---[queue]---+
    +------+

An application is expected to use multiple sinks, for example a sink per thread, or
sink per component. To support this sinks have a name associated with them which
is included in the output log message. Sink names do not need to be unique.

Creating and Writing to Sinks
-----------------------------

Sinks are created either by calling :cpp:func:`xtr::logger::get_sink`, via normal
construction followed by a call to :cpp:func:`xtr::logger::register_sink`, or by
copying another sink. Copied sinks are registered to the same logger and have the
same name as the source sink. Sinks may be renamed by calling :cpp:func:`xtr::sink::set_name`.
Once a sink has been created or registered, it may be written to using one of several
log macros which are described in the :ref:`log macros <log-macros>` section.

Examples
~~~~~~~~

Sink creation via :cpp:func:`xtr::logger::get_sink`:

.. code-block:: c++

    #include <xtr/logger.hpp>

    xtr::logger log;

    xtr::sink s = log.get_sink("Main");

    XTR_LOG(s, "Hello world");

View this example on `Compiler Explorer <https://godbolt.org/z/1GWbEPq8T>`__.

Sink creation via :cpp:func:`xtr::logger::register_sink`:

.. code-block:: c++

    #include <xtr/logger.hpp>

    xtr::logger log;
    xtr::sink s;

    log.register_sink(s, "Main");

    XTR_LOG(s, "Hello world");

View this example on `Compiler Explorer <https://godbolt.org/z/cobj4n3Gx>`__.

Sink creation via copying:

.. code-block:: c++

    #include <xtr/logger.hpp>

    xtr::logger log;

    xtr::sink s1 = log.get_sink("Main");
    xtr::sink s2 = s1;

    s2.set_name("Copy");

    XTR_LOG(s1, "Hello world");
    XTR_LOG(s2, "Hello world");

View this example on `Compiler Explorer <https://godbolt.org/z/9bGTG38ez>`__.

Format String Syntax
--------------------

.. https://docs.python.org/3/library/stdtypes.html#str.format

XTR uses `{fmt} <https://fmt.dev>`__ for formatting, so format strings follow the
same Python `str.format <https://docs.python.org/3/library/string.html#formatstrings>`__
style formatting as found in {fmt}. The {fmt} format string documentation can be found
`here <https://fmt.dev/latest/syntax.html>`__.

Example
~~~~~~~

.. code-block:: c++

    #include <xtr/logger.hpp>

    xtr::logger log;

    xtr::sink s = log.get_sink("Main");

    XTR_LOG(s, "Hello {}", 123); // Hello 123
    XTR_LOG(s, "Hello {}", 123.456); // Hello 123.456
    XTR_LOG(s, "Hello {:.1f}", 123.456); // Hello 123.1

View this example on `Compiler Explorer <https://godbolt.org/z/zxs7WThM6>`__.

.. _copy_val_ref:

Passing Arguments by Value or Reference
---------------------------------------

The default behaviour of the logger is to copy format arguments into the
specified sink by value. Note that no allocations are be performed by the
logger when this is done. If copying is undesirable then arguments may be
passed by reference by wrapping them in a call to :cpp:func:`xtr::nocopy`.

Example
~~~~~~~

.. code-block:: c++

    #include <xtr/logger.hpp>

    xtr::logger log;

    xtr::sink s = log.get_sink("Main");

    static std::string arg = "world";

    // Here 'arg' is passed by reference:
    XTR_LOG(s, "Hello {}", nocopy(arg));

    // Here 'arg' is passed by value:
    XTR_LOG(s, "Hello {}", arg);

View this example on `Compiler Explorer <https://godbolt.org/z/j5ebhWfdT>`__.

.. _string_args:

String Arguments
----------------

Passing strings to the logger is guaranteed to not allocate memory, and does
not assume anything about the lifetime of the string data. i.e. for the
following log statement:

.. code-block:: c++

    XTR_LOG(s, "{}", str);

If `str` is a :cpp:expr:`std::string`, :cpp:expr:`std::string_view`,
:cpp:expr:`char*` or :cpp:expr:`char[]` then the contents of `str` will be copied
into `sink` without incurring any allocations. The entire statement is guaranteed
to not allocate---i.e. even if :cpp:expr:`std::string` is passed, the
:cpp:expr:`std::string` copy constructor is not invoked and no allocation occurs.
String data is copied in order to provide safe default behaviour regarding the
lifetime of the string data. If copying the string data is undesirable then
string arguments may be wrapped in a call to :cpp:func:`xtr::nocopy`:

.. code-block:: c++

    XTR_LOG(sink, "{}", nocopy(str));

If this is done then only a pointer to the string data contained in `str` is
copied. The user is then responsible for ensuring that the string data remains
valid long enough for the logger to process the log statement. Note that only
the string data must remain valid---so for :cpp:expr:`std::string_view` the
object itself does not need to remain valid, just the data it references.

Log Levels
----------

The logger supports debug, info, warning, error and fatal log levels, which
are enumerated in the :cpp:enum:`xtr::log_level_t` enum. Log statements with
these levels may be produced using the :c:macro:`XTR_LOGL` macro, along with
additional macros that are described in the :ref:`log macros <log-macros>`
section of the API reference, all of which follow the convention of containing
"LOGL" in the macro name.

Each sink has its own log level, which can be programmatically set or queried
via :cpp:func:`xtr::sink::set_level` and :cpp:func:`xtr::sink::level`, and can
be set or queried from the command line using the :ref:`xtrctl <xtrctl>` tool.

Each log level has an order of importance. The listing of levels above is in
the order of increasing importance---so the least important level is 'debug'
and the most important level is 'fatal'. If a log statement is made with a
level that is lower than the current level of the sink then the log statement
is discarded. Note that this includes any calls made as arguments to the log,
so in the following example the function :cpp:func:`foo` is not called:

.. code-block:: c++

    #include <xtr/logger.hpp>

    xtr::logger log;

    xtr::sink s = log.get_sink("Main");

    s.set_level(xtr::log_level_t::error);

    XTR_LOGL(info, s, "Hello {}", foo());

View this example on `Compiler Explorer <https://godbolt.org/z/ss36qzo1c>`__.

Debug Log Statements
~~~~~~~~~~~~~~~~~~~~

Debug log statements can be disabled by defining XTR_NDEBUG.

Fatal Log Statements
~~~~~~~~~~~~~~~~~~~~

Fatal log statements will additionally call :cpp:func:`xtr::sink::sync` followed
by `abort(3) <https://www.man7.org/linux/man-pages/man3/abort.3.html>`__.

Thread Safety
-------------

 * All functions in :cpp:class:`xtr::logger` are thread-safe.
 * No functions in :cpp:class:`xtr::sink` are thread-safe other than
   :cpp:func:`xtr::sink::level` and :cpp:func:`xtr::sink::set_level`.
   This is because each thread is expected to have its own sink(s).

.. _custom-formatters:

Custom Formatters
-----------------

Custom formatters are implemented the same as in `{fmt} <https://fmt.dev>`__,
which is done either by:

* Providing a :cpp:func:`std::stream& operator<<(std::stream&, T&)` overload. Note
  that fmt/ostream.h must be included.
* Specializing :cpp:expr:`fmt::formatter<T>` and implementing the `parse` and
  `format` methods as described by the `{fmt}` documentation
  `here <https://fmt.dev/latest/api.html#formatting-user-defined-types>`__.

Examples
~~~~~~~~

Formatting a custom type via operator<<:

.. code-block:: c++

    #include <xtr/logger.hpp>

    #include <fmt/ostream.h>

    #include <ostream>

    namespace
    {
        struct custom {};

        std::ostream& operator<<(std::ostream& os, const custom&)
        {
            return os << "custom";
        }
    }

    int main()
    {
        xtr::logger log;

        xtr::sink s = log.get_sink("Main");

        XTR_LOG(s, "Hello {}", custom());

        return 0;
    }

View this example on `Compiler Explorer <https://godbolt.org/z/cK14z5Kr6>`__.

Formatting a custom type via fmt::formatter:

.. code-block:: c++

    #include <xtr/logger.hpp>

    namespace
    {
        struct custom {};
    }

    template<>
    struct fmt::formatter<custom>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const custom&, FormatContext& ctx) const
        {
            return format_to(ctx.out(), "custom");
        }
    };

    int main()
    {
        xtr::logger log;

        xtr::sink s = log.get_sink("Main");

        XTR_LOG(s, "Hello {}", custom());

        return 0;
    }

View this example on `Compiler Explorer <https://godbolt.org/z/W56zdWEh1>`__.

Formatting Containers, Tuples and Pairs
---------------------------------------

Formatters for containers, tuples and pairs are provided in
`xtr/formatters.hpp`. Types which will be formatted are:

* Any non-string iterable type---specifically any type that meets all of the
  following criteria;

  1. Is not constructible from :cpp:expr:`const char*`.
  2. :cpp:func:`std::begin()` and :cpp:func:`std::end()` are defined.
  3. Is not an associative container.

* Any associative container---specifically any type that provides a
  :cpp:type:`mapped_type` member.
* Any tuple-like type---specifically any type for which a
  :cpp:class:`std::tuple_size` overload is defined.

.. _time-sources:

Time Sources
------------

The logger provides a choice of time-sources when logging messages, each with
varying levels of accuracy and performance. The options are listed below.

+-----------------+----------+-------------+
| Source          | Accuracy | Performance |
+=================+==========+=============+
| Basic           | Low      | High        |
+-----------------+----------+-------------+
| Real-time Clock | Medium   | Medium      |
+-----------------+----------+-------------+
| TSC             | High     | Low/Medium  |
+-----------------+----------+-------------+
| User supplied   | -        | -           |
+-----------------+----------+-------------+

The performance of the TSC source is listed as either low or medium as it depends
on the particular CPU.

.. _basic-time-source:

Basic
~~~~~

The :c:macro:`XTR_LOG` macro and it's variants listed under the
:ref:`basic macros <log-macros>` section of the API reference all use the basic
time source. In these macros no timestamp is read when the log message is written
to the sink's queue, instead the logger's background thread reads the timestamp when
the log message is read from the queue. This is of course not accurate, but it is
fast.

:cpp:func:`std::chrono::system_clock` is used to read the current time, this can
be customised by passing an arbitrary function to the 'clock' parameter when
constructing the logger (see :cpp:func:`xtr::logger::logger`). In these macros 

Real-time Clock
~~~~~~~~~~~~~~~

The :c:macro:`XTR_LOG_RTC` macro and it's variants listed under the
:ref:`real-time clock macros <rtc-macros>` section of the API reference all use the
real-time clock source. In these macros the timestamp is read using
`clock_gettime(3) <https://www.man7.org/linux/man-pages/man3/clock_gettime.3.html>`__
with a clock source of either CLOCK_REALTIME_COARSE on Linux or CLOCK_REALTIME_FAST
on FreeBSD.

TSC
~~~

The :c:macro:`XTR_LOG_TSC` macro and it's variants listed under the
:ref:`TSC macros <tsc-macros>` section of the API reference all use the TSC
clock source. In these macros the timestamp is read from the CPU timestamp
counter via the RDTSC instruction. The TSC time source is is listed in the
table above as either low or medium performance as the cost of the RDTSC
instruction varies depending upon the host CPU microarchitecture.

User-Supplied Timestamp
~~~~~~~~~~~~~~~~~~~~~~~

The :c:macro:`XTR_LOG_TS` macro and it's variants listed under the
:ref:`user-supplied timestamp macros <user-supplied-timestamp-macros>` section of the
API reference all allow passing a user-supplied timestamp to the logger as the second
argument. Any type may be passed as long as it has a formatter defined
(see :ref:`custom formatters <custom-formatters>`).

Example
^^^^^^^

.. code-block:: c++

    #include <xtr/logger.hpp>

	template<>
	struct fmt::formatter<std::timespec>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext &ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const std::timespec& ts, FormatContext &ctx) const
		{
			return format_to(ctx.out(), "{}.{}", ts.tv_sec, ts.tv_nsec);
		}
	};

	int main()
	{
		xtr::logger log;

		xtr::sink s = log.get_sink("Main");

		XTR_LOG_TS(s, (std::timespec{123, 456}), "Hello world");

		return 0;
	}

View this example on `Compiler Explorer <https://godbolt.org/z/GcffPWjvz>`__.

Background Consumer Thread Details
----------------------------------

As no system calls are made when a log statement is made, the consumer
thread must spin waiting for input (it cannot block/wait as there would
be no way to signal that doesn't involve a system call). This is simply
done as a performance/efficiency trade-off; log statements become cheaper
at the cost of the consumer thread being wasteful.

Lifetime
~~~~~~~~

The consumer thread associated with a given logger will terminate only
when the logger and all associated sinks have been destructed, and is
joined by the logger destructor. This means that when the logger
destructs, it will block until all associated sinks have also destructed.

This is done to prevent creating 'orphan' sinks which are open but not being
read from by a logger. This should make using the logger easier as sinks will
never lose data and will never be disconnected from the associated logger
unless they are explicitly disconnected by closing the sink.

CPU Affinity
~~~~~~~~~~~~

To bind the background thread to a specific CPU
:cpp:func:`xtr::logger::consumer_thread_native_handle` can be used to obtain
the consumer thread's platform specific thread handle. The handle can then be
used with whatever platform specific functionality is available for setting
thread affinities---for example 
`pthread_setaffinity_np(3) <https://www.man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html>`__
on Linux.

Example
^^^^^^^

.. code-block:: c++

    #include <xtr/logger.hpp>

    #include <cerrno>

    #include <pthread.h>
    #include <sched.h>

    int main()
    {
        xtr::logger log;

        cpu_set_t cpus;
        CPU_ZERO(&cpus);
        CPU_SET(0, &cpus);

        const auto handle = log.consumer_thread_native_handle();

        if (const int errnum = ::pthread_setaffinity_np(handle, sizeof(cpus), &cpus))
        {
            errno = errnum;
            perror("pthread_setaffinity_np");
        }

        xtr::sink s = log.get_sink("Main");

        XTR_LOG(s, "Hello world");

        return 0;
    }

View this example on `Compiler Explorer <https://godbolt.org/z/1vh5exK4K>`__.

Disabling the Background Consumer Thread
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The consumer thread can be disabled by passing the
:cpp:enum:`xtr::option_flags_t::disable_worker_thread` flag to
:cpp:func:`xtr::logger::logger`. Users should then run
:cpp:func:`xtr::logger::pump_io` on a thread of their choosing in order to
process messages written to the logger.

Example
^^^^^^^

.. code-block:: c++

    #include <xtr/logger.hpp>

    #include <chrono>
    #include <cstdio>
    #include <thread>

    int main()
    {
        std::jthread thread;
        xtr::logger log(
            stderr,
            std::chrono::system_clock(),
            xtr::default_command_path(),
            xtr::default_log_level_style,
            xtr::option_flags_t::disable_worker_thread);

        thread = std::jthread(
            [&log]()
            {
                while (log.pump_io())
                    ;
            });

        xtr::sink s = log.get_sink("Main");

        XTR_LOG(s, "Hello world");

        return 0;
    }

View this example on `Compiler Explorer <https://godbolt.org/z/G6v5G1MrG>`__.

Note that :cpp:func:`xtr::logger::pump_io` will only return false after all
sinks have been closed and the logger has destructed, in order to ensure no log
data is lost. This means that the logger should be destructed before attempting
to join the background thread (otherwise a deadlock would occur).

Log Message Sanitizing
----------------------

Terminal escape sequences and unprintable characters in string arguments are escaped
for security. This is done because string arguments may contain user-supplied strings,
which a malicious user could take advantage of to place terminal escape sequences into
the log file. If these escape sequences are not removed by the logger then they could
be interpreted by the terminal emulator of a user viewing the log. Most terminal
emulators are sensible about the escape sequences they interpret, however it is still
good practice for a logger to err on the side of caution and remove them from string
arguments.
Please refer to
`this document <https://seclists.org/fulldisclosure/2003/Feb/att-341/Termulation.txt>`__
posted to the full-disclosure mailing list for a more thorough explanation of terminal
escape sequence attacks.

Log Rotation
------------

Please refer to the :ref:`reopening log files <reopening-log-files>` section of
the :ref:`xtrctl <xtrctl>` guide.

Custom Back-ends
----------------

The logger allows custom back-ends to be used. This is done by implementing the
:cpp:class:`xtr::storage_interface` interface:

.. code-block:: c++

    struct xtr::storage_interface
    {
        virtual std::span<char> allocate_buffer() = 0;

        virtual void submit_buffer(char* buf, std::size_t size) = 0;

        virtual void flush() = 0;

        virtual void sync() noexcept = 0;

        virtual int reopen() noexcept = 0;

        virtual ~storage_interface() = default;
    };

    using storage_interface_ptr = std::unique_ptr<storage_interface>;

Storage interface objects are then passed to the :cpp:type:`xtr::storage_interface_ptr`
argument of the `custom back-end constructor <api.html#_CPPv4I0EN3xtr6logger6loggerE21storage_interface_ptrRR5ClockNSt6stringE17log_level_style_t>`__.

.. NOTE::
   All back-end functions are invoked from the logger's background thread.

:allocate_buffer and submit_buffer:
    The *allocate_buffer* function is called by the logger to obtain a buffer
    where formatted log data will be written to. When the logger has finished
    writing to the buffer it is passed back to the back-end by calling
    *submit_buffer*.

    * The logger will not allocate a buffer without first submitting the previous
      buffer, if one exists.
    * Only the pointer returned by :cpp:expr:`std::span::data()` will be passed
      to the *buf* argument of submit_buffer.
    * Partial buffers may be submitted, i.e. the *size* argument passed to
      submit_buffer may be smaller than the span returned by allocate_buffer.
      After a partial buffer is submitted, the logger will allocate a new buffer.

:flush:
    The *flush* function is invoked to indicate that the back-end should write
    any buffered data to its associated backing store.

:sync:
    The *sync* function is invoked to indicate that the back-end should ensure
    that all data written to the associated backing store has reached permanent
    storage.

:reopen:
    The *reopen* function is invoked to indicate that if the back-end has a regular
    file opened for writing log data then the file should be reopened. This is
    intended to be used to implement log rotation via tool such as
    `logrotate(8) <https://www.man7.org/linux/man-pages/man8/logrotate.8.html>`__.
    Please refer to the :ref:`Reopening Log Files <reopening-log-files>` section
    of the :ref:`xtrctl <xtrctl>` documentation for further details.

    The return value should be either zero on success or an
    `errno(3) <https://www.man7.org/linux/man-pages/man3/errno.3.html>`__
    compatible error number on failure.

:destructor:
    Upon destruction the back-end should flush any buffered data and close the
    associated backing store.

Example
~~~~~~~

Using the `custom back-end constructor <api.html#_CPPv4I0EN3xtr6logger6loggerE21storage_interface_ptrRR5ClockNSt6stringE17log_level_style_t>`__
to create a logger with a storage back-end that discards all input:

.. code-block:: c++

    #include <xtr/logger.hpp>

    #include <span>
    #include <cstddef>

    namespace
    {
        struct discard_storage : xtr::storage_interface
        {
            std::span<char> allocate_buffer()
            {
                return buf;
            }

            void submit_buffer(char* buf, std::size_t size)
            {
            }

            void flush()
            {
            }

            void sync() noexcept
            {
            }

            int reopen() noexcept
            {
                return 0;
            }

            char buf[1024];
        };
    }

    int main()
    {
        xtr::logger log(std::make_unique<discard_storage>());

        xtr::sink s = log.get_sink("Main");

        XTR_LOG(s, "Hello world");

        return 0;
    }

View this example on `Compiler Explorer <https://godbolt.org/z/W4YE7YqPr>`__.

.. _custom-log-level-styles:

Custom Log Level Styles
-----------------------

The text at the beginning of each log statement representing the log level of
the statement can be customised via :cpp:func:`xtr::logger::set_log_level_style`,
which accepts a function pointer of type
:cpp:type:`xtr::log_level_style_t`. The passed function should accept
a single argument of type :cpp:enum:`xtr::log_level_t` and should return
a :cpp:expr:`const char*` string literal to be used as the log level prefix.
Please refer to the :cpp:type:`xtr::log_level_style_t` documentation for
further details.

Example
~~~~~~~

The following example will output::

    info: 2021-09-17 23:36:39.043028 Main <source>:18: Hello world
    not-info: 2021-09-17 23:36:39.043028 Main <source>:19: Hello world

.. code-block:: c++

    #include <xtr/logger.hpp>

    xtr::logger log;

    xtr::sink s = log.get_sink("Main");

    log.set_log_level_style(
        [](auto level)
        {
            return
                level == xtr::log_level_t::info ?
                    "info: " :
                    "not-info: ";
        });

    XTR_LOGL(info, s, "Hello world");
    XTR_LOGL(error, s, "Hello world");

View this example on `Compiler Explorer <https://godbolt.org/z/ohcW6ndoz>`__.

Logging to the Systemd Journal
------------------------------

To support logging to systemd, log level prefixes suitable for logging to the
systemd journal can be enabled by passing the
:cpp:func:`xtr::systemd_log_level_style` to
:cpp:func:`xtr::logger::set_log_level_style`. Please refer to the
:cpp:type:`xtr::log_level_style_t` documentation for further details on log
level styles.

Example
~~~~~~~

.. code-block:: c++

    #include <xtr/logger.hpp>

    xtr::logger log;

    log.set_log_level_style(xtr::systemd_log_level_style);

    // If the systemd journal is to be used then it is advisable to set the log
    // level to debug and use journalctl to filter by log level instead.
    log.set_default_log_level(xtr::log_level_t::debug);

    xtr::sink s = log.get_sink("Main");

    XTR_LOGL(debug, s, "Debug");
    XTR_LOGL(info, s, "Info");
    XTR_LOGL(warning, s, "Warning");
    XTR_LOGL(error, s, "Error");

View this example on `Compiler Explorer <https://godbolt.org/z/zvsjech4a>`__.

The output of the above example will be something like::

    <7>2022-12-18 16:01:16.205253 Main xtrdemo.cpp:12: Debug
    <6>2022-12-18 16:01:16.205259 Main xtrdemo.cpp:13: Info
    <4>2022-12-18 16:01:16.205259 Main xtrdemo.cpp:14: Warning
    <3>2022-12-18 16:01:16.205259 Main xtrdemo.cpp:15: Error

If the example is run under systemd via e.g. ``systemd-run --quiet --user --wait ./xtrdemo`` then
the messages logged to the journal can be viewed via e.g.
``journalctl --quiet --no-hostname --identifier xtrdemo``::

    Dec 18 16:01:16 xtrdemo[1008402]: 2022-12-18 16:01:16.205253 Main xtrdemo.cpp:13: Debug
    Dec 18 16:01:16 xtrdemo[1008402]: 2022-12-18 16:01:16.205259 Main xtrdemo.cpp:14: Info
    Dec 18 16:01:16 xtrdemo[1008402]: 2022-12-18 16:01:16.205259 Main xtrdemo.cpp:15: Warning
    Dec 18 16:01:16 xtrdemo[1008402]: 2022-12-18 16:01:16.205259 Main xtrdemo.cpp:16: Error

Disabling Exceptions
--------------------

Exceptions may be disabled by building XTR with the appropriate option:

* ``EXCEPTIONS=0`` if using Make.
* ``ENABLE_EXCEPTIONS=OFF`` if using CMake.
* ``xtr:enable_exceptions=False`` if using Conan.

This will cause XTR to be built with the ``-fno-exceptions`` flag.

If exceptions are disabled then when an error occurs that would have thrown an
exception, an error message is instead printed to `stderr` and the program
terminates via  `abort(3) <https://www.man7.org/linux/man-pages/man3/abort.3.html>`__.

.. rubric:: Footnotes

.. [#queue] Specifically the queue is a single-producer/single-consumer ring buffer.
