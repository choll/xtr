.. title:: XTR Logger API Reference

API Reference
=============

.. _log-macros:

Log Macros
----------

.. _basic-macros:

Basic Macros
~~~~~~~~~~~~

The following macros log without reading the time of day at the point of
logging, instead it is read when the log message is received by the background
thread. Use these macros if a loss of timestamp accuracy is acceptable to gain
performance.

.. doxygendefine:: XTR_LOG
.. doxygendefine:: XTR_LOGL
.. doxygendefine:: XTR_TRY_LOG
.. doxygendefine:: XTR_TRY_LOGL

.. _tsc-macros:

Time-Stamp Counter Timestamped Macros
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following macros log and read the time-stamp counter at the point of
logging. Use these macros if the most accurate timestamps are required. On some
systems reading rdtsc may be slower than using the real-time clock (see below).

.. doxygendefine:: XTR_LOG_TSC
.. doxygendefine:: XTR_LOGL_TSC
.. doxygendefine:: XTR_TRY_LOG_TSC
.. doxygendefine:: XTR_TRY_LOGL_TSC

.. _rtc-macros:

Real-Time Clock Timestamped Macros
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following macros log and read the system real-time clock at the point of
logging. Use these macros if accurate timestamps are required.

.. doxygendefine:: XTR_LOG_RTC
.. doxygendefine:: XTR_LOGL_RTC
.. doxygendefine:: XTR_TRY_LOG_RTC
.. doxygendefine:: XTR_TRY_LOGL_RTC

.. _user-supplied-timestamp-macros:

User-Supplied Timestamp Macros
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following macros log and accept a user-supplied timestamp.

.. doxygendefine:: XTR_LOG_TS
.. doxygendefine:: XTR_LOGL_TS
.. doxygendefine:: XTR_TRY_LOG_TS
.. doxygendefine:: XTR_TRY_LOGL_TS

.. _logger:

Logger
------

.. doxygenclass:: xtr::logger
    :members:

.. _sink:

Sink
----

.. doxygenclass:: xtr::sink
    :members:

Nocopy
------

.. doxygenfunction:: xtr::nocopy

.. _log-levels:

Log Levels
----------

.. doxygenenum:: xtr::log_level_t

If the *none* level is applied to a sink then all log statements will be
disabled. Fatal log statements will still call
`abort(3) <https://www.man7.org/linux/man-pages/man3/abort.3.html>`__, however.

.. doxygenfunction:: xtr::log_level_from_string

Log Level Styles
----------------

.. doxygentypedef:: xtr::log_level_style_t
.. doxygenfunction:: xtr::default_log_level_style
.. doxygenfunction:: xtr::systemd_log_level_style

Storage Interfaces
------------------

.. doxygenstruct:: xtr::storage_interface
    :members:

.. doxygentypedef:: xtr::storage_interface_ptr

.. doxygenclass:: xtr::io_uring_fd_storage
    :members:

.. doxygenclass:: xtr::posix_fd_storage
    :members:

.. doxygenfunction:: xtr::make_fd_storage(const char *path)

.. doxygenfunction:: xtr::make_fd_storage(FILE *fp, std::string reopen_path)

.. doxygenfunction:: xtr::make_fd_storage(int fd, std::string reopen_path)

.. doxygenvariable:: null_reopen_path

Default Command Path
--------------------

.. doxygenfunction:: default_command_path

Null Command Path
-----------------

.. doxygenvariable:: null_command_path

Configuration Variables
-----------------------

The header file `xtr/config.hpp` contains configuration variables that may be
overridden by users.

.. doxygendefine:: XTR_SINK_CAPACITY
.. doxygendefine:: XTR_USE_IO_URING
.. doxygendefine:: XTR_IO_URING_POLL
