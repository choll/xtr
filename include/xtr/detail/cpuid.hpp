#ifndef XTR_DETAIL_CPUID_HPP
#define XTR_DETAIL_CPUID_HPP

#include <array>
#include <cstdint>

// Model numbers are taken from:
// https://github.com/torvalds/linux/blob/master/arch/x86/include/asm/intel-family.h

namespace xtr::detail
{
    constexpr inline std::uint32_t intel_fam6_skylake_l          = 0x4E;
    constexpr inline std::uint32_t intel_fam6_skylake            = 0x5E;
    constexpr inline std::uint32_t intel_fam6_kabylake_l         = 0x8E;
    constexpr inline std::uint32_t intel_fam6_kabylake           = 0x9E;
    constexpr inline std::uint32_t intel_fam6_cometlake          = 0xA5;
    constexpr inline std::uint32_t intel_fam6_cometlake_l        = 0xA6;
    constexpr inline std::uint32_t intel_fam6_atom_tremont_d     = 0x86;
    constexpr inline std::uint32_t intel_fam6_atom_goldmont_d    = 0x5F;
    constexpr inline std::uint32_t intel_fam6_atom_goldmont      = 0x5C;
    constexpr inline std::uint32_t intel_fam6_atom_goldmont_plus = 0x7A;

    inline std::array<std::uint32_t, 4> cpuid(int leaf, int subleaf = 0) noexcept
    {
        std::array<std::uint32_t, 4> out;
        asm (
            "cpuid" :
            // output
            "=a" (out[0]),
            "=b" (out[1]),
            "=c" (out[2]),
            "=d" (out[3]) :
            // input
            "a" (leaf),
            "c" (subleaf));
        return out;
    }

    inline std::array<std::uint16_t, 2> get_family_model() noexcept
    {
        // See https://www.felixcloutier.com/x86/cpuid#fig-3-6
        const std::uint32_t fms = cpuid(0x1)[0];
        std::uint16_t model = (fms & 0xF0) >> 4;
        std::uint16_t family = (fms & 0xF00) >> 8;
        if (family == 0xF)
            family |= std::uint16_t((fms & 0xFF00000) >> 16);
        if (family == 0x6 || family == 0xF)
            model |= std::uint16_t((fms & 0xF0000) >> 12);
        return {family,  model};
    }
}

#endif

