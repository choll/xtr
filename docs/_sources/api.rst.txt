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
.. doxygendefine:: XTR_LOG_TSC_DEBUG
.. doxygendefine:: XTR_LOG_TSC_INFO
.. doxygendefine:: XTR_LOG_TSC_WARN
.. doxygendefine:: XTR_LOG_TSC_ERROR
.. doxygendefine:: XTR_LOG_TSC_FATAL

Real-time Clock
^^^^^^^^^^^^^^^

.. doxygendefine:: XTR_LOG_RTC
.. doxygendefine:: XTR_LOG_RTC_DEBUG
.. doxygendefine:: XTR_LOG_RTC_INFO
.. doxygendefine:: XTR_LOG_RTC_WARN
.. doxygendefine:: XTR_LOG_RTC_ERROR
.. doxygendefine:: XTR_LOG_RTC_FATAL

User-Supplied Timestamp
^^^^^^^^^^^^^^^^^^^^^^^

.. doxygendefine:: XTR_LOG_TS
.. doxygendefine:: XTR_LOG_TS_DEBUG
.. doxygendefine:: XTR_LOG_TS_INFO
.. doxygendefine:: XTR_LOG_TS_WARN
.. doxygendefine:: XTR_LOG_TS_ERROR
.. doxygendefine:: XTR_LOG_TS_FATAL

.. _logger:

Logger
------

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
