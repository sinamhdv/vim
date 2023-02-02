#include "mystring.h"

void string_init(String *str, size_t size)
{
	str->arr = malloc(size * sizeof(char));
	str->len = str->cap = size;
}

void string_free(String *str)
{
	free(str->arr);
}

void string_resize(String *str, size_t size)
{
	if (size == 0) size = 1;
	str->arr = realloc(str->arr, size * sizeof(char));
	str->cap = size;
	if (str->len > size) str->len = size;
}

void string_print(String *str)
{
	for (size_t i = 0; i < str->len; i++)
		putchar(str->arr[i]);
}

void string_pushc(String *str, char c)
{
	if (str->len == str->cap)
		string_resize(str, 2 * str->cap);
	str->arr[str->len++] = c;
}

void string_popc(String *str)
{
	str->len--;
}

void string_insert(String *str, char *s, size_t slen, size_t pos)
{
	while (str->cap < str->len + slen)
		string_resize(str, 2 * str->cap);
	memmove(str->arr + pos + slen, str->arr + pos, str->len - pos);
	memcpy(str->arr + pos, s, slen);
	str->len += slen;
}

void string_remove(String *str, size_t L, size_t R)
{
	memmove(str->arr + L, str->arr + R, str->len - R);
	str->len -= (R - L);
}