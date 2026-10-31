#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_OUT_OF_RANGE,
    STATUS_INVALID_NUMBER,
    STATUS_INVALID_INT_NUMBER,
    STATUS_INVALID_DOUBLE_NUMBER,
} status_t;

typedef enum
{
    SOLUTION_NONE,
    SOLUTION_ONE,
    SOLUTION_TWO,
    SOLUTION_ANY,
} solution_t;

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
        *msg = "аргумент не является числом";
        break;
    case STATUS_INVALID_INT_NUMBER:
        *msg = "аргумент не является целым числом";
        break;
    case STATUS_INVALID_DOUBLE_NUMBER:
        *msg = "аргумент не является вещественным числом";
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

status_t parse_int(const char *s, int *out)
{
    if (s == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    char *end = NULL;
    long long v = strtoll(s, &end, 10);

    if (end == s || *end != '\0')
        return STATUS_INVALID_INT_NUMBER;

    if (v < INT_MIN || v > INT_MAX)
        return STATUS_OUT_OF_RANGE;

    *out = (int)v;
    return STATUS_OK;
}

status_t parse_double(const char *s, double *out)
{
    if (s == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    char *end = NULL;
    double v = strtod(s, &end);

    if (end == s || *end != '\0')
        return STATUS_INVALID_DOUBLE_NUMBER;

    if (isnan(v) || isinf(v))
        return STATUS_OUT_OF_RANGE;

    *out = v;
    return STATUS_OK;
}

status_t solve_quadratic(const double eps, const double a, const double b,
                         const double c, double roots[2], solution_t *kind)
{
    if (roots == NULL || kind == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    if (fabs(a) < eps)
    {
        if (fabs(b) < eps)
        {
            if (fabs(c) < eps)
                *kind = SOLUTION_ANY;
            else
                *kind = SOLUTION_NONE;
            return STATUS_OK;
        }
        roots[0] = -c / b + 0.0;
        *kind = SOLUTION_ONE;
        return STATUS_OK;
    }

    const double d = b * b - 4 * a * c;
    if (fabs(d) < eps)
    {
        roots[0] = -b / (2 * a) + 0.0;
        *kind = SOLUTION_ONE;
    }
    else if (d < 0)
    {
        *kind = SOLUTION_NONE;
    }
    else
    {
        const double sq = sqrt(d);
        roots[0] = (-b - sq) / (2 * a) + 0.0;
        roots[1] = (-b + sq) / (2 * a) + 0.0;
        *kind = SOLUTION_TWO;
    }
    return STATUS_OK;
}

status_t unique_permutations(const double eps, const double coef[3],
                             double res[6][3], size_t *n)
{
    if (coef == NULL || res == NULL || n == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (eps <= 0)
    {
        return STATUS_OUT_OF_RANGE;
    }

    size_t count = 0;
    for (size_t i = 0; i < 3; i++)
    {
        for (size_t j = 0; j < 3; j++)
        {
            for (size_t k = 0; k < 3; k++)
            {
                if (i == j || i == k || j == k)
                {
                    continue;
                }

                bool duplicate = false;
                for (size_t m = 0; m < count; m++)
                {
                    if (fabs(res[m][0] - coef[i]) < eps &&
                        fabs(res[m][1] - coef[j]) < eps &&
                        fabs(res[m][2] - coef[k]) < eps)
                    {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate)
                {
                    res[count][0] = coef[i];
                    res[count][1] = coef[j];
                    res[count][2] = coef[k];
                    count++;
                }
            }
        }
    }

    *n = count;
    return STATUS_OK;
}

status_t is_multi(const int a, const int b, bool *is_m)
{
    if (is_m == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (a == 0 || b == 0)
    {
        return STATUS_OUT_OF_RANGE;
    }
    if (a % b == 0)
    {
        *is_m = true;
    }
    else
    {
        *is_m = false;
    }
    return STATUS_OK;
}

status_t is_right_triangle(const double eps, const double a, const double b,
                           const double c, bool *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    if (a < eps || b < eps || c < eps)
    {
        *res = false;
        return STATUS_OK;
    }

    *res = fabs(a * a + b * b - c * c) < eps ||
           fabs(a * a + c * c - b * b) < eps ||
           fabs(b * b + c * c - a * a) < eps;
    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("Число аргументов должно быть больше 3\n");
        return 1;
    }

    char flag;
    if ((argv[1][0] != '-' && argv[1][0] != '/') ||
        (argv[1][1] == '\0' || argv[1][2] != '\0'))
    {
        printf("Флаг должен иметь формат -n или /n\n");
        return 1;
    }
    flag = argv[1][1];

    status_t st;

    switch (flag)
    {
    case 'q':
    {
        if (argc != 6)
        {
            printf("Число аргументов для q должно быть равно 6\n");
            return 1;
        }
        double e;
        double coef[3];
        if (((parse_double(argv[2], &e)) != STATUS_OK) || (e < 0.0))
        {
            printf("Третий аргумент Для q должен быть вещественным числом больше 0\n");
            return 1;
        }

        for (int i = 0; i < 3; i++)
        {
            if (parse_double(argv[3 + i], &coef[i]) != STATUS_OK)
            {
                printf("4, 5, 6 аргументы для q должны быть вещественными числами\n");
                return 1;
            }
        }
        double perms[6][3];
        size_t n;
        st = unique_permutations(e, coef, perms, &n);
        if (st != STATUS_OK)
        {
            print_error(st);
            return 1;
        }

        for (size_t i = 0; i < n; i++)
        {
            double roots[2];
            solution_t kind;
            st = solve_quadratic(e, perms[i][0], perms[i][1], perms[i][2],
                                 roots, &kind);
            if (st != STATUS_OK)
            {
                print_error(st);
                return 1;
            }

            printf("a=%g b=%g c=%g: ", perms[i][0], perms[i][1], perms[i][2]);
            switch (kind)
            {
            case SOLUTION_NONE:
                printf("нет вещественных корней\n");
                break;
            case SOLUTION_ONE:
                printf("x = %g\n", roots[0]);
                break;
            case SOLUTION_TWO:
                printf("x1 = %g, x2 = %g\n", roots[0], roots[1]);
                break;
            case SOLUTION_ANY:
                printf("x - любое число\n");
                break;
            }
        }
        break;
    }
    case 'm':
    {
        if (argc != 4)
        {
            printf("Число аргументов для m должно быть равно 4\n");
            return 1;
        }
        int a, b;
        if (((parse_int(argv[2], &a)) != STATUS_OK) || ((parse_int(argv[3], &b)) != STATUS_OK))
        {
            printf("3 и 4 аргументы для m должны быть ненулевыми целыми числами\n");
            return 1;
        }
        bool is_m;
        st = is_multi(a, b, &is_m);
        if (st != STATUS_OK)
        {
            print_error(st);
            return 1;
        }
        if (is_m)
            printf("%d кратно %d\n", a, b);
        else
            printf("%d не кратно %d\n", a, b);
        break;
    }
    case 't':
    {
        if (argc != 6)
        {
            printf("Число аргументов для t должно быть равно 6\n");
            return 1;
        }
        double e;
        double nums[3];
        if (((parse_double(argv[2], &e)) != STATUS_OK) || (e <= 0.0))
        {
            printf("Третий аргумент для t должен быть вещественным числом больше 0\n");
            return 1;
        }
        for (int i = 0; i < 3; i++)
        {
            if (parse_double(argv[3 + i], &nums[i]) != STATUS_OK)
            {
                printf("4, 5, 6 аргументы для t должны быть вещественными числами\n");
                return 1;
            }
        }
        bool res;
        st = is_right_triangle(e, nums[0], nums[1], nums[2], &res);
        if (st != STATUS_OK)
        {
            print_error(st);
            return 1;
        }
        if (res)
            printf("Стороны могут принадлежать прямоугольному треугольнику\n");
        else
            printf("Стороны не могут принадлежать прямоугольному треугольнику\n");
        break;
    }
    default:
    {
        printf("Флаги: q, m, t\n");
        return 1;
    }
    }
    return 0;
}