#ifndef OVERPRINTF_H
#define OVERPRINTF_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "utils.h"

typedef enum
{
    OUTPUT_FILE,
    OUTPUT_STRING
} output_mode_t;

typedef struct
{
    output_mode_t mode;
    FILE *stream;
    char *buffer;
    size_t pos;
    size_t capacity;
} output_context_t;

static void output_char(output_context_t *ctx, char c)
{
    if (ctx->mode == OUTPUT_FILE)
    {
        fputc(c, ctx->stream);
    }
    else
    {
        if (ctx->pos < ctx->capacity - 1)
        {
            ctx->buffer[ctx->pos++] = c;
        }
    }
}

static void output_string(output_context_t *ctx, const char *s)
{
    while (*s)
    {
        output_char(ctx, *s++);
    }
}

static void format_roman(output_context_t *ctx, int value)
{
    if (value <= 0 || value > 3999)
    {
        output_string(ctx, "N/A");
        return;
    }

    const int values[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
    const char *symbols[] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};

    for (int i = 0; i < 13; i++)
    {
        while (value >= values[i])
        {
            output_string(ctx, symbols[i]);
            value -= values[i];
        }
    }
}

static void format_zeckendorf(output_context_t *ctx, unsigned int value)
{
    if (value == 0)
    {
        output_string(ctx, "1");
        return;
    }

    unsigned int fibs[40];
    int count = 0;
    fibs[0] = 1;
    fibs[1] = 2;
    count = 2;

    while (count < 40 && fibs[count - 1] <= value)
    {
        fibs[count] = fibs[count - 1] + fibs[count - 2];
        count++;
    }

    int bits[40] = {0};
    int bit_count = 0;

    for (int i = count - 1; i >= 0; i--)
    {
        if (value >= fibs[i])
        {
            bits[i] = 1;
            value -= fibs[i];
            if (i >= bit_count)
                bit_count = i + 1;
        }
    }

    for (int i = 0; i < bit_count; i++)
    {
        output_char(ctx, bits[i] ? '1' : '0');
    }
    output_char(ctx, '1');
}

static void format_custom_base(output_context_t *ctx, int value, int base, bool uppercase)
{
    if (base < 2 || base > 36)
        base = 10;

    if (value == 0)
    {
        output_char(ctx, '0');
        return;
    }

    char buf[65];
    int pos = 0;
    bool negative = false;
    unsigned int val;

    if (value < 0)
    {
        negative = true;
        val = (unsigned int)(-(value + 1)) + 1;
    }
    else
    {
        val = (unsigned int)value;
    }

    while (val > 0)
    {
        int digit = val % base;
        buf[pos++] = (digit < 10) ? ('0' + digit) : ((uppercase ? 'A' : 'a') + digit - 10);
        val /= base;
    }

    if (negative)
        output_char(ctx, '-');
    for (int i = pos - 1; i >= 0; i--)
    {
        output_char(ctx, buf[i]);
    }
}

static void format_to_decimal(output_context_t *ctx, const char *str, int base, bool uppercase)
{
    if (!str)
        return;
    if (base < 2 || base > 36)
        base = 10;

    long long result = 0;
    bool negative = false;

    if (*str == '-')
    {
        negative = true;
        str++;
    }
    else if (*str == '+')
    {
        str++;
    }

    while (*str)
    {
        char c = *str;
        int digit;

        if (uppercase && c >= 'a' && c <= 'z')
            c = c - 'a' + 'A';
        if (!uppercase && c >= 'A' && c <= 'Z')
            c = c - 'A' + 'a';

        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'a' && c <= 'z')
            digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z')
            digit = c - 'A' + 10;
        else
            break;

        if (digit >= base)
            break;
        result = result * base + digit;
        str++;
    }

    char buf[65];
    int pos = 0;

    if (result == 0)
    {
        output_char(ctx, '0');
        return;
    }

    long long val = negative ? -result : result;
    if (val < 0)
    {
        output_char(ctx, '-');
        val = -val;
    }

    while (val > 0)
    {
        buf[pos++] = '0' + (val % 10);
        val /= 10;
    }

    for (int i = pos - 1; i >= 0; i--)
    {
        output_char(ctx, buf[i]);
    }
}

static void format_memory_dump(output_context_t *ctx, const void *ptr, size_t size)
{
    const unsigned char *bytes = (const unsigned char *)ptr;

    for (size_t i = 0; i < size; i++)
    {
        if (i > 0)
            output_char(ctx, ' ');

        unsigned char byte = bytes[i];
        for (int bit = 7; bit >= 0; bit--)
        {
            output_char(ctx, (byte & (1 << bit)) ? '1' : '0');
        }
    }
}

