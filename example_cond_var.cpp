#include <queue>
#include <unistd.h>

#include "condition_variable.h"
#include "coroutine.h"
#include "coroutine_launcher.h"
#include "log.h"

std::queue<int> resource;

struct CoArg {
    std::string str_;
    hbco::CondVar* cond_;
    CoArg(const std::string& str, hbco::CondVar* cond)
      : str_(str)
      , cond_(cond) {}
};

void
Produce(void* arg) {
    CoArg* data = static_cast<CoArg*>(arg);
    while (true) {
        Display(data->str_);
        srand((unsigned)time(nullptr));
        int produced_num = rand() % 100;
        resource.push(produced_num);
        Display(produced_num);
        data->cond_->Signal();
        hbco::Coroutine::PollTime(1000);
    }
}

void
Resume(void* arg) {
    CoArg* data = static_cast<CoArg*>(arg);
    while (true) {
        if (resource.empty()) {
            data->cond_->WaitTime(1000);
            continue;
        }
        Display(data->str_);
        int resumed_num = resource.front();
        resource.pop();
        Display(resumed_num);
    }
}

int
main(int argc, char** argv) {
    hbco::CoroutineLauncher cl;
    hbco::CondVar cond;
    CoArg arg_produce("PRO", &cond);
    CoArg arg_resume("RES", &cond);
    hbco::Coroutine* co_producer = hbco::Coroutine::Create("co_producer", Produce, &arg_produce);
    hbco::Coroutine* co_resumer = hbco::Coroutine::Create("co_resumer", Resume, &arg_resume);
    hbco::Coroutine::Resume(co_resumer);
    hbco::Coroutine::Resume(co_producer);

    hbco::EventLoop();

    return 0;
}