#include <arpa/inet.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "coroutine.h"
#include "environment.h"
#include "log.h"
#include "port.h"

namespace hbco {

static int
create_and_bind(int port) {
    int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sfd == -1) {
        return -1;
    }
    struct sockaddr_in sa;
    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sfd, (struct sockaddr*)&sa, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        return -1;
    }
    return sfd;
}

CoroutineEnvironment::CoroutineEnvironment() {
    Coroutine* main_co(new Coroutine(MAIN_CO_NAME));
    callstack_.push_back(main_co);
    coroutines_.insert({ main_co, true });
    epoll_fd_ = epoll_create1(0);
    port_ = CurrListeningPort();
    listen_fd_ = port_ > 0 ? create_and_bind(port_) : -1;
    if (listen_fd_ > 0) {
        listen(listen_fd_, SOMAXCONN);
    }
}

CoroutineEnvironment::~CoroutineEnvironment() {
    close(epoll_fd_);
    close(listen_fd_);
}

int
CoroutineEnvironment::GetEpollFd(void) {
    return epoll_fd_;
}

int
CoroutineEnvironment::GetListeningFd(void) {
    return listen_fd_;
}

static std::map<pthread_t, CoroutineEnvironment*> env_manager;

CoroutineEnvironment*
CurrEnv(void) {
    auto curr_tid = pthread_self();
    auto result = env_manager.find(curr_tid);
    if (result == env_manager.end()) {
        CoroutineEnvironment* curr_env = new CoroutineEnvironment();
        env_manager.insert({ curr_tid, curr_env });
    }
    return env_manager[curr_tid];
}

void
ReleaseResources(void) {
    auto curr_tid = pthread_self();
    auto result = env_manager.find(curr_tid);
    if (result != env_manager.end()) {
        auto curr_env = result->second;
        Display(curr_env->callstack_.size());
        while (!curr_env->callstack_.empty()) {
            Display(curr_env->callstack_.back()->name_);
            auto* co = curr_env->callstack_.back();
            curr_env->callstack_.pop_back();
        }
        for (auto it = curr_env->coroutines_.begin(); it != curr_env->coroutines_.end();) {
            auto prev = it->first;
            it = curr_env->coroutines_.erase(it);
            Display(prev->name_);
            delete prev;
        }
        env_manager.erase(curr_tid);
        delete curr_env;
    }
}

} // namespace hbco