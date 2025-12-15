// Copyright 2025 Chris E. Holloway
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

#ifndef XTR_STREAMED_HPP
#define XTR_STREAMED_HPP

#include <fmt/ostream.h>

#include <type_traits>
#include <utility>

namespace xtr
{
    namespace detail
    {
        template<typename T>
        struct streamed_wrapper
        {
            T value;
        };
    }

    /**
     * Returns a wrapper that formats the given argument using its ostream stream
     * insertion operator (operator<<). The value is copied into the wrapper.
     *
     * @param value: The value to copy and wrap for stream-based formatting.
     *
     * @return A wrapper object suitable for passing as an argument to a log macro
     * such as @ref XTR_LOG.
     *
     * Refer to the <a href="guide.html#custom-formatters">custom formatters</a>
     * section of the user guide for example use.
     */
    template<typename T>
    constexpr detail::streamed_wrapper<std::remove_cv_t<std::remove_reference_t<T>>> streamed_copy(
        T&& value)
    {
        return {std::forward<T>(value)};
    }

    /**
     * Returns a wrapper that formats the given argument using its ostream
     * stream insertion operator (operator<<). The wrapper stores a reference to
     * the argument so callers must ensure that it remains valid until
     * formatting is completed.
     *
     * @param value: The value to copy and wrap for stream-based formatting.
     *
     * @return A wrapper object suitable for passing as an argument to a log
     * macro such as @ref XTR_LOG.
     *
     * Refer to the <a href="guide.html#custom-formatters">custom formatters</a>
     * section of the user guide for example use.
     */
    template<typename T>
    constexpr detail::streamed_wrapper<const T&> streamed_ref(const T& value)
    {
        return {value};
    }
}

template<typename T, typename Char>
struct fmt::formatter<xtr::detail::streamed_wrapper<T>, Char> : ostream_formatter
{
    template<typename FormatContext>
    auto format(const xtr::detail::streamed_wrapper<T>& sc, FormatContext& ctx) const
    {
        return ostream_formatter::format(sc.value, ctx);
    }
};

#endif
