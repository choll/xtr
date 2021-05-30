#include "xtr/logger.hpp"

int main()
{
    xtr::logger log("/dev/stdout");
    auto p = log.get_producer("test");
    XTR_LOG(p, "Hello world");
    return 0;
}

