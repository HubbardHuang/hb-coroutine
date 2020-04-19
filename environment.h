#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <list>
#include <map>
#include <pthread.h>
#include <set>
#include <vector>

#include "condition_variable.h"

namespace hbco {

class Coroutine;
class CoroutineEnvironment {
    friend class Coroutine;
    friend class CondVar;
    friend CoroutineEnvironment* CurrEnv(void);
    friend Coroutine* CurrCoroutine(void);
    friend void ReleaseResources(void);
    friend void EpollEventLoop(void);

private:
    int listen_fd_;
    int port_;
    int epoll_fd_;
    std::vector<Coroutine*> callstack_;
    std::map<Coroutine*, bool> coroutines_;
    CoroutineEnvironment();
    ~CoroutineEnvironment();

public:
    CondVar accept_cond_;
    std::list<Coroutine*> pending_;
    std::list<Coroutine*> runnable_;
    std::set<Coroutine*> epoll_items_;
    int GetEpollFd(void);
    int GetListeningFd(void);
};

extern CoroutineEnvironment* CurrEnv(void);
extern void ReleaseResources(void);

} // namespace hbco

#endif