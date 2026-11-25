// Copyright 2026 Chris E. Holloway
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef XTR_ENABLE_CONTAINER_SERIALIZATION_HPP
#define XTR_ENABLE_CONTAINER_SERIALIZATION_HPP

#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace xtr
{
    // XXX review comment
    /**
     * Containers for which enable_container_serialization is true are
     * serialized into the log record when logged, rather than copied, which
     * avoids heap allocation. User-defined containers may be enabled by
     * specializing this trait to derive from std::true_type. Containers whose
     * elements cannot be serialized (e.g. std::vector<std::shared_ptr<T>>)
     * are copied as usual.
     *
     * The standard sequence and associative containers are enabled by
     * default. Serialization may be disabled for a specific container type,
     * e.g. std::vector<my_type>, by specializing this trait for that type to
     * derive from std::false_type, in which case the container is copied.
     * This is useful if the container type has its own fmt::formatter
     * specialization, as serialized containers are formatted from their
     * serialized elements and not by a formatter for the container type.
     */
    template<typename T>
    struct enable_container_serialization : std::false_type
    {
    };

    template<typename T>
    inline constexpr bool enable_container_serialization_v =
        enable_container_serialization<T>::value;

    template<typename... Args>
    struct enable_container_serialization<std::deque<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::forward_list<Args...>> :
        std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::list<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::map<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::multimap<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::multiset<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::set<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::unordered_map<Args...>> :
        std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::unordered_multimap<Args...>> :
        std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::unordered_multiset<Args...>> :
        std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::unordered_set<Args...>> :
        std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::vector<Args...>> : std::true_type
    {
    };
}

#endif
