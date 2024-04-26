#include <common.h>
#include "syscall.h"

enum Syscall_type {
    SYS_exit,
    SYS_yield,
};

void sys_exit(Context *c);
void sys_yield(Context *c);

void do_syscall(Context *c) {
    uintptr_t a[4];
    a[0] = c->GPR1;

    switch (a[0]) {
        case SYS_exit:
            sys_exit(c);
            break;
        case SYS_yield:
            sys_yield(c);
            break;

        default:
            panic("Unhandled syscall ID = %d", a[0]);
    }
}

void sys_exit(Context *c) {
    halt(c->GPRx);
}

void sys_yield(Context *c) {
    yield();
    // set return value
    c->GPRx = 0;
}