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

#include "/home/hhb/practice/hb-coroutine/source/log.h"

epoll_event ev;
#define Display(x)                                                                                 \
    do {                                                                                           \
        std::cout << "[" __FILE__ << ":" << __LINE__ << "]" << #x << ": " << (x) << std::endl;     \
    } while (false)

struct EpollData;
using TaskFunc = void(int, int, EpollData*);

struct EpollData {
    int fd_;
    TaskFunc* task_;
    EpollData(int fd, TaskFunc* task)
      : fd_(fd)
      , task_(task) {}
};

static int
SetNonBlock(int iSock) {
    int iFlags;
    iFlags = fcntl(iSock, F_GETFL, 0);
    iFlags |= O_NONBLOCK;
    iFlags |= O_NDELAY;
    int ret = fcntl(iSock, F_SETFL, iFlags);
    return ret;
}

void Write1(int myfd, int epfd, EpollData* ed);
void Read1(int myfd, int epfd, EpollData* ed);
void Connect1(int myfd, int epfd, EpollData* ed);
void Send1(int myfd, int epfd, EpollData* ed);
void Recv1(int myfd, int epfd, EpollData* ed);

TaskFunc* gClientTask[100] = { Write1 };

void
MainTask(int fd, int epfd, EpollData* arg) {}

void
Write1(int myfd, int epfd, EpollData* ed) {
    // 1st. 执行动作
    write(myfd, "Reactor model.\n", 17);
    printf("1. 刚完成向客户端发送消息的写操作，等待客户端的数据到来再进行读操作...\n");
    // 2nd. 设置好下次等待事件
    ed->fd_ = myfd;
    // 2nd. 以及事件发生时触发哪个函数
    ed->task_ = Read1;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_MOD, myfd, &ev);
}

void
Read1(int myfd, int epfd, EpollData* ed) {
    char buffer[100] = { 0 };
    read(myfd, buffer, 100);
    printf(
      "2. 已经完成获取客户端消息的读操作，等待客户端的数据到来再进行向上级服务端的连接操作...\n");
    int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    SetNonBlock(conn_fd);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(9000);
    ed->fd_ = myfd;
    ed->task_ = Connect1;
    ev.data.ptr = ed;
    ev.events = EPOLLOUT | EPOLLET;
    if (connect(myfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        // perror("connect");
    }
    epoll_ctl(epfd, EPOLL_CTL_MOD, myfd, &ev);
}

void
Connect1(int myfd, int epfd, EpollData* ed) {
    printf(
      "3. "
      "已经完成连接上级服务端的操作，等待内核缓冲区准备好再进行向上级服务端发送消息的写操作...\n");
    sleep(2);
    int err = 0;
    socklen_t errlen = sizeof(err);
    int ret = getsockopt(myfd, SOL_SOCKET, SO_ERROR, &err, &errlen);

    ev.events = EPOLLOUT | EPOLLET;
    ed->task_ = Send1;
    ed->fd_ = myfd;
    ev.data.ptr = ed;
    epoll_ctl(epfd, EPOLL_CTL_MOD, myfd, &ev);
}

void
Send1(int myfd, int epfd, EpollData* ed) {
    printf(
      "4. 已经完成向上级服务端发送消息的写操作，等服务端消息到达后再进行获取上级服务端消息消息的读"
      "操作...\n");
    write(myfd, "Reactor model.\n", 17);
    ed->fd_ = myfd;
    ed->task_ = Recv1;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_MOD, myfd, &ev);
}

void
Recv1(int myfd, int epfd, EpollData* ed) {
    char buffer[100] = { 0 };
    recv(myfd, buffer, 100, 0);
    printf("5. 已经完成获取服务端消息的读操作\n");
    Display(buffer);
    printf("5个事件已全部完成，退出Reactor模型\n");
    epoll_ctl(epfd, EPOLL_CTL_DEL, myfd, nullptr);
    close(myfd);
    exit(0);
}

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

int
main(int argc, char* argv[]) {
    int epfd = epoll_create1(0);
    int listen_fd = create_and_bind(_port);
    SetNonBlock(listen_fd);
    listen(listen_fd, 128);

    EpollData main_ed(listen_fd, MainTask);
    printf("注册回调函数，等待客户端连接...\n");
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = &main_ed;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);
    while (true) {
        // use epoll-single-thread
        epoll_event ev_buf[1000];
        int nfds = epoll_wait(epfd, ev_buf, 1000, -1);
        for (int i = 0; i < nfds; i++) {
            if ((ev_buf[i].events & EPOLLIN) &&
                reinterpret_cast<EpollData*>(ev_buf[i].data.ptr)->fd_ == listen_fd) {
                static int count;
                if (count >= 1) {
                    continue;
                }
                struct sockaddr_in sa;
                socklen_t len = sizeof(sa);
                char hbuf[INET_ADDRSTRLEN] = { 0 };
                int client_fd = accept(listen_fd, (struct sockaddr*)&sa, &len);
                SetNonBlock(client_fd);
                ev.data.ptr = new EpollData(client_fd, gClientTask[0]);
                ev.events = EPOLLOUT | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
                count++;
            } else if ((ev_buf[i].events & EPOLLIN) || (ev_buf[i].events & EPOLLOUT)) {
                EpollData* curr_ed = reinterpret_cast<EpollData*>(ev_buf[i].data.ptr);
                int fd = curr_ed->fd_;
                TaskFunc* task = curr_ed->task_; // 取出事先注册的事件
                task(fd, epfd, curr_ed);         // 执行事件
            }
        }
    }
    return 0;
}