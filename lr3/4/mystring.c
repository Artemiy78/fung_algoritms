#include "../utils.h"
#include "mystring.h"

#include <string.h>

status_t string_create(const char *src, String *out)
{
    if (src == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    size_t len = strlen(src);
    if (len > INT_MAX)
        return STATUS_OUT_OF_RANGE;

    char *buf = malloc(len + 1);
    if (buf == NULL)
        return STATUS_MEMORY_ERROR;
    memcpy(buf, src, len + 1);

    out->str = buf;
    out->len = (int)len;
    return STATUS_OK;
}

status_t string_free(String *s)
{
    if (s == NULL)
        return STATUS_NULL_POINTER;
    free(s->str);
    s->str = NULL;
    s->len = 0;
    return STATUS_OK;
}

status_t string_lex(const String *a, const String *b, int *res)
{
    if (a == NULL || b == NULL || res == NULL || a->str == NULL || b->str == NULL)
        return STATUS_NULL_POINTER;

    int min = (a->len < b->len) ? a->len : b->len;
    for (int i = 0; i < min; i++)
    {
        unsigned char x = (unsigned char)a->str[i];
        unsigned char y = (unsigned char)b->str[i];
        if (x != y)
        {
            *res = (x > y) ? 1 : -1;
            return STATUS_OK;
        }
    }
    *res = (a->len > b->len) - (a->len < b->len);
    return STATUS_OK;
}

status_t string_cmp(const String *a, const String *b, int *res)
{
    if (a == NULL || b == NULL || res == NULL)
        return STATUS_NULL_POINTER;
    if (a->len != b->len)
    {
        *res = (a->len > b->len) ? 1 : -1;
        return STATUS_OK;
    }
    return string_lex(a, b, res);
}

status_t string_eq(const String *a, const String *b, bool *res)
{
    if (res == NULL)
        return STATUS_NULL_POINTER;
    int c = 0;
    status_t st = string_lex(a, b, &c);
    if (st != STATUS_OK)
        return st;
    *res = (c == 0);
    return STATUS_OK;
}

status_t string_copy(const String *src, String *dst)
{
    if (src == NULL || dst == NULL || src->str == NULL)
        return STATUS_NULL_POINTER;
    if (src == dst)
        return STATUS_OK;

    char *buf = malloc((size_t)src->len + 1);
    if (buf == NULL)
        return STATUS_MEMORY_ERROR;
    memcpy(buf, src->str, (size_t)src->len + 1);

    free(dst->str);
    dst->str = buf;
    dst->len = src->len;
    return STATUS_OK;
}

status_t string_clone(const String *src, String **out)
{
    if (src == NULL || out == NULL || src->str == NULL)
        return STATUS_NULL_POINTER;

    String *res = malloc(sizeof(String));
    if (res == NULL)
        return STATUS_MEMORY_ERROR;
    res->str = NULL;
    res->len = 0;

    status_t st = string_copy(src, res);
    if (st != STATUS_OK)
    {
        free(res);
        return st;
    }
    *out = res;
    return STATUS_OK;
}

status_t string_concat(String *a, const String *b)
{
    if (a == NULL || b == NULL || a->str == NULL || b->str == NULL)
        return STATUS_NULL_POINTER;
    if (a->len > INT_MAX - b->len)
        return STATUS_OUT_OF_RANGE;

    int new_len = a->len + b->len;
    char *buf = realloc(a->str, (size_t)new_len + 1);
    if (buf == NULL)
        return STATUS_MEMORY_ERROR;
    a->str = buf;

    memcpy(a->str + a->len, b->str, (size_t)b->len);
    a->str[new_len] = '\0';
    a->len = new_len;
    return STATUS_OK;
}