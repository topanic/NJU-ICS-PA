#include <common.h>
#include "syscall.h"

enum Syscall_type {
    SYS_exit,
    SYS_yield,
};

static void strace(char syscall_name[], Context *c) {
    // "\33[1;36m" cyan fg
    printf("\33[1;36m" "[STRACE]" "\33[0m" " %s | a1: %d, a2: %d, a3: %d, a4: %d | ret: %d\n",
           syscall_name, c->GPR1, c->GPR2, c->GPR3, c->GPR4, c->GPRx);
}

void sys_exit(Context *c);
void sys_yield(Context *c);

void do_syscall(Context *c) {
    uintptr_t a[4];
    a[0] = c->GPR1;

    switch (a[0]) {
        case SYS_exit:
            sys_exit(c);
            strace("SYS_exit", c);
            break;
        case SYS_yield:
            sys_yield(c);
            strace("SYS_yield", c);
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