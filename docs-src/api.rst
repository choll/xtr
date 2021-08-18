.. title:: XTR Logger API Reference

API Reference
=============

.. _log-macros:



Log Macros
----------

Basic Macros
~~~~~~~~~~~~

.. doxygendefine:: XTR_LOG
.. doxygendefine:: XTR_LOG_DEBUG
.. doxygendefine:: XTR_LOG_INFO
.. doxygendefine:: XTR_LOG_WARN
.. doxygendefine:: XTR_LOG_ERROR
.. doxygendefine:: XTR_LOG_FATAL

Timestamped Macros
~~~~~~~~~~~~~~~~~~

TSC
^^^

.. doxygendefine:: XTR_LOG_TSC

Real-time Clock
^^^^^^^^^^^^^^^

.. doxygendefine:: XTR_LOG_RTC

User Supplied Timestamp
^^^^^^^^^^^^^^^^^^^^^^^

.. doxygendefine:: XTR_LOG_TS

Non-Blocking, Timestamped Macros
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TSC
^^^

.. doxygendefine:: XTR_TRY_LOG_TSC

Real-time Clock
^^^^^^^^^^^^^^^

.. doxygendefine:: XTR_TRY_LOG_RTC

User Supplied Timestamp
^^^^^^^^^^^^^^^^^^^^^^^

.. doxygendefine:: XTR_TRY_LOG_TS

.. _logger:

Logger
------

:cpp:class:`xtr::logger` is the main logger class. When constructed a
background thread will be created. The background thread is joined when the
logger and all of its sinks have been destructed.

.. doxygenclass:: xtr::logger


.. doxygenfunction:: xtr::logger::logger(const char*, Clock&&, std::string)
.. doxygenfunction:: xtr::logger::logger(const char*, FILE*, FILE*, Clock&&, std::string)
.. doxygenfunction:: xtr::logger::logger(FILE*, FILE*, Clock&&, std::string)


.. doxygenfunction:: xtr::logger::~logger

.. doxygenfunction:: xtr::logger::get_sink
.. doxygenfunction:: xtr::logger::register_sink
.. doxygenfunction:: xtr::logger::consumer_thread_native_handle

.. doxygenfunction:: xtr::logger::set_output_stream
.. doxygenfunction:: xtr::logger::set_error_stream



.. doxygenfunction:: xtr::logger::set_output_function
.. doxygenfunction:: xtr::logger::set_error_function
.. doxygenfunction:: xtr::logger::set_flush_function
.. doxygenfunction:: xtr::logger::set_sync_function
.. doxygenfunction:: xtr::logger::set_reopen_function
.. doxygenfunction:: xtr::logger::set_close_function



.. doxygenfunction:: xtr::logger::set_command_path

.. _sink:

Sink
----

In order to write to the logger, an instance of :cpp:class:`xtr::logger::sink`
must must be created via a call to . Each
sink has its own queue which is used to send log messages to the logger. Sink
operations are not thread safe---threads are instead expected to each use
one or more sinks which are local to the thread.

.. doxygenclass:: xtr::logger::sink
.. doxygenfunction:: xtr::logger::sink::set_name
.. doxygenfunction:: xtr::logger::sink::set_level
.. doxygenfunction:: xtr::logger::sink::level
.. doxygenfunction:: xtr::logger::sink::close
.. doxygenfunction:: xtr::logger::sink::sync()
.. doxygenfunction:: xtr::logger::sink::log

Misc
----

.. doxygenfunction:: xtr::nocopy
.. doxygenenum:: xtr::log_level_t
