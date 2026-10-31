#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdbool.h>

#define EPS 1e-9

typedef struct
{
    double *coords;
    size_t n;
} vector_t;

typedef struct
{
    size_t *indices;
    size_t count;
} norm_result_t;

typedef status_t (*norm_func_t)(const double *coords, size_t n, double *out);

status_t norm_inf(const double *coords, size_t n, double *out)
{
    if (!coords || !out || n == 0)
        return STATUS_NULL_POINTER;
    double max = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        double val = fabs(coords[i]);
        if (val > max)
            max = val;
    }
    *out = max;
    return STATUS_OK;
}

status_t norm_p(const double *coords, size_t n, double *out)
{
    if (!coords || !out || n == 0)
        return STATUS_NULL_POINTER;
    const double p = 2.0;
    if (p < 1)
        return STATUS_OUT_OF_RANGE;
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i)
        sum += pow(fabs(coords[i]), p);
    *out = pow(sum, 1.0 / p);
    return STATUS_OK;
}

status_t norm_A(const double *coords, size_t n, double *out)
{
    if (!coords || !out || n == 0)
        return STATUS_NULL_POINTER;

    double sum = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        double Aii = (double)(i + 1);
        sum += Aii * coords[i] * coords[i];
    }

    if (sum < 0.0)
        return STATUS_OUT_OF_RANGE;

    *out = sqrt(sum);
    return STATUS_OK;
}

status_t find_longest_vectors(size_t n, size_t num_vectors, size_t num_norms,
                              norm_result_t **out, ...)
{
    if (num_vectors == 0 || num_norms == 0 || !out)
        return STATUS_NULL_POINTER;

    *out = calloc(num_norms, sizeof(norm_result_t));
    if (!*out)
        return STATUS_MEMORY_ERROR;

    va_list args;
    va_start(args, out);

    vector_t **vectors = malloc(num_vectors * sizeof(vector_t *));
    if (!vectors)
    {
        va_end(args);
        return STATUS_MEMORY_ERROR;
    }

    for (size_t i = 0; i < num_vectors; i++)
        vectors[i] = va_arg(args, vector_t *);

    for (size_t i = 0; i < num_norms; i++)
    {
        norm_func_t func = va_arg(args, norm_func_t);

        double max_norm = -1.0;
        size_t count = 0;
        double *norms = malloc(num_vectors * sizeof(double));
        if (!norms)
        {
            free(vectors);
            va_end(args);
            return STATUS_MEMORY_ERROR;
        }

        for (size_t j = 0; j < num_vectors; j++)
        {
            status_t st = func(vectors[j]->coords, n, &norms[j]);
            if (st != STATUS_OK)
            {
                free(norms);
                free(vectors);
                va_end(args);
                return st;
            }
            if (norms[j] > max_norm + EPS)
            {
                max_norm = norms[j];
                count = 1;
            }
            else if (fabs(norms[j] - max_norm) <= EPS)
            {
                count++;
            }
        }

        (*out)[i].count = count;
        (*out)[i].indices = malloc(count * sizeof(size_t));
        if (!(*out)[i].indices)
        {
            free(norms);
            free(vectors);
            va_end(args);
            return STATUS_MEMORY_ERROR;
        }

        size_t idx = 0;
        for (size_t j = 0; j < num_vectors; j++)
        {
            if (fabs(norms[j] - max_norm) <= EPS)
                (*out)[i].indices[idx++] = j;
        }
        free(norms);
    }

    free(vectors);
    va_end(args);
    return STATUS_OK;
}

int main(void)
{
    size_t n = 3;

    double c1[] = {1.0, 2.0, 3.0}, c2[] = {3.0, 2.0, 1.0};
    double c3[] = {0.0, 0.0, 4.0}, c4[] = {-4.0, 0.0, 0.0};
    vector_t v1 = {c1, n}, v2 = {c2, n}, v3 = {c3, n}, v4 = {c4, n};

    norm_result_t *results = NULL;

    status_t st = find_longest_vectors(n, 4, 3, &results,
                                       &v1, &v2, &v3, &v4,
                                       norm_inf, norm_p, norm_A);

    const char *names[] = {"inf-норма", "p-норма", "Матричная A-норма"};
    if (st == STATUS_OK)
    {
        for (size_t i = 0; i < 3; i++)
        {
            printf("[%s] Число самых длинный векторов: %zu. Их индексы: ", names[i], results[i].count);
            for (size_t j = 0; j < results[i].count; j++)
                printf("%zu ", results[i].indices[j]);
            printf("\n");
            free(results[i].indices);
        }
        free(results);
    }
    else
    {
        print_error(st);
    }

    return 0;
}