#include "xtr/logger.hpp"

// clang++ -o - -S -mllvm -std=c++17 -pipe -fno-rtti -isystem third_party/include -Ofast -march=native -DNDEBUG -fno-plt -Ibuild/clang++-release/xtr -Ixtr test/logger.cpp|llvm-mca -iterations=1
void llvm_mca_test1(xtr::logger::producer& p)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_no_arg");
    XTR_LOG(p, "Hello world");
    __asm volatile("# LLVM-MCA-END");
}

void llvm_mca_test2(xtr::logger::producer& p, int x)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_int_arg");
    XTR_LOG(p, "Hello world {}", x);
    __asm volatile("# LLVM-MCA-END");
}

void llvm_mca_test3(xtr::logger::producer& p, long x)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_long_arg");
    XTR_LOG(p, "Hello world {}", x);
    __asm volatile("# LLVM-MCA-END");
}

void llvm_mca_test3(xtr::logger::producer& p, long x, long y)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_long_long_arg");
    XTR_LOG(p, "Hello world {} {}", x, y);
    __asm volatile("# LLVM-MCA-END");
}

void llvm_mca_test5(xtr::logger::producer& p, const char* s)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_string_arg");
    XTR_LOG(p, "Hello world {}", s);
    __asm volatile("# LLVM-MCA-END");
}

void llvm_mca_test6(
    xtr::logger::producer& p, const char* s1, const char* s2)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_string_string_arg");
    XTR_LOG(p, "Hello world {} {}", s1, s2);
    __asm volatile("# LLVM-MCA-END");
}

void llvm_mca_test7(xtr::logger::producer& p, const std::string_view s)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_string_view_arg");
    XTR_LOG(p, "Hello world {}", s);
    __asm volatile("# LLVM-MCA-END");
}

