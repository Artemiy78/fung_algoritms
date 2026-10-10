#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_OUT_OF_RANGE,
    STATUS_INVALID_FORMAT,
    STATUS_INVALID_NUMBER,
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
    fprintf(stderr, "Ошибка: %s\n", msg);
    return STATUS_OK;
}

status_t parse_double(const char *str, double *res)
{
    if (str == NULL || res == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    double x;
    char extra; // для отсечения строк вида 1.5abc
    if (sscanf(str, "%lf%c", &x, &extra) != 1 || !isfinite(x))
    {
        return STATUS_INVALID_NUMBER;
    }

    *res = x;
    return STATUS_OK;
}

status_t parse_int(const char *str, int *res)
{
    if (str == NULL || res == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    int x;
    char extra;
    if (sscanf(str, "%d%c", &x, &extra) != 1)
    {
        return STATUS_INVALID_NUMBER;
    }

    *res = x;
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

    const int idx[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    size_t count = 0;

    for (size_t i = 0; i < 6; i++)
    {
        bool duplicate = false;
        for (size_t j = 0; j < count; j++)
        {
            if (fabs(res[j][0] - coef[idx[i][0]]) < eps &&
                fabs(res[j][1] - coef[idx[i][1]]) < eps &&
                fabs(res[j][2] - coef[idx[i][2]]) < eps)
            {
                duplicate = true;
                break;
            }
        }
        if (!duplicate)
        {
            for (size_t k = 0; k < 3; k++)
            {
                res[count][k] = coef[idx[i][k]];
            }
            count++;
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
        fprintf(stderr, "Число аргументов должно быть больше 3\n");
        return 1;
    }

    char flag;
    if ((argv[1][0] != '-' && argv[1][0] != '/') ||
        (argv[1][1] == '\0' || argv[1][2] != '\0'))
    {
        fprintf(stderr, "Флаг должен иметь формат -n или /n\n");
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
            fprintf(stderr, "Число аргументов должно быть равно 6\n");
            return 1;
        }
        double e;
        double coef[3];
        if (((parse_double(argv[2], &e)) != STATUS_OK) || (e < 0.0))
        {
            fprintf(stderr, "Третий аргумент должен быть вещественным числом больше 0\n");
            return 1;
        }

        for (int i = 0; i < 3; i++)
        {
            if (parse_double(argv[3 + i], &coef[i]) != STATUS_OK)
            {
                fprintf(stderr, "4, 5, 6 аргументы должен быть вещественными числами\n");
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
            fprintf(stderr, "Число аргументов должно быть равно 4\n");
            return 1;
        }
        int a, b;
        if (((parse_int(argv[2], &a)) != STATUS_OK) || ((parse_int(argv[3], &b)) != STATUS_OK))
        {
            fprintf(stderr, "3 и 4 аргументы должны быть ненулевыми целыми числами\n");
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
            fprintf(stderr, "Число аргументов должно быть равно 6\n");
            return 1;
        }
        double e;
        double nums[3];
        if (((parse_double(argv[2], &e)) != STATUS_OK) || (e <= 0.0))
        {
            fprintf(stderr, "Третий аргумент должен быть вещественным числом больше 0\n");
            return 1;
        }
        for (int i = 0; i < 3; i++)
        {
            if (parse_double(argv[3 + i], &nums[i]) != STATUS_OK)
            {
                fprintf(stderr, "4, 5, 6 аргументы должен быть вещественными числами\n");
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
        fprintf(stderr, "Флаги: q, m, t\n");
        return 1;
    }
    }
    return 0;
}