#include <fs.h>

typedef size_t (*ReadFn)(void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn)(const void *buf, size_t offset, size_t len);

typedef struct {
    char *name;
    size_t size;
    size_t disk_offset;
    ReadFn read;
    WriteFn write;

    size_t lseek_offset;
    int is_open;
} Finfo;

enum {
    FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB
};

size_t invalid_read(void *buf, size_t offset, size_t len) {
    panic("should not reach here");
    return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
    panic("should not reach here");
    return 0;
}

extern size_t serial_write(const void *buf, size_t offset, size_t len);
extern size_t events_read(void *buf, size_t offset, size_t len);
extern size_t dispinfo_read(void *buf, size_t offset, size_t len);
extern size_t fb_write(const void *buf, size_t offset, size_t len);

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
        [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
        [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
        [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},

#include "files.h"
};

#define FILE_TABLE_SIZE (sizeof(file_table) / sizeof(Finfo))

char *get_file_name_by_fd(int fd) {
    if (fd < FILE_TABLE_SIZE) {
        return file_table[fd].name;
    }
    return NULL;
}

void init_fs() {
    // TODO: initialize the size of /dev/fb
}


extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);

int fs_open(const char *pathname, int flags, int mode) {
    for (int i = 0; i < sizeof(file_table) / sizeof(Finfo); i++) {
        if (strcmp(file_table[i].name, pathname) == 0) {
            file_table[i].lseek_offset = 0;
            file_table[i].is_open = 1;
            return i;
        }
    }
    panic("file %s not found", pathname);
}

size_t fs_read(int fd, void *buf, size_t len) {
    Finfo *finfo = &file_table[fd];
    size_t read_len;
    if (finfo->read == NULL) {
        read_len = finfo->size - finfo->lseek_offset < len ? finfo->size - finfo->lseek_offset : len;
        assert(ramdisk_read(buf, finfo->disk_offset + finfo->lseek_offset, read_len) == read_len);
        finfo->lseek_offset += read_len;
    } else {
        // 意味着往标准输入读取的结果readlen是0？
        read_len = finfo->read(buf, finfo->lseek_offset, len);
        finfo->lseek_offset += read_len;
    }
    return read_len;
}

size_t fs_write(int fd, const void *buf, size_t len) {
    Finfo *finfo = &file_table[fd];
    size_t write_len;
    if (finfo->write == NULL) {
        write_len = finfo->size - finfo->lseek_offset < len ? finfo->size - finfo->lseek_offset : len;
        assert(ramdisk_write(buf, finfo->disk_offset + finfo->lseek_offset, write_len) == write_len);
        finfo->lseek_offset += write_len;
    } else {
        // 意味着往标准输出写入的结果writelen是0？
        write_len = finfo->write(buf, finfo->lseek_offset, len);
        finfo->lseek_offset += write_len;
    }
    return write_len;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
    Finfo *finfo = &file_table[fd];

    switch (whence) {
        case SEEK_SET :
            finfo->lseek_offset = offset;
            break;
        case SEEK_CUR :
            finfo->lseek_offset += offset;
            break;
        case SEEK_END :
            finfo->lseek_offset = finfo->size + offset;
            break;
        default:
            assert(0);
    }
    return finfo->lseek_offset;
}

int fs_close(int fd) {
    file_table[fd].lseek_offset = 0;
    file_table[fd].is_open = 0;
    return 0;
}