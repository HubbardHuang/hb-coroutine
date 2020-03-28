#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <map>
#include <pthread.h>
#include <queue>
#include <vector>

namespace hbco {

class Coroutine;
class CoroutineEnvironment {
    friend class Coroutine;
    friend CoroutineEnvironment* CurrEnv(void);
    friend Coroutine* CurrCoroutine(void);
    friend void ReleaseResources(void);

private:
    std::vector<Coroutine*> callstack_;
    std::vector<Coroutine*> coroutines_;
    CoroutineEnvironment();
    ~CoroutineEnvironment() = default;

public:
    std::queue<Coroutine*> pending_;
    std::queue<Coroutine*> runnable_;
};

extern CoroutineEnvironment* CurrEnv(void);
extern void ReleaseResources(void);
}

#endif