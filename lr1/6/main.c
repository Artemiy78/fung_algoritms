#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <math.h>

#define MAX_POINTS 1000

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_OUT_OF_RANGE,
    STATUS_OVERFLOW,
    STATUS_INVALID_NUMBER,
    STATUS_DIVISION_BY_ZERO,
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
    case STATUS_OVERFLOW:
        *msg = "переполнение при вычислении";
        break;
    case STATUS_INVALID_NUMBER:
        *msg = "некорректное число (NaN или недопустимый символ)";
        break;
    case STATUS_DIVISION_BY_ZERO:
        *msg = "деление на ноль";
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

typedef unsigned long long ull;

typedef double (*func_t)(double);

typedef struct
{
    double x, y;
} point_t;

status_t f1(const size_t n, bool *out, ...)
{
    if (n < 3 || n > MAX_POINTS)
        return STATUS_OUT_OF_RANGE;

    va_list args;
    va_start(args, out);
    point_t points[MAX_POINTS];
    for (size_t i = 0; i < n; i++)
    {
        double px = va_arg(args, double);
        double py = va_arg(args, double);

        if (isnan(px) || isinf(px) || isnan(py) || isinf(py))
        {
            va_end(args);
            return STATUS_INVALID_NUMBER;
        }

        points[i].x = px;
        points[i].y = py;
    }
    va_end(args);

    double first_z = 0;
    bool sign_defined = false;
    bool is_convex = true;
    const double EPS = 1e-9;

    for (size_t i = 0; i < n; i++)
    {
        const point_t *a = &points[i];
        const point_t *b = &points[(i + 1) % n];
        const point_t *c = &points[(i + 2) % n];

        double z = (b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x);
        if (fabs(z) <= EPS)
        {
            is_convex = false;
            break;
        }
        if (!sign_defined)
        {
            first_z = z;
            sign_defined = true;
        }
        else if ((first_z > 0) != (z > 0))
        {
            is_convex = false;
            break;
        }
    }

    *out = is_convex;
    return STATUS_OK;
}

status_t polynomial(const double x, const int n, double *out, ...)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;

    if (n < 0)
        return STATUS_OUT_OF_RANGE;

    if (isnan(x) || isinf(x))
    {
        return STATUS_OUT_OF_RANGE;
    }

    va_list args;
    va_start(args, out);

    double res = va_arg(args, double);
    if (isnan(res) || isinf(res))
    {
        va_end(args);
        return STATUS_OUT_OF_RANGE;
    }
    for (int i = 0; i < n; i++)
    {
        double coeff = va_arg(args, double);
        if (isnan(coeff) || isinf(coeff))
        {
            va_end(args);
            return STATUS_OUT_OF_RANGE;
        }
        res = res * x + coeff;
    }

    va_end(args);

    *out = res;
    return STATUS_OK;
}

status_t parse_in_base(const char *s, const int base, ull *out)
{
    if (s == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    ull result = 0;

    for (int i = 0; s[i] != '\0'; i++)
    {
        char c = s[i];
        int digit;

        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'a' && c <= 'z')
            digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z')
            digit = c - 'A' + 10;
        else
            return STATUS_INVALID_NUMBER;

        if (digit >= base)
            return STATUS_INVALID_NUMBER;

        result = result * base + digit;
    }

    *out = result;
    return STATUS_OK;
}

status_t is_kaprekar(const ull n, const int base, bool *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;

    if (n == 0 || n == 1)
    {
        *out = true;
        return STATUS_OK;
    }

    ull sq = n * n;

    int digits = 0;
    ull temp = n;
    while (temp > 0)
    {
        digits++;
        temp /= base;
    }

    ull del = 1;
    for (int i = 0; i < digits; i++)
        del *= base;

    ull left = sq / del;
    ull right = sq % del;

    *out = (right != 0) && (left + right == n);
    return STATUS_OK;
}

