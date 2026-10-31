#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef double (*func_t)(double);

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_OUT_OF_RANGE,
    STATUS_MEMORY_ERROR,
    STATUS_OVERFLOW,
    STATUS_INVALID_NUMBER,
    STATUS_IO_ERROR,
    STATUS_NO_CONVERGENCE,
} status_t;

status_t status_message(const status_t status, const char **msg)
{
    if (msg == NULL)
        return STATUS_NULL_POINTER;

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
    case STATUS_MEMORY_ERROR:
        *msg = "ошибка выделения памяти";
        break;
    case STATUS_OVERFLOW:
        *msg = "переполнение при вычислении";
        break;
    case STATUS_INVALID_NUMBER:
        *msg = "аргумент не является числом";
        break;
    case STATUS_IO_ERROR:
        *msg = "ошибка чтения или записи файла";
        break;
    case STATUS_NO_CONVERGENCE:
        *msg = "ряд не сходится или достигнут лимит итераций";
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

status_t parse_double(const char *s, double *out)
{
    if (s == NULL || out == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    char *end = NULL;
    double v = strtod(s, &end);

    if (end == s || *end != '\0')
    {
        return STATUS_INVALID_NUMBER;
    }
    if (isnan(v) || isinf(v))
    {
        return STATUS_OUT_OF_RANGE;
    }

    *out = v;
    return STATUS_OK;
}

status_t sum_a(const double x, const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 1000;
    double sum = 1;
    double member = 1;
    int n = 1;

    while (n < MAX_ITERATIONS)
    {
        member *= x / (double)n;
        sum += member;

        if (fabs(member) < eps)
            break;

        n++;
    }
    if (n >= MAX_ITERATIONS)
        return STATUS_NO_CONVERGENCE;

    *out = sum;
    return STATUS_OK;
}

status_t sum_b(const double x, const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 1000;
    double sum = 1;
    double member = 1;
    int n = 1;

    while (n < MAX_ITERATIONS)
    {
        member *= (-x * x) / ((double)(2 * n - 1) * (double)(2 * n));
        sum += member;

        if (fabs(member) < eps)
            break;

        n++;
    }

    if (n >= MAX_ITERATIONS)
        return STATUS_NO_CONVERGENCE;

    *out = sum;
    return STATUS_OK;
}

status_t sum_c(const double x, const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 1000;
    double sum = 1;
    double member = 1;
    int n = 1;

    while (n < MAX_ITERATIONS)
    {
        double numerator = 27 * (double)n * (double)n * (double)n * x * x;
        double denominator = ((double)(3 * n - 2) * (double)(3 * n - 1) * (double)(3 * n));
        member *= numerator / denominator;
        sum += member;

        if (fabs(member) < eps)
            break;

        n++;
    }

    if (n >= MAX_ITERATIONS)
        return STATUS_NO_CONVERGENCE;

    *out = sum;
    return STATUS_OK;
}

status_t sum_d(const double x, const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 1000000;

    double member = -x * x / 2;
    double sum = member;
    int n = 2;

    while (n < MAX_ITERATIONS)
    {
        member *= (-x * x) * (double)(2 * n - 1) / (double)(2 * n);
        sum += member;
        if (fabs(member) < eps)
            break;
        n++;
    }

    if (n >= MAX_ITERATIONS)
        return STATUS_NO_CONVERGENCE;

    *out = sum;
    return STATUS_OK;
}

static double f_1(const double x)
{
    if (fabs(x) < 1e-15)
    {
        return 1;
    }
    return log(1 + x) / x;
}

static double f_2(const double x)
{
    return exp(-x * x / 2);
}

static double f_3(const double x)
{
    if (x >= 1)
        return 0;
    return -log(1 - x);
}

static double f_4(const double x)
{
    if (fabs(x) < 1e-15)
        return 1;
    return pow(x, x);
}

status_t integrate(func_t f, const double a, const double b,
                   const double eps, double *out)
{
    if (f == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0 || b < a)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 1 << 28;
    int n = 1;
    double prev = 0;
    double cur = 0;

    while (n < MAX_ITERATIONS)
    {
        const double h = (b - a) / (double)n;
        double sum = 0;

        for (int i = 0; i < n; i++)
        {
            const double x_mid = a + i * h + 0.5 * h;
            sum += f(x_mid);
        }

        prev = cur;
        cur = h * sum;

        if (n > 1 && fabs(cur - prev) < eps)
            break;

        n *= 2;
    }

    *out = cur;

    if (n >= MAX_ITERATIONS)
        return STATUS_OVERFLOW;

    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Неверный формат. Верный: [программа] [e] \n");
        return 1;
    }

    double eps = 0;
    status_t st = parse_double(argv[1], &eps);

    if (st != STATUS_OK)
    {
        print_error(st);
        return 1;
    }

    if (eps <= 0 || eps >= 1)
    {
        printf("Неверный ввод эпсилон. Эпсилон должен быть в диапазоне (0,1) или эпсилон слишком мал\n");
        return 1;
    }

    const double MIN_EPS = 1e-13;

    if (eps < MIN_EPS)
    {
        printf("Epsilon слишком мал (%.2e), установлен минимум %.2e\n", eps, MIN_EPS);
        eps = MIN_EPS;
    }

    const double x_a = 1.0;
    const double x_b = 1.0;
    const double x_c = 0.5;
    const double x_d = 1.0;

    double s_a, s_b, s_c, s_d;

    printf("Вычисление сумм рядов\n\n");

    printf("1. Сумма ряда A (e^x) при x = %.1f:\n", x_a);
    st = sum_a(x_a, eps, &s_a);
    if (st == STATUS_OK)
        printf("Результат: %.10f\n", s_a);
    else
        print_error(st);
    printf("Эталон:    %.10f\n\n", exp(x_a));

    printf("2. Сумма ряда B (cos(x)) при x = %.1f:\n", x_b);
    st = sum_b(x_b, eps, &s_b);
    if (st == STATUS_OK)
        printf("Результат: %.10f\n", s_b);
    else
        print_error(st);
    printf("Эталон:    %.10f\n\n", cos(x_b));

    printf("3. Сумма ряда C при x = %.1f:\n", x_c);
    st = sum_c(x_c, eps, &s_c);
    if (st == STATUS_OK)
        printf("Результат: %.10f\n", s_c);
    else
        print_error(st);
    printf("Этот ряд сходится к гипергеометрической функции\n\n");

    printf("4. Сумма ряда D (e^(-x^2)) при x = %.1f:\n", x_d);
    st = sum_d(x_d, eps, &s_d);
    if (st == STATUS_OK)
        printf("Результат: %.10f\n", s_d);
    else
        print_error(st);
    printf("Эталон:    %.10f\n\n", 1.0 / sqrt(1.0 + x_d * x_d) - 1.0);

    printf("Вычисление интегралов с точностью eps = %.15f\n\n", eps);

    double i_1, i_2, i_3, i_4;

    printf("1. Интеграл ln(1+x)/x от 0 до 1:\n");
    st = integrate(f_1, 0, 1, eps, &i_1);
    if (st == STATUS_OK)
        printf("Результат: %.10f\n", i_1);
    else
        print_error(st);
    printf("Эталон:    0.8224670334\n\n");

    printf("2. Интеграл e^(-x^2/2) от 0 до 1:\n");
    st = integrate(f_2, 0, 1, eps, &i_2);
    if (st == STATUS_OK)
        printf("Результат: %.10f\n", i_2);
    else
        print_error(st);
    printf("Эталон:    0.8556243919\n\n");

    printf("3. Интеграл ln(1/(1-x)) от 0 до 1:\n");
    st = integrate(f_3, 0, 1, eps, &i_3);
    if (st == STATUS_OK)
        printf("Результат: %.10f\n", i_3);
    else
        print_error(st);
    printf("Эталон:    1.0000000000\n\n");

    printf("4. Интеграл x^x от 0 до 1:\n");
    st = integrate(f_4, 0, 1, eps, &i_4);
    if (st == STATUS_OK)
        printf("Результат: %.10f\n", i_4);
    else
        print_error(st);
    printf("Эталон:    0.7834305107\n\n");

    return 0;
}