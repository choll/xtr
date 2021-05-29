#ifndef XTR_TIMESPEC_HPP
#define XTR_TIMESPEC_HPP

#include <ctime>

#include <fmt/chrono.h>

namespace xtr
{
    // This class exists to avoid clashing with user code---if a formatter
    // was created for std::timespec then it may conflict with a user
    // defined formatter.
    struct timespec : std::timespec
    {
        timespec() = default;

        timespec(std::timespec ts)
        :
            std::timespec(ts)
        {
        }
    };
}

namespace fmt
{
    template<>
    struct formatter<xtr::timespec>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const xtr::timespec ts, FormatContext &ctx)
        {
            std::tm temp;
            return
                fmt::format_to(
                    ctx.out(),
                    "{:%Y-%m-%d %T}.{:06}",
                    *::gmtime_r(&ts.tv_sec, &temp),
                    ts.tv_nsec / 1000);
        }
    };
}

#endif
