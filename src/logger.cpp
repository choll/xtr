// Copyright 2019, 2020, 2021 Chris E. Holloway
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

#include "xtr/logger.hpp"

#include <cassert>

XTR_FUNC
xtr::sink xtr::logger::get_sink(std::string name)
{
    return sink(*this, std::move(name), default_log_level_);
}

XTR_FUNC
void xtr::logger::register_sink(sink& s, std::string name) noexcept
{
    assert(!s.open_);
    post(
        [&s, name = std::move(name)](detail::consumer& c, auto&)
        {
            c.add_sink(s, name);
        });
    s.open_ = true;
}

XTR_FUNC
void xtr::logger::set_command_path(std::string path) noexcept
{
    post(
        [s = std::move(path)](detail::consumer& c, auto&) mutable
        {
            c.set_command_path(std::move(s));
        });
    control_.sync();
}

XTR_FUNC
void xtr::logger::set_log_level_style(log_level_style_t level_style) noexcept
{
    post(
        [=](detail::consumer& c, auto&)
        {
            c.buf.lstyle = level_style;
        });
    control_.sync();
}

XTR_FUNC
void xtr::logger::set_default_log_level(log_level_t level)
{
    default_log_level_ = level;
}

XTR_FUNC
bool xtr::logger::pump_io()
{
    return consumer_.run_once();
}
