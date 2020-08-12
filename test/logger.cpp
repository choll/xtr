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

#include "xtr/logger.hpp"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <unistd.h>
#include <time.h>

namespace
{
    struct clock
    {
        typedef std::int64_t rep;
        typedef std::ratio<1, 1000000000> period;
        typedef std::chrono::duration<rep, period> duration;
        typedef std::chrono::time_point<clock> time_point;

        time_point now() const noexcept
        {
            return time_point(duration(nanos_->load()));
        }

        std::time_t to_time_t(time_point tp) const noexcept
        {
            return
                std::chrono::duration_cast<std::chrono::seconds>(
                    tp.time_since_epoch()).count();
        }

        std::atomic<std::int64_t>* nanos_;
    };

    struct file_buf
    {
        file_buf()
        {
            REQUIRE(fp_ != nullptr);
        }

        ~file_buf()
        {
            std::fclose(fp_);
            std::free(buf_);
        }

        void push_lines(std::vector<std::string>& lines)
        {
            if (fp_ == nullptr)
                return;
            // Note: buf_ may be updated by stdio/open_memstream (as in, a new
            // buffer may be allocated, and the buf_ pointer overwritten to
            // point to the new buffer), hence using std::string and not
            // std::string_view in lines_.
            std::fflush(fp_);
            if (size_ == 0)
                return;
            for (
                std::size_t nl = 0;
                nl = std::strcspn(&buf_[off_], "\n"), buf_[off_ + nl] != '\0';
                off_ += nl + 1)
            {
                lines.emplace_back(&buf_[off_], nl);
            }
        }

        std::size_t size_{};
        std::size_t off_{};
        char *buf_ = nullptr;
        FILE* fp_{::open_memstream(&buf_, &size_)};
    };

    auto make_write_func(std::vector<std::string>& v, std::mutex& m)
    {
        return
            [&v, &m](const char* buf, std::size_t length)
            {
                std::scoped_lock lock{m};
                assert(buf[length - 1] == '\n');
                v.push_back(std::string(buf, length - 1));
                return length;
            };
    }

    struct fixture
    {
        fixture() = default;

        template<typename... Args>
        fixture(Args&&... args)
        :
            log_(std::forward<Args>(args)...)
        {
        }

        virtual void sync()
        {
            p_.sync();
        }

        std::string last_line()
        {
            sync();
            std::scoped_lock lock{m_};
            REQUIRE(!lines_.empty());
            return lines_.back();
        }

        std::string last_err()
        {
            sync();
            std::scoped_lock lock{m_};
            REQUIRE(!errors_.empty());
            return errors_.back();
        }

        std::size_t line_count()
        {
            std::scoped_lock lock{m_};
            return lines_.size();
        }

        void clear_lines()
        {
            std::scoped_lock lock{m_};
            lines_.clear();
        }

        int line_{};
        std::mutex m_;
        std::vector<std::string> lines_;
        std::vector<std::string> errors_;
        std::atomic<std::int64_t> clock_nanos_{946688523123456789L};
        xtr::logger log_{
            make_write_func(lines_, m_),
            make_write_func(errors_, m_),
            clock{&clock_nanos_}};
        xtr::logger::producer p_ = log_.get_producer("Name");
    };

    struct file_fixture_base
    {
    protected:
        file_buf outbuf_;
        file_buf errbuf_;
    };

    struct file_fixture : file_fixture_base, fixture
    {
        file_fixture()
        :
            fixture(outbuf_.fp_, errbuf_.fp_, clock{&clock_nanos_})
        {
        }

        void sync() override
        {
            fixture::sync();
            outbuf_.push_lines(lines_);
            errbuf_.push_lines(errors_);
        }
    };

    FILE* fmktemp()
    {
        char path[] = "/tmp/xtr.test.XXXXXX";
        const int fd = ::mkstemp(path);
        REQUIRE(fd != -1);
        ::unlink(path);
        return ::fdopen(fd, "w");
    }

