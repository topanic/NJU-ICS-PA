#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
    if (s == NULL) {
        return 0;
    }
    size_t n = 0;
    while (s[n] != '\0') {
        ++n;
    }
    return n;
}

/* 统计字符串中字符个数(不包括\0)，如果个数大于count，则返回count，否则返回字符个数 */
size_t strnlen(const char *s, size_t count) {
    const char *sc;
    for (sc = s; count-- && *sc != '\0'; ++sc)
        /* nothing */;
    return sc - s;
}

char *strcpy(char *dst, const char *src) {
    if (src == NULL || dst == NULL) {
        return dst;
    }
    char *res = dst;
    do {
        *dst = *src;
        dst++;
        src++;
    } while (*src != '\0');
    return res;
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i;
    for (i = 0; src[i] != '\0' && i < n; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\n';
    return dst;
}

char *strcat(char *dst, const char *src) {
    size_t i = 0;
    while (dst[i] != '\0') {
        i++;
    }
    strcpy(dst + i, src);
    return dst;
}

int strcmp(const char *s1, const char *s2) {
    size_t i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] > s2[i])
            return 1;
        if (s1[i] < s2[i])
            return -1;
        i++;
    }
    if (s1[i] != '\0' && s2[i] == '\0')
        return 1;
    if (s1[i] == '\0' && s2[i] != '\0')
        return -1;
    return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n--) {
        if (*s1 > *s2)
            return 1;
        if (*s1 < *s2)
            return -1;
        s1++;
        s2++;
    }
    return 0;
}

void *memset(void *s, int c, size_t n) {
    char *ch = (char *) s;
    while (n-- > 0) {
        *ch++ = c;
    }
    return 0;
}

void *memmove(void *dst, const void *src, size_t n) {
    if (dst < src) {
        char *d = (char *) dst;
        char *s = (char *) src;
        while (n--) {
            *d = *s;
            d++;
            s++;
        }
    } else {
        char *d = (char *) dst + n - 1;
        char *s = (char *) src + n - 1;
        while (n--) {
            *d = *s;
            d--;
            s--;
        }
    }
    return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
    char *d = (char *) out;
    char *s = (char *) in;
    while (n--) {
        *d = *s;
        d++;
        s++;
    }
    return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    if (s1 == NULL || s2 == NULL) {
        return 0;
    }
    const unsigned char *src1 = s1;
    const unsigned char *src2 = s2;
    while (n != 0 && *src1 != '\0' && *src2 != '\0' && *src1 == *src2) {
        --n;
        ++src1;
        ++src2;
    }
    return *src1 == *src2 || n == 0 ? 0 : *src1 < *src2 ? -1 : 1;
}


#endif
