#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#define START_CAPACITY 16
#define MAX_NUMBER_STR 70

typedef unsigned long long ull;

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_OUT_OF_RANGE,
    STATUS_INVALID_NUMBER,
    STATUS_OVERFLOW,
    STATUS_MEMORY_ERROR,
    STATUS_IO_ERROR,
    STATUS_INVALID_NUMBER_OR_RANGE,
} status_t;

status_t status_message(const status_t status, const char **msg)
{
    if (msg == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    switch (status)
    {
    case STATUS_OK:
        *msg = "";
        break;
    case STATUS_NULL_POINTER:
        *msg = "нулевой указатель";
        break;
    case STATUS_OUT_OF_RANGE:
        *msg = "значение вне допустимого диапазона";
        break;
    case STATUS_INVALID_NUMBER:
        *msg = "некорректная запись числа (цифры 0-9 и прописные A-Z меньше основания)";
        break;
    case STATUS_OVERFLOW:
        *msg = "переполнение (число или сумма не помещаются в long long)";
        break;
    case STATUS_MEMORY_ERROR:
        *msg = "не удалось выделить память";
        break;
    case STATUS_IO_ERROR:
        *msg = "ошибка ввода-вывода";
        break;
    case STATUS_INVALID_NUMBER_OR_RANGE:
        *msg = "некорректная запись числа или значение вне допустимого диапазона";
        break;
    default:
        *msg = "неизвестная ошибка";
        break;
    }
    return STATUS_OK;
}

status_t print_error(const status_t st)
{
    const char *msg;
    status_t rc = status_message(st, &msg);
    if (rc != STATUS_OK)
    {
        return rc;
    }
    printf("Ошибка: %s\n", msg);
    return STATUS_OK;
}

status_t is_separator(const int c, bool *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (c < 0 || c > 255)
    {
        return STATUS_OUT_OF_RANGE;
    }

    *res = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    return STATUS_OK;
}

status_t read_token(FILE *in, char **token, size_t *len)
{
    if (in == NULL || token == NULL || len == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    *token = NULL;

    size_t capacity = START_CAPACITY;
    char *buf = malloc(capacity);
    if (buf == NULL)
    {
        return STATUS_MEMORY_ERROR;
    }

    size_t n = 0;
    int c;
    while ((c = fgetc(in)) != EOF)
    {
        bool sep;
        status_t st = is_separator(c, &sep);
        if (st != STATUS_OK)
        {
            free(buf);
            return st;
        }
        if (sep)
        {
            if (n == 0)
            {
                continue;
            }
            break;
        }

        if (n + 1 >= capacity)
        {
            if (capacity > SIZE_MAX / 2)
            {
                free(buf);
                return STATUS_MEMORY_ERROR;
            }
            capacity *= 2;
            char *bigger = realloc(buf, capacity);
            if (bigger == NULL)
            {
                free(buf);
                return STATUS_MEMORY_ERROR;
            }
            buf = bigger;
        }
        buf[n++] = (char)c;
    }
    if (ferror(in))
    {
        free(buf);
        return STATUS_IO_ERROR;
    }

    buf[n] = '\0';
    *token = buf;
    *len = n;
    return STATUS_OK;
}

status_t parse_base(const char *token, unsigned *base)
{
    if (token == NULL || base == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (token[0] == '\0' || strlen(token) > 2)
    {
        return STATUS_INVALID_NUMBER_OR_RANGE;
    }

    unsigned value = 0;
    for (size_t i = 0; token[i] != '\0'; i++)
    {
        if (token[i] < '0' || token[i] > '9')
        {
            return STATUS_INVALID_NUMBER;
        }
        value = value * 10 + (unsigned)(token[i] - '0');
    }
    if (value < 2 || value > 36)
    {
        return STATUS_OUT_OF_RANGE;
    }

    *base = value;
    return STATUS_OK;
}

status_t digit_value(const char c, unsigned *value)
{
    if (value == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    if (c >= '0' && c <= '9')
    {
        *value = (unsigned)(c - '0');
    }
    else if (c >= 'A' && c <= 'Z')
    {
        *value = (unsigned)(c - 'A') + 10;
    }
    else
    {
        return STATUS_INVALID_NUMBER;
    }
    return STATUS_OK;
}

status_t parse_number(const char *token, const unsigned base, long long *res)
{
    if (token == NULL || res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (base < 2 || base > 36)
    {
        return STATUS_OUT_OF_RANGE;
    }

    const bool negative = (token[0] == '-');
    const char *digits = negative ? token + 1 : token;
    if (digits[0] == '\0')
    {
        return STATUS_INVALID_NUMBER;
    }

    long long value = 0;
    for (size_t i = 0; digits[i] != '\0'; i++)
    {
        unsigned d;
        status_t st = digit_value(digits[i], &d);
        if (st != STATUS_OK)
        {
            return st;
        }
        if (d >= base)
        {
            return STATUS_INVALID_NUMBER;
        }

        if (value > (LLONG_MAX - (long long)d) / (long long)base)
        {
            return STATUS_OVERFLOW;
        }
        value = value * base + d;
    }

    *res = negative ? -value : value;
    return STATUS_OK;
}

status_t summ_checked(const long long a, const long long b, long long *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if ((b > 0 && a > LLONG_MAX - b) || (b < 0 && a < LLONG_MIN - b))
    {
        return STATUS_OVERFLOW;
    }

    *res = a + b;
    return STATUS_OK;
}

status_t module(const long long value, ull *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    *res = (value < 0) ? 0ULL - (ull)value : (ull)value;
    return STATUS_OK;
}

status_t number_to_base(const long long value, const unsigned base, char *buf,
                        const size_t size, size_t *len)
{
    if (buf == NULL || len == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (base < 2 || base > 36)
    {
        return STATUS_OUT_OF_RANGE;
    }

    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char tmp[64];
    ull rest;
    status_t st = module(value, &rest);
    if (st != STATUS_OK)
    {
        return st;
    }

    size_t n = 0;
    do
    {
        tmp[n++] = digits[rest % base];
        rest /= base;
    } while (rest > 0);

    const size_t sign = (value < 0) ? 1 : 0;
    if (sign + n + 1 > size)
    {
        return STATUS_OUT_OF_RANGE;
    }

    size_t pos = 0;
    if (sign == 1)
    {
        buf[pos++] = '-';
    }
    for (size_t i = n; i > 0; i--)
    {
        buf[pos++] = tmp[i - 1];
    }
    buf[pos] = '\0';

    *len = pos;
    return STATUS_OK;
}

status_t print_number(const char *title, const long long value, const unsigned base)
{
    if (title == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    if (printf("%s:\n", title) < 0)
    {
        return STATUS_IO_ERROR;
    }

    const unsigned bases[5] = {base, 9, 18, 27, 36};
    for (size_t i = 0; i < 5; i++)
    {
        char buf[MAX_NUMBER_STR];
        size_t len;
        status_t st = number_to_base(value, bases[i], buf, sizeof(buf), &len);
        if (st != STATUS_OK)
        {
            return st;
        }
        if (printf("  основание %2u: %s\n", bases[i], buf) < 0)
        {
            return STATUS_IO_ERROR;
        }
    }
    return STATUS_OK;
}

int main(void)
{
    char *token;
    size_t len;
    unsigned base = 0;

    printf("Введите основание системы счисления (2..36):\n");
    status_t st = read_token(stdin, &token, &len);
    if (st == STATUS_OK && len == 0)
    {
        st = STATUS_INVALID_NUMBER;
    }
    if (st == STATUS_OK)
    {
        st = parse_base(token, &base);
    }
    free(token);
    if (st != STATUS_OK)
    {
        print_error(st);
        return 1;
    }

    printf("Вводите числа через пробел или с новой строки, для окончания введите Stop:\n");

    size_t count = 0;
    long long sum = 0;
    long long max_value = 0;
    ull max_abs = 0;

    while (1)
    {
        st = read_token(stdin, &token, &len);
        if (st != STATUS_OK)
        {
            print_error(st);
            return 1;
        }
        if (len == 0 || strcmp(token, "Stop") == 0)
        {
            free(token);
            break;
        }

        long long value;
        ull abs_value;
        st = parse_number(token, base, &value);
        if (st == STATUS_OK)
        {
            st = summ_checked(sum, value, &sum);
        }
        if (st == STATUS_OK)
        {
            st = module(value, &abs_value);
        }
        if (st != STATUS_OK)
        {
            printf("Число '%.50s': ", token);
        }
        free(token);
        if (st != STATUS_OK)
        {
            print_error(st);
            return 1;
        }

        if (count == 0 || abs_value > max_abs)
        {
            max_value = value;
            max_abs = abs_value;
        }
        count++;
    }

    if (count == 0)
    {
        printf("Чисел не введено\n");
        return 0;
    }

    st = print_number("Максимальное по модулю", max_value, base);
    if (st == STATUS_OK)
    {
        st = print_number("Сумма", sum, base);
    }
    if (st != STATUS_OK)
    {
        print_error(st);
        return 1;
    }
    return 0;
}