/**
 * @file coroutine.h
 * @author HubbardHuang (hubbard_huang@sina.com)
 * @brief
 * @copyright Copyright (c) 2020
 */
#ifndef COROUTINE_H
#define COROUTINE_H

#include "environment.h"
#include <string>
#include <ucontext.h>

namespace hbco {

typedef void (*CoFunc)(void*);

struct Context {
    ucontext ctx_;
};

#define STACK_SIZE 1024 * 128

class Coroutine {
    friend void ReleaseResources(void);
    friend CoroutineEnvironment::CoroutineEnvironment();
    friend CoroutineEnvironment* CurrEnv(void);

private:
    bool can_run_next_time_;
    Context context_;
    char stack_[STACK_SIZE];
    Coroutine(const std::string& name);
    ~Coroutine() = default;

#ifdef DEBUG
public:
#else
private:
#endif
    std::string name_;

public:
    static void Yield();
    static void Resume(Coroutine* next_co);
    static Coroutine* Create(const std::string& name, CoFunc func, void* arg);
};

extern Coroutine* CurrCoroutine(void);
}

#endif