#include "environment.h"
#include "coroutine.h"

namespace hbco {

CoroutineEnvironment::CoroutineEnvironment() {
    std::shared_ptr<Coroutine> main_co(new Coroutine());
    callstack_.push_back(main_co);
}

static std::map<pthread_t, std::shared_ptr<CoroutineEnvironment>> env_manager;

std::shared_ptr<CoroutineEnvironment>
CurrEnv(void) {
    auto curr_tid = pthread_self();
    auto result = env_manager.find(curr_tid);
    if (result == env_manager.end()) {
        std::shared_ptr<CoroutineEnvironment> curr_env(new CoroutineEnvironment());
        env_manager.insert({ curr_tid, curr_env });
    }
    return env_manager[curr_tid];
}

}