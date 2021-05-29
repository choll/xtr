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


// So, the default file is /var/run/user/<uid>/xtrctl.<pid>.<count>
//
//
// So, commands are
//
// --status -s
// --set-level -l
// --reopen -r
//
// plus an optional regular expression or wildcard, what is the default?
//
// -E, --extended-regexp
//
// Seems like you could support:
//
// -G, --basic-regexp
//
// -W, --wildcard (the default)
//

#include "xtr/detail/commands/connect.hpp"
#include "xtr/detail/commands/ios.hpp"
#include "xtr/detail/commands/pattern.hpp"
#include "xtr/detail/commands/requests.hpp"
#include "xtr/detail/commands/responses.hpp"
#include "xtr/detail/commands/recv.hpp"
#include "xtr/detail/commands/send.hpp"
#include "xtr/detail/file_descriptor.hpp"
#include "xtr/detail/strzcpy.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string_view>

#include <getopt.h>

// command_client should go into detail?

namespace xtrd = xtr::detail;

namespace
{
    [[noreturn]] void usage(
        const char* progname,
        int status,
        const char* reason = nullptr)
    {
        if (reason != nullptr)
            std::cout << reason << "\n";

        (status ? std::cerr : std::cout)
            << "Usage: " << progname << " <command> [options] <socket path> [pattern]\n"
            << "Where <command> is one of:\n"
            << "-s, --status:\n"
            << "-l, --set-level:\n"
            << "-r, --reopen:\n"
            << "and [options] are:\n"
            << "-E, --extended-regexp:\n"
            << "-G, --basic-regexp:\n"
            << "-W, --wildcard:\n"
            << "-h, --help:\n";

        // [] -> optional
        // <> -> mandatory
        // xtrctl [options] <socket path> [pattern]

        std::exit(status);
    }

    template<typename... Args>
    [[noreturn]] void errx(Args&&... args)
    {
        (std::cerr << ... << args) << "\n";
        std::exit(EXIT_FAILURE);
    }

    template<typename... Args>
    [[noreturn]] void err(Args&&... args)
    {
        const int errnum = errno;
        errx(std::forward<Args>(args)..., ": ", std::strerror(errnum));
    }

    template<typename Frame>
    void send(int fd, const Frame& frame)
    {
        const ::ssize_t nwritten = xtrd::command_send(fd, &frame, sizeof(frame));
        if (nwritten != sizeof(frame))
            err("Error writing to socket");
    }

    template<typename Payload>
    Payload* frame_cast(void* buf, std::size_t nbytes)
    {
        if (nbytes != sizeof(xtrd::frame<Payload>))
            errx("Invalid frame length");
        return &static_cast<xtrd::frame<Payload>*>(buf)->payload;
    }
}

int main(int argc, char* argv[])
{
    const struct option long_options[] = {
        {"status",          no_argument,       nullptr, 's'},
        {"set-level",       required_argument, nullptr, 'l'},
        {"reopen",          no_argument,       nullptr, 'r'},
        {"extended-regexp", no_argument,       nullptr, 'E'},
        {"basic-regexp",    no_argument,       nullptr, 'G'},
        {"wildcard",        no_argument,       nullptr, 'W'},
        {"help",            no_argument,       nullptr, 'h'},
        {nullptr,           0,                 nullptr,  0}
    };

    int optc;
    xtr::log_level_t log_level = xtr::log_level_t::none;
    xtrd::pattern_type_t pattern_type = xtrd::pattern_type_t::none;
    bool status = false;
    bool reopen = false;

    std::map<std::string, xtr::log_level_t> levels_map{
        {"fatal", xtr::log_level_t::fatal},
        {"error", xtr::log_level_t::error},
        {"warning", xtr::log_level_t::warning},
        {"info", xtr::log_level_t::info},
        {"debug", xtr::log_level_t::debug}};

    while ((optc = getopt_long(argc, argv, "sl:rEGWh", long_options, nullptr)) != -1)
    {
        switch (optc)
        {
        case 's':
            status = true;
            break;
        case 'l':
            log_level = levels_map[optarg];
            break;
        case 'r':
            reopen = true;
            break;
        case 'E':
            pattern_type = xtrd::pattern_type_t::extended_regex;
            break;
        case 'G':
            pattern_type = xtrd::pattern_type_t::basic_regex;
            break;
        case 'W':
            pattern_type = xtrd::pattern_type_t::wildcard;
            break;
        case 'h':
            usage(argv[0], EXIT_SUCCESS);
        case '?':
            usage(argv[0], EXIT_FAILURE);
        }
    }

    if (argc > optind + 2)
        usage(argv[0], EXIT_FAILURE, "Too many arguments");

    if (argc <= optind)
        usage(argv[0], EXIT_FAILURE, "Please specify a socket path");

    const char* path = argv[optind];

    if ((log_level != xtr::log_level_t::none) + status + reopen != 1)
        usage(argv[0], EXIT_FAILURE, "Please specify one of -s, -l or -r");

    const char* pattern = argc == optind + 2 ? argv[optind + 1] : nullptr;

    if (pattern_type != xtrd::pattern_type_t::none && pattern == nullptr)
        usage(argv[0], EXIT_FAILURE, "Please specify a pattern");

    if (pattern != nullptr && pattern_type == xtrd::pattern_type_t::none)
        pattern_type = xtrd::pattern_type_t::basic_regex;

    const xtrd::file_descriptor fd = xtrd::command_connect(path);

    if (!fd)
        err("Failed to connect");

    if (status)
    {
        xtrd::frame<xtrd::status> st;
        if (pattern != nullptr)
        {
            st->pattern.type = pattern_type;
            xtrd::strzcpy(st->pattern.text, std::string_view{pattern});
        }
        const ::ssize_t nwritten = xtrd::command_send(fd.get(), &st, sizeof(st));
        if (nwritten != sizeof(st))
            err("Error writing to socket");
    }
    else if (log_level != xtr::log_level_t::none)
    {
        xtrd::frame<xtrd::set_level> sl;
        sl->level = log_level;
        if (pattern != nullptr)
        {
            sl->pattern.type = pattern_type;
            xtrd::strzcpy(sl->pattern.text, std::string_view{pattern});
        }
        send(fd.get(), sl);
    }
    else if (reopen)
    {
        xtrd::frame<xtrd::reopen> rot;
        send(fd.get(), rot);
    }

    std::vector<xtrd::sink_info> infos;
    xtrd::frame_buf buf;

    while (const ::ssize_t nbytes = xtrd::command_recv(fd.get(), buf))
    {
        if (nbytes == -1)
            err("Error reading from socket");

        if (nbytes < ::ssize_t(sizeof(xtrd::frame_header)))
            errx("Incomplete frame header");

        switch (buf.hdr.frame_id)
        {
        case xtrd::sink_info::frame_id:
            infos.push_back(*frame_cast<xtrd::sink_info>(&buf, nbytes));
            break;
        case xtrd::success::frame_id:
            std::cout << "Success\n";
            break;
        case xtrd::error::frame_id:
            errx("Error: ", frame_cast<xtrd::error>(&buf, nbytes)->reason);
        default:
            errx("Invalid frame id");
        }
    }

    std::sort(
        infos.begin(),
        infos.end(),
        [](const auto& a, const auto& b)
        {
            return std::strcmp(a.name, b.name) < 0;
        });

    for (const auto& info : infos)
        std::cout << info << "\n";

    return EXIT_SUCCESS;
}

