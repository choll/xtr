.. title:: XTR Logger Quick-Start Guide

Quick-Start Guide
=================

Installing
----------

The easiest way to install XTR is via Conan. If you are not using Conan, see the
`INSTALL <https://github.com/choll/xtr/blob/master/INSTALL.md>`__ guide, or for the
truly impatient there is a single-include file
`here <https://github.com/choll/xtr/blob/master/single_include/xtr/logger.hpp>`__.

Overview
--------

The logger is split into two main components, the :ref:`logger <logger>` class
and the :ref:`sink <sink>` class. The logger takes care of opening and closing the log file,
and is thread-safe. The sink class is used to write to the log. Sinks are
created by calling :cpp:func:`xtr::logger::get_sink` and are not thread
safe---the idea is that applications have many sinks, so threads should each
have their own set of separate sinks.

Log messages are written using various :ref:`macros <log-macros>` which accept
a sink as their first argument, followed by a Python
`str.format <https://docs.python.org/3/library/string.html#formatstrings>`__
style format string. The `{fmt} <https://fmt.dev>`__ library is used for
formatting.

Examples
--------

Creating a sink:

.. code-block:: c++

    #include <xtr/logger.hpp>

    xtr::logger log;

    xtr::sink s = log.get_sink("Main");

Writing to the log, blocking if the sink is full, reading the timestamp
in the background thread [#timestamps]_:

.. code-block:: c++

    XTR_LOG(s, "Hello world");

Write to the log, discarding the message if the sink is full, reading the
timestamp in the background thread:

.. code-block:: c++

    XTR_TRY_LOG(s, "Hello world");

Write to the log, immediately reading the timestamp from the TSC:

.. code-block:: c++

    XTR_LOG_TSC(s, "Hello world");

Write to the log, immediately reading the timestamp using
`clock_gettime(3) <https://www.man7.org/linux/man-pages/man3/clock_gettime.3.html>`__
with a clock source of either CLOCK_REALTIME_COARSE on Linux or CLOCK_REALTIME_FAST
on FreeBSD:

.. code-block:: c++

    XTR_LOG_RTC(s, "Hello world");

Write to the log if the log level of the sink is at the 'info' level or a level
with lower importance than 'info'. The default sink level 'info' so this
message will be logged:

.. code-block:: c++

    XTR_LOG_INFO(s, "Hello world");

@ref XTR_LOGI is short-hand for @ref XTR_LOG_INFO, as is @ref XTR_LOGE for
@ref XTR_LOG_ERROR etc:

.. code-block:: c++

    XTR_LOGI(s, "Hello world");

Set the log level of the sink 's' to 'error', causing messages with importance
lower than 'error' to be dropped. Available log levels are debug, info, warning,
error and fatal---see :cpp:enum:`xtr::log_level_t`.

.. code-block:: c++

    s.set_level(xtr::log_level_t::error);

    XTR_LOG_INFO(s, "Hello world"); // Dropped

Fatal errors will log and then terminate the program using
`abort(3) <https://www.man7.org/linux/man-pages/man3/abort.3.html>`__:

.. code-block:: c++

    XTR_LOG_FATAL(s, "Goodbye cruel world");
    // NOTREACHED

By default, objects and strings are copied into the sink. This is so that the
default behaviour is safe---i.e. to avoid creating dangling references the
logger does not assume anything about the lifetime of objects passed as
arguments:

.. code-block:: c++

    const std::string str1 = "Hello";
    const char* str2 = "world";
    XTR_LOG("{} {}", str1, str2);

To avoid copying, wrap arguments in a call to :cpp:func:`xtr::nocopy`:

.. code-block:: c++

    XTR_LOG("{} {}", nocopy(str1), nocopy(str2));

Arguments may also be moved in to the logger:

.. code-block:: c++

    std::string str3 = "world";
    XTR_LOG("Hello {}", std::move(str3));

.. rubric:: Footnotes

.. [#timestamps] The behaviour for XTR_LOG is that timestamps are read when
                 the background thread reads the event from the sink. This is
                 less accurate, but faster than reading the time at the log
                 call-site. If reading the time at the call-site is preferred,
                 use XTR_LOG_TSC or XTR_LOG_RTC. See the
                 :ref:`time sources <time-sources>` section of the user guide
                 for further information.
