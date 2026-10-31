#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

status_t shift_poly(double *c, size_t n, double a)
{
    if (c == NULL)
        return STATUS_NULL_POINTER;

    for (size_t i = 0; i <= n; i++)
    {
        for (size_t j = n; j > i; j--)
            c[j - 1] += a * c[j];
    }
    return STATUS_OK;
}

status_t all_finite(const double *c, size_t count)
{
    if (c == NULL)
        return STATUS_NULL_POINTER;

    for (size_t i = 0; i < count; i++)
    {
        if (!isfinite(c[i]))
            return STATUS_OUT_OF_RANGE;
    }
    return STATUS_OK;
}

status_t zero_small(double *c, size_t count, double eps)
{
    if (c == NULL)
        return STATUS_NULL_POINTER;

    for (size_t i = 0; i < count; i++)
    {
        if (fabs(c[i]) < eps)
            c[i] = 0.0;
    }
    return STATUS_OK;
}

status_t poly_value(const double *c, size_t count, double x, double *out)
{
    if (c == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    if (count == 0)
        return STATUS_INVALID_ARGUMENT;

    double r = c[count - 1];
    for (size_t k = count - 1; k > 0; k--)
        r = r * x + c[k - 1];
    *out = r;
    return STATUS_OK;
}

status_t shift_polynomial(double eps, double a, double **out, int n, ...)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (!isfinite(eps) || eps <= 0)
        return STATUS_INVALID_ARGUMENT;
    if (!isfinite(a))
        return STATUS_INVALID_DOUBLE_NUMBER;
    if (n < 0)
        return STATUS_OUT_OF_RANGE;

    size_t count = (size_t)n + 1;
    if (count > SIZE_MAX / sizeof(double))
        return STATUS_MEMORY_ERROR;

    double *c = malloc(count * sizeof(double));
    if (c == NULL)
        return STATUS_MEMORY_ERROR;

    status_t st = STATUS_OK;
    va_list args;
    va_start(args, n);
    for (size_t i = 0; i < count && st == STATUS_OK; i++)
    {
        c[i] = va_arg(args, double);
        if (!isfinite(c[i]))
            st = STATUS_INVALID_DOUBLE_NUMBER;
    }
    va_end(args);

    if (st == STATUS_OK)
        st = shift_poly(c, count - 1, a);
    if (st == STATUS_OK)
        st = all_finite(c, count);
    if (st == STATUS_OK)
        st = zero_small(c, count, eps);

    if (st != STATUS_OK)
    {
        free(c);
        return st;
    }
    *out = c;
    return STATUS_OK;
}

status_t make_base(double a, double eps, char *buf, size_t size)
{
    if (buf == NULL)
        return STATUS_NULL_POINTER;

    int len;
    if (fabs(a) < eps)
        len = snprintf(buf, size, "x");
    else if (a > 0)
        len = snprintf(buf, size, "(x - %g)", a);
    else
        len = snprintf(buf, size, "(x + %g)", -a);

    if (len < 0 || (size_t)len >= size)
        return STATUS_OUT_OF_RANGE;
    return STATUS_OK;
}

status_t print_poly(const double *c, size_t count, const char *base, double eps)
{
    if (c == NULL || base == NULL)
        return STATUS_NULL_POINTER;

    bool printed = false;
    for (size_t k = 0; k < count; k++)
    {
        if (fabs(c[k]) < eps)
            continue;
        if (!printed)
            printf("%g", c[k]);
        else if (c[k] < 0)
            printf(" - %g", -c[k]);
        else
            printf(" + %g", c[k]);

        if (k == 1)
            printf("*%s", base);
        else if (k > 1)
            printf("*%s^%zu", base, k);
        printed = true;
    }
    if (!printed)
        printf("0");
    printf("\n");
    return STATUS_OK;
}

