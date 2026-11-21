#include "post.h"
#include <string.h>
#include <time.h>

void free_address(Address *a)
{
    string_free(&a->city);
    string_free(&a->street);
    string_free(&a->building);
    string_free(&a->index);
}

void free_mail(Mail *m)
{
    free_address(&m->to);
    string_free(&m->id);
    string_free(&m->created);
    string_free(&m->deadline);
}

void free_post(Post *p)
{
    for (size_t i = 0; i < p->size; i++)
        free_mail(&p->mails[i]);
    free(p->mails);
}

ull make_key(const int y, const int mo, const int d, const int h, const int mi, const int s)
{
    return ((((((ull)y * 100 + mo) * 100 + d) * 100 + h) * 100 + mi) * 100 + s);
}

int num(const char *s, const int from, const int len)
{
    int v = 0;
    for (int i = from; i < from + len; i++)
        v = v * 10 + (s[i] - '0');
    return v;
}

status_t parse_time(const char *s, ull *key)
{
    if (s == NULL || key == NULL)
        return STATUS_NULL_POINTER;
    if (strlen(s) != 19)
        return STATUS_INVALID_ARGUMENT;
    for (int i = 0; i < 19; i++)
    {
        bool ok;
        if (i == 10)
        {
            ok = (s[i] == ' ');
        }
        else if (i == 2 || i == 5 || i == 13 || i == 16)
        {
            ok = (s[i] == ':');
        }
        else
        {
            ok = IS_DIG(s[i]);
        }
        if (!ok)
            return STATUS_INVALID_ARGUMENT;
    }

    int d = num(s, 0, 2), mo = num(s, 3, 2), y = num(s, 6, 4);
    int h = num(s, 11, 2), mi = num(s, 14, 2), sec = num(s, 17, 2);
    const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (mo < 1 || mo > 12 || y < 1 || h > 23 || mi > 59 || sec > 59)
        return STATUS_INVALID_ARGUMENT;

    bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    if (d < 1 || d > days[mo - 1] + ((mo == 2 && leap) ? 1 : 0))
        return STATUS_INVALID_ARGUMENT;

    *key = make_key(y, mo, d, h, mi, sec);
    return STATUS_OK;
}

status_t now_key(ull *key)
{
    if (key == NULL)
        return STATUS_NULL_POINTER;
    time_t t = time(NULL);
    const struct tm *lt = localtime(&t);
    if (t == (time_t)-1 || lt == NULL)
        return STATUS_INVALID_ARGUMENT;
    *key = make_key(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min,
                    lt->tm_sec);
    return STATUS_OK;
}

status_t mail_cmp(const Mail *a, const Mail *b, int *res)
{
    if (a == NULL || b == NULL)
        return STATUS_NULL_POINTER;
    status_t st = string_cmp(&a->to.index, &b->to.index, res);
    if (st == STATUS_OK && *res == 0)
        st = string_cmp(&a->id, &b->id, res);
    return st;
}

status_t find_index(const Post *p, const String *id, size_t *idx, bool *found)
{
    if (p == NULL || id == NULL || idx == NULL || found == NULL)
        return STATUS_NULL_POINTER;
    *found = false;
    for (size_t i = 0; i < p->size && !*found; i++)
    {
        status_t st = string_eq(&p->mails[i].id, id, found);
        if (st != STATUS_OK)
            return st;
        if (*found)
            *idx = i;
    }
    return STATUS_OK;
}

status_t post_add(Post *p, const Mail *m)
{
    if (p == NULL || m == NULL)
        return STATUS_NULL_POINTER;

    size_t idx = 0;
    bool dup = false;
    status_t st = find_index(p, &m->id, &idx, &dup);
    if (st != STATUS_OK)
        return st;
    if (dup)
        return STATUS_INVALID_ARGUMENT;

    size_t pos = 0;
    while (pos < p->size)
    {
        int c = 0;
        st = mail_cmp(&p->mails[pos], m, &c);
        if (st != STATUS_OK)
            return st;
        if (c > 0)
            break;
        pos++;
    }

    if (p->size == p->cap)
    {
        size_t new_cap = p->cap ? p->cap * 2 : 4;
        Mail *tmp = realloc(p->mails, new_cap * sizeof(Mail));
        if (tmp == NULL)
            return STATUS_MEMORY_ERROR;
        p->mails = tmp;
        p->cap = new_cap;
    }
    memmove(&p->mails[pos + 1], &p->mails[pos], (p->size - pos) * sizeof(Mail));
    p->mails[pos] = *m;
    p->size++;
    return STATUS_OK;
}

status_t post_remove(Post *p, const size_t idx)
{
    if (p == NULL)
        return STATUS_NULL_POINTER;
    if (idx >= p->size)
        return STATUS_OUT_OF_RANGE;

    free_mail(&p->mails[idx]);
    memmove(&p->mails[idx], &p->mails[idx + 1], (p->size - idx - 1) * sizeof(Mail));
    p->size--;
    return STATUS_OK;
}

int cmp_created(const void *a, const void *b)
{
    const Mail *x = *(const Mail *const *)a;
    const Mail *y = *(const Mail *const *)b;
    return (x->created_key > y->created_key) - (x->created_key < y->created_key);
}

status_t post_select(const Post *p, const ull now, const bool delivered, const Mail ***out,
                     size_t *count)
{
    if (p == NULL || out == NULL || count == NULL)
        return STATUS_NULL_POINTER;

    const Mail **res = malloc((p->size + 1) * sizeof(*res));
    if (res == NULL)
        return STATUS_MEMORY_ERROR;

    size_t c = 0;
    for (size_t i = 0; i < p->size; i++)
    {
        const Mail *m = &p->mails[i];
        bool take = false;

        if (delivered)
        {
            take = m->delivered;
        }
        else
        {
            bool overdue = m->deadline_key < now;
            take = !m->delivered && overdue;
        }

        if (take)
            res[c++] = m;
    }
    qsort(res, c, sizeof(*res), cmp_created);

    *out = res;
    *count = c;
    return STATUS_OK;
}