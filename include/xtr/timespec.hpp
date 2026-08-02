#ifndef XTR_TIMESPEC_HPP
#define XTR_TIMESPEC_HPP

#include "config.hpp"

#include <algorithm>
#include <cstdint>
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

    static_assert(
        XTR_TIMESTAMP_DIGITS >= 1 && XTR_TIMESTAMP_DIGITS <= 9,
        "XTR_TIMESTAMP_DIGITS must be between 1 and 9");

    namespace detail
    {
        consteval std::uint32_t pow10(std::size_t n)
        {
            std::uint32_t result = 1;
            while (n-- != 0)
                result *= 10U;
            return result;
        }

        // Writes the most significant Digits digits of nsec, working
        // backwards from out. nsec is required to be less than 10^9.
        template<std::size_t Digits, typename OutputIterator>
        inline void format_subseconds(OutputIterator out, std::uint32_t nsec)
        {
            static_assert(Digits >= 1 && Digits <= 9);

            std::uint32_t value = nsec / pow10(9 - Digits);

#if defined(__GNUC__)
#pragma GCC unroll 9
#endif
            for (std::size_t i = 0; i != Digits; ++i)
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
            // 19 characters of date and time, a decimal point, then the
            // sub-second digits.
            char buf[20 + XTR_TIMESTAMP_DIGITS] = {"1970-01-01 00:00:00."};
        } last;

        if (ts.tv_sec != last.sec) [[unlikely]]
        {
            fmt::format_to(
                last.buf,
                FMT_COMPILE("{:%Y-%m-%d %T}."),
                fmt::gmtime(ts.tv_sec));
            last.sec = ts.tv_sec;
        }

        xtr::detail::format_subseconds<XTR_TIMESTAMP_DIGITS>(
            std::end(last.buf), std::uint32_t(ts.tv_nsec));

        return std::copy(std::begin(last.buf), std::end(last.buf), ctx.out());
    }
};

#endif
