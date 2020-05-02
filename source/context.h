#ifndef CONTEXT_H
#define CONTEXT_H

#include <stddef.h>
#include <ucontext.h>

namespace hbco {

enum {
    kRDI = 7,
    kRSI = 8,
    kRETAddr = 9,
    kRSP = 13,
};

struct Context {
    void* register_[14];
    size_t stack_size_;
    char* stack_ptr_;
    ucontext ctx_;
};

extern "C" {
void SwapContext(hbco::Context* curr_ctx, hbco::Context* next_ctx) asm("SwapContext");
}

} // namespace hbco

#endif