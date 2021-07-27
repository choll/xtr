xtrctl
======

Displaying the Status of Sinks
------------------------------

Setting Log Levels
------------------

Rotating Log Files
------------------

TALK ABOUT SINKS HAVING A LOG LEVEL, SAFETY

formatting





Sinks pass log messages to the logger
via individual queues which are read by a thread associated with the
logger (each logger object has its own thread).


. The logger has an associated thread which reads from
the 



A logger has an
associated back-end, which is written to





Talk about how strings are copied into the log buffer, talk about safety.

Discuss logger and sinks?

+---------------------------+---------+
| Type                      | Copied? |
+===========================+=========+
| char*,                    | Yes     |
+---------------------------+---------+
| char[]                    | Yes     |
+---------------------------+---------+
| std::array<char, N>       | Yes     |
+---------------------------+---------+
| std::string               | Yes     |
+---------------------------+---------+
| std::string_view          | Yes     |
+---------------------------+---------+
| std::reference_wrapper<T> | No      |
+---------------------------+---------+
| std::reference_wrapper<T> | No      |
+---------------------------+---------+

::

                +-----------------------+
                | function pointer      |---> invokerS()
                +-----------------------+
                | record total size     |
                +-----------------------+
                | lambda:               |
          +-----/    variable size      /
 pointers | +---/    known at compile   /
 into     | |   |    time               |
 string   | |   +-----------------------+
 table    | |   | string table:         |
          | +-> /   variable size       /
          +---> /   known at run time   /
                |                       |
                +-----------------------+


