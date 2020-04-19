#include <map>
#include <pthread.h>

#include "port.h"

namespace hbco {

static std::map<pthread_t, int> ports;

void
SetPort(int port) {
    auto curr_tid = pthread_self();
    ports[curr_tid] = port;
}

int
CurrListeningPort(void) {
    auto curr_tid = pthread_self();
    auto result = ports.find(curr_tid);
    if (result == ports.end()) {
        return -1;
    }
    return ports[curr_tid];
}

} // namespace hbco