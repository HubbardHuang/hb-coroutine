#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <stdio.h>

#ifdef DEBUG
#define Display(x)                                                                                 \
    do {                                                                                           \
        std::cout << "[" __FILE__ << ":" << __LINE__ << "]" << #x << ": " << (x) << std::endl;     \
    } while (false)
#else
#define Display(x)
#endif

#ifdef DEBUG
#define DisplayWithPrefix(p, x)                                                                    \
    do {                                                                                           \
        std::cout << "[./" << __FILE__ << ":" << __LINE__ << "]{" << p << "}" << #x << ": " << (x) \
                  << std::endl;                                                                    \
    } while (false)
#else
#define Display(x)
#endif

#endif