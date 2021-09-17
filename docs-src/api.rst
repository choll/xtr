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
.. doxygendefine:: XTR_LOGL
.. doxygendefine:: XTR_TRY_LOG
.. doxygendefine:: XTR_TRY_LOGL

Timestamped Macros
~~~~~~~~~~~~~~~~~~

.. _tsc-macros:

TSC
^^^

.. doxygendefine:: XTR_LOG_TSC
.. doxygendefine:: XTR_LOGL_TSC
.. doxygendefine:: XTR_TRY_LOG_TSC
.. doxygendefine:: XTR_TRY_LOGL_TSC

.. _rtc-macros:

Real-time Clock
^^^^^^^^^^^^^^^

.. doxygendefine:: XTR_LOG_RTC
.. doxygendefine:: XTR_LOGL_RTC
.. doxygendefine:: XTR_TRY_LOG_RTC
.. doxygendefine:: XTR_TRY_LOGL_RTC

.. _user-supplied-timestamp-macros:

User-Supplied Timestamp
^^^^^^^^^^^^^^^^^^^^^^^

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

Log Level Styles
----------------

.. doxygentypedef:: xtr::log_level_style_t
.. doxygenfunction:: xtr::default_log_level_style
.. doxygenfunction:: xtr::systemd_log_level_style

Default command path
--------------------

.. doxygenfunction:: default_command_path

Null command path
-----------------

.. doxygenvariable:: null_command_path
