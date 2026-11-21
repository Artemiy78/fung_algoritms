#include "../utils.h"
#include <string.h>

typedef unsigned long long ull;

typedef struct node
{
    ull weight;
    int byte;
    struct node *left, *right;
} node_t;

typedef struct
{
    node_t **data;
    size_t size;
    size_t cap;
} heap_t;

void swap(node_t **a, node_t **b)
{
    node_t *tmp = *a;
    *a = *b;
    *b = tmp;
}

status_t heap_push(heap_t *h, node_t *n)
{
    if (h == NULL || n == NULL)
        return STATUS_NULL_POINTER;
    if (h->size >= h->cap)
        return STATUS_OUT_OF_RANGE;

    size_t i = h->size++;
    h->data[i] = n;
    while (i > 0 && h->data[i]->weight < h->data[(i - 1) / 2]->weight)
    {
        swap(&h->data[i], &h->data[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
    return STATUS_OK;
}

status_t heap_pop(heap_t *h, node_t **out)
{
    if (h == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    if (h->size == 0)
        return STATUS_OUT_OF_RANGE;

    *out = h->data[0];
    h->data[0] = h->data[--h->size];

    size_t i = 0;
    while (true)
    {
        size_t l = 2 * i + 1, r = l + 1, m = i;
        if (l < h->size && h->data[l]->weight < h->data[m]->weight)
            m = l;
        if (r < h->size && h->data[r]->weight < h->data[m]->weight)
            m = r;
        if (m == i)
            break;
        swap(&h->data[i], &h->data[m]);
        i = m;
    }
    return STATUS_OK;
}

status_t count_freq(const char *path, ull *freq)
{
    if (path == NULL || freq == NULL)
        return STATUS_NULL_POINTER;

    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return STATUS_FILE_OPEN_ERROR;

    int c;
    while ((c = fgetc(f)) != EOF)
        freq[c]++;

    status_t st = ferror(f) ? STATUS_FILE_READ_ERROR : STATUS_OK;
    fclose(f);
    return st;
}

status_t build_tree(const ull *freq, node_t **pool, node_t **root)
{
    if (freq == NULL || pool == NULL || root == NULL)
        return STATUS_NULL_POINTER;
    *pool = NULL;
    *root = NULL;

    size_t leaves = 0;
    for (int b = 0; b < 256; b++)
        if (freq[b] > 0)
            leaves++;
    if (leaves == 0)
        return STATUS_OK;

    node_t *nodes = malloc((2 * leaves - 1) * sizeof(node_t));
    heap_t h = {malloc(leaves * sizeof(node_t *)), 0, leaves};
    if (nodes == NULL || h.data == NULL)
    {
        free(nodes);
        free(h.data);
        return STATUS_MEMORY_ERROR;
    }

    size_t used = 0;
    status_t st = STATUS_OK;
    for (int b = 0; b < 256 && st == STATUS_OK; b++)
        if (freq[b] > 0)
        {
            nodes[used] = (node_t){freq[b], b, NULL, NULL};
            st = heap_push(&h, &nodes[used++]);
        }

    while (st == STATUS_OK && h.size > 1)
    {
        node_t *a = NULL, *c = NULL;
        st = heap_pop(&h, &a);
        if (st == STATUS_OK)
            st = heap_pop(&h, &c);
        if (st == STATUS_OK)
        {
            nodes[used] = (node_t){a->weight + c->weight, -1, a, c};
            st = heap_push(&h, &nodes[used++]);
        }
    }

    if (st == STATUS_OK)
    {
        *pool = nodes;
        *root = h.data[0];
    }
    else
        free(nodes);
    free(h.data);
    return st;
}

void fill_codes(const node_t *n, char (*codes)[256], char *path, const size_t depth)
{
    if (n->left == NULL && n->right == NULL)
    {
        path[depth] = '\0';
        strcpy(codes[n->byte], path);
        return;
    }
    path[depth] = '0';
    fill_codes(n->left, codes, path, depth + 1);
    path[depth] = '1';
    fill_codes(n->right, codes, path, depth + 1);
}

status_t build_codes(const node_t *root, char (*codes)[256])
{
    if (codes == NULL)
        return STATUS_NULL_POINTER;
    if (root == NULL)
        return STATUS_OK;
    if (root->left == NULL && root->right == NULL)
    {
        strcpy(codes[root->byte], "0");
        return STATUS_OK;
    }

    char *path = malloc(256);
    if (path == NULL)
        return STATUS_MEMORY_ERROR;
    fill_codes(root, codes, path, 0);
    free(path);
    return STATUS_OK;
}

status_t calc_stats(const ull *freq, char (*codes)[256], ull *size, ull *bits)
{
    if (freq == NULL || codes == NULL || size == NULL || bits == NULL)
        return STATUS_NULL_POINTER;

    *size = 0;
    *bits = 0;
    for (int b = 0; b < 256; b++)
    {
        *size += freq[b];
        *bits += freq[b] * strlen(codes[b]);
    }
    return STATUS_OK;
}

status_t print_report(const ull *freq, char (*codes)[256], const ull size, const ull bits)
{
    if (freq == NULL || codes == NULL)
        return STATUS_NULL_POINTER;

    if (size > 0)
    {
        printf("Байт  Частота  Код\n");
        for (int b = 0; b < 256; b++)
            if (freq[b] > 0)
                printf("%4d  %7llu  %s\n", b, freq[b], codes[b]);
    }

    ull bytes = (bits + 7) / 8;
    double saving = (size == 0) ? 0.0 : 100.0 * (1.0 - (double)bytes / (double)size);

    printf("Размер файла: %llu байт\n", size);
    printf("Закодировано: %llu бит (%llu байт)\n", bits, bytes);
    printf("Экономия: %.2f%%\n", saving);
    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Формат: [program] [путь к файлу]\n");
        return 1;
    }

    ull *freq = calloc(256, sizeof(ull));
    char (*codes)[256] = calloc(256, sizeof(*codes));
    node_t *pool = NULL, *root = NULL;
    ull size = 0, bits = 0;

    status_t st = (freq == NULL || codes == NULL) ? STATUS_MEMORY_ERROR : STATUS_OK;
    if (st == STATUS_OK)
        st = count_freq(argv[1], freq);
    if (st == STATUS_OK)
        st = build_tree(freq, &pool, &root);
    if (st == STATUS_OK)
        st = build_codes(root, codes);
    if (st == STATUS_OK)
        st = calc_stats(freq, codes, &size, &bits);
    if (st == STATUS_OK)
        st = print_report(freq, codes, size, bits);

    if (st != STATUS_OK)
        print_error(st);

    free(freq);
    free(codes);
    free(pool);
    return (st == STATUS_OK) ? 0 : 1;
}