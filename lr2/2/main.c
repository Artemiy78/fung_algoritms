#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/stat.h>
#endif

#include "utils.h"

#define EPS 1e-9
#define START_CAPACITY 16
#define FIELD_COUNT 4

typedef struct
{
    unsigned id;
    char *name;
    char *surname;
    double salary;
} Employee;

status_t parse_flag(const char *flag, bool *descending)
{
    if (flag == NULL || descending == NULL)
        return STATUS_NULL_POINTER;

    if ((flag[0] != '-' && flag[0] != '/') || (flag[1] != 'a' && flag[1] != 'd') ||
        flag[2] != '\0')
        return STATUS_INVALID_FLAG;

    *descending = (flag[1] == 'd');
    return STATUS_OK;
}

status_t is_same_file(const char *path_a, const char *path_b, bool *same)
{
    if (path_a == NULL || path_b == NULL || same == NULL)
        return STATUS_NULL_POINTER;

    *same = false;

#ifdef _WIN32
    char full_a[4096];
    char full_b[4096];
    if (_fullpath(full_a, path_a, sizeof(full_a)) == NULL ||
        _fullpath(full_b, path_b, sizeof(full_b)) == NULL)
        return STATUS_OK;

    size_t i = 0;
    while (full_a[i] != '\0' && full_b[i] != '\0')
    {
        char ca = full_a[i];
        char cb = full_b[i];
        if (ca >= 'A' && ca <= 'Z')
            ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z')
            cb = (char)(cb - 'A' + 'a');
        if (ca != cb)
            return STATUS_OK;
        i++;
    }
    *same = (full_a[i] == full_b[i]);
#else
    struct stat sa;
    struct stat sb;
    if (stat(path_a, &sa) != 0 || stat(path_b, &sb) != 0)
        return STATUS_OK;
    *same = (sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino);
#endif
    return STATUS_OK;
}

status_t read_line(FILE *f, char **line, bool *eof)
{
    if (f == NULL || line == NULL || eof == NULL)
        return STATUS_NULL_POINTER;

    size_t cap = START_CAPACITY;
    size_t len = 0;
    char *buf = malloc(cap);
    if (buf == NULL)
        return STATUS_MEMORY_ERROR;

    int c;
    while ((c = fgetc(f)) != EOF && c != '\n')
    {
        if (len + 1 >= cap)
        {
            if (cap > SIZE_MAX / 2)
            {
                free(buf);
                return STATUS_MEMORY_ERROR;
            }
            char *tmp = realloc(buf, cap * 2);
            if (tmp == NULL)
            {
                free(buf);
                return STATUS_MEMORY_ERROR;
            }
            buf = tmp;
            cap *= 2;
        }
        buf[len++] = (char)c;
    }

    if (c == EOF && ferror(f))
    {
        free(buf);
        return STATUS_FILE_READ_ERROR;
    }
    if (c == EOF && len == 0)
    {
        free(buf);
        *line = NULL;
        *eof = true;
        return STATUS_OK;
    }

    buf[len] = '\0';
    *line = buf;
    *eof = false;
    return STATUS_OK;
}

