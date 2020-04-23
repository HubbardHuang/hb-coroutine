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

hbco::Coroutine *main_co, *sub_co;

struct timespec start_time;
struct timespec end_time;

static void
SubCoroutine(void* arg) {
    clock_gettime(CLOCK_REALTIME, &start_time);
    SwapContext(sub_co->context_, main_co->context_);
    // swapcontext(&sub_co->uctx_, &main_co->uctx_);
}

int
main(int argc, char* argv[]) {
    hbco::CoroutineLauncher cl();

    sub_co = hbco::Coroutine::Create("sub_co", SubCoroutine, nullptr);
    main_co = hbco::CurrCoroutine();

    for (int i = 0; i < 50; i++) {
        start_time = { 0, 0 };
        end_time = { 0, 0 };
        clock_gettime(CLOCK_REALTIME, &start_time);
        clock_gettime(CLOCK_REALTIME, &end_time);
        // DisplayWithPrefix("Test time of `clock_gettime`", start_time.tv_sec);
        // DisplayWithPrefix("Test time of `clock_gettime`", start_time.tv_nsec);
        // DisplayWithPrefix("Test time of `clock_gettime`", end_time.tv_sec);
        // DisplayWithPrefix("Test time of `clock_gettime`", end_time.tv_nsec);
        std::cout << end_time.tv_nsec - start_time.tv_nsec << ' ';
        // DisplayWithPrefix("Test time of `clock_gettime`", end_time.tv_nsec - start_time.tv_nsec);
    }
    std::cout << std::endl;

    long t1 = end_time.tv_nsec - start_time.tv_nsec;
    start_time = { 0, 0 };
    end_time = { 0, 0 };

    SwapContext(main_co->context_, sub_co->context_);
    // swapcontext(&main_co->uctx_, &sub_co->uctx_);
    clock_gettime(CLOCK_REALTIME, &end_time);
    Display(start_time.tv_sec);
    Display(start_time.tv_nsec);
    Display(end_time.tv_sec);
    Display(end_time.tv_nsec);
    Display(end_time.tv_nsec - start_time.tv_nsec);
    long t2 = end_time.tv_nsec - start_time.tv_nsec;
    DisplayWithPrefix("Swapping Time", t2 - t1);

    return 0;
}