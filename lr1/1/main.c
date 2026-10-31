#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>

typedef unsigned long long ull;

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_OUT_OF_RANGE,
    STATUS_INVALID_NUMBER,
    STATUS_OVERFLOW,
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
    case STATUS_OUT_OF_RANGE:
        *msg = "число вне допустимого диапазона";
        break;
    case STATUS_NULL_POINTER:
        *msg = "нулевой указатель";
        break;
    case STATUS_INVALID_NUMBER:
        *msg = "аргумент не является целым числом";
        break;
    case STATUS_OVERFLOW:
        *msg = "значение числа слишком велико";
        break;
    default:
        *msg = "неизвестная ошибка";
        break;
    }
    return STATUS_OK;
}

status_t parse_int(const char *s, int *out)
{
    if (s == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    char *end = NULL;
    long long v = strtoll(s, &end, 10);

    if (end == s || *end != '\0')
        return STATUS_INVALID_NUMBER;

    if (v < INT_MIN || v > INT_MAX)
        return STATUS_OUT_OF_RANGE;

    *out = (int)v;
    return STATUS_OK;
}

status_t find_multiples(const int x, int *res, size_t *size)
{
    if (res == NULL || size == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (x < 1)
    {
        return STATUS_OUT_OF_RANGE;
    }

    size_t i = 0;
    for (int n = 1; n <= 100; n++)
    {
        if (n % x == 0)
        {
            res[i++] = n;
        }
    }

    *size = i;
    return STATUS_OK;
}

status_t check_prost(const int x, bool *is_prost)
{
    if (is_prost == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (x < 2)
    {
        return STATUS_OUT_OF_RANGE;
    }

    bool result = true;
    for (int i = 2; i * i <= x; i++)
    {
        if (x % i == 0)
        {
            result = false;
            break;
        }
    }

    *is_prost = result;
    return STATUS_OK;
}

status_t x_in_hex(const int x, char *res, size_t *size)
{
    if (res == NULL || size == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (x < 0)
    {
        return STATUS_OUT_OF_RANGE;
    }

    size_t i = 0;
    int temp = x;

    do
    {
        int num = temp % 16;
        char c;
        if (num > 9)
        {
            c = 'A' + (num - 10);
        }
        else
        {
            c = '0' + num;
        }
        res[i++] = c;
        temp /= 16;
    } while (temp > 0);

    *size = i;
    return STATUS_OK;
}

status_t pow_table(const int x, ull res[10][10])
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (x > 10 || x < 1)
    {
        return STATUS_OUT_OF_RANGE;
    }

    for (size_t i = 0; i < 10; i++)
    {
        ull power = 1;
        for (size_t j = 0; j < (size_t)x; j++)
        {
            if (power > ULLONG_MAX / (i + 1))
            {
                return STATUS_OVERFLOW;
            }
            power *= i + 1;
            res[i][j] = power;
        }
    }
    return STATUS_OK;
}

status_t summ(const int x, ull *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (x < 1)
    {
        return STATUS_OUT_OF_RANGE;
    }

    ull a = (ull)x;
    ull b = (ull)x + 1;
    if (a % 2 == 0)
    {
        a /= 2;
    }
    else
    {
        b /= 2;
    }
    if (a > ULLONG_MAX / b)
    {
        return STATUS_OVERFLOW;
    }

    *res = a * b;
    return STATUS_OK;
}

status_t fact(const int x, ull *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (x < 0)
    {
        return STATUS_OUT_OF_RANGE;
    }

    ull result = 1;
    for (ull i = 2; i <= (ull)x; i++)
    {
        if (result > ULLONG_MAX / i)
        {
            return STATUS_OVERFLOW;
        }
        result *= i;
    }

    *res = result;
    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Неверный формат. Верный: [программа] [число] [флаг]\n");
        return 1;
    }

    int x;
    status_t st = parse_int(argv[1], &x);
    if (st != STATUS_OK)
    {
        const char *msg;
        status_message(st, &msg);
        printf("Ошибка: %s\n", msg);
        return 1;
    }

    if ((argv[2][0] != '-' && argv[2][0] != '/') ||
        argv[2][1] == '\0' || argv[2][2] != '\0')
    {
        printf("Флаг должен иметь формат -n или /n\n");
        return 1;
    }
    char flag = argv[2][1];

    switch (flag)
    {
    case 'h':
    {
        int res[100];
        size_t count;
        st = find_multiples(x, res, &count);
        if (st != STATUS_OK)
        {
            const char *msg;
            status_message(st, &msg);
            printf("Ошибка: %s\n", msg);
            return 1;
        }
        if (count == 0)
        {
            printf("Нет чисел от 1 до 100, кратных '%d'", x);
        }
        for (size_t i = 0; i < count; i++)
        {
            printf("%d ", res[i]);
        }
        printf("\n");
        break;
    }
    case 'p':
    {
        bool is_prost;
        st = check_prost(x, &is_prost);
        if (st == STATUS_OUT_OF_RANGE)
        {
            printf("'%d' не является ни простым, ни составным числом\n", x);
            break;
        }
        else if (st != STATUS_OK)
        {
            const char *msg;
            status_message(st, &msg);
            printf("Ошибка: %s\n", msg);
            return 1;
        }
        if (is_prost)
        {
            printf("'%d' является простым числом\n", x);
        }
        else
        {
            printf("'%d' является составным числом\n", x);
        }
        break;
    }
    case 's':
    {
        char res[100];
        size_t count;
        st = x_in_hex(x, res, &count);
        if (st != STATUS_OK)
        {
            const char *msg;
            status_message(st, &msg);
            printf("Ошибка: %s\n", msg);
            return 1;
        }
        for (size_t i = count; i > 0; i--)
        {
            printf("%c ", res[i - 1]);
        }
        printf("\n");
        break;
    }
    case 'e':
    {
        ull res[10][10];
        st = pow_table(x, res);
        if (st == STATUS_OUT_OF_RANGE)
        {
            printf("Число должно быть от 1 до 10\n");
            return 1;
        }
        else if (st != STATUS_OK)
        {
            const char *msg;
            status_message(st, &msg);
            printf("Ошибка: %s\n", msg);
            return 1;
        }
        for (int i = 0; i < 10; i++)
        {
            for (int j = 0; j < x; j++)
            {
                printf("%*llu ", x + 1, res[i][j]);
            }
            printf("\n");
        }
        break;
    }
    case 'a':
    {
        ull res;
        st = summ(x, &res);
        if (st != STATUS_OK)
        {
            const char *msg;
            status_message(st, &msg);
            printf("Ошибка: %s\n", msg);
            return 1;
        }
        printf("%llu\n", res);
        break;
    }
    case 'f':
    {
        ull res;
        st = fact(x, &res);
        if (st != STATUS_OK)
        {
            const char *msg;
            status_message(st, &msg);
            printf("Ошибка: %s\n", msg);
            return 1;
        }
        printf("%llu\n", res);
        break;
    }
    default:
    {
        printf("Флаги: h, p, s, e, a, f\n");
        return 1;
    }
    }
    return 0;
}