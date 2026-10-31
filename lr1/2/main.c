#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define MAX_LIMIT_N 10000000

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_OUT_OF_RANGE,
    STATUS_INVALID_NUMBER,
    STATUS_OVERFLOW,
} status_t;

#define CHECK(st, value)   \
    if ((st) != STATUS_OK) \
    {                      \
        (value) = NAN;     \
    }

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
    case STATUS_OVERFLOW:
        *msg = "переполнение";
        break;
    case STATUS_INVALID_NUMBER:
        *msg = "неправильно введенное число";
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

status_t find_e_limit(const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    int n = 1;
    double prev, res = 0;
    while (n < MAX_LIMIT_N)
    {
        prev = res;
        res = pow(1 + (1 / (double)n), (double)n);
        if (n > 1 && fabs(res - prev) < eps)
        {
            break;
        }
        n++;
    }
    if (n >= MAX_LIMIT_N)
        return STATUS_OVERFLOW;
    *out = res;
    return STATUS_OK;
}

status_t find_e_series(const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 1000;
    double sum = 0;
    double member = 1;
    int n = 0;

    while (n < MAX_ITERATIONS)
    {
        sum += member;
        if (member < eps)
        {
            break;
        }
        n++;
        member /= (double)n;
    }

    if (n >= MAX_ITERATIONS)
        return STATUS_OVERFLOW;

    *out = sum;
    return STATUS_OK;
}

status_t solve_e(const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0.0)
        return STATUS_OUT_OF_RANGE;

    double a = 2;
    double b = 3;
    double fa = log(a) - 1;
    double fb = log(b) - 1;

    if (fa * fb >= 0)
        return STATUS_OUT_OF_RANGE;

    int n = 0;
    while ((b - a) > eps)
    {
        double mid = 0.5 * (a + b);
        double fm = log(mid) - 1;

        if (fa * fm < 0)
        {
            b = mid;
            fb = fm;
        }
        else
        {
            a = mid;
            fa = fm;
        }

        if (++n >= MAX_LIMIT_N)
            return STATUS_OVERFLOW;
    }

    *out = 0.5 * (a + b);
    return STATUS_OK;
}

status_t find_pi_limit(const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 1000000;
    int n = 1;
    double prev = 0;
    double res = 4;

    while (n < MAX_ITERATIONS)
    {
        prev = res;
        double numerator = 4 * (double)n * (double)(n + 1);
        double denominator = (2 * (double)n + 1) * (2 * (double)n + 1);
        res *= numerator / denominator;
        if (fabs(res - prev) < eps)
        {
            break;
        }
        n++;
    }

    *out = res;
    return STATUS_OK;
}

status_t find_pi_series(const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 1000000000;
    double sum = 0;
    double sign = 1;
    int n = 1;

    while (n < MAX_ITERATIONS)
    {
        double member = sign / (double)(2 * n - 1);
        sum += member;
        if (fabs(member) < eps)
        {
            break;
        }
        sign = -sign;
        n++;
    }

    *out = 4 * sum;
    return STATUS_OK;
}

