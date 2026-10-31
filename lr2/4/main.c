#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

#define START_CAPACITY 16

typedef unsigned long long ull;

typedef struct
{
    char *name;
    ull size;
    ull importance;
} FileInfo;

typedef struct
{
    const FileInfo **files;
    size_t count;
    ull total_size;
    ull total_importance;
} Backup;

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

status_t read_data_line(FILE *f, char **out)
{
    if (f == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    while (true)
    {
        char *line = NULL;
        bool eof = false;
        status_t st = read_line(f, &line, &eof);
        if (st != STATUS_OK)
            return st;
        if (eof)
            return STATUS_INVALID_ARGUMENT;

        size_t i = 0;
        while (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')
            i++;
        if (line[i] != '\0')
        {
            *out = line;
            return STATUS_OK;
        }
        free(line);
    }
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

status_t parse_value(const char *s, size_t len, ull *out)
{
    if (s == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    if (len == 0)
        return STATUS_INVALID_INT_NUMBER;

    ull value = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (s[i] < '0' || s[i] > '9')
            return STATUS_INVALID_INT_NUMBER;
        ull d = (ull)(s[i] - '0');
        if (value > (ULLONG_MAX - d) / 10)
            return STATUS_OUT_OF_RANGE;
        value = value * 10 + d;
    }
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

status_t parse_header(const char *line, size_t *n, ull *capacity)
{
    if (line == NULL || n == NULL || capacity == NULL)
        return STATUS_NULL_POINTER;

    const char *starts[2];
    size_t lengths[2];
    size_t count = 0;
    status_t st = split_tokens(line, starts, lengths, 2, &count);
    if (st != STATUS_OK)
        return st;
    if (count != 2)
        return STATUS_INVALID_ARGUMENT;

    ull n_value = 0;
    st = parse_value(starts[0], lengths[0], &n_value);
    if (st == STATUS_OK)
        st = parse_value(starts[1], lengths[1], capacity);
    if (st != STATUS_OK)
        return st;
    if (n_value == 0 || n_value > SIZE_MAX)
        return STATUS_OUT_OF_RANGE;

    *n = (size_t)n_value;
    return STATUS_OK;
}

status_t parse_file_line(const char *line, FileInfo *out)
{
    if (line == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    const char *starts[3];
    size_t lengths[3];
    size_t count = 0;
    status_t st = split_tokens(line, starts, lengths, 3, &count);
    if (st != STATUS_OK)
        return st;
    if (count != 3)
        return STATUS_INVALID_ARGUMENT;

    FileInfo f = {NULL, 0, 0};
    st = parse_value(starts[1], lengths[1], &f.size);
    if (st == STATUS_OK)
        st = parse_value(starts[2], lengths[2], &f.importance);
    if (st != STATUS_OK)
        return st;
    if (f.size == 0)
        return STATUS_OUT_OF_RANGE;

    st = copy_token(starts[0], lengths[0], &f.name);
    if (st != STATUS_OK)
        return st;

    *out = f;
    return STATUS_OK;
}

status_t free_files(FileInfo *files, size_t n)
{
    if (files == NULL)
        return STATUS_OK;
    for (size_t i = 0; i < n; i++)
        free(files[i].name);
    free(files);
    return STATUS_OK;
}

status_t free_backup(Backup *b)
{
    if (b == NULL)
        return STATUS_NULL_POINTER;
    free(b->files);
    b->files = NULL;
    b->count = 0;
    return STATUS_OK;
}

status_t read_input(FILE *in, FileInfo **files, size_t *n, ull *capacity)
{
    if (in == NULL || files == NULL || n == NULL || capacity == NULL)
        return STATUS_NULL_POINTER;

    char *line = NULL;
    status_t st = read_data_line(in, &line);
    if (st != STATUS_OK)
        return st;
    st = parse_header(line, n, capacity);
    free(line);
    if (st != STATUS_OK)
        return st;

    FileInfo *arr = calloc(*n, sizeof(FileInfo));
    if (arr == NULL)
        return STATUS_MEMORY_ERROR;

    for (size_t i = 0; i < *n && st == STATUS_OK; i++)
    {
        st = read_data_line(in, &line);
        if (st == STATUS_OK)
        {
            st = parse_file_line(line, &arr[i]);
            free(line);
        }
    }
    if (st != STATUS_OK)
    {
        free_files(arr, *n);
        return st;
    }
    *files = arr;
    return STATUS_OK;
}

status_t build_table(const FileInfo *files, size_t n, size_t cols, ull **table)
{
    if (files == NULL || table == NULL)
        return STATUS_NULL_POINTER;
    if (n >= SIZE_MAX || cols > SIZE_MAX / (n + 1))
        return STATUS_MEMORY_ERROR;

    ull *t = calloc((n + 1) * cols, sizeof(ull));
    if (t == NULL)
        return STATUS_MEMORY_ERROR;

    for (size_t i = 1; i <= n; i++)
    {
        for (size_t w = 0; w < cols; w++)
        {
            ull best = t[(i - 1) * cols + w];
            if (files[i - 1].size <= w)
            {
                ull with_file =
                    t[(i - 1) * cols + (w - files[i - 1].size)] + files[i - 1].importance;
                if (with_file > best)
                    best = with_file;
            }
            t[i * cols + w] = best;
        }
    }
    *table = t;
    return STATUS_OK;
}

status_t restore_selection(const ull *table, const FileInfo *files, size_t n, size_t cols,
                           Backup *out)
{
    if (table == NULL || files == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    unsigned char *taken = calloc(n + 1, 1);
    if (taken == NULL)
        return STATUS_MEMORY_ERROR;

    size_t count = 0;
    ull total_size = 0;
    ull total_importance = 0;
    size_t w = cols - 1;
    for (size_t i = n; i > 0; i--)
    {
        if (table[i * cols + w] != table[(i - 1) * cols + w])
        {
            taken[i - 1] = 1;
            count++;
            total_size += files[i - 1].size;
            total_importance += files[i - 1].importance;
            w -= (size_t)files[i - 1].size;
        }
    }

    const FileInfo **chosen = NULL;
    if (count > 0)
    {
        chosen = malloc(count * sizeof(*chosen));
        if (chosen == NULL)
        {
            free(taken);
            return STATUS_MEMORY_ERROR;
        }
        size_t k = 0;
        for (size_t i = 0; i < n; i++)
        {
            if (taken[i])
                chosen[k++] = &files[i];
        }
    }
    free(taken);

    out->files = chosen;
    out->count = count;
    out->total_size = total_size;
    out->total_importance = total_importance;
    return STATUS_OK;
}

status_t select_files(const FileInfo *files, size_t n, ull capacity, Backup *out)
{
    if (out == NULL || (files == NULL && n > 0))
        return STATUS_NULL_POINTER;

    ull sum_size = 0;
    ull sum_importance = 0;
    for (size_t i = 0; i < n; i++)
    {
        sum_size = (sum_size > ULLONG_MAX - files[i].size) ? ULLONG_MAX : sum_size + files[i].size;
        if (sum_importance > ULLONG_MAX - files[i].importance)
            return STATUS_OUT_OF_RANGE;
        sum_importance += files[i].importance;
    }

    ull limit = (capacity < sum_size) ? capacity : sum_size;
    if (limit >= SIZE_MAX)
        return STATUS_MEMORY_ERROR;
    size_t cols = (size_t)limit + 1;

    ull *table = NULL;
    status_t st = STATUS_OK;
    if (n > 0)
        st = build_table(files, n, cols, &table);
    if (st != STATUS_OK)
        return st;

    if (n > 0)
        st = restore_selection(table, files, n, cols, out);
    else
        *out = (Backup){NULL, 0, 0, 0};
    free(table);
    return st;
}

status_t print_backup(const Backup *b)
{
    if (b == NULL || (b->files == NULL && b->count > 0))
        return STATUS_NULL_POINTER;

    printf("TOTAL_SIZE %llu\n", b->total_size);
    printf("TOTAL_IMPORTANCE %llu\n", b->total_importance);
    printf("FILES %zu\n", b->count);
    for (size_t i = 0; i < b->count; i++)
        printf("%s\n", b->files[i]->name);
    return STATUS_OK;
}

int main(void)
{
    FileInfo *files = NULL;
    size_t n = 0;
    ull capacity = 0;
    Backup result = {NULL, 0, 0, 0};

    status_t st = read_input(stdin, &files, &n, &capacity);
    if (st == STATUS_OK)
        st = select_files(files, n, capacity, &result);
    if (st == STATUS_OK)
        st = print_backup(&result);
    if (st != STATUS_OK)
        print_error(st);

    free_backup(&result);
    free_files(files, n);
    return (st == STATUS_OK) ? 0 : 1;
}