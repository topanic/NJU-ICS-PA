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
    printf("\33[1;46m" "[STRACE] " "\33[0m" " %s | a1: %d, a2: %d, a3: %d, a4: %d | ret: %d\n",
           syscall_name, c->GPR1, c->GPR2, c->GPR3, c->GPR4, c->GPRx);
}

static void fstrace(int fd) {
    printf("\33[1;43m" "[FSTRACE]" "\33[0m" " fd: %d | filename: %s\n",
           fd, get_file_name_by_fd(fd));
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
            strace("SYS_exit", c);
            sys_exit(c);
            break;
        case SYS_yield:
            strace("SYS_yield", c);
            sys_yield(c);
            break;

        case SYS_open:
            strace("SYS_open", c);
            sys_open(c);
            break;
        case SYS_read:
            strace("SYS_read", c);
            sys_read(c);
            break;
        case SYS_write:
            strace("SYS_write", c);
            sys_write(c);
            break;

        case SYS_close:
            strace("SYS_close", c);
            sys_close(c);
            break;
        case SYS_lseek:
            strace("SYS_lseek", c);
            sys_lseek(c);
            break;

        case SYS_brk:
            strace("SYS_brk", c);
            sys_brk(c);
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
    fstrace(fd);
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
    fstrace(fd);
}

void sys_close(Context *c) {
    int fd = (int) c->GPR2;
    c->GPRx = fs_close(fd);
    fstrace(fd);
}

void sys_lseek(Context *c) {
    int fd = (int) c->GPR2;
    size_t offset = (size_t) c->GPR3;
    int whence = (int) c->GPR4;
    c->GPRx = fs_lseek(fd, offset, whence);
    fstrace(fd);
}



