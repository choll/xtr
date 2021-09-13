.. title:: XTR Logger API Reference

API Reference
=============

.. _log-macros:

Log Macros
----------

.. _basic-macros:

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

.. _tsc-macros:

TSC
^^^

.. doxygendefine:: XTR_LOG_TSC
.. doxygendefine:: XTR_LOG_TSC_DEBUG
.. doxygendefine:: XTR_LOG_TSC_INFO
.. doxygendefine:: XTR_LOG_TSC_WARN
.. doxygendefine:: XTR_LOG_TSC_ERROR
.. doxygendefine:: XTR_LOG_TSC_FATAL

.. _rtc-macros:

Real-time Clock
^^^^^^^^^^^^^^^

.. doxygendefine:: XTR_LOG_RTC
.. doxygendefine:: XTR_LOG_RTC_DEBUG
.. doxygendefine:: XTR_LOG_RTC_INFO
.. doxygendefine:: XTR_LOG_RTC_WARN
.. doxygendefine:: XTR_LOG_RTC_ERROR
.. doxygendefine:: XTR_LOG_RTC_FATAL

.. _user-supplied-timestamp-macros:

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

Default command path
--------------------

.. doxygenfunction:: default_command_path

Null command path
-----------------

.. doxygenvariable:: null_command_path
