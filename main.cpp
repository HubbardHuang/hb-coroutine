#include "coroutine.h"
#include <stdio.h>

void
func2(void* arg) {
    printf("func2.\n");
}

void
func1(void* arg) {
    printf("yield-1.\n");
    hbco::Coroutine::Yield();
    printf("yield-2.\n");
    hbco::Coroutine::Yield();
    printf("yield-3.\n");
    hbco::Coroutine::Resume(hbco::Coroutine::Create(func2, nullptr));
    printf("func1 end.\n");
}

pthread_t pth;

void*
func3(void* arg) {
    auto co1 = hbco::Coroutine::Create(func1, nullptr);
    printf("another thread: yield-1.\n");
    hbco::Coroutine::Resume(co1);
    hbco::Coroutine::Yield();
    printf("another thread: yield-2.\n");
    hbco::Coroutine::Resume(co1);
    hbco::Coroutine::Yield();
    printf("another thread: yield-3.\n");
    hbco::Coroutine::Resume(co1);
    printf("another thread: func3 end.\n");
}

int
main(int argc, char** argv) {
    // 创建协程
    pthread_create(&pth, nullptr, func3, nullptr);
    auto co1 = hbco::Coroutine::Create(func1, nullptr);
    printf("1111.\n");
    hbco::Coroutine::Resume(co1);
    printf("2222.\n");
    hbco::Coroutine::Resume(co1);
    printf("3333.\n");
    hbco::Coroutine::Resume(co1);
    printf("4444.\n");
    pthread_join(pth, nullptr);
    return 0;
}