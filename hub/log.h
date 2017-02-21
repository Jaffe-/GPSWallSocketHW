#pragma once
#include <iostream>
#include <sstream>
#include <iomanip>

extern int detail_enabled;

#define LOG_PREFIX                                          \
    std::cout << std::left << std::setw(10) << LOG_MODULE

#define LOG_WRITE(detail, exp)                                          \
    if (detail && detail_enabled)                                       \
        LOG_PREFIX << "  ( " << exp << " )" << std::endl;               \
    else if (!detail)                                                   \
        LOG_PREFIX << exp << std::endl;

#define LOG(exp) LOG_WRITE(0, exp)
#define LOG_DETAILED(exp) LOG_WRITE(1, exp)
