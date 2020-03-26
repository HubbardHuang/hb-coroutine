#include "environment.h"
#include "coroutine.h"
#include "log.h"

namespace hbco {

CoroutineEnvironment::CoroutineEnvironment() {
    Coroutine* main_co(new Coroutine("main_co"));
    callstack_.push_back(main_co);
    coroutines_.push_back(main_co);
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
        while (!curr_env->coroutines_.empty()) {
            auto last = curr_env->coroutines_.back();
            curr_env->coroutines_.pop_back();
            delete last;
        }
        env_manager.erase(curr_tid);
        delete curr_env;
    }
}

}