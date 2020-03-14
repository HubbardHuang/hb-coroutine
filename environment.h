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
    friend std::shared_ptr<CoroutineEnvironment> CurrEnv(void);

private:
    std::vector<std::shared_ptr<Coroutine>> callstack_;

public:
    CoroutineEnvironment();
};

extern std::shared_ptr<CoroutineEnvironment> CurrEnv(void);

}

#endif