status_t solve_pi(const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;
    double a = 3;
    double b = 4;
    double fa = sin(a);
    double fb = sin(b);

    if (fa * fb >= 0.0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 1000;
    int n = 0;
    while ((b - a) > eps)
    {
        double mid = 0.5 * (a + b);
        double fm = sin(mid);

        if (fa * fm < 0.0)
        {
            b = mid;
            fb = fm;
        }
        else
        {
            a = mid;
            fa = fm;
        }

        if (++n >= MAX_ITERATIONS)
            break;
    }

    *out = 0.5 * (a + b);
    return STATUS_OK;
}

status_t find_ln2_limit(const double eps, double *out)
{

    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    int n = 1;
    double prev, res = 0;
    while (n < MAX_LIMIT_N)
    {
        prev = res;
        res = n * (pow(2, 1 / (double)n) - 1);
        if (n > 1 && fabs(res - prev) < eps)
            break;
        n++;
    }
    *out = res;
    return STATUS_OK;
}

status_t find_ln2_series(const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 100000000;
    double sum = 0;
    double sign = 1;
    int n = 1;
    while (n < MAX_ITERATIONS)
    {
        double member = sign / (double)n;
        sum += member;
        if (fabs(member) < eps)
        {
            break;
        }
        sign = -sign;
        n++;
    }

    *out = sum;
    return STATUS_OK;
}

status_t solve_ln2(const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    double a = 0;
    double b = 1;
    double fa = exp(a) - 2;
    double fb = exp(b) - 2;
    if (fa * fb >= 0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 1000;
    int n = 0;
    while ((b - a) > eps)
    {
        double mid = 0.5 * (a + b);
        double fm = exp(mid) - 2;

        if (fa * fm < 0)
        {
            b = mid;
            fb = fm;
        }
        else
        {
            a = mid;
            fa = fm;
        }

        if (++n >= MAX_ITERATIONS)
            return STATUS_OVERFLOW;
    }

    *out = 0.5 * (a + b);
    return STATUS_OK;
}

status_t find_sqrt2_limit(const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0.0)
        return STATUS_OUT_OF_RANGE;

    int n = 0;
    double prev = 0;
    double res = -0.5;
    while (n < MAX_LIMIT_N)
    {
        prev = res;
        res = prev - (prev * prev) / 2 + 1;
        if (n > 0 && fabs(res - prev) < eps)
            break;
        n++;
    }
    *out = res;
    return STATUS_OK;
}

status_t find_sqrt2_series(const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 100000000;
    double res = 1;
    double prev = 0;
    int k = 2;
    while (k < MAX_ITERATIONS)
    {
        prev = res;
        res *= pow(2, pow(2, -1 * k));
        if (fabs(res - prev) < eps)
        {
            break;
        }
        k++;
    }

    if (k >= MAX_ITERATIONS)
        return STATUS_OVERFLOW;

    *out = res;
    return STATUS_OK;
}

status_t solve_sqrt2(const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    double a = 1;
    double b = 2;
    double fa = a * a - 2;
    double fb = b * b - 2;
    if (fa * fb >= 0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 1000;
    int n = 0;
    while ((b - a) > eps)
    {
        double mid = 0.5 * (a + b);
        double fm = mid * mid - 2;

        if (fa * fm < 0)
        {
            b = mid;
            fb = fm;
        }
        else
        {
            a = mid;
            fa = fm;
        }

        if (++n >= MAX_ITERATIONS)
            return STATUS_OVERFLOW;
    }

    *out = 0.5 * (a + b);
    return STATUS_OK;
}

status_t find_y_limit(const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_M = 40;
    double prev_sum = 0;
    double sum = 0;
    int m = 1;

    while (m < MAX_M)
    {
        prev_sum = sum;
        sum = 0;

        double Cmk = 1;
        double ln_k_fact = 0;
        double sign = -1;

        for (int k = 1; k <= m; ++k)
        {
            Cmk *= (double)(m - k + 1) / (double)k;
            ln_k_fact += log((double)k);
            sum += Cmk * sign * ln_k_fact / (double)k;
            sign = -sign;
        }
        if (isinf(sum) || isnan(sum))
            return STATUS_OVERFLOW;
        if (m > 1 && fabs(sum - prev_sum) < eps)
            break;

        ++m;
    }

    *out = sum;
    return STATUS_OK;
}

status_t find_y_series(const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    const int MAX_ITERATIONS = 100000000;
    double sum = 0;
    int k = 2;

    while (k < MAX_ITERATIONS)
    {
        double sq = floor(sqrt((double)k));
        double sq_sq = sq * sq;
        double member = 1 / sq_sq - 1 / (double)k;

        sum += member;

        k++;
    }
    const double PI_SQUARED_DIV_6 = 1.6449340668482264;
    *out = -PI_SQUARED_DIV_6 + sum;

    return STATUS_OK;
}

static status_t is_prime(const int n, bool *const res)
{
    if (res == NULL)
        return STATUS_NULL_POINTER;
    if (n < 0)
        return STATUS_OUT_OF_RANGE;
    if (n < 2)
    {
        *res = false;
        return STATUS_OK;
    }
    if (n == 2)
    {
        *res = true;
        return STATUS_OK;
    }
    if (n % 2 == 0)
    {
        *res = false;
        return STATUS_OK;
    }

    for (int d = 3; (long long)d * d <= n; d += 2)
    {
        if (n % d == 0)
        {
            *res = false;
            return STATUS_OK;
        }
    }

    *res = true;
    return STATUS_OK;
}

static status_t limit_solve_y(const int t, double *const res)
{
    if (res == NULL)
        return STATUS_NULL_POINTER;
    if (t < 2)
        return STATUS_OUT_OF_RANGE;

    double product = 1;
    for (int p = 2; p <= t; ++p)
    {
        bool prime = false;
        status_t st = is_prime(p, &prime);
        if (st != STATUS_OK)
        {
            return st;
        }

        if (prime)
        {
            product *= (double)(p - 1) / (double)p;
        }
    }
    double result = log((double)t) * product;
    if (isnan(result) || isinf(result))
    {
        return STATUS_OVERFLOW;
    }

    *res = result;
    return STATUS_OK;
}
status_t solve_y(const double eps, double *const out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0)
        return STATUS_OUT_OF_RANGE;

    const int T = 1000000;
    double L = 0;
    status_t st = limit_solve_y(T, &L);
    if (st != STATUS_OK)
    {
        return st;
    }
    if (L <= 0 || L >= 1)
    {
        return STATUS_OUT_OF_RANGE;
    }
    double a = 0;
    double b = 1;

    double fa = exp(-a) - L;
    double fb = exp(-b) - L;
    if (fa * fb >= 0)
    {
        return STATUS_OUT_OF_RANGE;
    }

    const int MAX_ITERATIONS = 1000;
    int n = 0;
    while ((b - a) > eps)
    {
        double mid = 0.5 * (a + b);
        double fm = exp(-mid) - L;

        if (fa * fm < 0)
        {
            b = mid;
            fb = fm;
        }
        else
        {
            a = mid;
            fa = fm;
        }

        if (++n >= MAX_ITERATIONS)
            return STATUS_OVERFLOW;
    }

    *out = 0.5 * (a + b);
    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Неверный формат. Верный: [программа] [e] \n");
        return 1;
    }

    double eps = 0.0;
    status_t st = parse_double(argv[1], &eps);

    if (st != STATUS_OK)
    {
        print_error(st);
        return 1;
    }

    if (eps <= 0 || eps >= 1)
    {
        printf("Неверный ввод эпсилон. Эпсилон должен быть в диапазоне (0,1)\n");
        return 1;
    }

    const double MIN_EPS = 1e-14;

    if (eps < MIN_EPS)
    {
        printf("Epsilon слишком мал (%.2e), установлен минимум %.2e\n", eps, MIN_EPS);
        eps = MIN_EPS;
    }

    double e_limit, e_series, e_solved;
    CHECK(find_e_limit(eps, &e_limit), e_limit);
    CHECK(find_e_series(eps, &e_series), e_series);
    CHECK(solve_e(eps, &e_solved), e_solved);

    double pi_limit, pi_series, pi_solved;
    CHECK(find_pi_limit(eps, &pi_limit), pi_limit);
    CHECK(find_pi_series(eps, &pi_series), pi_series);
    CHECK(solve_pi(eps, &pi_solved), pi_solved);

    double ln2_limit, ln2_series, ln2_solved;
    CHECK(find_ln2_limit(eps, &ln2_limit), ln2_limit);
    CHECK(find_ln2_series(eps, &ln2_series), ln2_series);
    CHECK(solve_ln2(eps, &ln2_solved), ln2_solved);

    double sq2_limit, sq2_series, sq2_solved;
    CHECK(find_sqrt2_limit(eps, &sq2_limit), sq2_limit);
    CHECK(find_sqrt2_series(eps, &sq2_series), sq2_series);
    CHECK(solve_sqrt2(eps, &sq2_solved), sq2_solved);

    double y_limit, y_series, y_solved;
    CHECK(find_y_limit(eps, &y_limit), y_limit);
    CHECK(find_y_series(eps, &y_series), y_series);
    CHECK(solve_y(eps, &y_solved), y_solved);

    printf("Эпсилон: 			%.15f\n", eps);
    printf("e (экспонента)\n");
    printf("Предел: 			%.10f\n", e_limit);
    printf("Ряд/Произведение: 		%.10f\n", e_series);
    printf("Уравнение: 			%.10f\n", e_solved);
    printf("Эталон: 			2.7182818285\n\n");
    printf("π\n");
    printf("Предел: 			%.10f\n", pi_limit);
    printf("Ряд/Произведение: 		%.10f\n", pi_series);
    printf("Уравнение: 			%.10f\n", pi_solved);
    printf("Эталон: 			3.1415926536\n\n");
    printf("ln 2 (Натуральный логарифм двух)\n");
    printf("Предел: 			%.10f\n", ln2_limit);
    printf("Ряд/Произведение: 		%.10f\n", ln2_series);
    printf("Уравнение: 			%.10f\n", ln2_solved);
    printf("Эталон: 			0.6931471806\n\n");
    printf("√2\n");
    printf("Предел: 			%.10f\n", sq2_limit);
    printf("Ряд/Произведение: 		%.10f\n", sq2_series);
    printf("Уравнение: 			%.10f\n", sq2_solved);
    printf("Эталон: 			1.4142135624\n\n");
    printf("γ (Постоянная Эйлера-Маскерони)\n");
    printf("Предел: 			%.10f\n", y_limit);
    printf("Ряд/Произведение: 		%.10f\n", y_series);
    printf("Уравнение: 			%.10f\n", y_solved);
    printf("Эталон: 			0.5772156649\n\n");

    return 0;
}