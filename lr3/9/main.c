#include "../utils.h"
#include <string.h>

#define LINE 1024

typedef unsigned long long ull;

typedef struct node
{
    char *word;
    ull count;
    struct node *left, *right;
} node_t;

status_t tree_add(node_t **root, const char *word, const ull n)
{
    if (root == NULL || word == NULL)
        return STATUS_NULL_POINTER;

    node_t **pp = root;
    while (*pp != NULL)
    {
        int c = strcmp(word, (*pp)->word);
        if (c == 0)
        {
            (*pp)->count += n;
            return STATUS_OK;
        }
        if (c < 0)
            pp = &(*pp)->left;
        else
            pp = &(*pp)->right;
    }

    node_t *node = calloc(1, sizeof(node_t));
    if (node == NULL)
        return STATUS_MEMORY_ERROR;
    size_t len = strlen(word) + 1;
    node->word = malloc(len);
    if (node->word == NULL)
    {
        free(node);
        return STATUS_MEMORY_ERROR;
    }
    memcpy(node->word, word, len);
    node->count = n;
    *pp = node;
    return STATUS_OK;
}

void tree_free(node_t *root)
{
    if (root == NULL)
        return;
    tree_free(root->left);
    tree_free(root->right);
    free(root->word);
    free(root);
}

status_t tree_find(const node_t *root, const char *word, ull *count)
{
    if (word == NULL || count == NULL)
        return STATUS_NULL_POINTER;

    *count = 0;
    const node_t *cur = root;
    while (cur != NULL)
    {
        int c = strcmp(word, cur->word);
        if (c == 0)
        {
            *count = cur->count;
            return STATUS_OK;
        }
        if (c < 0)
            cur = cur->left;
        else
            cur = cur->right;
    }
    return STATUS_OK;
}

size_t count_nodes(const node_t *r)
{
    if (r == NULL)
        return 0;
    return 1 + count_nodes(r->left) + count_nodes(r->right);
}

void fill(const node_t *r, const node_t **arr, size_t *i)
{
    if (r == NULL)
        return;
    fill(r->left, arr, i);
    arr[(*i)++] = r;
    fill(r->right, arr, i);
}

status_t tree_nodes(const node_t *root, const node_t ***out, size_t *n)
{
    if (out == NULL || n == NULL)
        return STATUS_NULL_POINTER;

    size_t cnt = count_nodes(root);
    const node_t **arr = malloc((cnt + 1) * sizeof(*arr));
    if (arr == NULL)
        return STATUS_MEMORY_ERROR;

    size_t i = 0;
    fill(root, arr, &i);
    *out = arr;
    *n = cnt;
    return STATUS_OK;
}

int cmp_count(const void *a, const void *b)
{
    const node_t *x = *(const node_t *const *)a;
    const node_t *y = *(const node_t *const *)b;
    if (x->count != y->count)
        return (y->count > x->count) - (y->count < x->count);
    return strcmp(x->word, y->word);
}

status_t top_words(const node_t *root, const size_t n, const node_t ***out, size_t *count)
{
    if (out == NULL || count == NULL)
        return STATUS_NULL_POINTER;

    size_t total = 0;
    status_t st = tree_nodes(root, out, &total);
    if (st != STATUS_OK)
        return st;

    qsort(*out, total, sizeof(**out), cmp_count);
    *count = total;
    if (n < total)
        *count = n;
    return STATUS_OK;
}

status_t find_extremes(const node_t *root, const char **longest, const char **shortest)
{
    if (longest == NULL || shortest == NULL)
        return STATUS_NULL_POINTER;

    const node_t **arr = NULL;
    size_t n = 0;
    status_t st = tree_nodes(root, &arr, &n);
    if (st != STATUS_OK)
        return st;
    if (n == 0)
    {
        free(arr);
        return STATUS_INVALID_ARGUMENT;
    }

    *longest = arr[0]->word;
    *shortest = arr[0]->word;
    for (size_t i = 1; i < n; i++)
    {
        size_t len = strlen(arr[i]->word);
        if (len > strlen(*longest))
            *longest = arr[i]->word;
        if (len < strlen(*shortest))
            *shortest = arr[i]->word;
    }
    free(arr);
    return STATUS_OK;
}

int depth_rec(const node_t *r)
{
    if (r == NULL)
        return 0;
    int left = depth_rec(r->left);
    int right = depth_rec(r->right);
    if (left > right)
        return 1 + left;
    return 1 + right;
}

status_t tree_depth(const node_t *root, int *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    *out = depth_rec(root);
    return STATUS_OK;
}

bool equal_rec(const node_t *a, const node_t *b)
{
    if (a == NULL || b == NULL)
        return a == b;
    return a->count == b->count && strcmp(a->word, b->word) == 0 &&
           equal_rec(a->left, b->left) && equal_rec(a->right, b->right);
}

