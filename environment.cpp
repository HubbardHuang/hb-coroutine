#include <sys/epoll.h>

#include "coroutine.h"
#include "environment.h"
#include "log.h"

namespace hbco {

CoroutineEnvironment::CoroutineEnvironment() {
    Coroutine* main_co(new Coroutine("main_co"));
    callstack_.push_back(main_co);
    coroutines_.insert({ main_co, true });
    epoll_fd_ = epoll_create1(0);
}

int
CoroutineEnvironment::GetEpollFd(void) {
    return epoll_fd_;
}

static std::map<pthread_t, CoroutineEnvironment*> env_manager;

CoroutineEnvironment*
CurrEnv(void) {
    auto curr_tid = pthread_self();
    auto result = env_manager.find(curr_tid);
    if (result == env_manager.end()) {
        CoroutineEnvironment* curr_env = new CoroutineEnvironment();
        env_manager.insert({ curr_tid, curr_env });
    }
    return env_manager[curr_tid];
}

void
ReleaseResources(void) {
    auto curr_tid = pthread_self();
    auto result = env_manager.find(curr_tid);
    if (result != env_manager.end()) {
        auto curr_env = result->second;
        Display(curr_env->callstack_.size());
        while (!curr_env->callstack_.empty()) {
            Display(curr_env->callstack_.back()->name_);
            auto* co = curr_env->callstack_.back();
            curr_env->callstack_.pop_back();
        }
        for (auto it = curr_env->coroutines_.begin(); it != curr_env->coroutines_.end();) {
            auto prev = it->first;
            it = curr_env->coroutines_.erase(it);
            Display(prev->name_);
            delete prev;
        }
        // while (!curr_env->coroutines_.empty()) {
        //     auto last = curr_env->coroutines_.back();
        //     curr_env->coroutines_.pop_back();
        //     delete last;
        // }
        env_manager.erase(curr_tid);
        delete curr_env;
    }
}

} // namespace hbco