#ifndef COUTINE_LAUNCHER_H
#define COUTINE_LAUNCHER_H

#include "coroutine.h"
#include "environment.h"

namespace hbco {

class CoroutineLauncher {
public:
    CoroutineLauncher() = delete;
    CoroutineLauncher(int port = -1);
    ~CoroutineLauncher();
};

} // namespace hbco

#endif