status_t tree_equal(const node_t *a, const node_t *b, bool *same)
{
    if (same == NULL)
        return STATUS_NULL_POINTER;
    *same = equal_rec(a, b);
    return STATUS_OK;
}

status_t save_rec(const node_t *r, FILE *f)
{
    if (r == NULL)
        return STATUS_OK;
    if (fprintf(f, "%llu %s\n", r->count, r->word) < 0)
        return STATUS_FILE_WRITE_ERROR;
    status_t st = save_rec(r->left, f);
    if (st == STATUS_OK)
        st = save_rec(r->right, f);
    return st;
}

status_t tree_save(const node_t *root, const char *path)
{
    if (path == NULL)
        return STATUS_NULL_POINTER;

    FILE *f = fopen(path, "wb");
    if (f == NULL)
        return STATUS_FILE_OPEN_ERROR;

    status_t st = save_rec(root, f);
    if (fclose(f) != 0 && st == STATUS_OK)
        st = STATUS_FILE_WRITE_ERROR;
    return st;
}

status_t tree_load(const char *path, node_t **root)
{
    if (path == NULL || root == NULL)
        return STATUS_NULL_POINTER;

    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return STATUS_FILE_OPEN_ERROR;

    node_t *tree = NULL;
    char line[LINE];
    status_t st = STATUS_OK;
    while (st == STATUS_OK && fgets(line, sizeof(line), f) != NULL)
    {
        if (strchr(line, '\n') == NULL && !feof(f))
        {
            st = STATUS_INVALID_ARGUMENT;
            continue;
        }
        line[strcspn(line, "\r\n")] = '\0';

        char *end = NULL;
        ull count = strtoull(line, &end, 10);
        if (line[0] < '0' || line[0] > '9' || *end != ' ' || end[1] == '\0' || count == 0)
            st = STATUS_INVALID_ARGUMENT;
        else
            st = tree_add(&tree, end + 1, count);
    }
    if (st == STATUS_OK && ferror(f))
        st = STATUS_FILE_READ_ERROR;
    fclose(f);

    if (st != STATUS_OK)
    {
        tree_free(tree);
        return st;
    }
    *root = tree;
    return STATUS_OK;
}

status_t make_seps(const int argc, char *argv[], bool *is_sep)
{
    if (argv == NULL || is_sep == NULL)
        return STATUS_NULL_POINTER;

    is_sep['\0'] = true;
    is_sep['\n'] = true;
    is_sep['\r'] = true;
    for (int i = 2; i < argc; i++)
    {
        if (strlen(argv[i]) != 1)
            return STATUS_INVALID_ARGUMENT;
        is_sep[(unsigned char)argv[i][0]] = true;
    }
    return STATUS_OK;
}

status_t read_words(const char *path, const bool *is_sep, node_t **root)
{
    if (path == NULL || is_sep == NULL || root == NULL)
        return STATUS_NULL_POINTER;

    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return STATUS_FILE_OPEN_ERROR;

    size_t cap = 64, len = 0;
    char *buf = malloc(cap);
    if (buf == NULL)
    {
        fclose(f);
        return STATUS_MEMORY_ERROR;
    }

    status_t st = STATUS_OK;
    int c;
    do
    {
        c = fgetc(f);
        if (c != EOF && !is_sep[c])
        {
            if (len + 1 >= cap)
            {
                char *tmp = realloc(buf, cap * 2);
                if (tmp == NULL)
                    st = STATUS_MEMORY_ERROR;
                else
                {
                    buf = tmp;
                    cap *= 2;
                }
            }
            if (st == STATUS_OK)
                buf[len++] = (char)c;
        }
        else if (len > 0)
        {
            buf[len] = '\0';
            st = tree_add(root, buf, 1);
            len = 0;
        }
    } while (c != EOF && st == STATUS_OK);

    if (st == STATUS_OK && ferror(f))
        st = STATUS_FILE_READ_ERROR;
    free(buf);
    fclose(f);
    return st;
}

status_t ask(const char *prompt, char *buf, const size_t size)
{
    printf("%s", prompt);
    if (fgets(buf, (int)size, stdin) == NULL)
        return STATUS_INVALID_ARGUMENT;

    size_t len = strcspn(buf, "\r\n");
    if (buf[len] == '\0' && !feof(stdin))
    {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
        return STATUS_INVALID_ARGUMENT;
    }
    buf[len] = '\0';
    if (len == 0)
        return STATUS_INVALID_ARGUMENT;
    return STATUS_OK;
}

status_t print_table(const node_t *root)
{
    const node_t **arr = NULL;
    size_t n = 0;
    status_t st = tree_nodes(root, &arr, &n);
    if (st != STATUS_OK)
        return st;

    if (n == 0)
        printf("Слов нет\n");
    for (size_t i = 0; i < n; i++)
        printf("%s: %llu\n", arr[i]->word, arr[i]->count);
    free(arr);
    return STATUS_OK;
}

