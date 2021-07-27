XTR
***

XTR is a C++ logging library aimed at applications with low-latency or real-time
requirements. The cost of log statements is minimised by delegating as much work
as possible to a background thread.

It is designed so that the cost of a log statement is consistently fast---i.e.
every call is fast, not just the average case. No allocations or system calls
are made when a log statement is made.

Loggers and Sinks
=================

The XTR API contains two classes; :cpp:class:`xtr::logger` and
:cpp:class:`xtr::logger::sink`. Sinks each contain a queue [#queue]_, and
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

Sinks are created by calling :cpp:func:`xtr::logger::get_sink` or via normal
construction followed by a call to :cpp:func:`xtr::logger::register_sink`.
Once a sink has been created or registered, it may be written to using one of several
log macros which are described :ref:`below <macros>`.

Format String Syntax
====================

XTR uses `{fmt} <https://fmt.dev>`__ for formatting, so format strings follow the
same Python `str.format <https://docs.python.org/3/library/stdtypes.html#str.format>`__
style formatting as found in `{fmt}`, the documentation for which can be found
`here <https://fmt.dev/latest/syntax.html>`__.

Examples
========

.. code-block:: c++

    #include "xtr/logger.hpp"

    xtr::logger log;

    auto s = log.get_sink("Main");

    XTR_LOG(s, "Hello world {} {}", 123, 123.456);

.. code-block:: c++

    #include "xtr/logger.hpp"

    xtr::logger log;

    xtr::logger::sink s;

    log.register_sink(s, "Main");

    XTR_LOG(s, "Hello world {} {}", 123, 123.456);

LINK TO GODBOLT?

:cpp:enum:log_level_t:

.. _copy_val_ref:

Passing Arguments by Value or Reference
=======================================

The default behaviour of the logger is to copy format arguments into the
specified sink by value. No allocations will be performed unless an argument type
is passed to the sink which has a copy constructor that allocates, with the exception
of `std::string` (see :ref:`below <string_args>` for details). If copying is undesirable then
arguments may be passed by reference by wrapping them in a call to :cpp:func:`xtr::nocopy`:

.. code-block:: c++

    XTR_LOG(sink, "{}", nocopy(arg));

versus copying by value:

.. code-block:: c++

    XTR_LOG(sink, "{}", arg);

.. _string_args:

String Arguments
================

Passing strings to the logger is guaranteed to not allocate memory, and does
not assume anything about the lifetime of the string data. i.e. for the
following log statement:

.. code-block:: c++

    XTR_LOG(sink, "{}", str);

If `str` is a :cpp:expr:`std::string`, :cpp:expr:`std::string_view`,
:cpp:expr:`char*` or :cpp:expr:`char[]` then the contents of `str` will be copied
into `sink` without incurring any allocations. String data is copied in order
to provide safe default behaviour regarding the lifetime of the string data. If
copying the string data is undesirable then string arguments may be wrapped in
a call to :cpp:func:`xtr::nocopy`:

.. code-block:: c++

    XTR_LOG(sink, "{}", nocopy(str));

If this is done then only a pointer to the string data contained in `str` is
copied. The user is then responsible for ensuring that the string data remains
valid long enough for the logger to process the log statement. Note that only
the string data must remain valid---so for :cpp:expr:`std::string_view` the
object itself does not need to remain valid, just the data it references.

Thread Safety
=============

 * All functions in :cpp:class:`xtr::logger` are thread-safe.
 * No functions in :cpp:class:`xtr::logger::sink` are thread-safe other than
   ::cpp:class:`xtr::logger::level` and ::cpp:class:`xtr::logger::set_level`.
   This is because each thread is expected to have its own independent
   sink (or set of sinks).

Custom Formatters
=================

Custom formatters are implemented the same as in `{fmt} <https://fmt.dev>`__,
which is done either by:

* Providing a :cpp:func:`std::stream& operator<<(std::stream&, T&)` overload.
* Specializing :cpp:expr:`fmt::formatter<T>` and implementing the `parse` and
  `format` methods as described by the `{fmt}` documentation
  `here <https://fmt.dev/latest/api.html#formatting-user-defined-types>`__.

Time Sources
============

As reading the current time of day can be done in various different ways, with different
trade-offs, XTR supports 

XTR supports multiple time-sources when logging messages.

* Asynchronous-default: The default time-source is to read `std::chrono::system_clock`
  *in the logger background thread*. This is to avoid the expense of reading the clock
  at the logging call-site. The trade-off is that this comes with a loss of accuracy
  due to the time the log message spends on the queue.
* Asynchronous-custom

* Synchronous-TSC
* Synchronous-Coarse
* Synchronous-Custom



Default
-------

TSC
---




TSC Calibration

Real-time Clock (clock_gettime)
-------------------------------

XTR_LOG_RTC, XTR_TRY_LOG_RTC

Arbitrary Sources
-----------------

XTR_LOG_TS,  XTR_TRY_LOG_TS

Customising the Time Format
===========================



Background Consumer Thread Details
==================================

As no system calls are made when a log statement is made, the consumer
thread must spin waiting for input (it cannot block/wait as there would
be no way to signal that doesn't involve a system call). This is simply
done as a performance/efficiency trade-off; log statements become cheaper
at the cost of the consumer thread being wasteful.

Lifetime
--------

The consumer thread associated with a given logger will terminate only
when the logger and all associated sinks have been destructed, and is
joined by the logger destructor. This means that when the logger
destructs, it will block until all associated sinks have also destructed.

This is done to make using the logger easier---sinks will never lose data
and will never be disconnected from the associated logger unless they are
explicitly disconnected by closing the sink.

CPU Affinity
------------

To bind the background thread to a specific CPU
:cpp:func:`xtr::logger::consumer_thread_native_handle` can be used to obtain
the consumer thread's platform specific thread handle. The handle can then be
used with whatever platform specific functionality is available for setting
thread affinities---for example 
`pthread_setaffinity_np(3) <https://www.man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html>`__
on Linux.

Log Message Sanitizing
======================

STRINGS ARE SANITIZED, PROVIDE CUSTOM FORMATTER TO WRITE BINARY DATA

Strings containing unprintable characters are sanitized 

Custom Back-ends
================



API Reference
=============

.. _macros:

Log Macros
----------

.. _logger:

Logger
------

.. doxygenclass:: xtr::logger
.. doxygenfunction:: xtr::logger::get_sink
.. doxygenfunction:: xtr::logger::register_sink

:cpp:class:`xtr::logger` is the main logger class. When constructed a
background thread will be created. The background thread is joined when the
logger and all of its sinks have been destructed.

.. _sink:

Sink
----

In order to write to the logger, an instance of :cpp:class:`xtr::logger::sink`
must must be created via a call to . Each
sink has its own queue which is used to communicate to the logger. Sink
operations are not thread safe---threads are instead expected to each use
one or more sinks which are local to the thread.

.. doxygenclass:: xtr::logger::sink
.. doxygenfunction:: xtr::logger::sink::close
.. doxygenfunction:: xtr::logger::sink::sync()
.. doxygenfunction:: xtr::logger::sink::set_name
.. doxygenfunction:: xtr::logger::sink::log
.. doxygenfunction:: xtr::logger::sink::set_level
.. doxygenfunction:: xtr::logger::sink::level

Misc
----

.. doxygenfunction:: xtr::nocopy
.. doxygenenum:: xtr::log_level_t

.. rubric:: Footnotes

.. [#queue] Specifically the queue is a single-producer/single-consumer ring buffer.


