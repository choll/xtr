#include "xtr/logger.hpp"

int main()
{
    xtr::logger log("/dev/stdout");
    auto s = log.get_sink("test");
    XTR_LOG(s, "Hello world");
    return 0;
}
