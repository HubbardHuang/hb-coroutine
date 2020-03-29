#ifndef CONDITION_VARIABLE_H
#define CONDITION_VARIABLE_H

#include <list>

namespace hbco {

class Coroutine;
class CondVar {
private:
    std::list<Coroutine*> pending_;

public:
    CondVar() = default;
    void Signal(void);
    void Wait(void);
    void WaitTime(long duration = -1);
};

}

#endif