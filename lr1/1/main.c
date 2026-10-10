#include <stdio.h>
#include <math.h>
#include <stdbool.h>

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_OUT_OF_RANGE,
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
    default:
        *msg = "неизвестная ошибка";
        break;
    }
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
    for (int i = 2; i < (int)(sqrt(x) + 1); i++)
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

status_t pow_table(const int x, size_t res[10][10])
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
        for (size_t j = 0; j < (size_t)x; j++)
        {
            res[i][j] = (size_t)pow(i + 1, j + 1);
        }
    }
    return STATUS_OK;
}

status_t summ(const int x, size_t *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (x < 1)
    {
        return STATUS_OUT_OF_RANGE;
    }

    size_t result = 0;
    for (size_t i = 1; i <= (size_t)x; i++)
    {
        result += i;
    }

    *res = result;
    return STATUS_OK;
}

status_t fact(const int x, size_t *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (x > 20 || x < 0)
    {
        return STATUS_OUT_OF_RANGE;
    }

    size_t result = 1;
    for (size_t i = 2; i <= (size_t)x; i++)
    {
        result *= i;
    }

    *res = result;
    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Неверный формат. Верный: [программа] [число] [флаг]\n");
        return 1;
    }

    int x;
    char extra; // для отсечения строк вида 10abc или 5.4
    if (sscanf(argv[1], "%d%c", &x, &extra) != 1)
    {
        fprintf(stderr, "Второй аргумент должен быть числом\n");
        return 1;
    }

    if ((argv[2][0] != '-' && argv[2][0] != '/') ||
        argv[2][1] == '\0' || argv[2][2] != '\0')
    {
        fprintf(stderr, "Флаг должен иметь формат -n или /n\n");
        return 1;
    }
    char flag = argv[2][1];

    status_t st;

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
            fprintf(stderr, "Ошибка: %s\n", msg);
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
            fprintf(stderr, "Ошибка: %s\n", msg);
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
            fprintf(stderr, "Ошибка: %s\n", msg);
            return 1;
        }
        for (int i = count - 1; i >= 0; i--)
        {
            printf("%c ", res[i]);
        }
        printf("\n");
        break;
    }
    case 'e':
    {
        size_t res[10][10];
        st = pow_table(x, res);
        if (st == STATUS_OUT_OF_RANGE)
        {
            fprintf(stderr, "Число должно быть от 1 до 10\n");
            return 1;
        }
        else if (st != STATUS_OK)
        {
            const char *msg;
            status_message(st, &msg);
            fprintf(stderr, "Ошибка: %s\n", msg);
            return 1;
        }
        for (int i = 0; i < 10; i++)
        {
            for (int j = 0; j < x; j++)
            {
                printf("%*zu ", x + 1, res[i][j]);
            }
            printf("\n");
        }
        break;
    }
    case 'a':
    {
        size_t res;
        st = summ(x, &res);
        if (st != STATUS_OK)
        {
            const char *msg;
            status_message(st, &msg);
            fprintf(stderr, "Ошибка: %s\n", msg);
            return 1;
        }
        printf("%zu\n", res);
        break;
    }
    case 'f':
    {
        size_t res;
        st = fact(x, &res);
        if (st != STATUS_OK)
        {
            const char *msg;
            status_message(st, &msg);
            fprintf(stderr, "Ошибка: %s\n", msg);
            return 1;
        }
        printf("%zu\n", res);
        break;
    }
    default:
    {
        fprintf(stderr, "Флаги: h, p, s, e, a, f\n");
        return 1;
    }
    }
    return 0;
}