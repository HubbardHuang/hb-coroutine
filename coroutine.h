/**
 * @file coroutine.h
 * @author HubbardHuang (hubbard_huang@sina.com)
 * @brief
 * @copyright Copyright (c) 2020
 */
#ifndef COROUTINE_H
#define COROUTINE_H

#include "environment.h"
#include <memory>
#include <ucontext.h>

namespace hbco {

typedef void (*CoFunc)(void*);

struct Context {
    ucontext ctx_;
};

#define STACK_SIZE 1024

class Coroutine {
private:
    std::shared_ptr<CoroutineEnvironment> env_;
    Context context_;
    char stack_[STACK_SIZE];

public:
    Coroutine();
    static void Yield();
    static void Resume(const std::shared_ptr<Coroutine>&);
    static std::shared_ptr<Coroutine> Create(CoFunc func, void* arg);
};
}

#endif