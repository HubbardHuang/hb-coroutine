#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include "condition_variable.h"
#include "context.h"
#include "coroutine.h"
#include "coroutine_launcher.h"
#include "event_loop.h"
#include "log.h"

#define ASM

int testing_frequency = 50;
hbco::Coroutine *main_co, *sub_co1, *sub_co2;
struct timespec start_time;
struct timespec end_time;
long ns_call, ns_m2s, ns_s2m;

static void
SubCoroutine1(void* arg) {
    while (true) {
        clock_gettime(CLOCK_REALTIME, &start_time);
        SwapContext(sub_co1->context_, main_co->context_);

        clock_gettime(CLOCK_REALTIME, &end_time);
        SwapContext(sub_co1->context_, main_co->context_);
    }
}

static void
SubCoroutine2(void* arg) {
    while (true) {
        clock_gettime(CLOCK_REALTIME, &start_time);
        swapcontext(&sub_co2->uctx_, &main_co->uctx_);
        clock_gettime(CLOCK_REALTIME, &end_time);
        swapcontext(&sub_co2->uctx_, &main_co->uctx_);
    }
}

int
main(int argc, char* argv[]) {
    hbco::CoroutineLauncher cl();
    FILE* result_file = fopen(_file_path, "r+");
    sub_co1 = hbco::Coroutine::Create("sub_co1", SubCoroutine1, nullptr);
    sub_co2 = hbco::Coroutine::Create("sub_co2", SubCoroutine2, nullptr);
    main_co = hbco::CurrCoroutine();

    fprintf(
      result_file,
      "`clock_gettime` call time/ns |\tasm switching time/ns |\tucontext switching time/ns\n");
    fprintf(result_file,
            "                             |\tmain->sub  sub->main  |\t  main->sub  sub->main\n");
    fprintf(result_file,
            "-----------------------------+------------------------+---------------------------\n");
    for (int i = 0; i < testing_frequency; i++) {
        // test `clock_gettime()`
        start_time = { 0, 0 };
        end_time = { 0, 0 };
        clock_gettime(CLOCK_REALTIME, &start_time);
        clock_gettime(CLOCK_REALTIME, &end_time);
        ns_call = end_time.tv_nsec - start_time.tv_nsec;

        // test asm swapping
        start_time = { 0, 0 };
        end_time = { 0, 0 };
        SwapContext(main_co->context_, sub_co1->context_);
        clock_gettime(CLOCK_REALTIME, &end_time);
        ns_s2m = end_time.tv_nsec - start_time.tv_nsec;

        start_time = { 0, 0 };
        end_time = { 0, 0 };
        clock_gettime(CLOCK_REALTIME, &start_time);
        SwapContext(main_co->context_, sub_co1->context_);
        ns_m2s = end_time.tv_nsec - start_time.tv_nsec;
        fprintf(result_file, "\t\t\t%ld\t\t\t\t |\t\t%ld\t\t\t%ld\t  |\t", ns_call, ns_s2m, ns_m2s);

        // test ucontext swapping
        start_time = { 0, 0 };
        end_time = { 0, 0 };
        swapcontext(&main_co->uctx_, &sub_co2->uctx_);
        clock_gettime(CLOCK_REALTIME, &end_time);
        ns_s2m = end_time.tv_nsec - start_time.tv_nsec;
        start_time = { 0, 0 };
        end_time = { 0, 0 };
        clock_gettime(CLOCK_REALTIME, &start_time);
        swapcontext(&main_co->uctx_, &sub_co2->uctx_);
        ns_m2s = end_time.tv_nsec - start_time.tv_nsec;
        fprintf(result_file, "\t%ld\t\t\t%ld\n", ns_s2m, ns_m2s);
    }

    return 0;
}