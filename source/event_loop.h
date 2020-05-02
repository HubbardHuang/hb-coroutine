#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <stdint.h>

namespace hbco {

extern void EpollEventLoop(void);
extern bool Poll(int fd, uint32_t epoll_events, long wait_time = 1000);

} // namespace hbco

#endif