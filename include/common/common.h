#pragma once

#define KERNEL_NAME "Iyen Kernel"
#define KERNEL_VERSION "master"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long uintptr_t;

typedef uint64_t size_t;
typedef uint64_t paddr_t;
typedef uint64_t vaddr_t;

#define align_up(value, align) __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
#define offsetof(type, member) __builtin_offsetof(type, member)

#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

#define PAGE_SIZE 4096

#define SYS_PUTCHAR 1
#define SYS_GETCHAR 2
#define SYS_EXIT 3

void *memset(void *buf, char c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void putchar(char ch);
long getchar(void);
void printf(const char *fmt, ...);
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);

struct sbiret {
	long error;
	long value;
};
