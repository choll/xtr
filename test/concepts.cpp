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

#include "xtr/detail/concepts.hpp"
#include "xtr/detail/vcopy_wrapper.hpp"
#include "xtr/nocopy.hpp"

#include <array>
#include <forward_list>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xtrd = xtr::detail;

// XXX review comment
// is_variable_length: true if a type is written to the variable-length area,
// either directly or by containing such a type.
static_assert(xtrd::is_variable_length_v<std::string>);
static_assert(xtrd::is_variable_length_v<const std::string&>);
static_assert(xtrd::is_variable_length_v<std::string_view>);
static_assert(xtrd::is_variable_length_v<const char*>);
static_assert(xtrd::is_variable_length_v<const char (&)[6]>);
static_assert(xtrd::is_variable_length_v<xtrd::vcopy_wrapper<int>>);
static_assert(xtrd::is_variable_length_v<std::tuple<std::string, int>>);
static_assert(xtrd::is_variable_length_v<std::tuple<std::string, int&>>);
static_assert(xtrd::is_variable_length_v<std::pair<std::string, std::string&>>);
static_assert(xtrd::is_variable_length_v<std::tuple<std::string, int>&>);
static_assert(xtrd::is_variable_length_v<std::pair<const std::string, int>>);
static_assert(xtrd::is_variable_length_v<const std::array<std::string, 2>&>);
static_assert(xtrd::is_variable_length_v<std::tuple<std::tuple<std::string>>>);
static_assert(xtrd::is_variable_length_v<std::vector<std::string>>);
static_assert(xtrd::is_variable_length_v<std::vector<int>>);
static_assert(xtrd::is_variable_length_v<std::map<int, int>>);

static_assert(!xtrd::is_variable_length_v<int>);
static_assert(!xtrd::is_variable_length_v<int&>);
static_assert(!xtrd::is_variable_length_v<std::tuple<>>);
static_assert(!xtrd::is_variable_length_v<std::tuple<int, double>>);
static_assert(!xtrd::is_variable_length_v<std::tuple<int&>>);
// XXX review comment
// Reference members of a tuple or pair are kept as references rather than
// serialized, so do not make it variable-length.
static_assert(!xtrd::is_variable_length_v<std::tuple<std::string&>>);
static_assert(!xtrd::is_variable_length_v<std::tuple<const std::string&>>);
static_assert(!xtrd::is_variable_length_v<std::tuple<int, std::string&>>);
static_assert(!xtrd::is_variable_length_v<std::tuple<const char*&>>);
static_assert(!xtrd::is_variable_length_v<std::tuple<char (&)[4]>>);
static_assert(!xtrd::is_variable_length_v<std::pair<std::string&, int&>>);
static_assert(!xtrd::is_variable_length_v<std::tuple<std::vector<int>&>>);
static_assert(!xtrd::is_variable_length_v<std::pair<int, int>>);
static_assert(!xtrd::is_variable_length_v<std::array<int, 2>>);
static_assert(!xtrd::is_variable_length_v<std::vector<std::shared_ptr<int>>>);

// XXX review comment
// container_serializable: enabled containers whose elements can be serialized.
static_assert(xtrd::container_serializable<std::vector<int>>);
static_assert(xtrd::container_serializable<std::vector<std::string>>);
static_assert(xtrd::container_serializable<std::map<std::string, int>>);
static_assert(xtrd::container_serializable<std::forward_list<int>>);
static_assert(xtrd::container_serializable<std::unordered_multimap<int, int>>);
static_assert(xtrd::container_serializable<std::vector<std::vector<std::string>>>);
static_assert(xtrd::container_serializable<std::vector<std::tuple<std::string, int>>>);

static_assert(!xtrd::container_serializable<std::vector<std::shared_ptr<int>>>);
static_assert(
    !xtrd::container_serializable<std::map<std::string, std::shared_ptr<int>>>);
static_assert(!xtrd::container_serializable<std::array<int, 2>>);
static_assert(!xtrd::container_serializable<std::string>);
static_assert(!xtrd::container_serializable<std::string_view>);
static_assert(!xtrd::container_serializable<std::span<int>>);

// XXX review comment
// nocopy: temporaries are rejected, as the reference held by the sink would
// dangle before the consumer thread reads it.
template<typename T>
concept nocopyable = requires(T&& t) { xtr::nocopy(std::forward<T>(t)); };

static_assert(nocopyable<std::vector<int>&>);
static_assert(nocopyable<const std::vector<int>&>);
static_assert(!nocopyable<std::vector<int>>);
static_assert(!nocopyable<const std::vector<int>>);
static_assert(nocopyable<std::string&>);
static_assert(nocopyable<const std::string&>);
static_assert(!nocopyable<std::string>);
static_assert(!nocopyable<const std::string>);
// XXX review comment
// Temporaries of non-owning string types are accepted.
static_assert(nocopyable<const char*>);
static_assert(nocopyable<const char (&)[6]>);
static_assert(nocopyable<std::string_view>);
