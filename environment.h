#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <list>
#include <map>
#include <pthread.h>
#include <vector>

namespace hbco {

class Coroutine;
class CoroutineEnvironment {
    friend class Coroutine;
    friend class CondVar;
    friend CoroutineEnvironment* CurrEnv(void);
    friend Coroutine* CurrCoroutine(void);
    friend void ReleaseResources(void);

private:
    std::vector<Coroutine*> callstack_;
    std::vector<Coroutine*> coroutines_;
    CoroutineEnvironment();
    ~CoroutineEnvironment() = default;

public:
    std::list<Coroutine*> pending_;
    std::list<Coroutine*> runnable_;
};

extern CoroutineEnvironment* CurrEnv(void);
extern void ReleaseResources(void);
}

#endif