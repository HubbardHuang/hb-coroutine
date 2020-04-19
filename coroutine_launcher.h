#ifndef COUTINE_LAUNCHER_H
#define COUTINE_LAUNCHER_H

#include "coroutine.h"
#include "environment.h"

namespace hbco {

class CoroutineLauncher {
public:
    CoroutineLauncher(int port);
    ~CoroutineLauncher();
};

} // namespace hbco

#endif