status_t verify_shift(const double *f, const double *g, size_t count, double a, double eps)
{
    if (f == NULL || g == NULL)
        return STATUS_NULL_POINTER;

    const double points[] = {0.0, 1.0, 3.0};
    for (size_t i = 0; i < sizeof(points) / sizeof(points[0]); i++)
    {
        double fx, gx;
        status_t st = poly_value(f, count, points[i], &fx);
        if (st != STATUS_OK)
            return st;
        st = poly_value(g, count, points[i] - a, &gx);
        if (st != STATUS_OK)
            return st;
        printf("    x = %g: f(x) = %g, разложение = %g, %s\n", points[i], fx, gx,
               fabs(fx - gx) < eps ? "совпадает" : "НЕ СОВПАДАЕТ");
    }
    return STATUS_OK;
}

status_t show_result(const double *f, size_t count, double a, double eps, status_t st, const double *g)
{
    char base[64];

    printf("a = %g\n", a);
    if (st != STATUS_OK)
        return print_error(st);

    status_t rc = make_base(a, eps, base, sizeof(base));
    if (rc != STATUS_OK)
        return rc;

    printf("  f(x) = ");
    rc = print_poly(f, count, "x", eps);
    if (rc != STATUS_OK)
        return rc;
    printf("       = ");
    rc = print_poly(g, count, base, eps);
    if (rc != STATUS_OK)
        return rc;
    rc = verify_shift(f, g, count, a, eps);
    printf("\n");
    return rc;
}

void report_status(const char *title, status_t st)
{
    printf("  %s: ", title);
    if (st == STATUS_OK)
        printf("ошибки нет\n");
    else
        print_error(st);
}

int main(void)
{
    const double eps = 1e-9;
    double *g = NULL;
    status_t st;

    double f1[] = {0, 0, 1};
    st = shift_polynomial(eps, 1.0, &g, 2, f1[0], f1[1], f1[2]);
    show_result(f1, 3, 1.0, eps, st, g);
    free(g);
    g = NULL;

    double f2[] = {0, 0, 0, 1};
    st = shift_polynomial(eps, 2.0, &g, 3, f2[0], f2[1], f2[2], f2[3]);
    show_result(f2, 4, 2.0, eps, st, g);
    free(g);
    g = NULL;

    double f3[] = {1, 2, 3};
    st = shift_polynomial(eps, 1.0, &g, 2, f3[0], f3[1], f3[2]);
    show_result(f3, 3, 1.0, eps, st, g);
    free(g);
    g = NULL;

    st = shift_polynomial(eps, 0.5, &g, 2, f1[0], f1[1], f1[2]);
    show_result(f1, 3, 0.5, eps, st, g);
    free(g);
    g = NULL;

    double f5[] = {-1, 0, 0, 0, 1};
    st = shift_polynomial(eps, -1.0, &g, 4, f5[0], f5[1], f5[2], f5[3], f5[4]);
    show_result(f5, 5, -1.0, eps, st, g);
    free(g);
    g = NULL;

    st = shift_polynomial(eps, 0.0, &g, 2, f3[0], f3[1], f3[2]);
    show_result(f3, 3, 0.0, eps, st, g);
    free(g);
    g = NULL;

    double f7[] = {5};
    st = shift_polynomial(eps, 3.0, &g, 0, f7[0]);
    show_result(f7, 1, 3.0, eps, st, g);
    free(g);
    g = NULL;

    printf("Проверка ошибок:\n");

    st = shift_polynomial(eps, 1.0, &g, -1);
    report_status("степень -1", st);
    free(g);
    g = NULL;

    st = shift_polynomial(0.0, 1.0, &g, 1, 1.0, 2.0);
    report_status("точность 0", st);
    free(g);
    g = NULL;

    st = shift_polynomial(eps, NAN, &g, 1, 1.0, 2.0);
    report_status("a = NaN", st);
    free(g);
    g = NULL;

    st = shift_polynomial(eps, 1.0, &g, 1, 1.0, INFINITY);
    report_status("коэффициент inf", st);
    free(g);
    g = NULL;

    st = shift_polynomial(eps, 1.0, NULL, 0, 1.0);
    report_status("out = NULL", st);

    st = shift_polynomial(eps, 1e200, &g, 3, 0.0, 0.0, 0.0, 1.0);
    report_status("переполнение результата", st);
    free(g);
    g = NULL;

    return 0;
}