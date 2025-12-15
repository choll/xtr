// Copyright 2022 Chris E. Holloway
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

#ifndef XTR_IO_DETAIL_FD_STORAGE_BASE_HPP
#define XTR_IO_DETAIL_FD_STORAGE_BASE_HPP

#include "xtr/detail/file_descriptor.hpp"
#include "xtr/io/storage_interface.hpp"

#include <string>

namespace xtr::detail
{
    class fd_storage_base;
}

class xtr::detail::fd_storage_base : public storage_interface
{
public:
    fd_storage_base(int fd, std::string reopen_path);

    void sync() noexcept override;

    int reopen() noexcept override;

protected:
    virtual void replace_fd(int newfd) noexcept;

    std::string reopen_path_;
    detail::file_descriptor fd_;
};

#endif
