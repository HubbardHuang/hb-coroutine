#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "coroutine.h"
#include "event_loop.h"
#include "global.h"
#include "log.h"
#include "time_record.h"

namespace hbco {

struct EpollData {
    Coroutine* co_;
    int fd_;
    bool time_out_;
    EpollData(int fd, Coroutine* co)
      : co_(co)
      , fd_(fd)
      , time_out_(true) {}
};

bool
Poll(int fd, uint32_t epoll_events, long wait_time) {
    Coroutine* curr_co = CurrCoroutine();
    CoroutineEnvironment* curr_env = CurrEnv();
    int epoll_fd = CurrEnv()->GetEpollFd();
    EpollData ed(fd, curr_co);
    struct epoll_event event;
    event.events = epoll_events;
    event.data.ptr = reinterpret_cast<void*>(&ed);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    // curr_env->epoll_items_.insert(curr_co);
    curr_env->epoll_items_.insert({ curr_co, &event });

    curr_co->sleep_.how_long_ = wait_time; // millisecond
    gettimeofday(&(curr_co->sleep_.when_), nullptr);
    curr_co->sleep_.wake_time_ = curr_co->sleep_.how_long_ + curr_co->sleep_.when_.tv_sec * 1000 +
                                 curr_co->sleep_.when_.tv_usec / 1000;
    Coroutine::Yield();

    curr_co->sleep_.how_long_ = -1;
    memset(&(curr_co->sleep_.when_), sizeof(timeval), 0);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    return ed.time_out_;
}

void
EpollEventLoop(int promt) {
    CoroutineEnvironment* curr_env = CurrEnv();
    int epoll_fd = curr_env->GetEpollFd();
    while (true) {
        epoll_event result_events[1000] = { 0 };
        int nfds =
          epoll_wait(epoll_fd, result_events, sizeof(result_events) / sizeof(result_events[0]), 1);
        for (int i = 0; i < nfds; i++) {
            EpollData* ed = reinterpret_cast<EpollData*>(result_events[i].data.ptr);
            ed->time_out_ = false;
            Coroutine* co = ed->co_;
            curr_env->runnable_.push_back(co);
            curr_env->epoll_items_.erase(co);
        }
        struct timeval curr_tv;
        gettimeofday(&curr_tv, nullptr);
        long curr_time = curr_tv.tv_sec * 1000 + curr_tv.tv_usec / 1000;
        for (auto i = curr_env->epoll_items_.begin(); i != curr_env->epoll_items_.end();) {
            // Coroutine* co = *i;
            Coroutine* co = i->first;
            // long ms_timestamp = co->sleep_.when_.tv_sec * 1000 + co->sleep_.when_.tv_usec / 1000;
            // if (co->sleep_.how_long_ < 0 || ms_timestamp + co->sleep_.how_long_ <= curr_time) {
            if (co->sleep_.wake_time_ < curr_time) {
                curr_env->runnable_.push_back(co);
                i = curr_env->epoll_items_.erase(i);
            } else {
                EpollData* ed = reinterpret_cast<EpollData*>(i->second->data.ptr);
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ed->fd_, i->second);
                i++;
            }
        }
        for (auto i = curr_env->pending_.begin(); i != curr_env->pending_.end();) {
            Coroutine* co = *i;
            // long ms_timestamp = co->sleep_.when_.tv_sec * 1000 + co->sleep_.when_.tv_usec / 1000;
            // if (co->sleep_.how_long_ < 0 || ms_timestamp + co->sleep_.how_long_ <= curr_time) {
            if (co->sleep_.wake_time_ < curr_time) {
                curr_env->runnable_.push_back(co);
                i = curr_env->pending_.erase(i);
            } else {
                i++;
            }
        }
        while (!curr_env->runnable_.empty()) {
            Coroutine* co = curr_env->runnable_.front();
            curr_env->runnable_.pop_front();
            Coroutine::Resume(co);
            // if (co->done_ && !co->can_add_to_pool_) {
            //     curr_env->coroutines_.erase(co);
            //     // Display(co->done_);
            //     delete co;
            // }
        }
        struct timeval ctv;
        static long long total_count;
        std::list<std::vector<int>> content;
        static int record_count;
        static long long run_time;
        gettimeofday(&ctv, nullptr);
        if (ctv.tv_sec >= gTime.tv_sec + 1) {
            gTime.tv_sec = ctv.tv_sec;
            gTime.tv_usec = ctv.tv_usec;
            gMaxCount = gCount > gMaxCount ? gCount : gMaxCount;
            total_count += gCount;
            ++run_time;
            // printf("协程数: %d\t网络IO单核并发数: %lld/s(当前) %lld/s(峰值)\n",
            // _coroutine_amount,
            //        gCount, gMaxCount);
            if (gCount > 0 && promt == 1) {
                fprintf(gDataTxt, "%lld\n", gCount);
                record_count++;
                if (record_count == 30) {
                    GO_DOWN_N_LINE(static_cast<long long>(2 + gContent.size()));
                    return;
                }
            }
            if (gContent.size() >= LINE_MAX) {
                gContent.pop_front();
            }
            std::vector<long long> curr_content(6);
            curr_content[0] = _coroutine_amount;
            curr_content[1] = gCount;
            curr_content[2] = gMaxCount;
            curr_content[3] = gClientConnectionCount;
            curr_content[4] = gServerConnectionCount;
            curr_content[5] = run_time;
            gContent.push_back(curr_content);
            if (promt == 1) {
                printf(COLOR(BLUE, "演示：对比单核情况下多线程与多协程的网络IO处理效率") "\n");
            } else if (promt == 2) {
                printf(COLOR(BLUE, "演示：多协程作为可复用的连接池") "\n");
            } else if (promt == 3) {
                gCount = 0;
                continue;
            }
            printf(b_GREEN_f_BLACK(
              "协程数  当前IO/s  峰值IO/s  客户端连接数  服务端连接数  已运行时间/s") "\n");
            int k = 0;
            for (auto& c : gContent) {
                ++k;
                if (k == gContent.size()) {
                    for (int i = 0; i < 68; i++) {
                        printf(b_BLUE_f_BLACK(" "));
                    }
                    // usleep(5000);
                    TO_Nth_COL(2);
                    printf(b_BLUE_f_BLACK("%lld"), c[0]);
                    TO_Nth_COL(10);
                    printf(b_BLUE_f_BLACK("%lld"), c[1]);
                    TO_Nth_COL(20);
                    printf(b_BLUE_f_BLACK("%lld"), c[2]);
                    TO_Nth_COL(33);
                    printf(b_BLUE_f_BLACK("%lld"), c[3]);
                    TO_Nth_COL(47);
                    printf(b_BLUE_f_BLACK("%lld"), c[4]);
                    TO_Nth_COL(60);
                    printf(b_BLUE_f_BLACK("%lld"), c[5]);
                } else {
                    for (int i = 0; i < 68; i++) {
                        printf(" ");
                    }
                    TO_Nth_COL(2);
                    printf(COLOR(WHITE, "%lld"), c[0]);
                    TO_Nth_COL(10);
                    printf(COLOR(WHITE, "%lld"), c[1]);
                    TO_Nth_COL(20);
                    printf(COLOR(WHITE, "%lld"), c[2]);
                    TO_Nth_COL(33);
                    printf(COLOR(WHITE, "%lld"), c[3]);
                    TO_Nth_COL(47);
                    printf(COLOR(WHITE, "%lld"), c[4]);
                    TO_Nth_COL(60);
                    printf(COLOR(WHITE, "%lld"), c[5]);
                }
                printf("\n");
            }
            fflush(stdout);
            GO_UP_N_LINE(static_cast<long long>(2 + gContent.size()));
            gCount = 0;
        }
    }
}

} // namespace hbco