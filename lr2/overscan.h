#ifndef OVERSCAN_H
#define OVERSCAN_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

typedef enum
{
    STATUS_OK,
    STATUS_ERR_NULL_ARG,
    STATUS_ERR_PARCE,
    STATUS_ERR_RANGE,
    STATUS_ERR_IO
} status_t;

typedef enum
{
    INPUT_FILE,
    INPUT_STRING
} input_mode_t;

typedef struct
{
    input_mode_t mode;
    FILE *stream;
    const char *buffer;
    size_t pos;
} input_context_t;

static int input_getc(input_context_t *ctx)
{
    if (ctx->mode == INPUT_FILE)
    {
        return fgetc(ctx->stream);
    }
    else
    {
        unsigned char c = (unsigned char)ctx->buffer[ctx->pos];
        if (c == '\0')
            return EOF;
        ctx->pos++;
        return c;
    }
}

static void input_ungetc(input_context_t *ctx, int c)
{
    if (c == EOF)
        return;
    if (ctx->mode == INPUT_FILE)
    {
        ungetc(c, ctx->stream);
    }
    else
    {
        if (ctx->pos > 0)
            ctx->pos--;
    }
}

static int roman_char_val(char c)
{
    switch (toupper((unsigned char)c))
    {
    case 'I':
        return 1;
    case 'V':
        return 5;
    case 'X':
        return 10;
    case 'L':
        return 50;
    case 'C':
        return 100;
    case 'D':
        return 500;
    case 'M':
        return 1000;
    default:
        return -1;
    }
}

static status_t parse_roman(const char *str, int *out)
{
    if (!str || !out || !*str)
        return STATUS_ERR_PARCE;

    int total = 0;
    int prev_val = 0;
    for (int i = (int)strlen(str) - 1; i >= 0; i--)
    {
        int val = roman_char_val(str[i]);
        if (val < 0)
            return STATUS_ERR_PARCE;
        if (val < prev_val)
            total -= val;
        else
            total += val;
        prev_val = val;
    }
    *out = total;
    return STATUS_OK;
}

static status_t parse_zeckendorf(const char *str, unsigned int *out)
{
    if (!str || !out || !*str)
        return STATUS_ERR_PARCE;

    size_t len = strlen(str);
    if (str[len - 1] != '1')
        return STATUS_ERR_PARCE;

    unsigned int result = 0;
    unsigned int fib_a = 1, fib_b = 2;

    for (int i = 0; i < (int)len - 1; i++)
    {
        if (str[i] == '1')
        {
            result += fib_a;
        }
        else if (str[i] != '0')
        {
            return STATUS_ERR_PARCE;
        }
        unsigned int next_fib = fib_a + fib_b;
        fib_a = fib_b;
        fib_b = next_fib;
    }
    *out = result;
    return STATUS_OK;
}

static status_t parse_custom_base(const char *str, int base, bool uppercase, int *out)
{
    if (!str || !out || base < 2 || base > 36)
        return STATUS_ERR_PARCE;

    char *endptr;
    long val = strtol(str, &endptr, base);

    if (endptr == str || (*endptr != '\0' && !isspace((unsigned char)*endptr)))
    {
        return STATUS_ERR_PARCE;
    }

    if (uppercase)
    {
        for (const char *p = str; p < endptr; p++)
        {
            if (*p >= 'a' && *p <= 'z')
                return STATUS_ERR_PARCE;
        }
    }
    else
    {
        for (const char *p = str; p < endptr; p++)
        {
            if (*p >= 'A' && *p <= 'Z')
                return STATUS_ERR_PARCE;
        }
    }

    *out = (int)val;
    return STATUS_OK;
}

