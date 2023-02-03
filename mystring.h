#ifndef __HEADER_MYSTRING_H__
#define __HEADER_MYSTRING_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>

typedef struct
{
	size_t len;	// current length
	size_t cap;	// allocated space
	char *arr;
} String;

void string_init(String *str, size_t size);
void string_free(String *str);
void string_resize(String *str, size_t size);
void string_print(String *str);
void string_pushc(String *str, char c);
void string_popc(String *str);
void string_insert(String *str, char *s, size_t slen, size_t pos);
void string_remove(String *str, size_t L, size_t R);	// [L, R)
void string_clear(String *str);
void string_printf(String *str, char *fmt, ...);
void string_null_terminate(String *str);
void string_replace(String *str, size_t L, size_t R, char *s, size_t slen);	// [L, R)

#endif	// __HEADER_MYSTRING_H__