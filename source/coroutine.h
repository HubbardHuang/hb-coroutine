/**
 * @file coroutine.h
 * @author HubbardHuang (hubbard_huang@sina.com)
 * @brief
 * @copyright Copyright (c) 2020
 */
#ifndef COROUTINE_H
#define COROUTINE_H

#include "environment.h"
#include "time_record.h"
#include <queue>
#include <string>
#include <ucontext.h>

namespace hbco {

typedef void (*CoFunc)(void*);

#define STACK_SIZE 1024 * 128
#define MAIN_CO_NAME "main_co"

struct Context;
class Coroutine {
    friend class CondVar;
    friend bool Poll(int fd, uint32_t epoll_events, long wait_time);
    friend void EpollEventLoop(void);
    friend void ReleaseResources(void);
    friend CoroutineEnvironment::CoroutineEnvironment();
    friend CoroutineEnvironment* CurrEnv(void);

private:
    CoroutineEnvironment* env_;
    SleepRecord sleep_;
    bool can_run_next_time_;
    bool done_;
    char* stack_;
    Coroutine(const std::string& name);
    ~Coroutine();

private:
    static void Container(void* arg);

#ifdef DEBUG
public:
#else
private:
#endif
    std::string name_;
    Context* context_;
    ucontext uctx_;
    long long io_count_;

public:
    // static bool Poll(int fd, uint32_t epoll_events, long wait_time = 1000);
    static void Sleep(long duration);
    static void Yield();
    static void Resume(Coroutine* next_co);
    static Coroutine* Create(const std::string& name, CoFunc func, void* arg);
};

extern Coroutine* CurrCoroutine(void);
extern int CurrListeningFd(void);

} // namespace hbco

#endif