status_t cmd_count(const node_t *root)
{
    char word[LINE];
    ull count = 0;
    status_t st = ask("Слово: ", word, sizeof(word));
    if (st == STATUS_OK)
        st = tree_find(root, word, &count);
    if (st == STATUS_OK)
        printf("Встретилось раз: %llu\n", count);
    return st;
}

status_t cmd_top(const node_t *root)
{
    char buf[LINE];
    int n = 0;
    status_t st = ask("n: ", buf, sizeof(buf));
    if (st == STATUS_OK)
        st = parse_int(buf, &n);
    if (st == STATUS_OK && n <= 0)
        st = STATUS_OUT_OF_RANGE;

    const node_t **list = NULL;
    size_t count = 0;
    if (st == STATUS_OK)
        st = top_words(root, (size_t)n, &list, &count);
    if (st == STATUS_OK && count == 0)
        printf("Слов нет\n");
    for (size_t i = 0; st == STATUS_OK && i < count; i++)
        printf("%zu. %s: %llu\n", i + 1, list[i]->word, list[i]->count);
    free(list);
    return st;
}

status_t cmd_extremes(const node_t *root)
{
    if (root == NULL)
    {
        printf("Слов нет\n");
        return STATUS_OK;
    }
    const char *longest = NULL, *shortest = NULL;
    status_t st = find_extremes(root, &longest, &shortest);
    if (st == STATUS_OK)
    {
        printf("Самое длинное: %s\n", longest);
        printf("Самое короткое: %s\n", shortest);
    }
    return st;
}

status_t cmd_depth(const node_t *root)
{
    int depth = 0;
    status_t st = tree_depth(root, &depth);
    if (st == STATUS_OK)
        printf("Глубина дерева: %d\n", depth);
    return st;
}

status_t cmd_save(const node_t *root)
{
    char path[LINE];
    status_t st = ask("Путь к файлу: ", path, sizeof(path));
    if (st == STATUS_OK)
        st = tree_save(root, path);
    if (st == STATUS_OK)
        printf("Сохранено\n");
    return st;
}

status_t cmd_load(node_t **root)
{
    char path[LINE];
    node_t *loaded = NULL;
    bool same = false;
    int depth = 0;

    status_t st = ask("Путь к файлу: ", path, sizeof(path));
    if (st == STATUS_OK)
        st = tree_load(path, &loaded);
    if (st == STATUS_OK)
        st = tree_equal(*root, loaded, &same);
    if (st == STATUS_OK)
        st = tree_depth(loaded, &depth);

    if (st == STATUS_OK)
    {
        if (same)
            printf("Дерево восстановлено, структура совпадает с прежним (глубина %d)\n", depth);
        else
            printf("Дерево восстановлено, оно отличается от прежнего (глубина %d)\n", depth);
        tree_free(*root);
        *root = loaded;
    }
    else
        tree_free(loaded);
    return st;
}

status_t run_menu(node_t **root)
{
    char buf[LINE];
    while (true)
    {
        printf("\n1 частота слова   2 первые n слов   3 длиннейшее и кратчайшее\n"
               "4 глубина   5 сохранить в файл   6 восстановить из файла   0 выход\n");

        int choice = 0;
        status_t st = ask("> ", buf, sizeof(buf));
        if (st != STATUS_OK && feof(stdin))
            break;
        if (st == STATUS_OK)
            st = parse_int(buf, &choice);
        if (st == STATUS_OK && choice == 0)
            break;

        if (st == STATUS_OK)
        {
            switch (choice)
            {
            case 1:
                st = cmd_count(*root);
                break;
            case 2:
                st = cmd_top(*root);
                break;
            case 3:
                st = cmd_extremes(*root);
                break;
            case 4:
                st = cmd_depth(*root);
                break;
            case 5:
                st = cmd_save(*root);
                break;
            case 6:
                st = cmd_load(root);
                break;
            default:
                st = STATUS_INVALID_ARGUMENT;
                break;
            }
        }
        if (st == STATUS_MEMORY_ERROR)
            return st;
        if (st != STATUS_OK)
            print_error(st);
    }
    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Формат: [program] [файл] [разделитель]...\n");
        return 1;
    }

    bool is_sep[256] = {false};
    node_t *root = NULL;

    status_t st = make_seps(argc, argv, is_sep);
    if (st == STATUS_OK)
        st = read_words(argv[1], is_sep, &root);
    if (st == STATUS_OK)
        st = print_table(root);
    if (st == STATUS_OK)
        st = run_menu(&root);

    if (st != STATUS_OK)
        print_error(st);

    tree_free(root);
    return (st == STATUS_OK) ? 0 : 1;
}