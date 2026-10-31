#include "utils.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_DIGITS 15

typedef unsigned long long ull;

status_t NOD(ull a, ull b, ull *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    while (b != 0)
    {
        ull t = a % b;
        a = b;
        b = t;
    }
    *out = a;
    return STATUS_OK;
}

status_t pow10(unsigned k, ull *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (k > MAX_DIGITS)
        return STATUS_OUT_OF_RANGE;
    ull p = 1;
    for (unsigned i = 0; i < k; i++)
        p *= 10;
    *out = p;
    return STATUS_OK;
}

status_t abs_double(double x, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    *out = (x < 0) ? -x : x;
    return STATUS_OK;
}

status_t reduced_denominator(double x, double eps, ull *out_q)
{
    if (out_q == NULL)
        return STATUS_NULL_POINTER;
    if (!isfinite(eps) || eps <= 0 || eps >= 0.5)
        return STATUS_INVALID_EPSILON;
    if (!isfinite(x) || x <= eps || 1.0 - x <= eps)
        return STATUS_OUT_OF_RANGE;

    for (unsigned k = 0; k <= MAX_DIGITS; k++)
    {
        ull p;
        status_t st = pow10(k, &p);
        if (st != STATUS_OK)
            return st;

        double y = x * (double)p;
        ull n = (ull)(y + 0.5);
        double diff = fabs(y - (double)n);
        if (diff < eps)
        {
            ull del;
            st = NOD(n, p, &del);
            if (st != STATUS_OK)
                return st;
            *out_q = p / del;
            return STATUS_OK;
        }
    }
    return STATUS_NOT_DECIMAL_FRACTION;
}

status_t denominator_fits_base(ull q, long long base, bool *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (base < 2)
        return STATUS_INVALID_BASE;

    ull d;
    status_t st = NOD(q, base, &d);
    if (st != STATUS_OK)
        return st;
    while (d > 1)
    {
        q /= d;
        st = NOD(q, base, &d);
        if (st != STATUS_OK)
            return st;
    }
    *out = (q == 1);
    return STATUS_OK;
}

status_t is_finite_in_base(double x, long long base, double eps, bool *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (base < 2)
        return STATUS_INVALID_BASE;

    ull q;
    status_t st = reduced_denominator(x, eps, &q);
    if (st != STATUS_OK)
        return st;
    return denominator_fits_base(q, base, out);
}

status_t check_finite(long long base, double eps, size_t count, bool **out, ...)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (base < 2)
        return STATUS_INVALID_BASE;
    if (!isfinite(eps) || eps <= 0 || eps >= 0.5)
        return STATUS_INVALID_EPSILON;
    if (count == 0)
        return STATUS_INVALID_COUNT;

    bool *res = malloc(count * sizeof(bool));
    if (res == NULL)
        return STATUS_MEMORY_ERROR;

    va_list args;
    va_start(args, out);

    for (size_t i = 0; i < count; i++)
    {
        double x = va_arg(args, double);
        status_t st = is_finite_in_base(x, base, eps, &res[i]);
        if (st != STATUS_OK)
        {
            va_end(args);
            free(res);
            return st;
        }
    }

    va_end(args);
    *out = res;
    return STATUS_OK;
}

status_t print_answers(unsigned base, const double *xs, const bool *res, size_t count)
{
    if (xs == NULL || res == NULL)
        return STATUS_NULL_POINTER;
    printf("Основание %u:\n", base);
    for (size_t i = 0; i < count; i++)
        printf("  %.15g -> %s\n", xs[i], res[i] ? "конечная" : "бесконечная");
    printf("\n");
    return STATUS_OK;
}

void report_status(const char *title, status_t st)
{
    printf("%s: ", title);
    if (st == STATUS_OK)
        printf("ошибки нет\n");
    else
        print_error(st);
}

int main(void)
{
    const double eps = 1e-9;
    bool *res = NULL;
    status_t st;

    double a[] = {0.5, 0.25, 0.125, 0.1, 0.75};
    st = check_finite(2, eps, 5, &res, a[0], a[1], a[2], a[3], a[4]);
    if (st == STATUS_OK)
        st = print_answers(2, a, res, 5);
    if (st != STATUS_OK)
        print_error(st);
    free(res);
    res = NULL;

    double b[] = {0.1, 0.5, 0.125, 0.3};
    st = check_finite(10, eps, 4, &res, b[0], b[1], b[2], b[3]);
    if (st == STATUS_OK)
        st = print_answers(10, b, res, 4);
    if (st != STATUS_OK)
        print_error(st);
    free(res);
    res = NULL;

    double c[] = {0.0625, 0.03125, 0.1, 0.2};
    st = check_finite(16, eps, 4, &res, c[0], c[1], c[2], c[3]);
    if (st == STATUS_OK)
        st = print_answers(16, c, res, 4);
    if (st != STATUS_OK)
        print_error(st);
    free(res);
    res = NULL;

    double d[] = {0.2, 0.04, 0.12, 0.5, 0.1};
    st = check_finite(15, eps, 5, &res, d[0], d[1], d[2], d[3], d[4]);
    if (st == STATUS_OK)
        st = print_answers(15, d, res, 5);
    if (st != STATUS_OK)
        print_error(st);
    free(res);
    res = NULL;

    printf("Проверка ошибок:\n");
    st = check_finite(10, eps, 1, &res, 0.0);
    report_status("число 0", st);
    free(res);
    res = NULL;

    st = check_finite(10, eps, 1, &res, 1.0);
    report_status("число 1", st);
    free(res);
    res = NULL;

    st = check_finite(10, eps, 1, &res, 1.5);
    report_status("число 1.5", st);
    free(res);
    res = NULL;

    st = check_finite(1, eps, 1, &res, 0.5);
    report_status("основание 1", st);
    free(res);
    res = NULL;

    st = check_finite(10, 0.0, 1, &res, 0.5);
    report_status("эпсилон 0", st);
    free(res);
    res = NULL;

    st = check_finite(10, eps, 1, &res, 0.3333333333333333);
    report_status("число 0.3333333333333333", st);
    free(res);
    res = NULL;

    st = check_finite(10, eps, 0, &res);
    report_status("количество чисел 0", st);
    free(res);
    res = NULL;

    return 0;
}