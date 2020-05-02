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

static void
ServerCoroutine(void* arg) {
    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);
    int client_fd = accept(hbco::CurrListeningFd(), (struct sockaddr*)&sa, &len);

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
        int read_count = read(client_fd, buffer, sizeof(buffer) / sizeof(buffer[0]));
        if (read_count == 0) {
            close(client_fd);
            break;
        }

        std::string s(buffer);
        for (int i = 0; i < 1000; i++) {
            s.push_back('a');
        }
        int written_count = write(client_fd, s.data(), s.size());
        if (written_count < 0) {
            perror("write to client");
        }

        if (send(conn_fd, "I'm huanghaobo\n", 14, 0) < 0) {
            perror("send to server");
        }
        memset(buffer, 0, 100);
        if (recv(conn_fd, buffer, sizeof(buffer), 0) < 0) {
            perror("recv from server");
        }
    }
}

int
main(int argc, char* argv[]) {
    gPort = _port;
    Display(_coroutine_amount);
    Display(_port);

    hbco::CoroutineLauncher cl(gPort);
    int fd = open("/home/hhb/practice/hb-coroutine/tmp.txt", O_RDWR | O_TRUNC);
    if (write(fd, "huanghaobo\n", 11) < 0) {
        perror("write");
    }
    close(fd);

    gettimeofday(&gTime, nullptr);
    for (uint64_t i = 0; i < _coroutine_amount; i++) {
        hbco::Coroutine* server_co =
          hbco::Coroutine::Create("server_co_" + std::to_string(i), ServerCoroutine, nullptr);
        hbco::Coroutine::Resume(server_co);
    }

    hbco::EpollEventLoop();

    return 0;
}