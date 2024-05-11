#pragma once
#include "util/print.hpp"

#define assert_v(cond, ret, ...)                                                    \
    if(!(cond)) {                                                                   \
        printf("assert failed at %s:%d: %s ", __FILE__, __LINE__, strerror(errno)); \
        print(__VA_ARGS__);                                                         \
        return ret;                                                                 \
    }

#define assert_n(cond, ...)                                                         \
    if(!(cond)) {                                                                   \
        printf("assert failed at %s:%d: %s ", __FILE__, __LINE__, strerror(errno)); \
        print(__VA_ARGS__);                                                         \
        return;                                                                     \
    }
