#ifndef XTR_DETAIL_GET_TIME_HPP
#define XTR_DETAIL_GET_TIME_HPP

#include "xtr/timespec.hpp"

#include <time.h>

namespace xtr::detail
{
    template<clockid_t ClockId>
    xtr::timespec get_time() noexcept
    {
        std::timespec result;
        ::clock_gettime(ClockId, &result);
        return result;
    }
}

#endif

