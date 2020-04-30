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
    static uint64_t times = 0;
    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);
    char hbuf[INET_ADDRSTRLEN] = { 0 };
    int client_fd = accept(hbco::CurrListeningFd(), (struct sockaddr*)&sa, &len);
    while (true) {
        char buffer[100] = { 0 };
        int read_count = read(client_fd, buffer, sizeof(buffer) / sizeof(buffer[0]));
        if (read_count == 0) {
            close(client_fd);
            break;
        }
        uint64_t res = 0;
        for (uint64_t i = 0; i < 6000; i++) {
            res++;
        }
        int write_count = write(client_fd, buffer, read_count);

        // int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
        // //向服务器（特定的IP和端口）发起请求
        // struct sockaddr_in serv_addr;
        // memset(&serv_addr, 0, sizeof(serv_addr));           //每个字节都用0填充
        // serv_addr.sin_family = AF_INET;                     //使用IPv4地址
        // serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //具体的IP地址
        // serv_addr.sin_port = htons(9500);                   //端口
        // connect(conn_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        // char buf[50] = { 0 };
        // read_count = read(conn_fd, buf, sizeof(buf) / sizeof(buf[0]));
        // Display(buf);
        // write(conn_fd, buf, read_count);
        // break;
    }
}

int
main(int argc, char* argv[]) {
    if (argc < 2) {
        exit(-1);
    }
    int port = atoi(argv[1]);
    Display(port);
    pid_t pid = getpid();
    Display(pid);

    // int file_fd = open("/home/hhb/practice/hb-coroutine/tmp.txt", O_RDWR | O_APPEND);
    // write(file_fd, "qqqqqqqqqqqqqqqqqqqqqqqqqq\n", 28);
    // lseek(file_fd, 0, 0);
    // char buf[1000] = { 0 };
    // int read_ret = read(file_fd, buf, 1000);
    // Display(buf);

    hbco::CoroutineLauncher cl(port);

    gettimeofday(&gTime, nullptr);
    for (uint64_t i = 0; i < 50; i++) {
        hbco::Coroutine* server_co =
          hbco::Coroutine::Create("server_co_" + std::to_string(i), ServerCoroutine, nullptr);
        hbco::Coroutine::Resume(server_co);
    }

    hbco::EpollEventLoop();

    return 0;
}