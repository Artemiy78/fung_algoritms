#include "../utils.h"
#include <string.h>

#define LINE 4096
#define WS " \t\r\n"

typedef struct node
{
    char *name;
    struct node *child;
    struct node *sibling;
} node_t;

void free_tree(node_t *node)
{
    if (node == NULL)
        return;
    node_t *c = node->child;
    while (c != NULL)
    {
        node_t *next = c->sibling;
        free_tree(c);
        c = next;
    }
    free(node->name);
    free(node);
}

static status_t new_node(const char *name, const size_t len, node_t **out)
{
    if (name == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    node_t *node = calloc(1, sizeof(node_t));
    if (node == NULL)
        return STATUS_MEMORY_ERROR;
    node->name = malloc(len + 1);
    if (node->name == NULL)
    {
        free(node);
        return STATUS_MEMORY_ERROR;
    }
    memcpy(node->name, name, len);
    node->name[len] = '\0';
    *out = node;
    return STATUS_OK;
}

static status_t parse_node(const char *s, size_t *pos, node_t **slot)
{
    if (s == NULL || pos == NULL || slot == NULL)
        return STATUS_NULL_POINTER;

    *pos += strspn(s + *pos, WS);
    size_t len = strcspn(s + *pos, "()," WS);
    if (len == 0)
        return STATUS_INVALID_ARGUMENT;

    status_t st = new_node(s + *pos, len, slot);
    if (st != STATUS_OK)
        return st;
    *pos += len;

    *pos += strspn(s + *pos, WS);
    if (s[*pos] != '(')
        return STATUS_OK;
    (*pos)++;

    node_t **child = &(*slot)->child;
    while (true)
    {
        st = parse_node(s, pos, child);
        if (st != STATUS_OK)
            return st;
        child = &(*child)->sibling;

        *pos += strspn(s + *pos, WS);
        if (s[*pos] == ',')
            (*pos)++;
        else if (s[*pos] == ')')
        {
            (*pos)++;
            return STATUS_OK;
        }
        else
            return STATUS_INVALID_ARGUMENT;
    }
}

status_t parse_line(const char *s, node_t **root)
{
    if (s == NULL || root == NULL)
        return STATUS_NULL_POINTER;

    *root = NULL;
    size_t pos = 0;
    status_t st = parse_node(s, &pos, root);
    if (st == STATUS_OK)
    {
        pos += strspn(s + pos, WS);
        if (s[pos] != '\0')
            st = STATUS_INVALID_ARGUMENT;
    }
    return st;
}

static status_t print_tree(FILE *out, const node_t *node, char *prefix, const bool root,
                           const bool last)
{
    if (out == NULL || node == NULL || prefix == NULL)
        return STATUS_NULL_POINTER;

    int rc;
    if (root)
        rc = fprintf(out, "%s\n", node->name);
    else
        rc = fprintf(out, "%s%s%s\n", prefix, last ? "`-- " : "|-- ", node->name);
    if (rc < 0)
        return STATUS_FILE_WRITE_ERROR;

    const size_t len = strlen(prefix);
    if (!root)
        strcpy(prefix + len, last ? "    " : "|   ");

    status_t st = STATUS_OK;
    for (const node_t *c = node->child; c != NULL && st == STATUS_OK; c = c->sibling)
        st = print_tree(out, c, prefix, false, c->sibling == NULL);

    prefix[len] = '\0';
    return st;
}

status_t process_file(const char *in_path, const char *out_path, size_t *line_no)
{
    if (in_path == NULL || out_path == NULL || line_no == NULL)
        return STATUS_NULL_POINTER;
    *line_no = 0;
    if (strcmp(in_path, out_path) == 0)
        return STATUS_SAME_FILE;

    FILE *in = fopen(in_path, "r");
    if (in == NULL)
        return STATUS_FILE_OPEN_ERROR;
    FILE *out = fopen(out_path, "w");
    if (out == NULL)
    {
        fclose(in);
        return STATUS_FILE_OPEN_ERROR;
    }

    char line[LINE];
    char prefix[2 * LINE + 8] = "";
    size_t n = 0;
    status_t st = STATUS_OK;

    while (st == STATUS_OK && fgets(line, sizeof(line), in) != NULL)
    {
        n++;
        if (strchr(line, '\n') == NULL && !feof(in))
        {
            st = STATUS_INVALID_ARGUMENT;
            *line_no = n;
            break;
        }
        line[strcspn(line, "\r\n")] = '\0';
        if (line[strspn(line, WS)] == '\0')
            continue;

        node_t *root = NULL;
        st = parse_line(line, &root);
        if (st == STATUS_OK && fprintf(out, "Строка %zu: %s\n", n, line) < 0)
            st = STATUS_FILE_WRITE_ERROR;
        if (st == STATUS_OK)
            st = print_tree(out, root, prefix, true, true);
        if (st == STATUS_OK && fprintf(out, "\n") < 0)
            st = STATUS_FILE_WRITE_ERROR;
        free_tree(root);
        if (st != STATUS_OK)
            *line_no = n;
    }

    if (st == STATUS_OK && ferror(in))
        st = STATUS_FILE_READ_ERROR;
    fclose(in);
    if (fclose(out) != 0 && st == STATUS_OK)
        st = STATUS_FILE_WRITE_ERROR;
    return st;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Формат: [program] [входной файл] [выходной файл]\n");
        return 1;
    }

    size_t line_no = 0;
    status_t st = process_file(argv[1], argv[2], &line_no);
    if (st != STATUS_OK)
    {
        if (line_no > 0)
            printf("Строка %zu. ", line_no);
        print_error(st);
        return 1;
    }
    printf("Готово: деревья записаны в %s\n", argv[2]);
    return 0;
}