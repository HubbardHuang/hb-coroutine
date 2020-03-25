#ifndef LOG_H
#define LOG_H

#include <iostream>

#ifdef DEBUG
#define Display(x)                                                                                 \
    do {                                                                                           \
        std::cout << "[./" << __FILE__ << ":" << __LINE__ << "]" << #x << ": " << (x)              \
                  << std::endl;                                                                    \
    } while (false)
#else
#define Display(x)
#endif

#endif