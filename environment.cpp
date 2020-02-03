#include "environment.h"
#include "coroutine.h"

namespace hbco {

CoroutineEnvironment::CoroutineEnvironment() {
    std::shared_ptr<Coroutine> main_co(new Coroutine());
    callstack_.push(main_co);
}
}