#include "condition_variable.h"
#include "coroutine.h"
#include "environment.h"

namespace hbco {

void
CondVar::Signal(void) {
    if (pending_.empty()) {
        return;
    }
    CoroutineEnvironment* curr_env = CurrEnv();
    struct timeval curr_tv;
    gettimeofday(&curr_tv, nullptr);
    long curr_time = curr_tv.tv_sec * 1000 + curr_tv.tv_usec / 1000;
    for (auto i = pending_.begin(); i != pending_.end();) {
        Coroutine* co = *i;
        long ms_timestamp = co->sleep_.when_.tv_sec * 1000 + co->sleep_.when_.tv_usec / 1000;
        if (co->sleep_.how_long_ < 0 || ms_timestamp + co->sleep_.how_long_ <= curr_time) {
            curr_env->runnable_.push_back(co);
            i = pending_.erase(i);
        } else {
            i++;
        }
    }
}

void
CondVar::Wait(void) {
    Coroutine* curr_co = CurrCoroutine();
    pending_.push_back(curr_co);
    Coroutine::Yield();
}

void
CondVar::WaitTime(long duration) {
    Coroutine* curr_co = CurrCoroutine();
    gettimeofday(&(curr_co->sleep_.when_), nullptr);
    curr_co->sleep_.how_long_ = duration;
    pending_.push_back(curr_co);
    Coroutine::Yield();
}

}