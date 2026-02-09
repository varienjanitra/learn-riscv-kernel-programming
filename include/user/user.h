#pragma once
#include <common/common.h>

void putchar(char ch);
long getchar(void);
int readfile(const char *filename, char *buf, size_t len);
int writefile(const char *filename, char *buf, size_t len);

void exit(void);