    struct throughput_fixture
    {
        ~throughput_fixture()
        {
            p_.sync();
            std::fclose(fp_);
        }

        FILE* fp_ = fmktemp();
        xtr::logger log_{fp_, fp_};
        xtr::logger::producer p_ = log_.get_producer("Name");
    };

#if __cpp_exceptions
    struct thrower {};

    std::ostream& operator<<(std::ostream& os, thrower)
    {
        throw std::runtime_error("Exception error text");
        return os;
    }
#endif

    struct custom_format
    {
        int x;
        int y;
    };

    struct streams_format
    {
        int x;
        int y;
    };

    struct non_copyable
    {
        explicit non_copyable(int x)
        :
            x_(x)
        {
        }

        non_copyable(const non_copyable&) = delete;
        non_copyable& operator=(const non_copyable&) = delete;
        non_copyable(non_copyable&&) = default;
        non_copyable& operator=(non_copyable&&) = default;

        int x_;
    };

    template<std::size_t Align>
    struct alignas(Align) align_format
    {
        int x;
    };

    std::ostream& operator<<(std::ostream& os, const streams_format& s)
    {
        return os << "(" << s.x << ", " << s.y << ")";
    }

    template<std::size_t Align>
    std::ostream& operator<<(std::ostream& os, const align_format<Align>& a)
    {
        return os << a.x;
    }

    std::ostream& operator<<(std::ostream& os, const non_copyable& n)
    {
        return os << n.x_;
    }

    struct blocker
    {
        struct data
        {
            std::mutex m_;
            std::condition_variable cv_;
            bool blocked_ = true;
        };

        void wait() const
        {
            {
                std::unique_lock lock{data_->m_};
                while (data_->blocked_)
                    data_->cv_.wait(lock);
            }
            delete data_;
        }

        void release()
        {
            std::scoped_lock lock{data_->m_};
            data_->blocked_ = false;
            data_->cv_.notify_one(); // Must be done under protection of m_
        }

        data* data_{new data};
    };
}

namespace fmt
{
    template<>
    struct formatter<custom_format>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const custom_format &c, FormatContext &ctx)
        {
            return format_to(ctx.out(), "({}, {})", c.x, c.y);
        }
    };

    template<>
    struct formatter<blocker>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const blocker &b, FormatContext &ctx)
        {
            b.wait();
            return format_to(ctx.out(), "<blocker>");
        }
    };
}

using namespace fmt::literals;

TEST_CASE_METHOD(fixture, "logger no arguments test", "[logger]")
{
    XTR_LOG(p_, "Test"), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger arithmetic types test", "[logger]")
{
    XTR_LOG(p_, "Test {}", (short)42), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));

    XTR_LOG(p_, "Test {}", (unsigned short)42), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));

    XTR_LOG(p_, "Test {}", 42), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));

    XTR_LOG(p_, "Test {}", 42U), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));

    XTR_LOG(p_, "Test {}", 42L), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));

    XTR_LOG(p_, "Test {}", 42UL), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));

    XTR_LOG(p_, "Test {}", 42LL), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));

    XTR_LOG(p_, "Test {}", 42ULL), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));

    XTR_LOG(p_, "Test {}", true), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test true"_format(line_));

    XTR_LOG(p_, "Test {:.2f}", 42.42f), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42.42"_format(line_));

    XTR_LOG(p_, "Test {}", 42.42), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42.42"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger mixed types test", "[logger]")
{
    XTR_LOG(p_, "Test {} {}", 42.0, 42), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42.0 42"_format(line_));
    XTR_LOG(p_, "Test {} {} {}", 42.0, 42, 42.0), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42.0 42 42.0"_format(line_));
    XTR_LOG(p_, "Test {} {} {} {}", 42.0, 42, 42.0, 42), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42.0 42 42.0 42"_format(line_));
}

