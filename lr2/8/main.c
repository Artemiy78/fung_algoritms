#include "utils.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN_BASE 2
#define MAX_BASE 36

status_t str_len(const char *str, size_t *out)
{
    if (str == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    size_t i = 0;
    while (str[i] != '\0')
        i++;
    *out = i;
    return STATUS_OK;
}

status_t char_to_digit(char c, unsigned base, unsigned *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (base < MIN_BASE || base > MAX_BASE)
        return STATUS_INVALID_ARGUMENT;

    unsigned v;
    if (c >= '0' && c <= '9')
        v = (unsigned)(c - '0');
    else if (c >= 'A' && c <= 'Z')
        v = (unsigned)(c - 'A') + 10;
    else if (c >= 'a' && c <= 'z')
        v = (unsigned)(c - 'a') + 10;
    else
        return STATUS_INVALID_ARGUMENT;

    if (v >= base)
        return STATUS_INVALID_ARGUMENT;
    *out = v;
    return STATUS_OK;
}

status_t digit_to_char(unsigned d, char *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (d >= MAX_BASE)
        return STATUS_INVALID_ARGUMENT;
    *out = (d < 10) ? (char)('0' + d) : (char)('A' + (d - 10));
    return STATUS_OK;
}

status_t validate_number(const char *str, unsigned base)
{
    if (str == NULL)
        return STATUS_NULL_POINTER;
    if (str[0] == '\0')
        return STATUS_INVALID_ARGUMENT;
    for (size_t i = 0; str[i] != '\0'; i++)
    {
        unsigned d;
        status_t st = char_to_digit(str[i], base, &d);
        if (st != STATUS_OK)
            return st;
    }
    return STATUS_OK;
}

status_t add_big(const char *a, const char *b, unsigned base, char **out)
{
    if (a == NULL || b == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    if (base < MIN_BASE || base > MAX_BASE)
        return STATUS_INVALID_ARGUMENT;

    status_t st = validate_number(a, base);
    if (st != STATUS_OK)
        return st;
    st = validate_number(b, base);
    if (st != STATUS_OK)
        return st;

    size_t la, lb;
    st = str_len(a, &la);
    if (st != STATUS_OK)
        return st;
    st = str_len(b, &lb);
    if (st != STATUS_OK)
        return st;

    size_t max_len = (la > lb) ? la : lb;

    char *rev = malloc(max_len + 2);
    if (rev == NULL)
        return STATUS_MEMORY_ERROR;

    size_t i = la;
    size_t j = lb;
    size_t n = 0;
    unsigned transfer = 0;

    while (i > 0 || j > 0 || transfer > 0)
    {
        unsigned da = 0;
        unsigned db = 0;
        if (i > 0)
        {
            i--;
            st = char_to_digit(a[i], base, &da);
            if (st != STATUS_OK)
            {
                free(rev);
                return st;
            }
        }
        if (j > 0)
        {
            j--;
            st = char_to_digit(b[j], base, &db);
            if (st != STATUS_OK)
            {
                free(rev);
                return st;
            }
        }
        unsigned s = da + db + transfer;
        char ch;
        st = digit_to_char(s % base, &ch);
        if (st != STATUS_OK)
        {
            free(rev);
            return st;
        }
        rev[n++] = ch;
        transfer = s / base;
    }

    while (n > 1 && rev[n - 1] == '0')
        n--;

    char *res = malloc(n + 1);
    if (res == NULL)
    {
        free(rev);
        return STATUS_MEMORY_ERROR;
    }
    for (size_t k = 0; k < n; k++)
        res[k] = rev[n - 1 - k];
    res[n] = '\0';

    free(rev);
    *out = res;
    return STATUS_OK;
}

status_t sum_in_base(unsigned base, size_t count, char **out, ...)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (base < MIN_BASE || base > MAX_BASE)
        return STATUS_INVALID_ARGUMENT;

    char *res = malloc(2);
    if (res == NULL)
        return STATUS_MEMORY_ERROR;
    res[0] = '0';
    res[1] = '\0';

    va_list args;
    va_start(args, out);

    for (size_t k = 0; k < count; k++)
    {
        const char *num = va_arg(args, const char *);
        char *next = NULL;
        status_t st = add_big(res, num, base, &next);
        free(res);
        if (st != STATUS_OK)
        {
            va_end(args);
            return st;
        }
        res = next;
    }

    va_end(args);
    *out = res;
    return STATUS_OK;
}

static void show(const char *title, status_t st, char *res)
{
    printf("%s -> ", title);
    if (st != STATUS_OK)
        print_error(st);
    else
        printf("%s\n", res);
    free(res);
}

int main(void)
{
    char *res = NULL;
    status_t st;

    st = sum_in_base(16, 3, &res, "FF", "01", "0A");
    show("base 16: FF + 01 + 0A", st, res);

    res = NULL;
    st = sum_in_base(2, 2, &res, "1", "1");
    show("base 2: 1 + 1", st, res);

    res = NULL;
    st = sum_in_base(2, 2, &res, "1011", "101");
    show("base 2: 1011 + 101", st, res);

    res = NULL;
    st = sum_in_base(10, 2, &res, "999", "1");
    show("base 10: 999 + 1", st, res);

    res = NULL;
    st = sum_in_base(36, 2, &res, "Z", "1");
    show("base 36: Z + 1", st, res);

    res = NULL;
    st = sum_in_base(10, 2, &res, "000", "00");
    show("base 10: 000 + 00", st, res);

    res = NULL;
    st = sum_in_base(10, 3, &res,
                     "99999999999999999999999999999999999999999999",
                     "1",
                     "123456789012345678901234567890");
    show("base 10, длинные числа", st, res);

    res = NULL;
    st = sum_in_base(10, 0, &res);
    show("base 10, count = 0", st, res);

    res = NULL;
    st = sum_in_base(1, 1, &res, "1");
    show("ошибка: base = 1", st, res);

    res = NULL;
    st = sum_in_base(37, 1, &res, "1");
    show("ошибка: base = 37", st, res);

    res = NULL;
    st = sum_in_base(2, 2, &res, "101", "12");
    show("ошибка: цифра 2 в двоичной системе", st, res);

    res = NULL;
    st = sum_in_base(10, 2, &res, "12", "");
    show("ошибка: пустая строка", st, res);

    return 0;
}