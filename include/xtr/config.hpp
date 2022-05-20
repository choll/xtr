#ifndef XTR_CONFIG_HPP
#define XTR_CONFIG_HPP

/**
 * Sets the capacity (in bytes) of the queue that sinks use to send log data
 * to the background thread. Each sink will have an individual queue of this
 * size. Users are permitted to define this variable in order to set a custom
 * capacity. User provided capacities may be rounded up\---to obtain the
 * actual capacity invoke @ref xtr::sink::capacity.
 *
 * Note that if the single header include file is not used then this setting
 * may only be defined in either config.hpp or by overriding CXXFLAGS, and
 * requires rebuilding libxtr if set.
 */
#if !defined(XTR_SINK_CAPACITY)
#define XTR_SINK_CAPACITY (256 * 1024)
#endif

/**
 * Set to 1 to enable io_uring support. If this setting is not manually defined
 * then io_uring support will be automatically detected. If libxtr is built with
 * io_uring support enabled then the library will still function on kernels that
 * do not have io_uring support, as a run-time check will be performed before
 * attempting to use any io_uring system calls.
 *
 * Note that if the single header include file is not used then this setting
 * may only be defined in either config.hpp or by overriding CXXFLAGS, and
 * requires rebuilding libxtr if set.
 */
#if !defined(XTR_USE_IO_URING) || defined(DOXYGEN)
#define XTR_USE_IO_URING __has_include(<liburing.h>)
#endif

/**
 * Set to 1 to enable submission queue polling when using io_uring. If enabled
 * the IORING_SETUP_SQPOLL flag will be passed to io_uring_setup(2).
 *
 * Note that if the single header include file is not used then this setting
 * may only be defined in either config.hpp or by overriding CXXFLAGS, and
 * requires rebuilding libxtr if set.
 */
#if !defined(XTR_IO_URING_POLL)
#define XTR_IO_URING_POLL 0
#endif

#endif
