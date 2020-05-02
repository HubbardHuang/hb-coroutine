#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <iostream>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

#define Display(x)                                                                                 \
    do {                                                                                           \
        std::cout << "[" << __FILE__ << ":" << __LINE__ << "]" << #x << ": " << (x) << std::endl;  \
    } while (false)

struct ThreadData {
    int client_fd_;
    ThreadData()
      : client_fd_(-1) {}
    ThreadData(int fd)
      : client_fd_(fd) {}
};

int gListenFd;
int epfd;
long long gCount;
long long gMax;
struct timeval gTime;
ThreadData gData[_thread_amount];
std::mutex gCountMutex;
std::mutex gAcceptMutex;
std::thread t[_thread_amount];

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

void*
Worker(void* arg) {
    ThreadData* data = reinterpret_cast<ThreadData*>(arg);
    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);
    char hbuf[INET_ADDRSTRLEN] = { 0 };
    {
        std::lock_guard<std::mutex> l(gAcceptMutex);
        data->client_fd_ = accept(gListenFd, (struct sockaddr*)&sa, &len);
    }
    int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    Display(conn_fd);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;                     //使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //具体的IP地址
    serv_addr.sin_port = htons(9000);                   //端口
    if (connect(conn_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
    }
    while (true) {
        char buffer[100] = { 0 };
        if (read(data->client_fd_, buffer, 100) > 0) {
            std::lock_guard<std::mutex> l(gCountMutex);
            ++gCount;
        } else {
            break;
        }
        // std::string s(buffer);
        // for (int i = 0; i < 1000; i++) {
        //     s.push_back('a');
        // }

        // if (write(data->client_fd_, s.data(), s.size()) > 0) {
        if (write(data->client_fd_, "I'm huanghaobo\n", 14) > 0) {
            std::lock_guard<std::mutex> l(gCountMutex);
            ++gCount;
        }
        if (send(conn_fd, "I'm huanghaobo\n", 14, 0) > 0) {
            std::lock_guard<std::mutex> l(gCountMutex);
            ++gCount;
        }
        memset(buffer, 0, 100);
        if (recv(conn_fd, buffer, sizeof(buffer), 0) > 0) {
            std::lock_guard<std::mutex> l(gCountMutex);
            ++gCount;
        }
    }
}

int
main(int argc, char* argv[]) {
    Display(_thread_amount);
    Display(_port);
    gListenFd = create_and_bind(_port);
    listen(gListenFd, SOMAXCONN);
    epfd = epoll_create1(0);
    epoll_event ev;
    ev.data.fd = gListenFd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, gListenFd, &ev);
    gCount = 0;
    gMax = 0;
    gettimeofday(&gTime, nullptr);

    std::list<std::pair<pthread_t, ThreadData*>> pool;
    for (int i = 0; i < _thread_amount; i++) {
        gData[i].client_fd_ = -1;
        t[i] = std::thread(Worker, &gData[i]);
    }

    while (true) {
        // // use epoll-single-thread
        // epoll_event ev_buf[1000];
        // int nfds = epoll_wait(epfd, ev_buf, 1000, 1);
        // for (int i = 0; i < nfds; i++) {
        //     if (ev_buf[i].events & EPOLLOUT) {
        //         write(ev_buf[i].data.fd, "echo", 4);
        //         ev.data.fd = ev_buf[i].data.fd;
        //         ev.events = EPOLLIN;
        //         epoll_ctl(epfd, EPOLL_CTL_MOD, ev_buf[i].data.fd, &ev);
        //         ++gCount;
        //     } else if (ev_buf[i].events & EPOLLIN && ev_buf[i].data.fd == listen_fd) {
        //         struct sockaddr_in sa;
        //         socklen_t len = sizeof(sa);
        //         char hbuf[INET_ADDRSTRLEN] = { 0 };
        //         int client_fd = accept(listen_fd, (struct sockaddr*)&sa, &len);
        //         Display(client_fd);
        //         ev.data.fd = client_fd;
        //         ev.events = EPOLLIN | EPOLLOUT;
        //         epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
        //     } else if (ev_buf[i].events & EPOLLIN && ev_buf[i].data.fd != listen_fd) {
        //         char buf[100] = { 0 };
        //         read(ev_buf[i].data.fd, buf, 100);
        //         ev.data.fd = ev_buf[i].data.fd;
        //         ev.events = EPOLLOUT;
        //         epoll_ctl(epfd, EPOLL_CTL_MOD, ev_buf[i].data.fd, &ev);
        //         gCount++;
        //     }
        // }
        struct timeval curr_time;
        gettimeofday(&curr_time, nullptr);
        static long long total_count;
        if (curr_time.tv_sec >= gTime.tv_sec + 1) {
            total_count += gCount;
            gMax = gCount > gMax ? gCount : gMax;
            printf("%d 个线程的每秒 IO 读写数: 当前 %lld, 最大值 %lld\n", _thread_amount, gCount,
                   gMax);
            gTime.tv_sec = curr_time.tv_sec;
            gTime.tv_usec = curr_time.tv_usec;
            std::lock_guard<std::mutex> l(gCountMutex);
            gCount = 0;
        }
    }
    return 0;
}