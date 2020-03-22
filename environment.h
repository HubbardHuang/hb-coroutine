#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <map>
#include <memory>
#include <pthread.h>
#include <vector>

namespace hbco {

class Coroutine;
class CoroutineEnvironment {
    friend class Coroutine;
    friend const std::shared_ptr<CoroutineEnvironment>& CurrEnv(void);
    friend const std::shared_ptr<Coroutine>& CurrCoroutine(void);

private:
    std::vector<std::shared_ptr<Coroutine>> callstack_;
    CoroutineEnvironment();
};

extern const std::shared_ptr<CoroutineEnvironment>& CurrEnv(void);

}

#endif