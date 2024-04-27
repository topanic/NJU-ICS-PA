#include <common.h>
#include "syscall.h"

#define STD_IN 0
#define STD_OUT 1
#define STD_ERR 2

enum Syscall_type {
    SYS_exit,
    SYS_yield,
    SYS_open,
    SYS_read,
    SYS_write,
    SYS_kill,
    SYS_getpid,
    SYS_close,
    SYS_lseek,
    SYS_brk,
    SYS_fstat,
    SYS_time,
    SYS_signal,
    SYS_execve,
    SYS_fork,
    SYS_link,
    SYS_unlink,
    SYS_wait,
    SYS_times,
    SYS_gettimeofday
};

static void strace(char syscall_name[], Context *c) {
    // "\33[1;36m" cyan fg
    printf("\33[1;36m" "[STRACE]" "\33[0m" " %s | a1: %d, a2: %d, a3: %d, a4: %d | ret: %d\n",
           syscall_name, c->GPR1, c->GPR2, c->GPR3, c->GPR4, c->GPRx);
}

void sys_exit(Context *c);
void sys_yield(Context *c);
void sys_write(Context *c);
void sys_brk(Context *c);

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

        case SYS_write:
            sys_write(c);
            strace("SYS_write", c);
            break;
        case SYS_brk:
            sys_brk(c);
            strace("SYS_brk", c);
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

void sys_write(Context *c) {
    // int fd = (int)c->GPR2; // "fd" is used in "_write" function
    const void *buf = (void*)c->GPR2;
    size_t len = (size_t)c->GPR3;
    for (int i = 0; i < len; i++) {
        putch(*((char*)buf + i));
    }
    // set return value:
    c->GPRx = len;
}

void sys_brk(Context *c) {
    // TODO: it will change in pa4
    c->GPRx = 0;
}