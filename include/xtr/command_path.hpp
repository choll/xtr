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

#ifndef XTR_COMMAND_PATH_HPP
#define XTR_COMMAND_PATH_HPP

#include <string>

namespace xtr
{
    /**
     * When passed to the @ref command_path_arg "command_path" argument of
     * @ref logger::logger (or other logger constructors) indicates that no
     * command socket should be created.
     */
    inline constexpr auto null_command_path = "";

    /**
     * Returns the default command path used for the @ref command_path_arg
     * "command_path" argument of @ref logger::logger (and other logger
     * constructors). A string with the format /run/user/<uid>/xtrctl.<pid>.<N>
     * is returned, where N begins at 0 and increases for each call to the
     * function. If the /run/user/<uid> directory does not exist or is
     * inaccessible then the /tmp directory is used instead.
     */
    std::string default_command_path();
}

#endif
