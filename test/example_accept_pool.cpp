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

    while (true) {
        char buffer[100] = { 0 };
        int read_count = read(client_fd, buffer, sizeof(buffer) / sizeof(buffer[0]));
        if (read_count < 0) {
            perror("read");
        }
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
            // close(client_fd);
            // break;
        }
    }
}

int
main(int argc, char* argv[]) {
    gPort = _port;

    hbco::CoroutineLauncher cl(gPort);
    gDataTxt = fopen("/home/hhb/practice/hb-coroutine/coroutine_io_data.txt", "w");
    fprintf(gDataTxt, "多协程网络IO每秒读写次数\n");

    gettimeofday(&gTime, nullptr);
    for (uint64_t i = 0; i < _coroutine_amount; i++) {
        hbco::Coroutine* server_co =
          hbco::Coroutine::Create("server_co_" + std::to_string(i), ServerCoroutine, nullptr);
        hbco::Coroutine::Resume(server_co);
    }

    hbco::EpollEventLoop(2);

    return 0;
}