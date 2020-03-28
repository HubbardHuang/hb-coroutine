#include <queue>
#include <unistd.h>

#include "coroutine.h"
#include "coroutine_launcher.h"
#include "log.h"

std::queue<int> resource;

void
Produce(void* arg) {
    while (true) {
        srand((unsigned)time(nullptr));
        int produced_num = rand() % 100;
        resource.push(produced_num);
        Display(produced_num);
        hbco::Coroutine::CondVarSignal();
        hbco::Coroutine::Poll();
        sleep(1);
    }
}

void
Resume(void* arg) {
    while (true) {
        if (resource.empty()) {
            hbco::Coroutine::CondVarWait();
            continue;
        }
        int resumed_num = resource.front();
        resource.pop();
        Display(resumed_num);
    }
}

int
main(int argc, char** argv) {
    hbco::CoroutineLauncher cl;
    hbco::Coroutine* co_producer = hbco::Coroutine::Create("co_producer", Produce, nullptr);
    hbco::Coroutine* co_resumer = hbco::Coroutine::Create("co_resumer", Resume, nullptr);
    hbco::Coroutine::Resume(co_resumer);
    hbco::Coroutine::Resume(co_producer);

    hbco::EventLoop();

    return 0;
}