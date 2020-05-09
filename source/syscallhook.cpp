#include <bitset>

#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "coroutine.h"
#include "environment.h"
#include "event_loop.h"
#include "global.h"
#include "log.h"

static std::bitset<1000> can_use_syscall_hook;

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
using RecvFunc = ssize_t(int, void*, size_t, int);
static RecvFunc* g_syscall_recv = reinterpret_cast<RecvFunc*>(dlsym(RTLD_NEXT, "recv"));
using SendFunc = ssize_t(int, const void*, size_t, int);
static SendFunc* g_syscall_send = reinterpret_cast<SendFunc*>(dlsym(RTLD_NEXT, "send"));
using CloseFunc = int(int);
static CloseFunc* g_syscall_close = reinterpret_cast<CloseFunc*>(dlsym(RTLD_NEXT, "close"));

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
    can_use_syscall_hook.set(socket_fd);
    return socket_fd;
}

int
accept(int fd, struct sockaddr* address, socklen_t* length) {
    hbco::CoroutineEnvironment* curr_env = hbco::CurrEnv();
    curr_env->accept_cond_.Wait();
    hbco::Coroutine* curr_co = hbco::CurrCoroutine();
    curr_co->can_add_to_pool_ = true;
    uint32_t epoll_events = EPOLLIN;
    int client_fd = -1;
    while (true) {
        int time_out = hbco::Poll(fd, epoll_events);
        if (!time_out) {
            break;
        }
    }
    client_fd = g_syscall_accept(fd, address, length);
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("accept->fcntl-get");
        close(client_fd);
        return -1;
    }
    flags |= O_NDELAY;
    flags |= O_NONBLOCK;
    if (fcntl(client_fd, F_SETFL, flags) == -1) {
        perror("accept->fnctl-set");
        close(client_fd);
        return -1;
    }
    can_use_syscall_hook.set(client_fd);
    gFdType[client_fd] = ClientConnection;
    gClientConnectionCount++;
    return client_fd;
}

int
connect(int fd, const struct sockaddr* address, socklen_t address_length) {
    int ret = g_syscall_connect(fd, address, address_length);
    if (ret >= 0) {
        return ret;
    }
    if (!(ret < 0 && errno == EINPROGRESS)) {
        return ret;
    }
    uint32_t epoll_events = EPOLLOUT;
    bool time_out = true;
    long max_time = 75000; // ms
    long try_count = 3;
    long time_per_try = max_time / try_count; // ms
    for (int i = 0; i < try_count; i++) {
        time_out = hbco::Poll(fd, epoll_events, time_per_try);
        if (!time_out) {
            break;
        }
    }
    if (time_out) {
        return -1;
    }
    int err = 0;
    socklen_t errlen = sizeof(err);
    ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    if (ret < 0) {
        return ret;
    } else if (err != 0) {
        errno = err;
        return -1;
    }
    errno = 0;
    gFdType[fd] = ServerConnection;
    gServerConnectionCount++;
    return 0;
}

ssize_t
read(int fd, void* buffer, size_t length) {
    if (!can_use_syscall_hook.test(fd)) {
        return g_syscall_read(fd, buffer, length);
    }
    uint32_t epoll_events = EPOLLIN | EPOLLERR | EPOLLHUP;
    ssize_t ret = -1;
    bool time_out = hbco::Poll(fd, epoll_events, 1000);
    ret = g_syscall_read(fd, buffer, length);
#ifdef DEBUG
    gCount++;
#endif
    return ret;
}

ssize_t
write(int fd, const void* buffer, size_t length) {
    if (!can_use_syscall_hook.test(fd)) {
        return g_syscall_write(fd, buffer, length);
    }
    uint32_t epoll_events = EPOLLOUT | EPOLLERR | EPOLLHUP;
    ssize_t wrote_length = 0;
    int ret = g_syscall_write(fd, (const char*)buffer + wrote_length, length - wrote_length);
    if (ret == 0) {
        return ret;
    }
    if (ret > 0) {
        wrote_length += ret;
    }
    while (wrote_length < length) {
        hbco::Poll(fd, epoll_events);
        ret = g_syscall_write(fd, (const char*)buffer + wrote_length, length - wrote_length);
        if (ret <= 0) {
            break;
        }
        wrote_length += ret;
    }
    if (ret <= 0 && wrote_length == 0) {
        return ret;
    }
#ifdef DEBUG
    gCount++;
#endif
    return wrote_length;
}

ssize_t
send(int fd, const void* buffer, size_t length, int flags) {
    uint32_t epoll_events = EPOLLOUT | EPOLLERR | EPOLLHUP;
    ssize_t sent_length = 0;
    int ret = g_syscall_send(fd, (const char*)buffer + sent_length, length - sent_length, flags);
    if (ret == 0) {
        return ret;
    }
    if (ret > 0) {
        sent_length += ret;
    }
    while (sent_length < length) {
        hbco::Poll(fd, epoll_events);
        ret = g_syscall_send(fd, (const char*)buffer + sent_length, length - sent_length, flags);
        if (ret <= 0) {
            break;
        }
        sent_length += ret;
    }
    if (ret <= 0 && sent_length == 0) {
        return ret;
    }
#ifdef DEBUG
    gCount++;
#endif
    return sent_length;
}

ssize_t
recv(int fd, void* buffer, size_t length, int flags) {
    uint32_t epoll_events = EPOLLIN | EPOLLERR | EPOLLHUP;
    ssize_t ret = -1;
    while (true) {
        bool time_out = hbco::Poll(fd, epoll_events, 1000);
        if (!time_out) {
            break;
        }
    }
    ret = g_syscall_recv(fd, buffer, length, flags);
#ifdef DEBUG
    gCount++;
#endif
    return ret;
}

int
close(int fd) {
    if (can_use_syscall_hook.test(fd)) {
        can_use_syscall_hook.reset(fd);
    }
    if (gFdType[fd] == ClientConnection) {
        gClientConnectionCount--;
    } else if (gFdType[fd] == ServerConnection) {
        gServerConnectionCount--;
    }
    gFdType[fd] = OTHER;
    return g_syscall_close(fd);
}