TEST_CASE_METHOD(file_fixture, "logger file buffer test", "[logger]")
{
    XTR_LOG(p_, "Test"), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test"_format(line_));

    XTR_LOG(p_, "Test {}", 42), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));

    const char*s = "Hello world";
    XTR_LOG(p_, "Test {}", s), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test Hello world"_format(line_));

    const std::string_view sv{"Hello world"};
    XTR_LOG(p_, "Test {}", sv), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test Hello world"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger string copy test", "[logger]")
{
    {
        blocker b;
        const char s[] = "String 1 contents";
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", s), line_ = __LINE__;
        std::strcpy(const_cast<char*>(s), "DEADBEEF");
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test String 1 contents"_format(line_));
    }

    {
        blocker b;
        char s[] = "String 2 contents";
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", s), line_ = __LINE__;
        std::strcpy(s, "DEADBEEF");
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test String 2 contents"_format(line_));
    }

    {
        blocker b;
        char s[] = "String 3 contents";
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", std::move(s)), line_ = __LINE__;
        std::strcpy(s, "DEADBEEF");
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test String 3 contents"_format(line_));
    }

    {
        blocker b;
        char storage[] = "String 4 contents";
        const char* s = storage;
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", s), line_ = __LINE__;
        std::strcpy(storage, "DEADBEEF");
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test String 4 contents"_format(line_));
    }

    {
        blocker b;
        char storage[] = "String 5 contents";
        char* s = storage;
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", s), line_ = __LINE__;
        std::strcpy(storage, "DEADBEEF");
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test String 5 contents"_format(line_));
    }

    {
        blocker b;
        char storage[] = "String 6 contents";
        char* s = storage;
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", std::move(s)), line_ = __LINE__;
        std::strcpy(storage, "DEADBEEF");
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test String 6 contents"_format(line_));
    }

    {
        blocker b;
        char storage[] = "String 7 contentsBADCODE";
        const std::string_view s{storage, 17};
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", s), line_ = __LINE__;
        std::memcpy(storage, "DEADBEEF", 8);
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test String 7 contents"_format(line_));
    }

    {
        blocker b;
        char storage[] = "String 8 contentsBADCODE";
        std::string_view s{storage, 17};
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", s), line_ = __LINE__;
        std::memcpy(storage, "DEADBEEF", 8);
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test String 8 contents"_format(line_));
    }

    {
        blocker b;
        char storage[] = "String 9 contentsBADCODE";
        std::string_view s{storage, 17};
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", std::move(s)), line_ = __LINE__;
        std::memcpy(storage, "DEADBEEF", 8);
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test String 9 contents"_format(line_));
    }

    {
        blocker b;
        const std::string s = "String 10 contents";
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", s), line_ = __LINE__;
        std::strcpy(const_cast<char*>(&s[0]), "DEADBEEF");
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test String 10 contents"_format(line_));
    }

    {
        blocker b;
        std::string s = "String 11 contents";
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", s), line_ = __LINE__;
        std::strcpy(&s[0], "DEADBEEF");
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test String 11 contents"_format(line_));
    }
}

