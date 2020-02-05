#include "coroutine.h"

namespace hbco {

Coroutine::Coroutine() {}

void
Coroutine::Yield() {
    if (CurrEnv()->callstack_.size() == 1) {
        return;
    }
    auto curr_co = CurrEnv()->callstack_.back();
    CurrEnv()->callstack_.pop_back();
    auto prev_co = CurrEnv()->callstack_.back();
    swapcontext(&curr_co->context_.ctx_, &prev_co->context_.ctx_);
}

void
Coroutine::Resume(const std::shared_ptr<Coroutine>& next_co) {
    auto curr_co = CurrEnv()->callstack_.back();
    CurrEnv()->callstack_.push_back(next_co);
    swapcontext(&curr_co->context_.ctx_, &next_co->context_.ctx_);
}

std::shared_ptr<Coroutine>
Coroutine::Create(CoFunc func, void* arg) {
    std::shared_ptr<Coroutine> co(new Coroutine());
    co->env_ = CurrEnv();
    getcontext(&co->context_.ctx_);
    co->context_.ctx_.uc_stack.ss_sp = co->stack_;
    co->context_.ctx_.uc_stack.ss_size = sizeof(co->stack_);
    co->context_.ctx_.uc_stack.ss_flags = 0;
    co->context_.ctx_.uc_link = &(CurrEnv()->callstack_.back()->context_.ctx_);
    makecontext(&co->context_.ctx_, (void (*)(void))func, 0);
    return co;
}
}