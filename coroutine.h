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
#include <string>
#include <ucontext.h>

namespace hbco {

typedef void (*CoFunc)(void*);

struct Context {
    ucontext ctx_;
};

#define STACK_SIZE 1024 * 128

class Coroutine {
    friend CoroutineEnvironment::CoroutineEnvironment();
    friend const std::shared_ptr<CoroutineEnvironment>& CurrEnv(void);

private:
    bool can_run_next_time_;
    Context context_;
    char stack_[STACK_SIZE];
    Coroutine(const std::string& name);

#ifdef DEBUG
public:
#else
    static std::shared_ptr<Coroutine> Create(const std::string& name, CoFunc func, void* arg);
#endif
    std::string name_;

public:
    static void Yield();
    static void Resume(const std::shared_ptr<Coroutine>&);
    static std::shared_ptr<Coroutine> Create(CoFunc func, void* arg);
};

extern const std::shared_ptr<Coroutine>& CurrCoroutine(void);
}

#endif