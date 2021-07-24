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

#include "xtr/command_path.hpp"
#include "xtr/detail/commands/command_dispatcher.hpp"
#include "xtr/detail/commands/responses.hpp"

#include "command_client.hpp"

#include <catch2/catch.hpp>

#include <atomic>
#include <string>
#include <stdexcept>
#include <thread>

namespace xtrd = xtr::detail;

namespace
{
#if __cpp_exceptions
    struct thrower
    {
        static constexpr auto frame_id = 1;
    };
#endif

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
        fixture(const std::string& path = xtr::default_command_path())
        :
            cmd_(path)
        {
            REQUIRE(cmd_.is_open());

            connect(path);

            cmd_thread_ = std::thread([&](){ run(); });

#if __cpp_exceptions
            cmd_.register_callback<thrower>(
                [](int, const thrower&)
                {
                    throw std::runtime_error("Reason");
                });
#endif

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
            stop_.store(true);
            // reconnect is called to cause process_commands() to return,
            // to allow run() to exit
            reconnect();
            cmd_thread_.join();
        }

        void run()
        {
            while (!stop_)
                cmd_.process_commands(-1);
        }

        std::atomic<bool> stop_{false};
        xtrd::command_dispatcher cmd_;
        std::thread cmd_thread_;
    };

#if defined(__linux__)
    struct abstract_socket_fixture : fixture
    {
        abstract_socket_fixture()
        :
            fixture(socket_path())
        {
        }

        std::string socket_path() const
        {
            using namespace std::literals::string_literals;
            return "\0command_socket"s;
        }
    };
#endif
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

#if defined(__linux__)
TEST_CASE_METHOD(
    abstract_socket_fixture,
    "command_dispatcher abstract socket test",
    "[command_dispatcher]")
{
    xtrd::frame<sum> request;

    request->x = 1;
    request->y = 2;

    const auto responses = send_frame<sum_reply>(request);

    using namespace std::literals::string_view_literals;

    REQUIRE(responses.size() == 1);
    REQUIRE(responses[0].result == 3);
}
#endif

#if __cpp_exceptions
TEST_CASE_METHOD(fixture, "command_dispatcher throw test", "[command_dispatcher]")
{
    const auto errors = send_frame<xtrd::error>(xtrd::frame<thrower>());

    using namespace std::literals::string_view_literals;

    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].reason == "Reason"sv);
}
#endif

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
