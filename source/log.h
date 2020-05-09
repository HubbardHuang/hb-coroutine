#ifndef LOG_H
#define LOG_H

#include <iostream>

#include <stdio.h>

#ifdef DEBUG
#define Display(x)                                                                                 \
    do {                                                                                           \
        std::cout << "[" __FILE__ << ":" << __LINE__ << "]" << #x << ": " << (x) << std::endl;     \
    } while (false)
#define COLOR(x, f) "\033[" x "m\033[01m\033[05m" f "\033[0m"
#define BLUE "36"
#define GREEN "32"
#define RED "31"
#define YELLOW "33"
#define WHITE "77"
#define b_GREEN_f_BLACK(f) "\033[102;30m\033[5m" f "\033[0m"
#define b_BLUE_f_BLACK(f) "\033[106;30m\033[5m" f "\033[0m"
#define GO_UP_N_LINE(n) printf("\33[%lldA", n)
#define GO_DOWN_N_LINE(n)                                                                          \
    do {                                                                                           \
        printf("\r\33[%lldB", n);                                                                  \
        fflush(stdout);                                                                            \
    } while (false)
#define TO_Nth_COL(n) printf("\r\33[" #n "C")
#else
#define Display(x)
#define COLOR(x, f)
#define BLUE
#define GREEN
#define RED
#define YELLOW
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