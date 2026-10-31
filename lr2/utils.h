#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdbool.h>

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_OUT_OF_RANGE,
    STATUS_INVALID_NUMBER,
    STATUS_INVALID_INT_NUMBER,
    STATUS_INVALID_DOUBLE_NUMBER,
    STATUS_MEMORY_ERROR,
    STATUS_INVALID_FLAG,
    STATUS_INVALID_ARGUMENT,
    STATUS_FILE_READ_ERROR,
    STATUS_FILE_OPEN_ERROR,
    STATUS_EMPTY_SUBSTRING,
    STATUS_INVALID_BASE,
    STATUS_INVALID_EPSILON,
    STATUS_INVALID_COUNT,
    STATUS_NOT_DECIMAL_FRACTION,
    STATUS_FILE_WRITE_ERROR,
    STATUS_SAME_FILE,
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
        *msg = "аргумент не является числом";
        break;
    case STATUS_INVALID_INT_NUMBER:
        *msg = "аргумент не является целым числом";
        break;
    case STATUS_INVALID_DOUBLE_NUMBER:
        *msg = "аргумент не является вещественным числом";
        break;
    case STATUS_MEMORY_ERROR:
        *msg = "не удалось выделить память";
        break;
    case STATUS_INVALID_FLAG:
        *msg = "неверный флаг";
        break;
    case STATUS_INVALID_ARGUMENT:
        *msg = "неверный аргумент";
        break;
    case STATUS_EMPTY_SUBSTRING:
        *msg = "подстрока пустая";
        break;
    case STATUS_FILE_OPEN_ERROR:
        *msg = "не удалось открыть файл";
        break;
    case STATUS_FILE_READ_ERROR:
        *msg = "не удалось прочитать файл";
        break;
    case STATUS_INVALID_BASE:
        *msg = "основание должно быть целым числом не меньше 2";
        break;
    case STATUS_INVALID_EPSILON:
        *msg = "эпсилон должен быть в диапазоне (0; 0.5)";
        break;
    case STATUS_INVALID_COUNT:
        *msg = "количество чисел должно быть больше нуля";
        break;
    case STATUS_NOT_DECIMAL_FRACTION:
        *msg = "число не удалось представить конечной десятичной дробью";
        break;
    case STATUS_FILE_WRITE_ERROR:
        *msg = "не удалось записать в файл";
        break;
    case STATUS_SAME_FILE:
        *msg = "одинаковые файлы";
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

status_t parse_unsigned(const char *s, int *out)
{
    if (s == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    char *end = NULL;
    long long v = strtoll(s, &end, 10);

    if (end == s || *end != '\0')
        return STATUS_INVALID_INT_NUMBER;

    if (v < 0 || v > UINT_MAX)
        return STATUS_OUT_OF_RANGE;

    *out = (unsigned)v;
    return STATUS_OK;
}

status_t is_digit(const int c, bool *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (c < 0 || c > 255)
    {
        return STATUS_OUT_OF_RANGE;
    }

    *res = (c >= '0' && c <= '9');
    return STATUS_OK;
}

status_t is_latin(const int c, bool *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (c < 0 || c > 255)
    {
        return STATUS_OUT_OF_RANGE;
    }

    *res = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    return STATUS_OK;
}

status_t is_alpha(const int c, bool *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (c < 0 || c > 255)
    {
        return STATUS_OUT_OF_RANGE;
    }

    *res = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    return STATUS_OK;
}

#endif