static status_t parse_and_format(output_context_t *ctx, const char *format, va_list args)
{
    if (!ctx || !format)
        return STATUS_NULL_POINTER;

    while (*format)
    {
        if (*format != '%')
        {
            output_char(ctx, *format++);
            continue;
        }

        format++;
        char spec = *format++;

        if (spec == 'R' && *format == 'o')
        {
            int val = va_arg(args, int);
            format_roman(ctx, val);
            format++;
        }
        else if (spec == 'Z' && *format == 'r')
        {
            unsigned int val = va_arg(args, unsigned int);
            format_zeckendorf(ctx, val);
            format++;
        }
        else if (spec == 'C' && (*format == 'v' || *format == 'V'))
        {
            int val = va_arg(args, int);
            int base = va_arg(args, int);
            format_custom_base(ctx, val, base, *format == 'V');
            format++;
        }
        else if (spec == 't' && *format == 'o')
        {
            const char *str = va_arg(args, const char *);
            int base = va_arg(args, int);
            format_to_decimal(ctx, str, base, false);
            format++;
        }
        else if (spec == 'T' && *format == 'O')
        {
            const char *str = va_arg(args, const char *);
            int base = va_arg(args, int);
            format_to_decimal(ctx, str, base, true);
            format++;
        }
        else if (spec == 'm')
        {
            if (*format == 'i')
            {
                int val = va_arg(args, int);
                format_memory_dump(ctx, &val, sizeof(int));
                format++;
            }
            else if (*format == 'u')
            {
                unsigned int val = va_arg(args, unsigned int);
                format_memory_dump(ctx, &val, sizeof(unsigned int));
                format++;
            }
            else if (*format == 'd')
            {
                double val = va_arg(args, double);
                format_memory_dump(ctx, &val, sizeof(double));
                format++;
            }
            else if (*format == 'f')
            {
                float val = (float)va_arg(args, double);
                format_memory_dump(ctx, &val, sizeof(float));
                format++;
            }
        }
        else
        {
            char temp[1024];
            int len = 0;

            switch (spec)
            {
            case 'd':
            case 'i':
            {
                int val = va_arg(args, int);
                len = snprintf(temp, sizeof(temp), "%d", val);
                break;
            }
            case 'u':
            {
                unsigned int val = va_arg(args, unsigned int);
                len = snprintf(temp, sizeof(temp), "%u", val);
                break;
            }
            case 'f':
            case 'e':
            case 'g':
            {
                double val = va_arg(args, double);
                len = snprintf(temp, sizeof(temp), "%f", val);
                break;
            }
            case 's':
            {
                char *val = va_arg(args, char *);
                len = snprintf(temp, sizeof(temp), "%s", val);
                break;
            }
            case 'c':
            {
                int val = va_arg(args, int);
                len = snprintf(temp, sizeof(temp), "%c", val);
                break;
            }
            case 'x':
            case 'X':
            {
                unsigned int val = va_arg(args, unsigned int);
                len = snprintf(temp, sizeof(temp), spec == 'x' ? "%x" : "%X", val);
                break;
            }
            case 'p':
            {
                void *val = va_arg(args, void *);
                len = snprintf(temp, sizeof(temp), "%p", val);
                break;
            }
            case '%':
            {
                temp[0] = '%';
                temp[1] = '\0';
                len = 1;
                break;
            }
            default:
                break;
            }

            if (len > 0)
            {
                output_string(ctx, temp);
            }
        }
    }

    if (ctx->mode == OUTPUT_STRING)
    {
        ctx->buffer[ctx->pos] = '\0';
    }

    return STATUS_OK;
}

status_t overfprintf(FILE *stream, const char *format, ...)
{
    if (!stream || !format)
        return STATUS_NULL_POINTER;

    output_context_t ctx = {
        .mode = OUTPUT_FILE,
        .stream = stream,
        .buffer = NULL,
        .pos = 0,
        .capacity = 0};

    va_list args;
    va_start(args, format);
    status_t st = parse_and_format(&ctx, format, args);
    va_end(args);

    return st;
}

status_t oversprintf(char *str, const char *format, ...)
{
    if (!str || !format)
        return STATUS_NULL_POINTER;

    output_context_t ctx = {
        .mode = OUTPUT_STRING,
        .stream = NULL,
        .buffer = str,
        .pos = 0,
        .capacity = 4096};

    va_list args;
    va_start(args, format);
    status_t st = parse_and_format(&ctx, format, args);
    va_end(args);

    return st;
}

#endif