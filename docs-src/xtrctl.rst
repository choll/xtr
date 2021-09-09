.. _xtrctl:

xtrctl
======

Synopsis
--------

xtrctl [--help] <command> [<args>] <socket path>

Description
-----------

xtrctl is a command line tool that can be used to query the status of log
sinks, modify log levels and reopen log files (for rotation) for the xtr
logger.

Commands
--------

Querying Sink Status
~~~~~~~~~~~~~~~~~~~~

xtrctl status [options] [pattern] <socket path>

The status command displays status information for sinks matching the given
pattern, or for all sinks if no pattern is specified. The sink name, current
log level, buffer capacity, current used buffer space and number of dropped
log messages are displayed. For example::

    ExampleName (info) 64K capacity, 0K used, 0 dropped

For an explanation of *pattern* and *options* please refer to the
see :ref:`PATTERNS <patterns>` and see :ref:`OPTIONS <options>` sections.

Setting Log Levels
~~~~~~~~~~~~~~~~~~

xtrctl level <level> [options] [pattern] <socket path>

The level command sets the log level to *level* for sinks matching the given
pattern, or for all sinks if no pattern is specified. Valid values for *level*
are 'none', 'fatal', 'error', 'warning', 'info' or 'debug' (please refer to the
:ref:`log levels <log-levels>` section of the API reference or **libxtr**\(3\)).

.. _reopening-log-files:

Reopening Log Files
~~~~~~~~~~~~~~~~~~~

xtrctl reopen <socket path>

The reopen command simply reopens the log file (if a file is being logged to).
This command exists to support log file rotation by integrating with existing
tools such as **logrotate**\(8\), for example by sending a reopen command from
the *postrotate* script defined in logrotate's configuration file::

    /path/to/your/log {
        rotate 5
        weekly
        postrotate
            xtrctl reopen /path/to/xtrctl/socket
        endscript
    }

.. _patterns:

Patterns
--------

In the status and level commands *pattern* is a regular expression or wildcard
that may be used to selectively apply the command to sinks with names matching
the given pattern. If no pattern is specified then the command applies to all
sinks. By default the pattern is interpreted as a basic regular expression,
to use an extended regular expression or wildcard please refer to the
:ref:`OPTIONS <options>` section.

.. _options:

Options
-------

The *status* and *level* commands support the following options:

**-E, --extended-regexp**
    Interpret *pattern* as extended regular expressions (see **regex**\(7\)).

**-G, --basic-regex**
    Interpret *pattern* options as basic regular expressions (see **regex**\(7\)).

**-W, --wildcard**
    Interpret *pattern* options as shell wildcard patterns (see **glob**\(7\)).

.. _socket-paths:

Socket Paths
------------

Please refer to the documentation for :cpp:func:`xtr::default_command_path` in
the API reference or **libxtr**\(3\) for an explanation of how the default
socket path is constructed. Custom command paths may be specified either by
passing an argument to :cpp:func:`xtr::logger::logger` or by calling
:cpp:func:`xtr::logger::set_command_path`, the documentation for both may also
be found in the API reference or **libxtr**\(3\).

Examples
--------

Querying the status of all sinks::

    > xtrctl status /run/user/1000/xtrctl.7852.0 
    Test0 (info) 64K capacity, 0K size, 0 dropped
    Test1 (info) 64K capacity, 0K size, 0 dropped
    Test2 (info) 64K capacity, 0K size, 0 dropped
    Test3 (info) 64K capacity, 0K size, 0 dropped
    Test4 (info) 64K capacity, 0K size, 0 dropped

Setting the level of all sinks to 'error'::

    > xtrctl level error /run/user/1000/xtrctl.7852.0 
    Success

Setting the level of sinks matching a pattern to 'warning'::

    > xtrctl level warning 'Test[0-2]' /run/user/1000/xtrctl.7852.0 
    Success

Querying the status of sinks matching a pattern::

    > xtrctl status 'Test[0-2]' /run/user/1000/xtrctl.7852.0 
    Test0 (warning) 64K capacity, 0K size, 0 dropped
    Test1 (warning) 64K capacity, 0K size, 0 dropped
    Test2 (warning) 64K capacity, 0K size, 0 dropped

Accessing xtrctl via Conan
--------------------------

Add a `virtualenv` generator, for example if using conanfile.txt:

.. code-block:: c++

    [generators]
    virtualenv
    ...etc..

After running :code:`conan install` run :code:`source activate.sh` and xtrctl
should be in your $PATH. A man page for xtrctl (this file) will also be added
to your $MANPATH.
