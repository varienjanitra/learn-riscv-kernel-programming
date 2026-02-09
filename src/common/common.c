#include <common/common.h>

struct sbiret sbi_call(
	long arg0, long arg1, long arg2, long arg3,
	long arg4, long arg5, long fid, long eid
)
{
	register long a0 __asm__("a0") = arg0;
	register long a1 __asm__("a1") = arg1;
	register long a2 __asm__("a2") = arg2;
	register long a3 __asm__("a3") = arg3;
	register long a4 __asm__("a4") = arg4;
	register long a5 __asm__("a5") = arg5;
	register long a6 __asm__("a6") = fid;
	register long a7 __asm__("a7") = eid;

	__asm__ __volatile("ecall"
			: "=r"(a0), "=r"(a1)
			: "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), \
			"r"(a5), "r"(a6), "r"(a7)
			: "memory");

	return (struct sbiret){.error = a0, .value = a1};
}

void *memset(void *buf, char c, size_t n)
{
	uint8_t *p = (uint8_t * ) buf;

	while (n--) {
		*p++ = c;
	}

	return buf;
}

void *memcpy(void *dst, const void *src, size_t n)
{
	uint8_t *d = (uint8_t *) dst;
	const uint8_t *s = (const uint8_t *) src;

	while (n--) {
		*d++ = *s++;
	}

	return dst;
}

__attribute__((weak))
void putchar(char ch)
{
	sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}

__attribute__((weak))
long getchar(void)
{
	struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
	return ret.error;
}

void printf(const char *fmt, ...)
{
	va_list vargs;
	va_start(vargs, fmt);

	while (*fmt) {
		if (*fmt == '%') {
			fmt++;

			switch (*fmt) {
			case '\0':
				putchar('%');

				goto end;
			case '%':
				putchar('%');

				break;
			case 's': {
				const char *s = va_arg(vargs, const char *);

				if (!s) s = "(null)";

				while (*s) {
					putchar (*s);
					s++;
				}

				break;
			}
			case 'd': {
				long value = va_arg(vargs, long);
				unsigned long magnitude = value;

				if (value < 0) {
					putchar('-');
					magnitude = -magnitude;
				}

				if (value == 0) {
					putchar('0');
					break;
				}

				unsigned long divisor = 1;

				while (magnitude / divisor > 9) {
					divisor *= 10;
				}

				while (divisor > 0) {
					putchar('0' + magnitude / divisor);
					magnitude %= divisor;
					divisor /= 10;
				}

				break;
			}
			case 'c':
				putchar((char)va_arg(vargs, int));
				break;
			case 'x':
			case 'p': {
				if (*fmt == 'p') {
					putchar('0');
					putchar('x');
				}
				uint64_t value = va_arg(vargs, uint64_t);

				bool started = false;
				for (int i = 15; i >= 0; i--) {
					uint64_t nibble = (value >> (i * 4)) & 0xf;
					if (nibble > 0 || started || i == 0) {
						putchar("0123456789abcdef"[nibble]);
						started = true;
					}

				}
				break;
			}
			}
		} else {
			putchar(*fmt);
		}

		fmt++;
	}
end:
	va_end(vargs);
}

char *strcpy(char *dst, const char *src)
{
	char *d = dst;

	while (*src) {
		*d++ = *src++;
	}

	*d = '\0';

	return dst;
}

int strcmp(const char *s1, const char *s2)
{
	while (*s1 && *s2) {
		if (*s1 != *s2) {
			break;
		}

		s1++;
		s2++;
	}

	return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	while (n > 0 && *s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}

	if (n == 0) {
		return 0;
	}

	return *(unsigned char *)s1 - *(unsigned char *)s2;
}

char *strncpy(char *dest, const char *src, size_t n)
{
	size_t i;

	for (i = 0; i < n && src[i] != '\0'; i++) {
		dest[i] = src[i];
	}

	for(; i < n; i++) {
		dest[i] = '\0';
	}

	return dest;
}

size_t strlen(const char *s) {
	size_t len = 0;

	while (s[len]) {
		len++;
	}

	return len;
}
