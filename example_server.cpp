#include <arpa/inet.h>
#include <fcntl.h>
#include <strings.h>
#include <unistd.h>

#include "condition_variable.h"
#include "coroutine.h"
#include "coroutine_launcher.h"
#include "log.h"

int g_listen_fd = -1;
hbco::CondVar g_accept_cond;

static void
ServerCoroutine(void* arg) {
    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);
    char hbuf[INET_ADDRSTRLEN] = { 0 };
    int client_fd = accept(g_listen_fd, (struct sockaddr*)&sa, &len);
    while (true) {
        char buffer[100] = { 0 };
        int read_count = read(client_fd, buffer, sizeof(buffer) / sizeof(buffer[0]));
        std::string str(buffer);
        if (str == "exit\n") {
            close(client_fd);
            break;
        }
        write(client_fd, buffer, read_count);
        Display(client_fd);
        Display(read_count);
        Display(buffer);
    }
}

static void
AcceptCoroutine(void* arg) {
    hbco::CoroutineEnvironment* curr_env = hbco::CurrEnv();
    while (true) {
        curr_env->accept_cond_.Signal();
        hbco::Coroutine::PollTime(1000);
    }
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
        return -1;
    }
    return sfd;
}

int
main(int argc, char* argv[]) {
    if (argc < 2) {
        exit(-1);
    }
    int port = atoi(argv[1]);
    g_listen_fd = create_and_bind(port);
    listen(g_listen_fd, SOMAXCONN);

    hbco::CoroutineLauncher cl;
    hbco::Coroutine* accept_co = hbco::Coroutine::Create("accept_co", AcceptCoroutine, nullptr);
    hbco::Coroutine::Resume(accept_co);
    hbco::Coroutine* server_co1 = hbco::Coroutine::Create("server_co1", ServerCoroutine, nullptr);
    hbco::Coroutine::Resume(server_co1);
    // hbco::Coroutine* server_co2 = hbco::Coroutine::Create("server_co2", ServerCoroutine,
    // nullptr); hbco::Coroutine::Resume(server_co2);

    hbco::EpollEventLoop();

    return 0;
}