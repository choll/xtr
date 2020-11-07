#ifndef XTR_DETAIL_CLOCK_IDS_HPP
#define XTR_DETAIL_CLOCK_IDS_HPP

#include <time.h>

// Clock source used by XTR_LOG_RTC
#if defined(CLOCK_REALTIME_COARSE)
#define XTR_CLOCK_REALTIME_FAST CLOCK_REALTIME_COARSE
#elif defined (CLOCK_REALTIME_FAST)
#define XTR_CLOCK_REALTIME_FAST CLOCK_REALTIME_FAST
#else
#define XTR_CLOCK_REALTIME_FAST CLOCK_REALTIME
#endif

// Clock source used when estimating TSC Hz
#if defined(CLOCK_MONOTONIC_RAW)
#define XTR_CLOCK_MONOTONIC CLOCK_MONOTONIC_RAW
#else
#define XTR_CLOCK_MONOTONIC CLOCK_MONOTONIC
#endif

// Clock source used to translate TSC to wall time
#define XTR_CLOCK_WALL CLOCK_REALTIME

#endif

