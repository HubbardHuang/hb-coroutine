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

#include "/home/hhb/practice/hb-coroutine/source/log.h"

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
long long gMaxCount;
struct timeval gTime;
ThreadData gData[_thread_amount];
std::mutex gCountMutex;
std::mutex gAcceptMutex;
std::mutex gConnectMutex;
std::thread t[_thread_amount];

long long gClientConnectionCount;
long long gServerConnectionCount;
std::list<std::vector<long long>> gContent;
#define LINE_MAX 5

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
        gClientConnectionCount++;
    }
    // int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    // struct sockaddr_in serv_addr;
    // memset(&serv_addr, 0, sizeof(serv_addr));
    // serv_addr.sin_family = AF_INET;                        //使用IPv4地址
    // serv_addr.sin_addr.s_addr = inet_addr("192.168.56.1"); //具体的IP地址
    // serv_addr.sin_port = htons(9000);                      //端口
    // if (connect(conn_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    //     perror("connect");
    // } else {
    //     std::lock_guard<std::mutex> l(gConnectMutex);
    //     gServerConnectionCount++;
    // }
    while (true) {
        char buffer[100] = { 0 };
        int read_count = read(data->client_fd_, buffer, 100);
        if (read_count > 0) {
            std::lock_guard<std::mutex> l(gCountMutex);
            ++gCount;
        } else if (read_count == 0) {
            close(data->client_fd_);
            break;
        }
        std::string s(buffer);
        for (int i = 0; i < 1000; i++) {
            s.push_back('a');
        }

        if (write(data->client_fd_, s.data(), s.size()) > 0) {
            // if (write(data->client_fd_, "I'm huanghaobo\n", 14) > 0) {
            std::lock_guard<std::mutex> l(gCountMutex);
            ++gCount;
        }
        // if (send(conn_fd, s.data(), s.size(), 0) > 0) {
        //     std::lock_guard<std::mutex> l(gCountMutex);
        //     ++gCount;
        // }
        // memset(buffer, 0, 100);
        // int recv_count = recv(conn_fd, buffer, sizeof(buffer), 0);
        // if (recv_count > 0) {
        //     std::lock_guard<std::mutex> l(gCountMutex);
        //     ++gCount;
        // } else {
        //     close(conn_fd);
        //     break;
        // }
    }
}

int
main(int argc, char* argv[]) {
    FILE* data_file;
    {
        std::lock_guard<std::mutex> l(gCountMutex);
        data_file = fopen("/home/hhb/practice/hb-coroutine/thread_io_data.txt", "w");
        fprintf(data_file, "多线程网络IO每秒读写次数\n");
    }
    // return 0;
    gListenFd = create_and_bind(_port);
    listen(gListenFd, SOMAXCONN);
    epfd = epoll_create1(0);
    epoll_event ev;
    ev.data.fd = gListenFd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, gListenFd, &ev);
    gCount = 0;
    gMaxCount = 0;
    gettimeofday(&gTime, nullptr);

    std::list<std::pair<pthread_t, ThreadData*>> pool;
    for (int i = 0; i < _thread_amount; i++) {
        gData[i].client_fd_ = -1;
        t[i] = std::thread(Worker, &gData[i]);
    }

    while (true) {
        struct timeval curr_time;
        gettimeofday(&curr_time, nullptr);
        static int record_count;
        static long long run_time;
        if (curr_time.tv_sec >= gTime.tv_sec + 1) {
            gTime.tv_sec = curr_time.tv_sec;
            gTime.tv_usec = curr_time.tv_usec;
            std::lock_guard<std::mutex> l(gCountMutex);
            run_time++;
            gMaxCount = gCount > gMaxCount ? gCount : gMaxCount;
            // printf("%d 个线程的每秒 IO 读写数: 当前 %lld, 最大值 %lld\n", _thread_amount, gCount,
            //        gMaxCount);
            if (gCount > 0) {
                fprintf(data_file, "%lld\n", gCount);
                record_count++;
                if (record_count == 30) {
                    GO_DOWN_N_LINE(static_cast<long long>(2 + gContent.size()));
                    exit(1);
                }
            }
            if (gContent.size() >= LINE_MAX) {
                gContent.pop_front();
            }
            std::vector<long long> curr_content(6);
            curr_content[0] = _thread_amount;
            curr_content[1] = gCount;
            curr_content[2] = gMaxCount;
            curr_content[3] = gClientConnectionCount;
            curr_content[4] = gServerConnectionCount;
            curr_content[5] = run_time;
            gContent.push_back(curr_content);
            printf(COLOR(BLUE, "演示环节：对比单核情况下多线程与多协程的网络IO处理效率") "\n");
            printf(b_GREEN_f_BLACK(
              "线程数  当前IO/s  峰值IO/s  客户端连接数  服务端连接数  已运行时间/s") "\n");
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
    return 0;
}