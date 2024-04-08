#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

static uintptr_t addr = 0;  // 上次分配内存的位置

void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
//  panic("Not implemented");

  if (addr == 0) {  // 如果是第一次调用malloc
    addr = (uintptr_t)heap.start;  // 从堆区开始分配
  }

  if (addr + size > (uintptr_t)heap.end) {  // 如果堆区剩余空间不足
    return NULL;  // 返回NULL表示分配失败
  }

  void *ret = (void *)addr;  // 返回的内存区域的起始地址
  addr += size;  // 更新addr

  return ret;

#endif
  return NULL;
}

void free(void *ptr) {
}

#endif
