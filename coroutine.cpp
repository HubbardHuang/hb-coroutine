#include <string.h>

#include "coroutine.h"
#include "log.h"

namespace hbco {

Coroutine::Coroutine(const std::string& name)
  : name_(name) {
    sleep_.how_long_ = -1;
    memset(&sleep_.when_, sizeof(timeval), 0);
}

void
Coroutine::CondVarSignal(void) {
    CoroutineEnvironment* curr_env = CurrEnv();
    if (curr_env->pending_.empty()) {
        return;
    }
    Coroutine* co = curr_env->pending_.front();
    curr_env->pending_.pop_front();
    curr_env->runnable_.push_back(co);
}

void
Coroutine::CondVarWait(void) {
    CoroutineEnvironment* curr_env = CurrEnv();
    Coroutine* curr_co = curr_env->callstack_.back();
    curr_env->pending_.push_back(curr_co);
    Yield();
}

void
Coroutine::PollTime(long duration = -1) {
    CoroutineEnvironment* curr_env = CurrEnv();
    Coroutine* curr_co = curr_env->callstack_.back();
    gettimeofday(&(curr_co->sleep_.when_), nullptr);
    curr_co->sleep_.how_long_ = duration;
    curr_env->pending_.push_back(curr_co);
    Yield();
}

void
Coroutine::Yield() {
    if (CurrEnv()->callstack_.size() == 1) {
        return;
    }
    Coroutine* curr_co = CurrEnv()->callstack_.back();
    curr_co->can_run_next_time_ = true;
    CurrEnv()->callstack_.pop_back();
    Coroutine* prev_co = CurrEnv()->callstack_.back();
    CurrEnv()->callstack_.push_back(curr_co);
    swapcontext(&curr_co->context_.ctx_, &prev_co->context_.ctx_);
    curr_co->can_run_next_time_ = false;
}

void
Coroutine::Resume(Coroutine* next_co) {
    if (next_co->can_run_next_time_ == false) {
        return;
    }
    auto curr_co = CurrEnv()->callstack_.back();
    CurrEnv()->callstack_.push_back(next_co);
    swapcontext(&curr_co->context_.ctx_, &next_co->context_.ctx_);
    CurrEnv()->callstack_.pop_back();
}

Coroutine*
Coroutine::Create(const std::string& name, CoFunc func, void* arg) {
    Coroutine* co = new Coroutine(name);
    co->can_run_next_time_ = true;
    getcontext(&co->context_.ctx_);
    co->context_.ctx_.uc_stack.ss_sp = co->stack_;
    co->context_.ctx_.uc_stack.ss_size = sizeof(co->stack_);
    co->context_.ctx_.uc_stack.ss_flags = 0;
    co->context_.ctx_.uc_link = &(CurrEnv()->callstack_.back()->context_.ctx_);
    makecontext(&co->context_.ctx_, (void (*)(void))func, 0);
    CurrEnv()->coroutines_.push_back(co);
    return co;
}

Coroutine*
CurrCoroutine(void) {
    return CurrEnv()->callstack_.back();
}

void
EventLoop(void) {
    CoroutineEnvironment* curr_env = CurrEnv();
    while (true) {
        struct timeval curr_tv;
        gettimeofday(&curr_tv, nullptr);
        long curr_time = curr_tv.tv_sec * 1000 + curr_tv.tv_usec / 1000;
        for (auto i = curr_env->pending_.begin(); i != curr_env->pending_.end();) {
            Coroutine* co = *i;
            long ms_timestamp = co->sleep_.when_.tv_sec * 1000 + co->sleep_.when_.tv_usec / 1000;
            if (co->sleep_.how_long_ < 0 || ms_timestamp + co->sleep_.how_long_ <= curr_time) {
                curr_env->runnable_.push_back(co);
                i = curr_env->pending_.erase(i);
            } else {
                i++;
            }
        }
        while (!curr_env->runnable_.empty()) {
            Coroutine* co = curr_env->runnable_.front();
            curr_env->runnable_.pop_front();
            Coroutine::Resume(co);
        }
    }
}

}