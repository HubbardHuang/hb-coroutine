#include "coroutine.h"

namespace hbco {

Coroutine::Coroutine() {}

void
Coroutine::Yield() {
    if (CurrEnv()->callstack_.size() == 1) {
        return;
    }
    auto curr_co = CurrEnv()->callstack_.back();
    curr_co->can_run_next_time_ = true;
    CurrEnv()->callstack_.pop_back();
    auto prev_co = CurrEnv()->callstack_.back();
    CurrEnv()->callstack_.push_back(curr_co);
    swapcontext(&curr_co->context_.ctx_, &prev_co->context_.ctx_);
    curr_co->can_run_next_time_ = false;
}

void
Coroutine::Resume(const std::shared_ptr<Coroutine>& next_co) {
    if (next_co->can_run_next_time_ == false) {
        return;
    }
    auto curr_co = CurrEnv()->callstack_.back();
    CurrEnv()->callstack_.push_back(next_co);
    swapcontext(&curr_co->context_.ctx_, &next_co->context_.ctx_);
    CurrEnv()->callstack_.pop_back();
}

std::shared_ptr<Coroutine>
Coroutine::Create(CoFunc func, void* arg) {
    std::shared_ptr<Coroutine> co(new Coroutine());
    co->env_ = CurrEnv();
    co->can_run_next_time_ = true;
    getcontext(&co->context_.ctx_);
    co->context_.ctx_.uc_stack.ss_sp = co->stack_;
    co->context_.ctx_.uc_stack.ss_size = sizeof(co->stack_);
    co->context_.ctx_.uc_stack.ss_flags = 0;
    co->context_.ctx_.uc_link = &(CurrEnv()->callstack_.back()->context_.ctx_);
    makecontext(&co->context_.ctx_, (void (*)(void))func, 0);
    return co;
}
}