status_t is_blank_line(const char *line, bool *out)
{
    if (line == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    for (size_t i = 0; line[i] != '\0'; i++)
    {
        if (line[i] != ' ' && line[i] != '\t' && line[i] != '\r')
        {
            *out = false;
            return STATUS_OK;
        }
    }
    *out = true;
    return STATUS_OK;
}

status_t split_tokens(const char *line, const char **starts, size_t *lengths, size_t max,
                      size_t *count)
{
    if (line == NULL || starts == NULL || lengths == NULL || count == NULL)
        return STATUS_NULL_POINTER;

    size_t n = 0;
    size_t i = 0;
    while (line[i] != '\0')
    {
        while (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')
            i++;
        if (line[i] == '\0')
            break;

        size_t begin = i;
        while (line[i] != '\0' && line[i] != ' ' && line[i] != '\t' && line[i] != '\r')
            i++;

        if (n == max)
            return STATUS_INVALID_ARGUMENT;
        starts[n] = line + begin;
        lengths[n] = i - begin;
        n++;
    }
    *count = n;
    return STATUS_OK;
}

status_t parse_id(const char *s, size_t len, unsigned *out)
{
    if (s == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    if (len == 0)
        return STATUS_INVALID_INT_NUMBER;

    unsigned value = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (s[i] < '0' || s[i] > '9')
            return STATUS_INVALID_INT_NUMBER;
        unsigned d = (unsigned)(s[i] - '0');
        if (value > (UINT_MAX - d) / 10)
            return STATUS_OUT_OF_RANGE;
        value = value * 10 + d;
    }
    *out = value;
    return STATUS_OK;
}

status_t check_name(const char *s, size_t len)
{
    if (s == NULL)
        return STATUS_NULL_POINTER;
    if (len == 0)
        return STATUS_INVALID_ARGUMENT;

    for (size_t i = 0; i < len; i++)
    {
        bool latin = (s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z');
        if (!latin)
            return STATUS_INVALID_ARGUMENT;
    }
    return STATUS_OK;
}

status_t parse_salary(const char *s, size_t len, double *out)
{
    if (s == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    if (len == 0)
        return STATUS_INVALID_DOUBLE_NUMBER;

    size_t digits = 0;
    size_t dots = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (s[i] >= '0' && s[i] <= '9')
            digits++;
        else if (s[i] == '.')
            dots++;
        else
            return STATUS_INVALID_DOUBLE_NUMBER;
    }
    if (digits == 0 || dots > 1)
        return STATUS_INVALID_DOUBLE_NUMBER;

    char *end = NULL;
    double value = strtod(s, &end);
    if (end != s + len)
        return STATUS_INVALID_DOUBLE_NUMBER;
    if (!isfinite(value))
        return STATUS_OUT_OF_RANGE;

    *out = value;
    return STATUS_OK;
}

status_t copy_token(const char *s, size_t len, char **out)
{
    if (s == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    char *res = malloc(len + 1);
    if (res == NULL)
        return STATUS_MEMORY_ERROR;
    for (size_t i = 0; i < len; i++)
        res[i] = s[i];
    res[len] = '\0';
    *out = res;
    return STATUS_OK;
}

status_t parse_employee(const char *line, Employee *out)
{
    if (line == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    const char *starts[FIELD_COUNT];
    size_t lengths[FIELD_COUNT];
    size_t count = 0;
    status_t st = split_tokens(line, starts, lengths, FIELD_COUNT, &count);
    if (st != STATUS_OK)
        return st;
    if (count != FIELD_COUNT)
        return STATUS_INVALID_ARGUMENT;

    Employee e = {0, NULL, NULL, 0.0};
    st = parse_id(starts[0], lengths[0], &e.id);
    if (st == STATUS_OK)
        st = check_name(starts[1], lengths[1]);
    if (st == STATUS_OK)
        st = check_name(starts[2], lengths[2]);
    if (st == STATUS_OK)
        st = parse_salary(starts[3], lengths[3], &e.salary);
    if (st != STATUS_OK)
        return st;

    st = copy_token(starts[1], lengths[1], &e.name);
    if (st != STATUS_OK)
        return st;
    st = copy_token(starts[2], lengths[2], &e.surname);
    if (st != STATUS_OK)
    {
        free(e.name);
        return st;
    }

    *out = e;
    return STATUS_OK;
}

status_t free_employee_fields(Employee *e)
{
    if (e == NULL)
        return STATUS_NULL_POINTER;
    free(e->name);
    free(e->surname);
    e->name = NULL;
    e->surname = NULL;
    return STATUS_OK;
}

status_t free_employees(Employee *arr, size_t count)
{
    if (arr == NULL)
        return STATUS_OK;
    for (size_t i = 0; i < count; i++)
        free_employee_fields(&arr[i]);
    free(arr);
    return STATUS_OK;
}

status_t push_employee(Employee **arr, size_t *count, size_t *cap, Employee e)
{
    if (arr == NULL || count == NULL || cap == NULL)
        return STATUS_NULL_POINTER;

    if (*count == *cap)
    {
        size_t new_cap = (*cap == 0) ? START_CAPACITY : *cap * 2;
        if (new_cap > SIZE_MAX / sizeof(Employee))
            return STATUS_MEMORY_ERROR;
        Employee *tmp = realloc(*arr, new_cap * sizeof(Employee));
        if (tmp == NULL)
            return STATUS_MEMORY_ERROR;
        *arr = tmp;
        *cap = new_cap;
    }
    (*arr)[*count] = e;
    (*count)++;
    return STATUS_OK;
}

status_t read_employees(const char *path, Employee **out, size_t *out_count)
{
    if (path == NULL || out == NULL || out_count == NULL)
        return STATUS_NULL_POINTER;

    FILE *f = fopen(path, "r");
    if (f == NULL)
        return STATUS_FILE_OPEN_ERROR;

    Employee *arr = NULL;
    size_t count = 0;
    size_t cap = 0;
    status_t st = STATUS_OK;
    bool eof = false;

    while (st == STATUS_OK && !eof)
    {
        char *line = NULL;
        st = read_line(f, &line, &eof);
        if (st != STATUS_OK || eof)
            break;

        bool blank = false;
        st = is_blank_line(line, &blank);
        if (st == STATUS_OK && !blank)
        {
            Employee e;
            st = parse_employee(line, &e);
            if (st == STATUS_OK)
            {
                st = push_employee(&arr, &count, &cap, e);
                if (st != STATUS_OK)
                    free_employee_fields(&e);
            }
        }
        free(line);
    }

    fclose(f);

    if (st != STATUS_OK)
    {
        free_employees(arr, count);
        return st;
    }
    *out = arr;
    *out_count = count;
    return STATUS_OK;
}

status_t compare_double(double a, double b, double eps, int *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;

    if (fabs(a - b) < eps)
        *out = 0;
    else if (a < b)
        *out = -1;
    else
        *out = 1;
    return STATUS_OK;
}

int compare_employees_asc(const void *p1, const void *p2)
{
    const Employee *a = (const Employee *)p1;
    const Employee *b = (const Employee *)p2;

    int r = 0;
    compare_double(a->salary, b->salary, EPS, &r);
    if (r != 0)
        return r;

    r = strcmp(a->surname, b->surname);
    if (r != 0)
        return r;

    r = strcmp(a->name, b->name);
    if (r != 0)
        return r;

    if (a->id < b->id)
        return -1;
    if (a->id > b->id)
        return 1;
    return 0;
}

int compare_employees_desc(const void *p1, const void *p2)
{
    return compare_employees_asc(p2, p1);
}

status_t sort_employees(Employee *arr, size_t count, bool descending)
{
    if (arr == NULL && count > 0)
        return STATUS_NULL_POINTER;
    if (count < 2)
        return STATUS_OK;

    qsort(arr, count, sizeof(Employee),
          descending ? compare_employees_desc : compare_employees_asc);
    return STATUS_OK;
}

status_t write_employees(const char *path, const Employee *arr, size_t count)
{
    if (path == NULL || (arr == NULL && count > 0))
        return STATUS_NULL_POINTER;

    FILE *f = fopen(path, "w");
    if (f == NULL)
        return STATUS_FILE_OPEN_ERROR;

    status_t st = STATUS_OK;
    for (size_t i = 0; i < count && st == STATUS_OK; i++)
    {
        if (fprintf(f, "%u %s %s %.2f\n", arr[i].id, arr[i].name, arr[i].surname,
                    arr[i].salary) < 0)
            st = STATUS_FILE_WRITE_ERROR;
    }
    if (fclose(f) != 0 && st == STATUS_OK)
        st = STATUS_FILE_WRITE_ERROR;
    return st;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        print_error(STATUS_INVALID_ARGUMENT);
        printf("Формат: %s [входной файл] [-a|-d|/a|/d] [выходной файл]\n", argv[0]);
        return 1;
    }

    bool descending = false;
    status_t st = parse_flag(argv[2], &descending);
    if (st != STATUS_OK)
    {
        print_error(st);
        return 1;
    }

    bool same = false;
    st = is_same_file(argv[1], argv[3], &same);
    if (st == STATUS_OK && same)
        st = STATUS_SAME_FILE;
    if (st != STATUS_OK)
    {
        print_error(st);
        return 1;
    }

    Employee *arr = NULL;
    size_t count = 0;

    st = read_employees(argv[1], &arr, &count);
    if (st == STATUS_OK)
        st = sort_employees(arr, count, descending);
    if (st == STATUS_OK)
        st = write_employees(argv[3], arr, count);

    if (st != STATUS_OK)
        print_error(st);

    free_employees(arr, count);
    return (st == STATUS_OK) ? 0 : 1;
}