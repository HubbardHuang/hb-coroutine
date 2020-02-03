#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <memory>
#include <stack>

namespace hbco {

class Coroutine;
struct CoroutineEnvironment {
    std::stack<std::shared_ptr<Coroutine>> callstack_;
    CoroutineEnvironment();
};
}

#endif