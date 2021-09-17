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

Examples
~~~~~~~~

.. code-block:: c++

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

Examples
~~~~~~~~

.. code-block:: c++

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

    XTR_LOG(info, s, "Hello {}", foo());

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
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const custom&, FormatContext &ctx)
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

View this example on `Compiler Explorer <https://godbolt.org/z/Woov3fMsr>`__.

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

Examples
^^^^^^^^

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
		auto format(const std::timespec& ts, FormatContext &ctx)
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

Examples
^^^^^^^^

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

The logger allows custom back-ends to be used. This is done by constructing the logger
with functions that implement the back-end functionality, which are listed below:

+----------+-------------------------------------------------------------+-------------+
| Function | Signature                                                   | Description |
+==========+=============================================================+=============+
| Output   | :cpp:func:`::ssize_t out(const char* buf, std::size_t size)`| |output|    |
+----------+-------------------------------------------------------------+-------------+
| Error    | :cpp:func:`void err(const char* buf, std::size_t size)`     | |error|     |
+----------+-------------------------------------------------------------+-------------+
| Flush    | :cpp:func:`void flush()`                                    | |flush|     |
+----------+-------------------------------------------------------------+-------------+
| Sync     | :cpp:func:`void sync()`                                     | |sync|      |
+----------+-------------------------------------------------------------+-------------+
| Reopen   | :cpp:func:`void reopen()`                                   | |reopen|    |
+----------+-------------------------------------------------------------+-------------+
| Close    | :cpp:func:`void close()`                                    | |close|     |
+----------+-------------------------------------------------------------+-------------+

.. |output| replace:: replacement text

.. |error| replace:: replacement text

.. |flush| replace:: replacement text

.. |sync| replace:: replacement text

.. |reopen| replace:: replacement text

.. |close| replace:: replacement text

NEED TO MAKE IT CLEAR THAT OUT IS CALLED ONCE PER LOG LINE

Examples
~~~~~~~~

Custom back-end using the
`basic custom back-end constructor <api.html#_CPPv4I000EN3xtr6logger6loggerERR14OutputFunctionRR13ErrorFunctionRR5ClockNSt6stringE>`__:

.. code-block:: c++

    std::vector<std::string> lines;
    std::mutex lines_mutex;

    xtr::logger log(
        [&](const char* buf, std::size_t size)
        {
            std::unique_lock lock{lines_mutex};
            lines.emplace_back(buf, size);
            return size;
        },
        [](const char* buf, std::size_t size)
        {
            std::cout << std::string_view{buf, size} << "\n";
        }
    );

    xtr::sink s = log.get_sink("Main");

    XTR_LOG(s, "Hello world");

    s.sync(); // Ensure all log statements are delivered to the back-end

    std::cout << lines[0] << "\n";

Custom back-end using the
`custom back-end constructor <api.html#_CPPv4I0000000EN3xtr6logger6loggerERR14OutputFunctionRR13ErrorFunctionRR13FlushFunctionRR12SyncFunctionRR14ReopenFunctionRR13CloseFunctionRR5ClockNSt6stringE>`__:

.. code-block:: c++

    TODO

Custom Log Level Styles
-----------------------

The text at the beginning of each log statement representing the log level of
the statement can be customised via :cpp:func:`xtr::logger::set_log_level_style`,
which accepts a function pointer of type
:cpp:type:`xtr::log_level_style_t`. The passed function should accept
a single argument of type :cpp:enum:`xtr::log_level_t` and should return
a :cpp:expr:`const char*` string literal.

Examples
~~~~~~~~

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

Will output::

    info: 2021-09-17 23:36:39.043028 Main <source>:18: Hello world
    not-info: 2021-09-17 23:36:39.043028 Main <source>:19: Hello world

.. rubric:: Footnotes

.. [#queue] Specifically the queue is a single-producer/single-consumer ring buffer.
