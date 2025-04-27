// Copyright 2019, 2020 Chris E. Holloway
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

#ifndef XTR_DETAIL_STRING_HPP
#define XTR_DETAIL_STRING_HPP

#include <cstddef>
#include <string_view>

namespace xtr::detail
{
    template<std::size_t N>
    struct string
    {
        char str[N + 1];
    };

    template<std::size_t N>
    string(const char (&str)[N]) -> string<N - 1>;

    template<std::size_t N1, std::size_t N2>
    constexpr auto operator+(const string<N1>& s1, const string<N2>& s2) noexcept
    {
        string<N1 + N2> result{};

        for (std::size_t i = 0; i < N1; ++i)
            result.str[i] = s1.str[i];

        for (std::size_t i = 0; i < N2; ++i)
            result.str[i + N1] = s2.str[i];

        result.str[N1 + N2] = '\0';

        return result;
    }

    template<std::size_t Pos, std::size_t N>
    constexpr auto rcut(const char (&s)[N]) noexcept
    {
        constexpr std::size_t count = N - Pos - 1;

        string<count> result{};

        for (std::size_t i = 0; i < count; ++i)
            result.str[i] = s[Pos + i];

        result.str[count] = '\0';

        return result;
    }

    template<std::size_t N>
    constexpr auto rindex(const char (&s)[N], char c) noexcept
    {
        std::size_t i = N - 1;
        while (i > 0 && s[--i] != c)
            ;
        return s[i] == c ? i : std::size_t(-1);
    }

#if defined(XTR_ENABLE_TEST_STATIC_ASSERTIONS)
    static_assert((string<3>{"foo"} + string{"bar"}).str[0] == 'f');
    static_assert((string<3>{"foo"} + string{"bar"}).str[1] == 'o');
    static_assert((string<3>{"foo"} + string{"bar"}).str[2] == 'o');
    static_assert((string<3>{"foo"} + string{"bar"}).str[3] == 'b');
    static_assert((string<3>{"foo"} + string{"bar"}).str[4] == 'a');
    static_assert((string<3>{"foo"} + string{"bar"}).str[5] == 'r');
    static_assert((string<3>{"foo"} + string{"bar"}).str[6] == '\0');

    static_assert(rcut<0>("123").str[0] == '1');
    static_assert(rcut<0>("123").str[1] == '2');
    static_assert(rcut<0>("123").str[2] == '3');
    static_assert(rcut<0>("123").str[3] == '\0');
    static_assert(rcut<1>("123").str[0] == '2');
    static_assert(rcut<1>("123").str[1] == '3');
    static_assert(rcut<1>("123").str[2] == '\0');
    static_assert(rcut<2>("123").str[0] == '3');
    static_assert(rcut<2>("123").str[1] == '\0');
    static_assert(rcut<3>("123").str[0] == '\0');

    static_assert(rindex("foo", '/') == std::size_t(-1));
    static_assert(rindex("/foo", '/') == 0);
    static_assert(rindex("./foo", '/') == 1);
    static_assert(rindex("/foo/bar", '/') == 4);
    static_assert(rindex("/", '/') == 0);
#endif
}

#endif