status_t find_kaprekar(const int base, const int count, int *out_count,
                       ull *out_array, ...)
{
    if (out_count == NULL || out_array == NULL)
        return STATUS_NULL_POINTER;
    if (base < 2 || base > 36)
        return STATUS_OUT_OF_RANGE;
    if (count <= 0)
    {
        *out_count = 0;
        return STATUS_OK;
    }

    va_list args;
    va_start(args, out_array);

    int found = 0;
    status_t result = STATUS_OK;

    for (int i = 0; i < count; i++)
    {
        const char *s = va_arg(args, const char *);
        if (s == NULL)
            continue;

        ull n = 0;
        status_t st = parse_in_base(s, base, &n);
        if (st != STATUS_OK)
        {
            result = st;
            continue;
        }

        bool is_kpr = false;
        st = is_kaprekar(n, base, &is_kpr);
        if (st != STATUS_OK)
        {
            result = st;
            continue;
        }

        if (is_kpr)
            out_array[found++] = n;
    }

    va_end(args);
    *out_count = found;
    return result;
}

status_t geom(double *out, const int count, ...)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;

    if (count <= 0)
        return STATUS_OUT_OF_RANGE;

    va_list args;
    va_start(args, count);

    double sum = 0;

    for (int i = 0; i < count; i++)
    {
        double x = va_arg(args, double);
        if (x <= 0 || isnan(x) || isinf(x))
        {
            va_end(args);
            return STATUS_OUT_OF_RANGE;
        }
        sum += log(x);
    }
    va_end(args);
    *out = exp(sum / (double)count);
    return STATUS_OK;
}

status_t fast_pow(const double base, const int degree, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;

    if (isnan(base) || isinf(base))
        return STATUS_INVALID_NUMBER;

    if (degree < 0)
    {
        if (base == 0)
            return STATUS_DIVISION_BY_ZERO;
        double result;
        status_t st = fast_pow(base, -degree, &result);
        if (st != STATUS_OK)
            return st;

        *out = 1.0 / result;
        return STATUS_OK;
    }
    if (degree == 0)
    {
        *out = 1;
        return STATUS_OK;
    }
    double half;
    status_t st = fast_pow(base, degree / 2, &half);
    if (st != STATUS_OK)
        return st;

    if (degree % 2 == 0)
    {
        *out = half * half;
    }
    else
    {
        *out = base * half * half;
    }

    return STATUS_OK;
}

