#include <dlfcn.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "coroutine.h"
#include "environment.h"
#include "event_loop.h"
#include "log.h"

using SocketFunc = int(int, int, int);
static SocketFunc* g_syscall_socket = reinterpret_cast<SocketFunc*>(dlsym(RTLD_NEXT, "socket"));
using AcceptFunc = int(int, struct sockaddr*, socklen_t*);
static AcceptFunc* g_syscall_accept = reinterpret_cast<AcceptFunc*>(dlsym(RTLD_NEXT, "accept"));
using ConnectFunc = int(int fd, const struct sockaddr*, socklen_t);
static ConnectFunc* g_syscall_connect = reinterpret_cast<ConnectFunc*>(dlsym(RTLD_NEXT, "connect"));
using ReadFunc = ssize_t(int, void*, size_t);
static ReadFunc* g_syscall_read = reinterpret_cast<ReadFunc*>(dlsym(RTLD_NEXT, "read"));
using WriteFunc = ssize_t(int, const void*, size_t);
static WriteFunc* g_syscall_write = reinterpret_cast<WriteFunc*>(dlsym(RTLD_NEXT, "write"));

int
socket(int domain, int type, int protocol) {
    int socket_fd = g_syscall_socket(domain, type, protocol);
    if (socket_fd == -1) {
        perror("socket");
        return socket_fd;
    }

    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("socket->fcntl-get");
        close(socket_fd);
        return -1;
    }
    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("socket->fnctl-set");
        close(socket_fd);
        return -1;
    }
    return socket_fd;
}

int
accept(int fd, struct sockaddr* address, socklen_t* length) {
    std::cout << "hahahaahah" << std::endl;
    hbco::CurrEnv()->accept_cond_.Wait();
    uint32_t epoll_events = EPOLLIN;
    int client_fd = -1;
    while (true) {
        int time_out = hbco::Poll(fd, epoll_events);
        if (!time_out) {
            break;
        }
    }
    client_fd = g_syscall_accept(fd, address, length);

    Display(client_fd);
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("accept->fcntl-get");
        close(client_fd);
        return -1;
    }
    if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("accept->fnctl-set");
        close(client_fd);
        return -1;
    }
    return client_fd;
}

int
connect(int fd, const struct sockaddr* address, socklen_t address_length) {
    int ret = g_syscall_connect(fd, address, address_length);
    if (ret >= 0) {
        return ret;
    }
    uint32_t epoll_events = EPOLLOUT;
    bool time_out = true;
    for (int i = 0; i < 75; i++) {
        time_out = hbco::Poll(fd, epoll_events);
        if (!time_out) {
            break;
        }
    }
    return time_out ? -1 : 0;
}

ssize_t
read(int fd, void* buffer, size_t length) {
    uint32_t epoll_events = EPOLLIN;
    ssize_t ret = -1;
    while (true) {
        bool time_out = hbco::Poll(fd, epoll_events);
        if (!time_out) {
            break;
        }
    }
    ret = g_syscall_read(fd, buffer, length);
    return ret;
}

ssize_t
write(int fd, const void* buffer, size_t length) {
    uint32_t epoll_events = EPOLLOUT;
    size_t wrote_length = 0;
    while (wrote_length < length) {
        hbco::Poll(fd, epoll_events);
        int ret = g_syscall_write(fd, (const char*)buffer + wrote_length, length - wrote_length);
        if (ret < 0) {
            return ret;
        }
        wrote_length += ret;
    }
    return wrote_length;
}