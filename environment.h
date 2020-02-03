#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <map>
#include <memory>
#include <pthread.h>
#include <stack>

namespace hbco {

class Coroutine;
struct CoroutineEnvironment {
    std::stack<std::shared_ptr<Coroutine>> callstack_;
    CoroutineEnvironment();
};

extern std::shared_ptr<CoroutineEnvironment> CurrEnv(void);

}

#endif