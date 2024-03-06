#ifndef __YUAN_MACRO_HPP__
#define __YUAN_MACRO_HPP__

#include<string.h>
#include<string.h>
#include"yuan_until.hpp"
#include "yuan_log.hpp"

#define YUAN_ASSERT1(x)\
    if(!(x))\
    {\
        YUAN_LOG_ERROR(YUAN_LOG_ROOT()) << "ASSERTION: " #x \
                                        << "\nbacktrace:\n" \
                                        << yuan::BacktraceToString(100,2,"    "); \
        assert(x);\
    }

#define YUAN_ASSERT2(x,w)\
    if(!(x))\
    {\
        YUAN_LOG_ERROR(YUAN_LOG_ROOT()) << "ASSERTION: " #x \
                                        << "\n" << w \
                                        << "\nbacktrace:\n" \
                                        << yuan::BacktraceToString(100,2,"    "); \
        assert(x);\
    }
#define YUAN_ASSERT3(x)\
    if(!(x))\
    {\
        std::cout << "YUAN_ASSERT3" << std::endl;\
    }

#endif