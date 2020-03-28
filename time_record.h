#ifndef TIME_RECORD_H
#define TIME_RECORD_H

#include <sys/time.h>

namespace hbco {

class SleepRecord {
public:
    struct timeval when_;
    long how_long_;
    SleepRecord() = default;
    SleepRecord(timeval when, long how_long)
      : how_long_(how_long) {
        when_.tv_sec = when.tv_sec;
        when_.tv_usec = when.tv_usec;
    }
};
}

#endif