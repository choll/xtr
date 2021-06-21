#include "xtr/logger.hpp"

// clang++ -o - -S -mllvm -std=c++17 -pipe -fno-rtti -isystem third_party/include -Ofast -march=native -DNDEBUG -fno-plt -Ibuild/clang++-release/xtr -Ixtr test/logger.cpp|llvm-mca -iterations=1
void llvm_mca_test1(xtr::logger::sink& s)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_no_arg");
    XTR_LOG(s, "Hello world");
    __asm volatile("# LLVM-MCA-END");
}

void llvm_mca_test2(xtr::logger::sink& s, int x)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_int_arg");
    XTR_LOG(s, "Hello world {}", x);
    __asm volatile("# LLVM-MCA-END");
}

void llvm_mca_test3(xtr::logger::sink& s, long x)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_long_arg");
    XTR_LOG(s, "Hello world {}", x);
    __asm volatile("# LLVM-MCA-END");
}

void llvm_mca_test3(xtr::logger::sink& s, long x, long y)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_long_long_arg");
    XTR_LOG(s, "Hello world {} {}", x, y);
    __asm volatile("# LLVM-MCA-END");
}

void llvm_mca_test5(xtr::logger::sink& s, const char* str)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_string_arg");
    XTR_LOG(s, "Hello world {}", str);
    __asm volatile("# LLVM-MCA-END");
}

void llvm_mca_test6(xtr::logger::sink& s, const char* str1, const char* str2)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_string_string_arg");
    XTR_LOG(s, "Hello world {} {}", str1, str2);
    __asm volatile("# LLVM-MCA-END");
}

void llvm_mca_test7(xtr::logger::sink& s, const std::string_view str)
{
    __asm volatile("# LLVM-MCA-BEGIN logger_string_view_arg");
    XTR_LOG(s, "Hello world {}", str);
    __asm volatile("# LLVM-MCA-END");
}
