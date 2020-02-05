#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <map>
#include <memory>
#include <pthread.h>
#include <vector>

namespace hbco {

class Coroutine;
struct CoroutineEnvironment {
    std::vector<std::shared_ptr<Coroutine>> callstack_;
    CoroutineEnvironment();
};

extern std::shared_ptr<CoroutineEnvironment> CurrEnv(void);

}

#endif