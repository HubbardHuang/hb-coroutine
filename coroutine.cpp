#include "coroutine.h"

namespace hbco {

CoroutineEnvironment Coroutine::env_;

void
Coroutine::Yield() {
    if (env_.callstack_.size() == 1) {
        return;
    }
    auto curr_co = env_.callstack_.top();
    env_.callstack_.pop();
    auto prev_co = env_.callstack_.top();
    swapcontext(&curr_co->context_.ctx_, &prev_co->context_.ctx_);
}

void
Coroutine::Resume(const std::shared_ptr<Coroutine>& next_co) {
    auto curr_co = env_.callstack_.top();
    env_.callstack_.push(next_co);
    swapcontext(&curr_co->context_.ctx_, &next_co->context_.ctx_);
}

std::shared_ptr<Coroutine>
Coroutine::Create(CoFunc func, void* arg) {
    // if (env_.callstack_.empty()) {
    //     env_.callstack_.push(new Coroutine());
    // }
    std::shared_ptr<Coroutine> co(new Coroutine());
    getcontext(&co->context_.ctx_);
    co->context_.ctx_.uc_stack.ss_sp = co->stack_;
    co->context_.ctx_.uc_stack.ss_size = sizeof(co->stack_);
    co->context_.ctx_.uc_stack.ss_flags = 0;
    co->context_.ctx_.uc_link = &(env_.callstack_.top()->context_.ctx_);
    makecontext(&co->context_.ctx_, (void (*)(void))func, 0);
    return co;
}
}