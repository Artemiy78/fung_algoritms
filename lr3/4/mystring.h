

#ifndef MYSTRING_H
#define MYSTRING_H

#include "../utils.h"

typedef struct
{
    char *str;
    int len;
} String;

status_t string_create(const char *src, String *out);
status_t string_free(String *s);
status_t string_lex(const String *a, const String *b, int *res);
status_t string_cmp(const String *a, const String *b, int *res);
status_t string_eq(const String *a, const String *b, bool *res);
status_t string_copy(const String *src, String *dst);
status_t string_clone(const String *src, String **out);
status_t string_concat(String *a, const String *b);

#endif