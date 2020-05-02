#include <string.h>
#include <sys/epoll.h>

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
EpollEventLoop(void) {
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
            // if (co->done_) {
            // curr_env->coroutines_.erase(co);
            // Display(co->done_);
            // delete co;
            // }
        }
        struct timeval ctv;
        static long long total_count;
        gettimeofday(&ctv, nullptr);
        if (ctv.tv_sec >= gTime.tv_sec + 1) {
            gTime.tv_sec = ctv.tv_sec;
            gTime.tv_usec = ctv.tv_usec;
            gMaxCount = gCount > gMaxCount ? gCount : gMaxCount;
            total_count += gCount;
            printf("%d 个协程的每秒 IO 读写数: 当前 %lld, 最大值 %lld\n", _coroutine_amount, gCount,
                   gMaxCount);
            // for (auto i : curr_env->coroutines_) {
            //     auto co = i.first;
            //     std::cout << co->name_ << ": " << co->io_count_ << std::endl;
            //     co->io_count_ = 0;
            // }
            // pthread_mutex_lock(&gCountMutex);
            gCount = 0;
            // pthread_mutex_unlock(&gCountMutex);
        }
    }
}

} // namespace hbco