#ifndef XTR_DETAIL_TSC_HPP
#define XTR_DETAIL_TSC_HPP

#include "xtr/timespec.hpp"

#include <cstdint>
#include <ctime>

#include <fmt/chrono.h>
#include <fmt/format.h>

namespace xtr::detail
{
    struct tsc;

    std::uint64_t read_tsc_hz() noexcept;
    std::uint64_t estimate_tsc_hz() noexcept;

    inline std::uint64_t get_tsc_hz() noexcept
    {
        __extension__ static std::uint64_t tsc_hz =
            read_tsc_hz() ?: estimate_tsc_hz();
        return tsc_hz;
    }
}

struct xtr::detail::tsc
{
    inline static tsc now() noexcept
    {
        std::uint32_t a, d;
        asm volatile(
            "rdtsc;"
            : "=a" (a), "=d" (d)); // output, a=eax, d=edx
        return
            {static_cast<std::uint64_t>(a) | (static_cast<uint64_t>(d) << 32)};
    }

    static std::timespec to_timespec(tsc ts);

    std::uint64_t ticks;
};

namespace fmt
{
    template<>
    struct formatter<xtr::detail::tsc>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const xtr::detail::tsc ticks, FormatContext &ctx)
        {
            const auto ts = xtr::detail::tsc::to_timespec(ticks);
            return formatter<xtr::timespec>().format(ts, ctx);
        }
    };
}

#endif