TEST_CASE_METHOD(fixture, "logger string move test", "[logger]")
{
    blocker b;
    std::string s = "String contents..";
    char* storage = &s[0];
    XTR_LOG(p_, "{}", b);
    XTR_LOG(p_, "Test {}", std::move(s)), line_ = __LINE__;
    std::strcpy(storage, "Replaced contents");
    b.release();
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test Replaced contents"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger string reference test", "[logger]")
{
    {
        blocker b;
        const char s[] = "String 1 contents";
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", nocopy(s)), line_ = __LINE__;
        std::strcpy(const_cast<char*>(s), "Replaced contents");
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test Replaced contents"_format(line_));
    }

    {
        blocker b;
        char s[] = "String 2 contents";
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", nocopy(s)), line_ = __LINE__;
        std::strcpy(s, "Replaced contents");
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test Replaced contents"_format(line_));
    }

    {
        blocker b;
        char storage[] = "String 3 contents";
        const char* s = storage;
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", nocopy(s)), line_ = __LINE__;
        std::strcpy(storage, "Replaced contents");
        // Also replace the string pointed to by s, in case a
        // reference to s was incorrectly taken.
        s = "String 3 CODEBAD";
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test Replaced contents"_format(line_));
    }

    {
        blocker b;
        char storage[] = "String 4 contents";
        char* s = storage;
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", nocopy(s)), line_ = __LINE__;
        std::strcpy(storage, "Replaced contents");
        char storage2[] = "String 4 CODEBAD";
        s = storage2;
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test Replaced contents"_format(line_));
    }

    {
        blocker b;
        char storage[] = "String 5 contentsBADCODE";
        const std::string_view s{storage, 17};
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", nocopy(s)), line_ = __LINE__;
        std::memcpy(storage, "Replaced contents", 17);
        const_cast<std::string_view&>(s) = "String 5 contentsCODEBAD";
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test Replaced contents"_format(line_));
    }

    {
        blocker b;
        char storage[] = "String 6 contentsBADCODE";
        std::string_view s{storage, 17};
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", nocopy(s)), line_ = __LINE__;
        std::memcpy(storage, "Replaced contents", 17);
        s = "String 6 contentsCODEBAD";
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test Replaced contents"_format(line_));
    }

    {
        blocker b;
        const std::string s = "String 7 contents";
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", nocopy(s)), line_ = __LINE__;
        std::strcpy(const_cast<char*>(&s[0]), "Replaced contents");
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test Replaced contents"_format(line_));
    }

    {
        blocker b;
        std::string s = "String 8 contents";
        XTR_LOG(p_, "{}", b);
        XTR_LOG(p_, "Test {}", nocopy(s)), line_ = __LINE__;
        std::strcpy(&s[0], "Replaced contents");
        b.release();
        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test Replaced contents"_format(line_));
    }
}

