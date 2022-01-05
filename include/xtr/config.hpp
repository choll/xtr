#ifndef XTR_CONFIG_HPP
#define XTR_CONFIG_HPP

/**
 * Sets the capacity (in bytes) of the queue that sinks use to send log data
 * to the background thread. Each sink will have an individual queue of this
 * size. Users are permitted to define this variable in order to set a custom
 * capacity. User provided capacities may be rounded up\---to obtain the
 * actual capacity invoke @ref xtr::sink::capacity.
 */
#if !defined(XTR_SINK_CAPACITY)
#define XTR_SINK_CAPACITY (256 * 1024)
#endif

#endif
