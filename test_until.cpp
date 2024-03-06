#include<assert.h>
#include "yuan_log.cpp"
#include "yuan_until.hpp"
#include "yuan_macro.hpp"

yuan::Logger::ptr g_logger = YUAN_LOG_ROOT();

void test_assert()
{
    //YUAN_LOG_INFO(g_logger) << yuan::BacktraceToString(10);
    YUAN_ASSERT(false);
    YUAN_ASSERT2(0 == 1,"abcdef xx");
}

int main()
{
    test_assert();
    return 0;
}