TEST_CASE_METHOD(fixture, "logger string table test", "[logger]")
{
    // Try to stress the string table building
    const char* s1 = "foo";
    const std::string_view s2{"barBADCODE", 3};
    const char* s3 = "baz";
    const std::string_view s4{"blepBADCODE", 4};
    const std::string_view s5{"blopBADCODE", 4};
    const char* s6 = "";
    const char* s7 = "slightly longer string";
    XTR_LOG(p_, "Test {} {} {} {} {} {} {}", s1, s2, s3, s4, s5, s6, s7), line_ = __LINE__;
    REQUIRE(
        last_line() ==
        "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: "
        "Test foo bar baz blep blop  slightly longer string"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger string overflow test", "[logger]")
{
    // Three pointers are for the formatter pointer, string pointer and record
    // size -1 is for the terminating nul on the string.
    std::string s(64UL * 1024UL - sizeof(void*) * 3 - 1, char('X'));
    XTR_LOG(p_, "Test {}", s), line_ = __LINE__;
    REQUIRE(
        last_line() ==
        "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test {}"_format(line_, s));

    s += 'Y';

    XTR_LOG(p_, "Test {}", s), line_ = __LINE__;

    REQUIRE(
        last_line() ==
        "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test <truncated>"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger const string overflow test", "[logger]")
{
    const std::string s(64UL * 1024UL - sizeof(void*) * 3, char('X'));
    XTR_LOG(p_, "Test {}", s), line_ = __LINE__;
    REQUIRE(
        last_line() ==
        "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test <truncated>"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger string_view overflow test", "[logger]")
{
    // Three pointers are for the formatter pointer, string pointer and record
    // size -1 is for the terminating nul on the string.
    std::string s(64UL * 1024UL - sizeof(void*) * 3 - 1, char('X'));
    std::string_view sv{s};
    XTR_LOG(p_, "Test {}", sv), line_ = __LINE__;
    REQUIRE(
        last_line() ==
        "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test {}"_format(line_, sv));

    s += 'Y';
    sv = std::string_view{s};

    XTR_LOG(p_, "Test {}", sv), line_ = __LINE__;

    REQUIRE(
        last_line() ==
        "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test <truncated>"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger c string overflow test", "[logger]")
{
    // Three pointers are for the formatter pointer, string pointer and record
    // size -1 is for the terminating nul on the string.
    std::string s(64UL * 1024UL - sizeof(void*) * 3 - 1, char('X'));
    XTR_LOG(p_, "Test {}", s.c_str()), line_ = __LINE__;
    REQUIRE(
        last_line() ==
        "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test {}"_format(line_, s));

    s += 'Y';

    XTR_LOG(p_, "Test {}", s.c_str()), line_ = __LINE__;

    REQUIRE(
        last_line() ==
        "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test <truncated>"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger streams formatter test", "[logger]")
{
    streams_format s{10, 20};
    XTR_LOG(p_, "Streams {}", s), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Streams (10, 20)"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger custom formatter test", "[logger]")
{
    custom_format c{10, 20};
    XTR_LOG(p_, "Custom {}", c), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Custom (10, 20)"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger timestamp test", "[logger]")
{
    clock_nanos_ = 0;
    sync();
    XTR_LOG(p_, "Test"), line_ = __LINE__;
    REQUIRE(last_line() == "1970-01-01 00:00:00.000000: Name: logger.cpp:{}: Test"_format(line_));

    clock_nanos_ = 1000;
    sync();
    XTR_LOG(p_, "Test"), line_ = __LINE__;
    REQUIRE(last_line() == "1970-01-01 00:00:00.000001: Name: logger.cpp:{}: Test"_format(line_));

    clock_nanos_ = 4858113906123456000;
    sync();
    XTR_LOG(p_, "Test"), line_ = __LINE__;
    REQUIRE(last_line() == "2123-12-13 04:05:06.123456: Name: logger.cpp:{}: Test"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger producer arbitrary timestamp test", "[logger]")
{
    std::timespec ts;
    ts.tv_sec = 631155723;
    ts.tv_nsec = 654321000;

    xtr::timespec ts1(ts);
    ts1 = ts;

    XTR_LOG_TS(p_, "Test {}", ts1, 42), line_ = __LINE__;
    REQUIRE(last_line() == "1990-01-01 01:02:03.654321: Name: logger.cpp:{}: Test 42"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger producer rtc timestamp test", "[logger]")
{
    // Only somewhat sane way I can think of to test XTR_LOG_RTC
    const auto ts = xtr::detail::get_time<CLOCK_REALTIME_COARSE>();
    XTR_LOG_TS(p_, "Test {}", ts, 42), line_ = __LINE__;
    REQUIRE(last_line() == "{}: Name: logger.cpp:{}: Test 42"_format(ts, line_));
}



TEST_CASE_METHOD(fixture, "logger producer tsc timestamp test", "[logger]")
{
    // Only somewhat sane way I can think of to test XTR_LOG_TSC
    const auto ts = xtr::detail::tsc::now();
    XTR_LOG_TS(p_, "Test {}", ts, 42), line_ = __LINE__;
    const auto logged = last_line();
    const auto expected = "{}: Name: logger.cpp:{}: Test 42"_format(ts, line_);
    // The timestamps in logged and expected nay be off by a small margin due
    // to tsc::to_timespec's calibration state being stored in thread_local
    // variables, which may differ between the test thread and the logger
    // thread. This could be solved by applying dependency injection to all
    // of the clock related code, IMHO doing so would unacceptably compromise
    // the code, so instead the test is just bodged below to allow a small
    // difference in the timestamps.
    const auto timestamp_to_micros =
        [](const char* str)
        {
            std::tm tm;
            const char* pos = ::strptime(str, "%Y-%m-%d %T", &tm);
            REQUIRE(pos != nullptr);
            REQUIRE(*pos == '.');
            ++pos;
            // note that mktime converts local time, while str will be utc, so
            // the result of this function will be off by whatever the local
            // timezone adjustment is. This does not matter for the purpose of
            // this test.
            const std::time_t secs = std::mktime(&tm);
            std::int64_t micros;
            std::from_chars(pos, pos + 6, micros);
            return secs * 1000000L + micros;
        };
    REQUIRE(
        std::abs(
            timestamp_to_micros(logged.c_str()) -
            timestamp_to_micros(expected.c_str())) < 10);
}

TEST_CASE_METHOD(fixture, "tsc estimation test", "[logger]")
{
    // If the tsc hz can be queried, verify that the estimated hz is close
    if (const auto hz = xtr::detail::read_tsc_hz())
    {
        const auto estimated_hz = xtr::detail::estimate_tsc_hz();
        REQUIRE(estimated_hz == Approx(hz));
    }
}

#if __cpp_exceptions
TEST_CASE_METHOD(fixture, "logger error handling test", "[logger]")
{
    XTR_LOG(p_, "Test {}", thrower{}), line_ = __LINE__;
    REQUIRE(last_err() == "Exception error text");
    REQUIRE(lines_.empty());
}
#endif

TEST_CASE_METHOD(fixture, "logger argument destruction test", "[logger]")
{
    std::weak_ptr<int> w;
    {
        auto p{std::make_shared<int>(42)};
        w = p;
        XTR_LOG(p_, "Test {}", p), line_ = __LINE__;
        sync();
        REQUIRE(!lines_.empty());
    }
    REQUIRE(w.expired());
}

TEST_CASE("logger set output file test", "[logger]")
{
    file_buf buf;
    fixture f;

    f.log_.set_output_stream(buf.fp_);

    XTR_LOG(f.p_, "Test"), f.line_ = __LINE__;

    f.sync();
    REQUIRE(f.lines_.empty());
    REQUIRE(f.errors_.empty());

    std::vector<std::string> lines;
    buf.push_lines(lines);
    REQUIRE(lines.size() == 1);
    REQUIRE(lines.back() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test"_format(f.line_));
}

#if __cpp_exceptions
TEST_CASE("logger set error file test", "[logger]")
{
    file_buf buf;
    fixture f;

    f.log_.set_error_stream(buf.fp_);
    f.log_.set_output_function(
        [](const char*, std::size_t)
        {
            return -1;
        });

    XTR_LOG(f.p_, "Test"), f.line_ = __LINE__;

    f.sync();
    REQUIRE(f.lines_.empty());
    REQUIRE(f.errors_.empty());

    std::vector<std::string> lines;
    buf.push_lines(lines);
    REQUIRE(lines.size() == 1);
    REQUIRE(lines.back() == "Write error");
}
#endif

TEST_CASE_METHOD(fixture, "logger set output func test", "[logger]")
{
    std::string output;
    log_.set_output_function(
        [&output](const char* buf, std::size_t size)
        {
            output = std::string(buf, size);
            return size;
        });

    XTR_LOG(p_, "Test"), line_ = __LINE__;

    sync();
    REQUIRE(lines_.empty());
    REQUIRE(errors_.empty());

    REQUIRE(output == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test\n"_format(line_));
}

#if __cpp_exceptions
TEST_CASE_METHOD(fixture, "logger set error func test", "[logger]")
{
    std::string error;
    log_.set_output_function(
        [](const char*, std::size_t)
        {
            return -1;
        });
    log_.set_error_function(
        [&error](const char* buf, std::size_t size)
        {
            error = std::string(buf, size);
        });

    XTR_LOG(p_, "Test"), line_ = __LINE__;

    sync();
    REQUIRE(lines_.empty());
    REQUIRE(errors_.empty());

    REQUIRE(error == "Write error\n");
}
#endif

TEST_CASE_METHOD(fixture, "logger producer change name test", "[logger]")
{
    XTR_LOG(p_, "Test"), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test"_format(line_));

    p_.set_name("A new name");
    XTR_LOG(p_, "Test"), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: A new name: logger.cpp:{}: Test"_format(line_));

    p_.set_name("An even newer name");
    XTR_LOG(p_, "Test"), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: An even newer name: logger.cpp:{}: Test"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger no macro test", "[logger]")
{
    static constexpr xtr::detail::string test1{"{}: {}: Test\n"};
    p_.log<&test1>();
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: Test"_format(line_));

    static constexpr xtr::detail::string test2{"{}: {}: Test {}\n"};
    p_.log<&test2>(42);
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: Test 42"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger alignment test", "[logger]")
{
    XTR_LOG(p_, "Test {}", align_format<4>{42}), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));

    XTR_LOG(p_, "Test {}", align_format<8>{42}), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));

    XTR_LOG(p_, "Test {}", align_format<16>{42}), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));

    XTR_LOG(p_, "Test {}", align_format<32>{42}), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));

    XTR_LOG(p_, "Test {}", align_format<64>{42}), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger move test", "[logger]")
{
    non_copyable nc(42);
    XTR_LOG(p_, "Test {}", std::move(nc)), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test 42"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger non-blocking test", "[logger]")
{
    XTR_TRY_LOG(p_, "Test"), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger non-blocking drop test", "[logger]")
{
    // 64kb default size buffer, 8 bytes per log record, 16 bytes taken
    // by blocker.
    const std::size_t n_dropped = 100;
    const std::size_t n = (64 * 1024 - 16) / 8 + n_dropped;

    // Run the test a few times to verify that the dropped count is reset
    // each time the count is printed.
    for (std::size_t i = 0; i < 3; ++i)
    {
        blocker b;
        XTR_LOG(p_, "{}", b);

        for (std::size_t j = 0; j < n; ++j)
            XTR_TRY_LOG(p_, "Test");

        b.release();
        sync();

        // Plus 2 is because the blocker emits a line (the other line being
        // the `messages dropped' line that is being tested)
        const std::size_t expected_line_count = n - n_dropped + 2;

        // Wait for lines to be printed, there is no clean way of synchronizing
        // this as the count is communicated "out-of-band" (i.e not via the
        // queue)
        for (std::size_t j = 0; line_count() != expected_line_count; ++j)
        {
            REQUIRE(j < 1000); // Wait for at most 1 second
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: {} messages dropped"_format(n_dropped));

        clear_lines();
    }
}

TEST_CASE_METHOD(fixture, "logger soak test", "[logger]")
{
    constexpr std::size_t n = 100000;

    for (std::size_t i = 0; i < n; ++i)
    {
        XTR_LOG(p_, "Test {}", i), line_ = __LINE__;
        XTR_LOG(p_, "Test {}", i);
    }

    sync();
    REQUIRE(line_count() == n * 2);

    for (std::size_t i = 0; i < n * 2; i += 2)
    {
        REQUIRE(lines_[i] == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test {}"_format(line_, i / 2));
        REQUIRE(lines_[i + 1] == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test {}"_format(line_ + 1, i / 2));
    }
}

TEST_CASE_METHOD(fixture, "logger string soak test", "[logger]")
{
    constexpr std::size_t n = 100000;
    const char* s = "A string to be copied into the logger buffer";

    for (std::size_t i = 0; i < n; ++i)
    {
        XTR_LOG(p_, "Test {}", s), line_ = __LINE__;
        XTR_LOG(p_, "Test {}", s);
    }

    sync();
    REQUIRE(line_count() == n * 2);

    for (std::size_t i = 0; i < n * 2; i += 2)
    {
        REQUIRE(lines_[i] == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test {}"_format(line_, s));
        REQUIRE(lines_[i + 1] == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: Test {}"_format(line_ + 1, s));
    }
}

TEST_CASE_METHOD(fixture, "logger unprintable characters test", "[logger]")
{
    const char *s = "\nTest\r\nTest";
    XTR_LOG(p_, "{}", s), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: \\x0ATest\\x0D\\x0ATest"_format(line_));

    XTR_LOG(p_, "{}", nocopy(s)), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: \\x0ATest\\x0D\\x0ATest"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger escape sequence test", "[logger]")
{
    const char* s = "\x1b]0;Test\x07";
    XTR_LOG(p_, "{}", s), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: \\x1B]0;Test\\x07"_format(line_));

    XTR_LOG(p_, "{}", nocopy(s)), line_ = __LINE__;
    REQUIRE(last_line() == "2000-01-01 01:02:03.123456: Name: logger.cpp:{}: \\x1B]0;Test\\x07"_format(line_));
}

TEST_CASE_METHOD(fixture, "logger sync test", "[logger]")
{
    std::atomic<std::size_t> sync_count{};

    log_.set_sync_function(
        [&sync_count]()
        {
            ++sync_count;
        });

    // set_sync_function calls sync
    REQUIRE(sync_count == 1);

    const std::size_t n = 10;

    for (std::size_t i = 0; i < n; ++i)
    {
        std::size_t target_count = sync_count + 1;
        sync();
        REQUIRE(sync_count == target_count);
    }

    // Set an empty function to avoid accessing a dangling reference
    // to sync_count
    log_.set_sync_function([](){});
}

TEST_CASE_METHOD(fixture, "logger flush test", "[logger]")
{
    std::atomic<std::size_t> flush_count{};

    log_.set_flush_function(
        [&flush_count]()
        {
            ++flush_count;
        });

    // set_flush_function calls sync, which calls flush, so here flush_count
    // is either 1, or 2 if flush was called in between set_flush_function
    // and sync being processed.
    REQUIRE((flush_count == 1 || flush_count == 2));

    const std::size_t n = 10;

    for (std::size_t i = 0; i < n; ++i)
    {
        std::size_t target_count = flush_count + 1;
        XTR_LOG(p_, "Test");
        // Wait for flush to be called
        for (std::size_t j = 0; flush_count < target_count; ++j)
        {
            REQUIRE(j < 1000); // Wait for at most 1 second
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // Set an empty function to avoid accessing a dangling reference
    // to flush_count
    log_.set_flush_function([](){});
}

TEST_CASE_METHOD(throughput_fixture, "logger throughput", "[.logger]")
{
    using clock = std::chrono::high_resolution_clock;

    constexpr std::size_t n = 10000000;

    static constexpr char fmt[] =
        "{}{} Test message of length 80 chars Test message of length 80 chars Test message of\n";

    const auto print_result =
        [](auto t0, auto t1, const char* name)
        {
            const std::chrono::duration<double> delta = t1 - t0;
            const double messages_sec = n / delta.count();
            // 33 is # of chars in timestamp and name
            const double bytes_per_message = double(sizeof(fmt) - 1 + 33);
            const double mb_sec = (messages_sec * bytes_per_message) / (1024 * 1024);
            std::cout
                << name << " messages/s: " << std::size_t(messages_sec) << ", "
                << "MB/s: " << mb_sec << "\n";
        };

    {
        const auto t0 = clock::now();
        for (std::size_t i = 0; i < n; ++i)
            p_.log<&fmt>();
        p_.sync();
        const auto t1 = clock::now();
        print_result(t0, t1, "Logger");
    }

    {
        // This isn't printing exactly what the logger does, but it's close enough
        const auto t0 = clock::now();
        for (std::size_t i = 0; i < n; ++i)
            fmt::print(fp_, fmt, "2019-10-17 20:59:03: Name   ", i);
        p_.sync();
        const auto t1 = clock::now();
        print_result(t0, t1, "Fmt");
    }
}