static status_t over_vscan(input_context_t *ctx, const char *format, va_list ap)
{
    if (!ctx || !format)
        return STATUS_ERR_NULL_ARG;

    const char *p = format;

    while (*p)
    {
        if (isspace((unsigned char)*p))
        {
            int c;
            while ((c = input_getc(ctx)) != EOF && isspace((unsigned char)c))
                ;
            input_ungetc(ctx, c);
            p++;
            continue;
        }

        if (*p != '%')
        {
            int c = input_getc(ctx);
            if (c == EOF || c != (unsigned char)*p)
            {
                input_ungetc(ctx, c);
                return STATUS_ERR_PARCE;
            }
            p++;
            continue;
        }

        p++;

        if (*p == 'R' && *(p + 1) == 'o')
        {
            p += 2;
            int *out = va_arg(ap, int *);
            if (!out)
                return STATUS_ERR_NULL_ARG;

            int c;
            while ((c = input_getc(ctx)) != EOF && isspace((unsigned char)c))
                ;
            input_ungetc(ctx, c);

            char tmp[64];
            size_t len = 0;
            while ((c = input_getc(ctx)) != EOF && len + 1 < sizeof(tmp) && roman_char_val((char)c) >= 0)
            {
                tmp[len++] = (char)c;
            }
            input_ungetc(ctx, c);

            if (len == 0)
                return STATUS_ERR_PARCE;
            tmp[len] = '\0';

            if (parse_roman(tmp, out) != STATUS_OK)
                return STATUS_ERR_PARCE;
            continue;
        }

        if (*p == 'Z' && *(p + 1) == 'r')
        {
            p += 2;
            unsigned int *out = va_arg(ap, unsigned int *);
            if (!out)
                return STATUS_ERR_NULL_ARG;

            int c;
            while ((c = input_getc(ctx)) != EOF && isspace((unsigned char)c))
                ;
            input_ungetc(ctx, c);

            char tmp[128];
            size_t len = 0;
            while ((c = input_getc(ctx)) != EOF && len + 1 < sizeof(tmp) && (c == '0' || c == '1'))
            {
                tmp[len++] = (char)c;
            }
            input_ungetc(ctx, c);

            if (len == 0)
                return STATUS_ERR_PARCE;
            tmp[len] = '\0';

            if (parse_zeckendorf(tmp, out) != STATUS_OK)
                return STATUS_ERR_PARCE;
            continue;
        }

        if (*p == 'C' && (*(p + 1) == 'v' || *(p + 1) == 'V'))
        {
            p += 2;
            int *out = va_arg(ap, int *);
            int base = va_arg(ap, int);
            if (!out)
                return STATUS_ERR_NULL_ARG;

            bool uppercase = (*(p - 1) == 'V');

            int c;
            while ((c = input_getc(ctx)) != EOF && isspace((unsigned char)c))
                ;
            input_ungetc(ctx, c);

            char tmp[128];
            size_t len = 0;
            while ((c = input_getc(ctx)) != EOF && len + 1 < sizeof(tmp))
            {
                bool is_digit = (c >= '0' && c <= '9');
                bool is_lower = (c >= 'a' && c <= 'z');
                bool is_upper = (c >= 'A' && c <= 'Z');
                bool is_sign = (c == '-' || c == '+') && len == 0;

                if (is_digit || (uppercase && is_upper) || (!uppercase && is_lower) || is_sign)
                {
                    tmp[len++] = (char)c;
                }
                else
                {
                    break;
                }
            }
            input_ungetc(ctx, c);

            if (len == 0)
                return STATUS_ERR_PARCE;
            tmp[len] = '\0';

            if (parse_custom_base(tmp, base, uppercase, out) != STATUS_OK)
                return STATUS_ERR_PARCE;
            continue;
        }

        char spec[32];
        int si = 0;
        spec[si++] = '%';

        while (*p && *p != '%')
        {
            if (isalpha((unsigned char)*p) && strchr("diuoxXfFeEgGaAcspn", *p) != NULL)
                break;
            if (si < (int)sizeof(spec) - 2)
                spec[si++] = *p;
            p++;
        }

        if (*p == '\0')
            return STATUS_ERR_PARCE;
        char type = *p++;
        spec[si++] = type;
        spec[si] = '\0';

        if (type == 'c')
        {
            int c = input_getc(ctx);
            if (c == EOF)
                return STATUS_ERR_PARCE;
            char *out = va_arg(ap, char *);
            if (!out)
                return STATUS_ERR_NULL_ARG;
            *out = (char)c;
            continue;
        }

        int c;
        while ((c = input_getc(ctx)) != EOF && isspace((unsigned char)c))
            ;
        if (c == EOF)
            return STATUS_ERR_PARCE;
        input_ungetc(ctx, c);

        char tmp[256];
        size_t len = 0;
        while ((c = input_getc(ctx)) != EOF && len + 1 < sizeof(tmp) && !isspace((unsigned char)c))
        {
            tmp[len++] = (char)c;
        }
        input_ungetc(ctx, c);

        if (len == 0)
            return STATUS_ERR_PARCE;
        tmp[len] = '\0';

        char spec_n[40];
        snprintf(spec_n, sizeof(spec_n), "%s%%n", spec);
        int consumed = 0, matched = 0;

        switch (type)
        {
        case 'd':
        case 'i':
        {
            int *o = va_arg(ap, int *);
            if (!o)
                return STATUS_ERR_NULL_ARG;
            matched = sscanf(tmp, spec_n, o, &consumed);
            break;
        }
        case 'u':
        case 'x':
        case 'X':
        case 'o':
        {
            unsigned int *o = va_arg(ap, unsigned int *);
            if (!o)
                return STATUS_ERR_NULL_ARG;
            matched = sscanf(tmp, spec_n, o, &consumed);
            break;
        }
        case 's':
        {
            char *o = va_arg(ap, char *);
            if (!o)
                return STATUS_ERR_NULL_ARG;
            matched = sscanf(tmp, spec_n, o, &consumed);
            break;
        }
        case 'f':
        case 'e':
        case 'g':
        case 'a':
        {
            if (strchr(spec, 'l') != NULL)
            {
                double *o = va_arg(ap, double *);
                if (!o)
                    return STATUS_ERR_NULL_ARG;
                matched = sscanf(tmp, spec_n, o, &consumed);
            }
            else
            {
                float *o = va_arg(ap, float *);
                if (!o)
                    return STATUS_ERR_NULL_ARG;
                matched = sscanf(tmp, spec_n, o, &consumed);
            }
            break;
        }
        case 'p':
        {
            void **o = va_arg(ap, void **);
            if (!o)
                return STATUS_ERR_NULL_ARG;
            matched = sscanf(tmp, spec_n, o, &consumed);
            break;
        }
        case '%':
        {
            if (tmp[0] != '%')
                return STATUS_ERR_PARCE;
            matched = 1;
            consumed = 1;
            break;
        }
        default:
            return STATUS_ERR_PARCE;
        }

        if (matched != 1)
            return STATUS_ERR_PARCE;

        for (size_t i = len; i > (size_t)consumed; i--)
        {
            input_ungetc(ctx, (unsigned char)tmp[i - 1]);
        }
    }

    return STATUS_OK;
}

status_t overfscanf(FILE *stream, const char *format, ...)
{
    if (!stream || !format)
        return STATUS_ERR_NULL_ARG;

    input_context_t ctx = {
        .mode = INPUT_FILE,
        .stream = stream,
        .buffer = NULL,
        .pos = 0};

    va_list ap;
    va_start(ap, format);
    status_t rc = over_vscan(&ctx, format, ap);
    va_end(ap);

    return rc;
}

status_t oversscanf(const char *str, const char *format, ...)
{
    if (!str || !format)
        return STATUS_ERR_NULL_ARG;

    input_context_t ctx = {
        .mode = INPUT_STRING,
        .stream = NULL,
        .buffer = str,
        .pos = 0};

    va_list ap;
    va_start(ap, format);
    status_t rc = over_vscan(&ctx, format, ap);
    va_end(ap);

    return rc;
}

#endif