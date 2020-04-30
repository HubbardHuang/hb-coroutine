#include <string.h>
#include <sys/epoll.h>

#include "context.h"
#include "coroutine.h"
#include "log.h"

namespace hbco {

struct CoArg {
    CoFunc func_;
    void* arg_;
    CoArg(CoFunc func, void* arg)
      : func_(func)
      , arg_(arg) {}
};

Coroutine::Coroutine(const std::string& name)
  : name_(name) {
    sleep_.how_long_ = -1;
    memset(&sleep_.when_, sizeof(timeval), 0);
    io_count_ = 0;
    stack_ = (name == MAIN_CO_NAME) ? nullptr : (new char[STACK_SIZE]);
    context_ = new Context();
}

Coroutine::~Coroutine() {
    if (name_ != MAIN_CO_NAME) {
        delete[] stack_;
    }
    delete context_;
}

void
Coroutine::Sleep(long duration = -1) {
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
    CoroutineEnvironment* curr_env = CurrEnv();
    Coroutine* curr_co = curr_env->callstack_.back();
    curr_co->can_run_next_time_ = true;
    curr_env->callstack_.pop_back();
    Coroutine* prev_co = curr_env->callstack_.back();
    curr_env->callstack_.push_back(curr_co);
    SwapContext(curr_co->context_, prev_co->context_);
    curr_co->can_run_next_time_ = false;
}

void
Coroutine::Resume(Coroutine* next_co) {
    if (next_co->can_run_next_time_ == false) {
        return;
    }
    auto curr_env = CurrEnv();
    auto curr_co = curr_env->callstack_.back();
    curr_env->callstack_.push_back(next_co);
    SwapContext(curr_co->context_, next_co->context_);
    curr_env->callstack_.pop_back();
}

void
Coroutine::Container(void* arg) {
    Coroutine* curr_co = CurrCoroutine();
    curr_co->done_ = false;
    // Display(curr_co->name_);
    CoArg* co_arg = reinterpret_cast<CoArg*>(arg);
    co_arg->func_(co_arg->arg_);
    Display(curr_co->name_);
    curr_co->done_ = true;

    CurrEnv()->callstack_.pop_back();
    Coroutine* prev_co = CurrEnv()->callstack_.back();
    CurrEnv()->callstack_.push_back(curr_co);
    SwapContext(curr_co->context_, prev_co->context_);
}

Coroutine*
Coroutine::Create(const std::string& name, CoFunc func, void* arg) {
    Coroutine* co = new Coroutine(name);
    co->can_run_next_time_ = true;
    CoArg* co_arg = new CoArg(func, arg);
    // make context
    co->context_->stack_ptr_ = co->stack_;
    co->context_->stack_size_ = STACK_SIZE;
    void* stack_top = co->context_->stack_ptr_ + co->context_->stack_size_ - sizeof(void*);
    stack_top = reinterpret_cast<void*>(reinterpret_cast<unsigned long>(stack_top) & ~(1 << 4));
    memset(co->context_->register_, 0, sizeof(co->context_->register_));
    void** ret_addr = reinterpret_cast<void**>(stack_top);
    *ret_addr = reinterpret_cast<void*>(Container);
    co->context_->register_[kRSP] = stack_top;
    co->context_->register_[kRETAddr] = reinterpret_cast<void*>(Container);
    co->context_->register_[kRDI] = reinterpret_cast<void*>(co_arg);
    co->context_->register_[kRSI] = reinterpret_cast<void*>(0);

    getcontext(&co->uctx_);
    co->uctx_.uc_stack.ss_sp = co->stack_;
    co->uctx_.uc_stack.ss_size = STACK_SIZE;
    co->uctx_.uc_stack.ss_flags = 0;
    co->uctx_.uc_link = &(CurrEnv()->callstack_.back()->uctx_);
    makecontext(&co->uctx_, (void (*)(void))Container, 1, co_arg);
    CurrEnv()->coroutines_.insert({ co, true });
    return co;
}

Coroutine*
CurrCoroutine(void) {
    return CurrEnv()->callstack_.back();
}

int
CurrListeningFd(void) {
    return CurrEnv()->GetListeningFd();
}

} // namespace hbco