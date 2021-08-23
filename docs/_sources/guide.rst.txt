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

Sinks are created by calling :cpp:func:`xtr::logger::get_sink` or via normal
construction followed by a call to :cpp:func:`xtr::logger::register_sink`.
Once a sink has been created or registered, it may be written to using one of several
log macros which are described in the :ref:`log macros <log-macros>` section.

Examples
~~~~~~~~

.. code-block:: c++

    #include <xtr/logger.hpp>

    xtr::logger log;

    xtr::sink s = log.get_sink("Main");

    XTR_LOG(s, "Hello world");

View this example on `Compiler Explorer <https://godbolt.org/z/1GWbEPq8T>`__.

.. code-block:: c++

    #include <xtr/logger.hpp>

    xtr::logger log;
    xtr::sink s;

    log.register_sink(s, "Main");

    XTR_LOG(s, "Hello world");

View this example on `Compiler Explorer <https://godbolt.org/z/cobj4n3Gx>`__.

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

The logger supports debug, info, warning, error and fatal log levels.
Log statements with these levels may be produced using the
:c:macro:`XTR_LOG_DEBUG`, :c:macro:`XTR_LOG_INFO`, :c:macro:`XTR_LOG_WARN`
:c:macro:`XTR_LOG_ERROR` and :c:macro:`XTR_LOG_FATAL` macros, along with
additional macros which are described in the :ref:`log macros <log-macros>`
section of the API reference.

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

    XTR_LOG_INFO(s, "Hello {}", foo());

View this example on `Compiler Explorer <https://godbolt.org/z/G4P4zfP6r>`__.

Debug Log Statements
~~~~~~~~~~~~~~~~~~~~

Debug log statements can be disabled by defining XTR_NDEBUG.

Fatal Log Statements
~~~~~~~~~~~~~~~~~~~~

Fatal log statements will additionally call :cpp:func:`xtr::sink::sync` followed
by `abort(3) <https://www.man7.org/linux/man-pages/man3/abort.3.html>`__.

Setting the default log level
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TODO

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
        auto format(const custom &, FormatContext &ctx)
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

View this example on `Compiler Explorer <https://godbolt.org/z/h5vsv354E>`__.

Time Sources
------------

XTR supports multiple time-sources when logging messages, with varying levels of
accuracy and performance.

* Asynchronous-default: The default time-source is to read `std::chrono::system_clock`
  *in the logger background thread*. This is to avoid the expense of reading the clock
  at the logging call-site. The trade-off is that this comes with a loss of accuracy
  due to the time the log message spends on the queue.
* Asynchronous-custom

* Synchronous-TSC
* Synchronous-Coarse
* Synchronous-Custom

Basic with Default Clock
~~~~~~~~~~~~~~~~~~~~~~~~

For :cpp:def


See the :ref:`basic log macros <basic-macros>` section of the API reference for
details.

Basic with Custom Clock
~~~~~~~~~~~~~~~~~~~~~~~


TSC
~~~

TSC Calibration

Real-time Clock
~~~~~~~~~~~~~~~

XTR_LOG_RTC, XTR_TRY_LOG_RTC

User-Supplied Timestamp
~~~~~~~~~~~~~~~~~~~~~~~

XTR_LOG_TS,  XTR_TRY_LOG_TS

Customising the Time Format
---------------------------

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

This is done to make using the logger easier---sinks will never lose data
and will never be disconnected from the associated logger unless they are
explicitly disconnected by closing the sink.

CPU Affinity
~~~~~~~~~~~~

To bind the background thread to a specific CPU
:cpp:func:`xtr::logger::consumer_thread_native_handle` can be used to obtain
the consumer thread's platform specific thread handle. The handle can then be
used with whatever platform specific functionality is available for setting
thread affinities---for example 
`pthread_setaffinity_np(3) <https://www.man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html>`__
on Linux.



Log Message Sanitizing
----------------------

STRINGS ARE SANITIZED, PROVIDE CUSTOM FORMATTER TO WRITE BINARY DATA

Strings containing unprintable characters are sanitized 

Custom Back-ends
----------------

.. rubric:: Footnotes

.. [#queue] Specifically the queue is a single-producer/single-consumer ring buffer.
