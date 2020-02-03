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

int
main(int argc, char** argv) {
    // 创建协程
    auto co1 = hbco::Coroutine::Create(func1, nullptr);
    printf("1111.\n");
    hbco::Coroutine::Resume(co1);
    printf("2222.\n");
    hbco::Coroutine::Resume(co1);
    printf("3333.\n");
    hbco::Coroutine::Resume(co1);
    printf("4444.\n");
    return 0;
}