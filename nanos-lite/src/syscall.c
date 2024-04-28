#include <common.h>
#include "syscall.h"
#include "fs.h"

//#define STD_IN 0
//#define STD_OUT 1
//#define STD_ERR 2

/* this  is defined in "syscall.h" */
//enum Syscall_type {
//    SYS_exit,
//    SYS_yield,
//    SYS_open,
//    SYS_read,
//    SYS_write,
//    SYS_kill,
//    SYS_getpid,
//    SYS_close,
//    SYS_lseek,
//    SYS_brk,
//    SYS_fstat,
//    SYS_time,
//    SYS_signal,
//    SYS_execve,
//    SYS_fork,
//    SYS_link,
//    SYS_unlink,
//    SYS_wait,
//    SYS_times,
//    SYS_gettimeofday
//};

static void strace(char syscall_name[], Context *c) {
    // "\33[1;36m" cyan fg
    printf("\33[1;36m" "[STRACE]" "\33[0m" " %s | a1: %d, a2: %d, a3: %d, a4: %d | ret: %d\n",
           syscall_name, c->GPR1, c->GPR2, c->GPR3, c->GPR4, c->GPRx);
}

void sys_exit(Context *c);
void sys_yield(Context *c);
void sys_write(Context *c);
void sys_brk(Context *c);
void sys_open(Context *c);
void sys_read(Context *c);
void sys_close(Context *c);
void sys_lseek(Context *c);

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

        case SYS_open:
            sys_open(c);
            strace("SYS_open", c);
            break;
        case SYS_read:
            sys_read(c);
            strace("SYS_read", c);
            break;
        case SYS_write:
            sys_write(c);
            strace("SYS_write", c);
            break;

        case SYS_close:
            sys_close(c);
            strace("SYS_close", c);
            break;
        case SYS_lseek:
            sys_lseek(c);
            strace("SYS_lseek", c);
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

void sys_brk(Context *c) {
    // TODO: it will change in pa4
    c->GPRx = 0;
}

void sys_open(Context *c) {
    const char *path = (char *) c->GPR2;
    int flags = (int) c->GPR3;
    int mode = (int) c->GPR4;
    c->GPRx = fs_open(path, flags, mode);
}

void sys_read(Context *c) {
    int fd = (int) c->GPR2;
    void *buf = (void *) c->GPR3;
    size_t len = (size_t) c->GPR4;
    c->GPRx = fs_read(fd, buf, len);
}

//void sys_write(Context *c) {
//    // int fd = (int)c->GPR2; // "fd" is used in "_write" function
//    const void *buf = (void*)c->GPR2;
//    size_t len = (size_t)c->GPR3;
//    for (int i = 0; i < len; i++) {
//        putch(*((char*)buf + i));
//    }
//    // set return value:
//    c->GPRx = len;
//}

void sys_write(Context *c) {
    int fd = (int) c->GPR2;
    const void *buf = (void *) c->GPR3;
    size_t len = (size_t) c->GPR4;
    c->GPRx = fs_write(fd, buf, len);
}

void sys_close(Context *c) {
    int fd = (int) c->GPR2;
    c->GPRx = fs_close(fd);
}

void sys_lseek(Context *c) {
    int fd = (int) c->GPR2;
    size_t offset = (size_t) c->GPR3;
    int whence = (int) c->GPR4;
    c->GPRx = fs_lseek(fd, offset, whence);
}