status_t bisection(const double min, const double max, const double eps, func_t f, double *out)
{
    if (f == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    if (isnan(min) || isinf(min) || isnan(max) || isinf(max) || isnan(eps) || isinf(eps))
        return STATUS_INVALID_NUMBER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;
    if (min >= max)
        return STATUS_OUT_OF_RANGE;

    double a = min;
    double b = max;
    double fa = f(a);
    double fb = f(b);
    if (fa * fb > 0)
        return STATUS_OUT_OF_RANGE;
    if (fa == 0)
    {
        *out = a;
        return STATUS_OK;
    }
    if (fb == 0)
    {
        *out = b;
        return STATUS_OK;
    }

    int n = 0;
    while (fabs(b - a) > eps)
    {
        double c = (a + b) / 2.0;
        double fc = f(c);
        if (fc == 0.0)
        {
            *out = c;
            return STATUS_OK;
        }
        if (fa * fc < 0.0)
        {
            b = c;
            fb = fc;
        }
        else
        {
            a = c;
            fa = fc;
        }

        if (++n >= 1000000)
            return STATUS_OVERFLOW;
    }

    *out = (a + b) / 2.0;
    return STATUS_OK;
}

static double func_sin(double x)
{
    return sin(x);
}

static double func_cubic(double x)
{
    return x * x * x - x - 2.0;
}

static double func_exp(double x)
{
    return exp(x) - 3.0;
}

int main()
{
    printf("1. Проверка выпуклости многоугольника:\n");
    bool is_convex;
    status_t st = f1(4, &is_convex, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0);
    if (st == STATUS_OK)
        printf("Многоугольник (0,0), (1,0), (1,1), (0,1): %s\n",
               is_convex ? "выпуклый" : "не выпуклый");
    else
        print_error(st);

    st = f1(4, &is_convex, 0.0, 0.0, 2.0, 0.0, 1.0, 1.0, 1.0, 2.0);
    if (st == STATUS_OK)
        printf("Многоугольник (0,0), (2,0), (1,1), (1,2): %s\n",
               is_convex ? "выпуклый" : "не выпуклый");
    else
        print_error(st);
    printf("\n");

    printf("2. Вычисление многочлена:\n");
    double poly_result;
    double x_poly1 = INFINITY;
    int n1 = 3;
    double coeff1[] = {2.0, NAN, -5.0, 7.0};
    st = polynomial(x_poly1, n1, &poly_result, coeff1[0], coeff1[1], coeff1[2], coeff1[3]);
    if (st == STATUS_OK)
        printf("P(%.1f) = %.1f (коэффициенты: %.1f, %.1f, %.1f, %.1f)\n",
               x_poly1, poly_result, coeff1[0], coeff1[1], coeff1[2], coeff1[3]);
    else
        print_error(st);

    printf("\n");

    printf("3. Числа Капрекара:\n");
    ull kaprekar_array[10];
    int kaprekar_count = 0;

    st = find_kaprekar(10, 9, &kaprekar_count, kaprekar_array,
                       "1", "9", "45", "55", "99", "297", "703", "999", "1000");
    if (st == STATUS_OK)
    {
        printf("Десятичная система, найдено: %d из 9\n", kaprekar_count);
        for (int i = 0; i < kaprekar_count; i++)
            printf("   - %llu\n", kaprekar_array[i]);
    }
    else
        print_error(st);

    st = find_kaprekar(16, 7, &kaprekar_count, kaprekar_array,
                       "1", "f", "4f", "50", "ff", "2fe", "703");
    if (st == STATUS_OK)
    {
        printf("Шестнадцатеричная система, найдено: %d из 7\n", kaprekar_count);
        for (int i = 0; i < kaprekar_count; i++)
            printf("   - %llu\n", kaprekar_array[i]);
    }
    else
        print_error(st);
    printf("\n");

    printf("4. Среднее геометрическое:\n");
    double geom_result;
    st = geom(&geom_result, 4, 2.0, 8.0, 4.0, 16.0);
    if (st == STATUS_OK)
        printf("GM(2, 8, 4, 16) = %.6f\n", geom_result);
    else
        print_error(st);

    st = geom(&geom_result, 2, 10.0, INFINITY);
    if (st == STATUS_OK)
        printf("GM(10, INFINITY) = %.6f\n", geom_result);
    else
        print_error(st);
    printf("\n");

    printf("5. Быстрое возведение в степень:\n");
    double pow_result;
    st = fast_pow(2.0, 10, &pow_result);
    if (st == STATUS_OK)
        printf("2^10 = %.1f\n", pow_result);
    else
        print_error(st);

    st = fast_pow(3.0, 5, &pow_result);
    if (st == STATUS_OK)
        printf("3^5 = %.1f\n", pow_result);
    else
        print_error(st);

    st = fast_pow(2.0, -3, &pow_result);
    if (st == STATUS_OK)
        printf("2^(-3) = %.6f\n", pow_result);
    else
        print_error(st);

    st = fast_pow(0.0, -3, &pow_result);
    if (st == STATUS_OK)
        printf("0^(-3) = %.6f\n", pow_result);
    else
        print_error(st);
    printf("\n");

    printf("6. Метод дихотомии:\n");
    double root;
    st = bisection(1.0, 2.0, 1e-6, func_cubic, &root);
    if (st == STATUS_OK)
        printf("x^3 - x - 2 = 0 на [1, 2]: x = %.10f\n", root);
    else
        print_error(st);

    st = bisection(3.0, 4.0, 1e-8, func_sin, &root);
    if (st == STATUS_OK)
        printf("sin(x) = 0 на [3, 4]: x = %.10f\n", root);
    else
        print_error(st);

    st = bisection(0.0, 2.0, 1e-12, func_exp, &root);
    if (st == STATUS_OK)
        printf("e^x - 3 = 0 на [0, 2]: x = %.10f\n", root);
    else
        print_error(st);
    printf("\n");

    return 0;
}