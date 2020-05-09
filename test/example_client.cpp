#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include "condition_variable.h"
#include "coroutine.h"
#include "coroutine_launcher.h"
#include "event_loop.h"
#include "global.h"
#include "log.h"

// void xxx(int a, int b){}
void
ClientCoroutine(void* arg) {
    // struct sockaddr_in sa;
    // socklen_t len = sizeof(sa);
    // int client_fd = accept(hbco::CurrListeningFd(), (struct sockaddr*)&sa, &len);
    int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;             //使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr(_ip); //具体的IP地址
    serv_addr.sin_port = htons(_port);          //端口
    if (connect(conn_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
    }
    std::string msg("a client coroutine\n");
    int written_length = write(conn_fd, msg.data(), msg.size());
    if (written_length < 0) {
        perror("write");
    }
    return;
}

int
main(int argc, char* argv[]) {
    gPort = 12000;

    hbco::CoroutineLauncher cl(gPort);

    gettimeofday(&gTime, nullptr);
    for (uint64_t i = 0; i < _coroutine_amount; i++) {
        hbco::Coroutine* client_co =
          hbco::Coroutine::Create("server_co_" + std::to_string(i), ClientCoroutine, nullptr);
        hbco::Coroutine::Resume(client_co);
    }

    hbco::EpollEventLoop(3);

    return 0;
}