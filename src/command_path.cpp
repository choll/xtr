// Copyright 2019, 2020, 2021 Chris E. Holloway
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

#include "xtr/command_path.hpp"

#include <atomic>
#include <cstdio>

#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

namespace xtr::detail
{
    XTR_FUNC std::string get_rundir()
    {
        if (const char* rundir = ::getenv("XDG_RUNTIME_DIR"))
            return rundir;
        const unsigned long uid = ::geteuid();
        char rundir[32];
        std::snprintf(rundir, sizeof(rundir), "/run/user/%lu", uid);
        return rundir;
    }

    XTR_FUNC std::string get_tmpdir()
    {
        if (const char* tmpdir = ::getenv("TMPDIR"))
            return tmpdir;
        return "/tmp";
    }
}

XTR_FUNC
std::string xtr::default_command_path()
{
    static std::atomic<unsigned> ctl_count{0};
    const long pid = ::getpid();
    const unsigned n = ctl_count++;
    char path[PATH_MAX];

    std::string dir = detail::get_rundir();

    if (::access(dir.c_str(), W_OK) != 0)
        dir = detail::get_tmpdir();

    std::snprintf(path, sizeof(path), "%s/xtrctl.%ld.%u", dir.c_str(), pid, n);

    return path;
}
