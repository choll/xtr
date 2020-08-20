// Copyright 2020 Chris E. Holloway
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "xtr/detail/tsc.hpp"
#include "xtr/detail/clock_ids.hpp"
#include "xtr/detail/cpuid.hpp"

#include <array>
#include <chrono>
#include <thread>

#include <time.h>

namespace xtrd = xtr::detail;

// Crystal clock frequencies are taken from:
// https://github.com/torvalds/linux/blob/master/tools/power/x86/turbostat/turbostat.c

std::uint64_t xtrd::read_tsc_hz() noexcept
{
    constexpr int tsc_leaf = 0x15;

    // Check if CPU supports TSC info leaf
    if (cpuid(0)[0] < tsc_leaf)
        return 0;

    // ratio_den (EAX) : An unsigned integer which is the denominator of the
    //                   TSC / CCC ratio.
    // ratio_num (EBX) : An unsigned integer which is the numerator of the
    //                   TSC / CCC ratio.
    // ccc_hz (ECX)    : An unsigned integer which is the nominal frequency of
    //                   the CCC in Hz.
    //
    // Where CCC = core crystal clock.
    auto [ratio_den, ratio_num, ccc_hz, unused] = cpuid(0x15);

    if (ccc_hz == 0)
    {
        // Core crystal clock frequency not provided, must infer
        // from CPU model number.
        const std::uint16_t model = get_family_model()[1];
        switch(model) {
        case intel_fam6_skylake_l:
        case intel_fam6_skylake:
        case intel_fam6_kabylake_l:
        case intel_fam6_kabylake:
        case intel_fam6_cometlake_l:
        case intel_fam6_cometlake:
            ccc_hz = 24000000; // 24 MHz
            break;
        case intel_fam6_atom_tremont_d:
        case intel_fam6_atom_goldmont_d:
        // Skylake-X is not included as the crystal clock may be either 24 MHz
        // or 25 MHz, with further issues on the 25 MHz variant, for details
        // see: // https://bugzilla.kernel.org/show_bug.cgi?id=197299
            ccc_hz = 25000000; // 25 MHz
            break;
        case intel_fam6_atom_goldmont:
        case intel_fam6_atom_goldmont_plus:
            ccc_hz = 19200000; // 19.2 MHz
            break;
        default:
            return 0; // Unknown CPU or crystal frequency
        }
    }

    // As we have:
    //
    // TSC Hz = ccc_hz * TSC/CCR
    //
    // and:
    //
    // TSC/CCR = ratio_num/ratio_den
    //
    // then:
    //
    // TSC Hz = ccc_hz * ratio_num/ratio_den
    return std::uint64_t(ccc_hz) * ratio_num / ratio_den;
}

std::uint64_t xtrd::estimate_tsc_hz() noexcept
{
    const std::uint64_t tsc0 = tsc::now().ticks;
    std::timespec ts0;
    ::clock_gettime(XTR_CLOCK_MONOTONIC, &ts0);

    std::array<std::uint64_t, 5> history;
    std::size_t n = 0;

    const auto sleep_time = std::chrono::milliseconds(10);
    const auto max_sleep_time = std::chrono::seconds(2);
    const std::size_t tick_range = 1000;
    const std::size_t max_iters = max_sleep_time / sleep_time;

    // Read the TSC and system clock every 10ms for up to 2 seconds or until
    // last 5 TSC frequency estimations are within a 1000 tick range, whichever
    // occurs first.

    for (;;)
    {
        std::this_thread::sleep_for(sleep_time);

        const std::uint64_t tsc1 = tsc::now().ticks;
        std::timespec ts1;
        ::clock_gettime(XTR_CLOCK_MONOTONIC, &ts1);

        const auto elapsed_nanos =
            std::uint64_t(ts1.tv_sec - ts0.tv_sec) * 1000000000UL +
                std::uint64_t(ts1.tv_nsec) - std::uint64_t(ts0.tv_nsec);

        const std::uint64_t elapsed_ticks = tsc1 - tsc0;

        const std::uint64_t tsc_hz =
            std::uint64_t(double(elapsed_ticks) * 1e9 / double(elapsed_nanos));

        history[n++ % history.size()] = tsc_hz;

        if (n >= history.size())
        {
            const auto min = *std::min_element(history.begin(), history.end());
            const auto max = *std::max_element(history.begin(), history.end());
            if (max - min < tick_range || n >= max_iters)
                return tsc_hz;
        }
    }
}

std::timespec xtrd::tsc::to_timespec(tsc ts)
{
    thread_local tsc last_tsc{};
    thread_local std::int64_t last_epoch_nanos;
    static const std::uint64_t one_minute_ticks = 60 * get_tsc_hz();
    static const double tsc_multiplier = 1e9 / double(get_tsc_hz());

    // Sync up TSC and wall clocks every minute
    if (ts.ticks > last_tsc.ticks + one_minute_ticks)
    {
        last_tsc = tsc::now();
        std::timespec temp;
        ::clock_gettime(XTR_CLOCK_TAI, &temp);
        last_epoch_nanos = temp.tv_sec * 1000000000L + temp.tv_nsec;
    }

    const auto tick_delta = std::int64_t(ts.ticks - last_tsc.ticks);
    const auto nano_delta = std::int64_t(double(tick_delta) * tsc_multiplier);
    const auto total_nanos = std::uint64_t(last_epoch_nanos + nano_delta);

    std::timespec result;
    result.tv_sec = total_nanos / 1000000000UL;
    result.tv_nsec = total_nanos % 1000000000UL;

    return result;
}

