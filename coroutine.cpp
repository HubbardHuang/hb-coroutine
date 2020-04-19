#include <string.h>
#include <sys/epoll.h>

#include "coroutine.h"
#include "log.h"

namespace hbco {

Coroutine::Coroutine(const std::string& name)
  : name_(name) {
    sleep_.how_long_ = -1;
    memset(&sleep_.when_, sizeof(timeval), 0);
    stack_ = (name == MAIN_CO_NAME) ? nullptr : (new char[STACK_SIZE]);
}

Coroutine::~Coroutine() {
    if (name_ != MAIN_CO_NAME) {
        delete[] stack_;
    }
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

void
Coroutine::Container(CoFunc func, void* arg) {
    Coroutine* curr_co = CurrCoroutine();
    curr_co->done_ = false;
    std::cout << "hi~";
    Display(curr_co->name_);
    func(arg);
    std::cout << "bye~";
    Display(curr_co->name_);
    curr_co->done_ = true;
}

Coroutine*
Coroutine::Create(const std::string& name, CoFunc func, void* arg) {
    Coroutine* co = new Coroutine(name);
    co->can_run_next_time_ = true;
    getcontext(&co->context_.ctx_);
    co->context_.ctx_.uc_stack.ss_sp = co->stack_;
    co->context_.ctx_.uc_stack.ss_size = STACK_SIZE;
    co->context_.ctx_.uc_stack.ss_flags = 0;
    co->context_.ctx_.uc_link = &(CurrEnv()->callstack_.back()->context_.ctx_);
    makecontext(&co->context_.ctx_, (void (*)(void))Container, 2, func, arg);
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