#ifndef XTR_TIMESPEC_HPP
#define XTR_TIMESPEC_HPP

#include <algorithm>
#include <ctime>
#include <iterator>

#include <fmt/chrono.h>
#include <fmt/compile.h>

namespace xtr
{
    // This class exists to avoid clashing with user code---if a formatter
    // was created for std::timespec then it may conflict with a user
    // defined formatter.
    struct timespec : std::timespec
    {
        timespec() = default;

        // lack of explicit is intentional
        timespec(std::timespec ts) :
            std::timespec(ts)
        {
        }
    };

    namespace detail
    {
        template<typename OutputIterator, typename T>
        inline void format_micros(OutputIterator out, T value)
        {
#pragma GCC unroll 6
            for (std::size_t i = 0; i != 6; ++i)
            {
                *--out = static_cast<char>('0' + value % 10);
                value /= 10;
            }
        }
    }
}

template<>
struct fmt::formatter<xtr::timespec>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const timespec& ts, FormatContext& ctx) const
    {
        thread_local struct
        {
            std::time_t sec;
            char buf[26] = {"1970-01-01 00:00:00."};
        } last;

        if (ts.tv_sec != last.sec) [[unlikely]]
        {
            fmt::format_to(
                last.buf,
                FMT_COMPILE("{:%Y-%m-%d %T}."),
                fmt::gmtime(ts.tv_sec));
            last.sec = ts.tv_sec;
        }

        xtr::detail::format_micros(std::end(last.buf), ts.tv_nsec / 1000);

        return std::copy(std::begin(last.buf), std::end(last.buf), ctx.out());
    }
};

#endif
