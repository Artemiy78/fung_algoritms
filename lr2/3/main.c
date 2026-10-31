#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define START_CAPACITY 4

typedef struct
{
    size_t line;
    size_t col;
} Entry;

typedef struct
{
    const char *path;
    status_t status;
    Entry *entries;
    size_t count;
} FileResult;

typedef struct
{
    const char *sub;
    size_t len;
    char *window;
    size_t head;
    size_t filled;
    size_t line;
    size_t col;
} Searcher;

status_t searcher_init(Searcher *s, const char *sub)
{
    if (s == NULL || sub == NULL)
        return STATUS_NULL_POINTER;

    size_t len = strlen(sub);
    if (len == 0)
        return STATUS_EMPTY_SUBSTRING;

    char *window = malloc(len);
    if (window == NULL)
        return STATUS_MEMORY_ERROR;

    s->sub = sub;
    s->len = len;
    s->window = window;
    s->head = 0;
    s->filled = 0;
    s->line = 1;
    s->col = 1;
    return STATUS_OK;
}

status_t searcher_free(Searcher *s)
{
    if (s == NULL)
        return STATUS_NULL_POINTER;
    free(s->window);
    s->window = NULL;
    return STATUS_OK;
}

status_t window_matches(const Searcher *s, bool *out)
{
    if (s == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    *out = false;
    if (s->filled < s->len)
        return STATUS_OK;
    for (size_t k = 0; k < s->len; k++)
    {
        if (s->window[(s->head + k) % s->len] != s->sub[k])
            return STATUS_OK;
    }
    *out = true;
    return STATUS_OK;
}

status_t searcher_feed(Searcher *s, char ch, bool *found)
{
    if (s == NULL || found == NULL)
        return STATUS_NULL_POINTER;

    if (s->filled == s->len)
    {
        if (s->window[s->head] == '\n')
        {
            s->line++;
            s->col = 1;
        }
        else
        {
            s->col++;
        }
        s->window[s->head] = ch;
        s->head = (s->head + 1) % s->len;
    }
    else
    {
        s->window[(s->head + s->filled) % s->len] = ch;
        s->filled++;
    }
    return window_matches(s, found);
}

status_t add_entry(Entry **entries, size_t *count, size_t *cap, size_t line, size_t col)
{
    if (entries == NULL || count == NULL || cap == NULL)
        return STATUS_NULL_POINTER;

    if (*count == *cap)
    {
        size_t new_cap = (*cap == 0) ? START_CAPACITY : *cap * 2;
        if (new_cap > SIZE_MAX / sizeof(Entry))
            return STATUS_MEMORY_ERROR;
        Entry *tmp = realloc(*entries, new_cap * sizeof(Entry));
        if (tmp == NULL)
            return STATUS_MEMORY_ERROR;
        *entries = tmp;
        *cap = new_cap;
    }
    (*entries)[*count].line = line;
    (*entries)[*count].col = col;
    (*count)++;
    return STATUS_OK;
}

status_t free_results(FileResult *res, size_t count)
{
    if (res == NULL)
        return STATUS_OK;
    for (size_t i = 0; i < count; i++)
        free(res[i].entries);
    free(res);
    return STATUS_OK;
}

status_t search_file(const char *path, const char *sub, Entry **entries, size_t *count)
{
    if (path == NULL || sub == NULL || entries == NULL || count == NULL)
        return STATUS_NULL_POINTER;

    FILE *f = fopen(path, "r");
    if (f == NULL)
        return STATUS_FILE_OPEN_ERROR;

    Searcher s;
    status_t st = searcher_init(&s, sub);
    if (st != STATUS_OK)
    {
        fclose(f);
        return st;
    }

    Entry *list = NULL;
    size_t n = 0;
    size_t cap = 0;
    int c;
    while (st == STATUS_OK && (c = fgetc(f)) != EOF)
    {
        bool found = false;
        st = searcher_feed(&s, (char)c, &found);
        if (st == STATUS_OK && found)
            st = add_entry(&list, &n, &cap, s.line, s.col);
    }
    if (st == STATUS_OK && ferror(f))
        st = STATUS_FILE_READ_ERROR;

    fclose(f);
    searcher_free(&s);

    if (st != STATUS_OK)
    {
        free(list);
        return st;
    }
    *entries = list;
    *count = n;
    return STATUS_OK;
}

status_t find_in_files(const char *sub, size_t file_count, FileResult **out, ...)
{
    if (sub == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    if (sub[0] == '\0')
        return STATUS_EMPTY_SUBSTRING;
    if (file_count == 0)
    {
        *out = NULL;
        return STATUS_OK;
    }

    FileResult *res = calloc(file_count, sizeof(FileResult));
    if (res == NULL)
        return STATUS_MEMORY_ERROR;

    va_list args;
    va_start(args, out);
    for (size_t k = 0; k < file_count; k++)
    {
        res[k].path = va_arg(args, const char *);
        if (res[k].path == NULL)
            res[k].status = STATUS_NULL_POINTER;
        else
            res[k].status = search_file(res[k].path, sub, &res[k].entries, &res[k].count);
    }
    va_end(args);

    *out = res;
    return STATUS_OK;
}

status_t print_visible(const char *s)
{
    if (s == NULL)
        return STATUS_NULL_POINTER;
    for (size_t i = 0; s[i] != '\0'; i++)
    {
        if (s[i] == '\n')
            printf("\\n");
        else if (s[i] == '\t')
            printf("\\t");
        else if (s[i] == ' ')
            putchar('_');
        else
            putchar(s[i]);
    }
    return STATUS_OK;
}

status_t print_results(const char *sub, const FileResult *res, size_t count)
{
    if (sub == NULL || (res == NULL && count > 0))
        return STATUS_NULL_POINTER;

    printf("Подстрока: \"");
    print_visible(sub);
    printf("\"\n");
    for (size_t i = 0; i < count; i++)
    {
        printf("Файл %s: ", res[i].path != NULL ? res[i].path : "(null)");
        if (res[i].status != STATUS_OK)
        {
            print_error(res[i].status);
            continue;
        }
        printf("вхождений: %zu\n", res[i].count);
        for (size_t j = 0; j < res[i].count; j++)
            printf("строка %zu, символ %zu\n", res[i].entries[j].line,
                   res[i].entries[j].col);
    }
    printf("\n");
    return STATUS_OK;
}

status_t report(const char *sub, status_t st, FileResult **res, size_t count)
{
    if (res == NULL)
        return STATUS_NULL_POINTER;

    if (st == STATUS_OK)
        st = print_results(sub, *res, count);
    else
    {
        printf("Подстрока \"%s\": ", sub);
        print_error(st);
        printf("\n");
    }
    free_results(*res, count);
    *res = NULL;
    return st;
}

int main(void)
{

    FileResult *res = NULL;
    status_t st;

    st = find_in_files("abc", 4, &res, "t1.txt", "t2.txt", "empty.txt", "no_such_file.txt");
    report("abc", st, &res, 4);

    st = find_in_files("c\na", 1, &res, "t1.txt");
    report("c\na", st, &res, 1);

    st = find_in_files("aa", 1, &res, "t2.txt");
    report("aa", st, &res, 1);

    st = find_in_files("  ", 1, &res, "t3.txt");
    report("  ", st, &res, 1);

    st = find_in_files("\n\t", 1, &res, "t3.txt");
    report("\n\t", st, &res, 1);

    st = find_in_files("abcabc\nabcX", 1, &res, "t1.txt");
    report("abcabc\nabcX", st, &res, 1);

    st = find_in_files("needle", 1, &res, "big.txt");
    report("needle", st, &res, 1);

    st = find_in_files("", 1, &res, "t1.txt");
    report("", st, &res, 1);

    return 0;
}