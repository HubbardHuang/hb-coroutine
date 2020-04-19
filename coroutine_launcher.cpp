#include "coroutine_launcher.h"
#include "coroutine.h"
#include "port.h"

namespace hbco {

static void
AcceptCoroutine(void* arg) {
    hbco::CoroutineEnvironment* curr_env = hbco::CurrEnv();
    while (true) {
        curr_env->accept_cond_.Signal();
        hbco::Coroutine::Sleep(1000);
    }
}

CoroutineLauncher::CoroutineLauncher(int port) {
    SetPort(port);
    hbco::Coroutine* accept_co = hbco::Coroutine::Create("accept_co", AcceptCoroutine, nullptr);
    hbco::Coroutine::Resume(accept_co);
}

CoroutineLauncher::~CoroutineLauncher() {
    ReleaseResources();
}

} // namespace hbco