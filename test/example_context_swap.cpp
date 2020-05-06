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

int testing_frequency = 5;
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
#ifdef TO_TEXT_FILE
    FILE* result_file = fopen(_file_path, "r+");
#endif
    sub_co1 = hbco::Coroutine::Create("sub_co1", SubCoroutine1, nullptr);
    sub_co2 = hbco::Coroutine::Create("sub_co2", SubCoroutine2, nullptr);
    main_co = hbco::CurrCoroutine();

#ifdef TO_TEXT_FILE
    fprintf(
      result_file,
      "`clock_gettime` call time/ns |\tasm switching time/ns |\tucontext switching time / ns\n");
    fprintf(result_file,
            "                             |\tmain->sub  sub->main  |\t  main->sub  sub->main\n");
    fprintf(result_file,
            "-----------------------------+------------------------+---------------------------\n");
#endif
    printf(COLOR(BLUE, "演示：对比C标准库ucontext接口和asm接口进行上下文切换的效率") "\n");
    printf(COLOR(BLUE, "注：上下文切换实际耗时 = 测得耗时 - `clock_gettime`调用耗时") "\n");
    printf(b_GREEN_f_BLACK("`clock_gettime`调用耗时/ns  asm测得耗时/ns  ucontext测得耗时/ns  "
                           "asm实际耗时/ns  ucontext实际耗时/ns") "\n");
    for (int i = 0; i < testing_frequency + 1; i++) {
        std::vector<long long> curr_data(5);
        // test `clock_gettime()`
        start_time = { 0, 0 };
        end_time = { 0, 0 };
        clock_gettime(CLOCK_REALTIME, &start_time);
        clock_gettime(CLOCK_REALTIME, &end_time);
        ns_call = end_time.tv_nsec - start_time.tv_nsec;
        if (i == 0) {
            continue;
        }
        curr_data[0] = ns_call;

        // test asm swapping
        start_time = { 0, 0 };
        end_time = { 0, 0 };
        SwapContext(main_co->context_, sub_co1->context_);
        clock_gettime(CLOCK_REALTIME, &end_time);
        ns_s2m = end_time.tv_nsec - start_time.tv_nsec;
        curr_data[1] = ns_s2m;

        start_time = { 0, 0 };
        end_time = { 0, 0 };
        clock_gettime(CLOCK_REALTIME, &start_time);
        SwapContext(main_co->context_, sub_co1->context_);
        ns_m2s = end_time.tv_nsec - start_time.tv_nsec;
#ifdef TO_TEXT_FILE
        fprintf(result_file, "\t\t\t%ld\t\t\t\t |\t\t%ld\t\t\t%ld\t  |\t", ns_call, ns_s2m, ns_m2s);
#endif

        // test ucontext swapping
        start_time = { 0, 0 };
        end_time = { 0, 0 };
        swapcontext(&main_co->uctx_, &sub_co2->uctx_);
        clock_gettime(CLOCK_REALTIME, &end_time);
        ns_s2m = end_time.tv_nsec - start_time.tv_nsec;
        curr_data[2] = ns_s2m;
        start_time = { 0, 0 };
        end_time = { 0, 0 };
        clock_gettime(CLOCK_REALTIME, &start_time);
        swapcontext(&main_co->uctx_, &sub_co2->uctx_);
        ns_m2s = end_time.tv_nsec - start_time.tv_nsec;
#ifdef TO_TEXT_FILE
        fprintf(result_file, "\t%ld\t\t\t%ld\n", ns_s2m, ns_m2s);
#endif

        curr_data[3] = curr_data[1] - curr_data[0];
        curr_data[4] = curr_data[2] - curr_data[0];
        TO_Nth_COL(14);
        printf("%lld", curr_data[0]);
        TO_Nth_COL(35);
        printf("%lld", curr_data[1]);
        TO_Nth_COL(53);
        printf("%lld", curr_data[2]);
        TO_Nth_COL(71);
        printf("%lld", curr_data[3]);
        TO_Nth_COL(88);
        printf("%lld", curr_data[4]);
        printf("\n");
        fflush(stdout);
    }

    return 0;
}