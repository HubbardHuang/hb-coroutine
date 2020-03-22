#include "coroutine.h"
#include <stdio.h>
#include <unistd.h>

void
tmp(void* arg) {
    printf("func2.\n");
}

void
co_routine(void* arg) {
    printf("yield-1.\n");
    hbco::Coroutine::Yield();
    printf("yield-2.\n");
    hbco::Coroutine::Yield();
    printf("yield-3.\n");
    hbco::Coroutine::Resume(hbco::Coroutine::Create(tmp, nullptr));
    printf("func1 end.\n");
}

void*
task(void* arg) {
    auto co1 = hbco::Coroutine::Create(co_routine, nullptr);
    printf("%s1\n", (char*)arg);
    hbco::Coroutine::Resume(co1);
    sleep(1);
    printf("%s2\n", (char*)arg);
    hbco::Coroutine::Resume(co1);
    sleep(1);
    printf("%s3\n", (char*)arg);
    hbco::Coroutine::Resume(co1);
    sleep(1);
    printf("%s4\n", (char*)arg);
    hbco::Coroutine::Resume(co1);
    sleep(1);
}

void
threads_test(void) {
    pthread_t id[2];
    char str1[] = "thread 1: ";
    char str2[] = "thread 2: ";
    pthread_create(&id[0], nullptr, task, str1);
    pthread_create(&id[1], nullptr, task, str2);
    pthread_join(id[0], nullptr);
    pthread_join(id[1], nullptr);
}

int
main(int argc, char** argv) {
    threads_test();
    return 0;
}