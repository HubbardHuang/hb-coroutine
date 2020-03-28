#include "coroutine.h"
#include "log.h"

namespace hbco {

Coroutine::Coroutine(const std::string& name)
  : name_(name) {}

void
Coroutine::CondVarSignal(void) {
    CoroutineEnvironment* curr_env = CurrEnv();
    if (curr_env->pending_.empty()) {
        return;
    }
    Coroutine* co = curr_env->pending_.front();
    curr_env->pending_.pop();
    curr_env->runnable_.push(co);
}

void
Coroutine::CondVarWait(void) {
    CoroutineEnvironment* curr_env = CurrEnv();
    Coroutine* curr_co = curr_env->callstack_.back();
    curr_env->pending_.push(curr_co);
    Yield();
}

void
Coroutine::Poll(void) {
    CoroutineEnvironment* curr_env = CurrEnv();
    Coroutine* curr_co = curr_env->callstack_.back();
    curr_env->pending_.push(curr_co);
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
        while (!curr_env->pending_.empty()) {
            Coroutine* co = curr_env->pending_.front();
            curr_env->pending_.pop();
            curr_env->runnable_.push(co);
        }
        while (!curr_env->runnable_.empty()) {
            Coroutine* co = curr_env->runnable_.front();
            curr_env->runnable_.pop();
            Coroutine::Resume(co);
        }
    }
}

}