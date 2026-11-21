#include "../utils.h"

#define IS_DIG(c) ((c) >= '0' && (c) <= '9')

typedef struct
{
    int id;
    int start;
    int finish;
} talk_t;

int cmp_finish(const void *a, const void *b)
{
    const talk_t *x = a;
    const talk_t *y = b;
    return (x->finish > y->finish) - (x->finish < y->finish);
}

status_t max_talks(const talk_t *talks, const size_t n, size_t k, size_t *out)
{
    if (talks == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    if (n == 0 || k == 0)
        return STATUS_INVALID_COUNT;
    if (k > n)
        k = n;

    talk_t *sorted = malloc(n * sizeof(talk_t));
    int *room_end = calloc(k, sizeof(int));
    if (sorted == NULL || room_end == NULL)
    {
        free(sorted);
        free(room_end);
        return STATUS_MEMORY_ERROR;
    }
    for (size_t i = 0; i < n; i++)
        sorted[i] = talks[i];
    qsort(sorted, n, sizeof(talk_t), cmp_finish);

    size_t count = 0;
    for (size_t i = 0; i < n; i++)
    {
        size_t best = k;
        for (size_t j = 0; j < k; j++)
            if (room_end[j] <= sorted[i].start &&
                (best == k || room_end[j] > room_end[best]))
                best = j;

        if (best != k)
        {
            room_end[best] = sorted[i].finish;
            count++;
        }
    }

    free(sorted);
    free(room_end);
    *out = count;
    return STATUS_OK;
}

status_t read_int(FILE *in, int *out)
{
    char s[32];
    if (fscanf(in, "%31s", s) != 1)
        return STATUS_INVALID_NUMBER;
    return parse_int(s, out);
}

status_t read_time(FILE *in, int *out)
{
    char s[32];
    if (fscanf(in, "%31s", s) != 1)
        return STATUS_INVALID_NUMBER;

    if (!(IS_DIG(s[0]) && IS_DIG(s[1]) && s[2] == ':' &&
          IS_DIG(s[3]) && IS_DIG(s[4]) && s[5] == '\0'))
        return STATUS_INVALID_ARGUMENT;

    int h = (s[0] - '0') * 10 + (s[1] - '0');
    int m = (s[3] - '0') * 10 + (s[4] - '0');
    if (h > 23 || m > 59)
        return STATUS_INVALID_ARGUMENT;

    *out = h * 60 + m;
    return STATUS_OK;
}

status_t read_talk(FILE *in, talk_t *t)
{
    status_t st = read_int(in, &t->id);
    if (st != STATUS_OK)
        return st;
    st = read_time(in, &t->start);
    if (st != STATUS_OK)
        return st;
    st = read_time(in, &t->finish);
    if (st != STATUS_OK)
        return st;

    return (t->id > 0 && t->start < t->finish) ? STATUS_OK : STATUS_INVALID_ARGUMENT;
}

status_t read_input(FILE *in, talk_t **talks, size_t *n, size_t *k)
{
    if (in == NULL || talks == NULL || n == NULL || k == NULL)
        return STATUS_NULL_POINTER;

    int nn, kk;
    status_t st = read_int(in, &nn);
    if (st != STATUS_OK)
        return st;
    st = read_int(in, &kk);
    if (st != STATUS_OK)
        return st;
    if (nn <= 0 || kk <= 0)
        return STATUS_INVALID_COUNT;

    talk_t *arr = malloc((size_t)nn * sizeof(talk_t));
    if (arr == NULL)
        return STATUS_MEMORY_ERROR;

    for (int i = 0; i < nn && st == STATUS_OK; i++)
    {
        st = read_talk(in, &arr[i]);
        for (int j = 0; j < i && st == STATUS_OK; j++)
            if (arr[j].id == arr[i].id)
                st = STATUS_INVALID_ARGUMENT;
    }
    if (st != STATUS_OK)
    {
        free(arr);
        return st;
    }

    *talks = arr;
    *n = (size_t)nn;
    *k = (size_t)kk;
    return STATUS_OK;
}

int main(void)
{
    talk_t *talks = NULL;
    size_t n = 0, k = 0, result = 0;

    status_t st = read_input(stdin, &talks, &n, &k);
    if (st == STATUS_OK)
        st = max_talks(talks, n, k, &result);

    if (st == STATUS_OK)
        printf("%zu\n", result);
    else
        print_error(st);

    free(talks);
    return (st == STATUS_OK) ? 0 : 1;
}