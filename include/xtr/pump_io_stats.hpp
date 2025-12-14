#ifndef XTR_PUMP_IO_STATS_HPP
#define XTR_PUMP_IO_STATS_HPP

#include <cstddef>

namespace xtr
{
    struct pump_io_stats;
}

/**
 * Statistics struct yielded by @ref xtr::logger::pump_io.
 */
struct xtr::pump_io_stats
{
    /**
     * Number of events processed. This includes log statements, sink
     * construction and destruction events, sync requests etc.
     */
    std::size_t n_events;
};

#endif
