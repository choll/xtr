// Copyright 2021 Chris E. Holloway
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

#include "xtr/detail/commands/command_dispatcher.hpp"
#include "xtr/detail/commands/responses.hpp"

#include "command_client.hpp"

#include <string>
#include <stdexcept>
#include <stop_token>
#include <thread>

#include <catch2/catch.hpp>

namespace xtrd = xtr::detail;

namespace
{
    struct thrower
    {
        static constexpr auto frame_id = 1;
    };

    struct sum
    {
        static constexpr auto frame_id = 2;

        int x;
        int y;
    };

    struct sum_reply
    {
        static constexpr auto frame_id = 3;

        int result;
    };

    struct bad_frame_id
    {
        static constexpr auto frame_id = 42;
    };

    struct bad_frame_length
    {
        static constexpr auto frame_id = sum::frame_id;
    };

    struct fixture : xtrd::command_client
    {
        fixture()
        :
            cmd_(socket_path())
        {
            REQUIRE(cmd_.is_open());

            connect(socket_path());

            cmd_thread_ = std::jthread([&](std::stop_token st){ run(st); });

            cmd_.register_callback<thrower>(
                [](int, const thrower&)
                {
                    throw std::runtime_error("Reason");
                });

            cmd_.register_callback<sum>(
                [&](int fd, const sum& s)
                {
                    xtrd::frame<sum_reply> reply;
                    reply->result = s.x + s.y;
                    cmd_.send(fd, reply);
                });
        }

        ~fixture()
        {
            cmd_thread_.request_stop();
            reconnect();
        }

        std::string socket_path() const
        {
            using namespace std::literals::string_literals;
            return "\0command_socket"s;
        }

        void run(std::stop_token st)
        {
            while (!st.stop_requested())
                cmd_.process_commands(-1);
        }

        xtrd::command_dispatcher cmd_;
        std::jthread cmd_thread_;
    };
}

TEST_CASE_METHOD(fixture, "command_dispatcher request response test", "[command_dispatcher]")
{
    xtrd::frame<sum> request;

    request->x = 1;
    request->y = 2;

    const auto responses = send_frame<sum_reply>(request);

    using namespace std::literals::string_view_literals;

    REQUIRE(responses.size() == 1);
    REQUIRE(responses[0].result == 3);
}

TEST_CASE_METHOD(fixture, "command_dispatcher throw test", "[command_dispatcher]")
{
    const auto errors = send_frame<xtrd::error>(xtrd::frame<thrower>());

    using namespace std::literals::string_view_literals;

    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].reason == "Reason"sv);
}

TEST_CASE_METHOD(fixture, "command_dispatcher incomplete header test", "[command_dispatcher]")
{
    ::msghdr hdr{};
    ::iovec iov;

    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;

    char bad_header = 42;

    iov.iov_base = &bad_header;
    iov.iov_len = sizeof(bad_header);

    REQUIRE(
        XTR_TEMP_FAILURE_RETRY(::sendmsg(fd_.get(), &hdr, MSG_NOSIGNAL)) ==
            sizeof(bad_header));

    const auto errors = read_reply<xtrd::error>();

    using namespace std::literals::string_view_literals;

    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].reason == "Incomplete frame header"sv);
}

TEST_CASE_METHOD(fixture, "command_dispatcher invalid frame id test", "[command_dispatcher]")
{
    xtrd::frame<bad_frame_id> bf;

    const auto errors = send_frame<xtrd::error>(bf);

    using namespace std::literals::string_view_literals;

    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].reason == "Invalid frame id"sv);
}

TEST_CASE_METHOD(fixture, "command_dispatcher invalid frame length test", "[command_dispatcher]")
{
    xtrd::frame<bad_frame_length> bf;

    const auto errors = send_frame<xtrd::error>(bf);

    using namespace std::literals::string_view_literals;

    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].reason == "Invalid frame length"sv);
}
