#pragma once
#include <iostream>
#include <sstream>
#include <iomanip>

extern int detail_enabled;

#define LOG_WRITE(detail, exp)                                    \
    if (detail && detail_enabled)                                 \
        std::cout << '\t' << exp << std::endl;                    \
    else if (!detail)                                             \
        std::cout << exp << std::endl;

#define LOG(exp) LOG_WRITE(0, exp)
#define LOG_DETAILED(exp) LOG_WRITE(1, exp)
