#include "../utils.h"
#include <string.h>
#include <math.h>

#define LINE 512
#define WS " \t\r\n"
#define EPS 1e-9

typedef struct
{
    int day, month, year;
} date_t;

typedef struct
{
    char *surname, *name, *patronymic;
    date_t birth;
    char gender;
    double income;
} Liver;

typedef struct node
{
    Liver data;
    struct node *next;
} node_t;

typedef struct undo_node
{
    node_t *snapshot;
    struct undo_node *prev;
} undo_node_t;

typedef struct
{
    node_t *head;
    undo_node_t *undo_stack;
    int n_mods;
} context_t;

void free_liver(Liver *l)
{
    free(l->surname);
    free(l->name);
    free(l->patronymic);
    l->surname = l->name = l->patronymic = NULL;
}

void free_list(node_t *head)
{
    while (head)
    {
        node_t *next = head->next;
        free_liver(&head->data);
        free(head);
        head = next;
    }
}

void free_undo(undo_node_t *stack)
{
    while (stack)
    {
        undo_node_t *prev = stack->prev;
        free_list(stack->snapshot);
        free(stack);
        stack = prev;
    }
}

status_t dup_str(const char *src, char **out)
{
    if (!src || !out)
        return STATUS_NULL_POINTER;
    size_t len = strlen(src) + 1;
    *out = malloc(len);
    if (!*out)
        return STATUS_MEMORY_ERROR;
    memcpy(*out, src, len);
    return STATUS_OK;
}

status_t copy_liver(const Liver *src, Liver *dst)
{
    status_t st = dup_str(src->surname, &dst->surname);
    if (st == STATUS_OK)
        st = dup_str(src->name, &dst->name);
    if (st == STATUS_OK)
        st = dup_str(src->patronymic ? src->patronymic : "", &dst->patronymic);
    if (st != STATUS_OK)
    {
        free_liver(dst);
        return st;
    }
    dst->birth = src->birth;
    dst->gender = src->gender;
    dst->income = src->income;
    return STATUS_OK;
}

status_t copy_list(const node_t *src, node_t **out)
{
    if (!out)
        return STATUS_NULL_POINTER;
    node_t *new_head = NULL, *tail = NULL;
    for (const node_t *cur = src; cur; cur = cur->next)
    {
        node_t *node = calloc(1, sizeof(node_t));
        if (!node)
        {
            free_list(new_head);
            return STATUS_MEMORY_ERROR;
        }
        status_t st = copy_liver(&cur->data, &node->data);
        if (st != STATUS_OK)
        {
            free(node);
            free_list(new_head);
            return st;
        }
        node->next = NULL;
        if (!new_head)
            new_head = node;
        else
            tail->next = node;
        tail = node;
    }
    *out = new_head;
    return STATUS_OK;
}

int cmp_date_desc(const date_t *a, const date_t *b)
{
    if (a->year != b->year)
        return a->year - b->year;
    if (a->month != b->month)
        return a->month - b->month;
    return a->day - b->day;
}

status_t parse_date(const char *d, const char *m, const char *y, date_t *out)
{
    int day, month, year;
    status_t st = parse_int(d, &day);
    if (st == STATUS_OK)
        st = parse_int(m, &month);
    if (st == STATUS_OK)
        st = parse_int(y, &year);
    if (st != STATUS_OK)
        return st;

    if (month < 1 || month > 12 || year < 1)
        return STATUS_OUT_OF_RANGE;

    const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    int max_d = days[month - 1] + (month == 2 && leap ? 1 : 0);

    if (day < 1 || day > max_d)
        return STATUS_OUT_OF_RANGE;

    out->day = day;
    out->month = month;
    out->year = year;
    return STATUS_OK;
}

status_t parse_liver(char *line, Liver *out)
{
    char *tok[9];
    int cnt = 0;
    for (char *t = strtok(line, WS); t && cnt < 9; t = strtok(NULL, WS))
        tok[cnt++] = t;
    if (cnt < 7 || cnt > 8)
        return STATUS_INVALID_ARGUMENT;

    status_t st = check_latin(tok[0]);
    if (st == STATUS_OK)
        st = check_latin(tok[1]);
    if (st != STATUS_OK)
        return st;

    int idx = 2;
    char *patr = NULL;
    int dummy;
    if (parse_int(tok[2], &dummy) != STATUS_OK)
    {
        st = check_latin(tok[2]);
        if (st != STATUS_OK)
            return st;
        patr = tok[2];
        idx = 3;
    }
    if (cnt != idx + 5)
        return STATUS_INVALID_ARGUMENT;

    date_t birth;
    st = parse_date(tok[idx], tok[idx + 1], tok[idx + 2], &birth);
    if (st != STATUS_OK)
        return st;

    if (strlen(tok[idx + 3]) != 1 || (tok[idx + 3][0] != 'M' && tok[idx + 3][0] != 'W'))
        return STATUS_INVALID_ARGUMENT;

    double income;
    st = parse_double(tok[idx + 4], &income);
    if (st == STATUS_OK && income < -EPS)
        st = STATUS_OUT_OF_RANGE;
    if (st != STATUS_OK)
        return st;

    st = dup_str(tok[0], &out->surname);
    if (st == STATUS_OK)
        st = dup_str(tok[1], &out->name);
    if (st == STATUS_OK)
        st = dup_str(patr ? patr : "", &out->patronymic);
    if (st != STATUS_OK)
    {
        free_liver(out);
        return st;
    }

    out->birth = birth;
    out->gender = tok[idx + 3][0];
    out->income = income;
    return STATUS_OK;
}

status_t push_undo(context_t *ctx)
{
    node_t *snap = NULL;
    status_t st = copy_list(ctx->head, &snap);
    if (st != STATUS_OK)
        return st;
    undo_node_t *node = malloc(sizeof(undo_node_t));
    if (!node)
    {
        free_list(snap);
        return STATUS_MEMORY_ERROR;
    }
    node->snapshot = snap;
    node->prev = ctx->undo_stack;
    ctx->undo_stack = node;
    ctx->n_mods++;
    return STATUS_OK;
}

status_t insert_sorted(context_t *ctx, Liver *data)
{
    node_t *node = calloc(1, sizeof(node_t));
    if (!node)
        return STATUS_MEMORY_ERROR;
    if (copy_liver(data, &node->data) != STATUS_OK)
    {
        free(node);
        return STATUS_MEMORY_ERROR;
    }

    if (!ctx->head || cmp_date_desc(&node->data.birth, &ctx->head->data.birth) > 0)
    {
        node->next = ctx->head;
        ctx->head = node;
        return STATUS_OK;
    }

    node_t *cur = ctx->head;
    while (cur->next && cmp_date_desc(&node->data.birth, &cur->next->data.birth) <= 0)
        cur = cur->next;
    node->next = cur->next;
    cur->next = node;
    return STATUS_OK;
}

status_t read_file(const char *path, context_t *ctx)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return STATUS_FILE_OPEN_ERROR;

    char line[LINE];
    status_t st = STATUS_OK;
    while (st == STATUS_OK && fgets(line, sizeof(line), f))
    {
        if (line[strspn(line, WS)] == '\0')
            continue;
        Liver l = {0};
        st = parse_liver(line, &l);
        if (st == STATUS_OK)
            st = insert_sorted(ctx, &l);
        free_liver(&l);
    }
    if (st == STATUS_OK && ferror(f))
        st = STATUS_FILE_READ_ERROR;
    fclose(f);
    return st;
}

status_t write_file(const char *path, node_t *head)
{
    FILE *f = fopen(path, "w");
    if (!f)
        return STATUS_FILE_OPEN_ERROR;

    status_t st = STATUS_OK;
    for (node_t *cur = head; cur && st == STATUS_OK; cur = cur->next)
    {
        if (strlen(cur->data.patronymic) > 0)
            fprintf(f, "%s %s %s %d %d %d %c %.2f\n",
                    cur->data.surname, cur->data.name, cur->data.patronymic,
                    cur->data.birth.day, cur->data.birth.month, cur->data.birth.year,
                    cur->data.gender, cur->data.income);
        else
            fprintf(f, "%s %s %d %d %d %c %.2f\n",
                    cur->data.surname, cur->data.name,
                    cur->data.birth.day, cur->data.birth.month, cur->data.birth.year,
                    cur->data.gender, cur->data.income);
        if (ferror(f))
            st = STATUS_FILE_WRITE_ERROR;
    }
    fclose(f);
    return st;
}

status_t find_node(node_t *head, const char *surname, const char *name, node_t **out)
{
    if (!surname || !name || !out)
        return STATUS_NULL_POINTER;
    for (node_t *cur = head; cur; cur = cur->next)
        if (strcmp(cur->data.surname, surname) == 0 && strcmp(cur->data.name, name) == 0)
        {
            *out = cur;
            return STATUS_OK;
        }
    return STATUS_INVALID_ARGUMENT;
}

status_t do_add(context_t *ctx, char *line)
{
    Liver l = {0};
    status_t st = parse_liver(line, &l);
    if (st == STATUS_OK)
        st = push_undo(ctx);
    if (st == STATUS_OK)
        st = insert_sorted(ctx, &l);
    free_liver(&l);
    return st;
}

status_t do_delete(context_t *ctx, const char *surname, const char *name)
{
    if (!ctx->head)
        return STATUS_INVALID_ARGUMENT;

    if (strcmp(ctx->head->data.surname, surname) == 0 && strcmp(ctx->head->data.name, name) == 0)
    {
        status_t st = push_undo(ctx);
        if (st != STATUS_OK)
            return st;
        node_t *tmp = ctx->head;
        ctx->head = ctx->head->next;
        free_liver(&tmp->data);
        free(tmp);
        return STATUS_OK;
    }

    node_t *cur = ctx->head;
    while (cur->next)
    {
        if (strcmp(cur->next->data.surname, surname) == 0 && strcmp(cur->next->data.name, name) == 0)
        {
            status_t st = push_undo(ctx);
            if (st != STATUS_OK)
                return st;
            node_t *tmp = cur->next;
            cur->next = tmp->next;
            free_liver(&tmp->data);
            free(tmp);
            return STATUS_OK;
        }
        cur = cur->next;
    }
    return STATUS_INVALID_ARGUMENT;
}

status_t do_edit(context_t *ctx, const char *surname, const char *name, char *new_data_line)
{
    node_t *node = NULL;
    status_t st = find_node(ctx->head, surname, name, &node);
    if (st != STATUS_OK)
        return st;

    Liver new_l = {0};
    st = parse_liver(new_data_line, &new_l);
    if (st != STATUS_OK)
        return st;

    st = push_undo(ctx);
    if (st != STATUS_OK)
    {
        free_liver(&new_l);
        return st;
    }

    node_t *new_node = calloc(1, sizeof(node_t));
    if (!new_node)
    {
        free_liver(&new_l);
        return STATUS_MEMORY_ERROR;
    }
    if (copy_liver(&new_l, &new_node->data) != STATUS_OK)
    {
        free(new_node);
        free_liver(&new_l);
        return STATUS_MEMORY_ERROR;
    }
    new_node->next = NULL;

    if (ctx->head == node)
    {
        ctx->head = node->next;
    }
    else
    {
        node_t *cur = ctx->head;
        while (cur->next != node)
            cur = cur->next;
        cur->next = node->next;
    }
    free_liver(&node->data);
    free(node);

    if (!ctx->head || cmp_date_desc(&new_node->data.birth, &ctx->head->data.birth) > 0)
    {
        new_node->next = ctx->head;
        ctx->head = new_node;
    }
    else
    {
        node_t *cur = ctx->head;
        while (cur->next && cmp_date_desc(&new_node->data.birth, &cur->next->data.birth) <= 0)
            cur = cur->next;
        new_node->next = cur->next;
        cur->next = new_node;
    }

    free_liver(&new_l);
    return STATUS_OK;
}

status_t do_undo(context_t *ctx, int *undone)
{
    if (!ctx || !undone)
        return STATUS_NULL_POINTER;
    int k = ctx->n_mods / 2;
    *undone = k;
    if (k == 0)
        return STATUS_OK;

    free_list(ctx->head);

    for (int i = 0; i < k - 1; i++)
    {
        undo_node_t *tmp = ctx->undo_stack;
        ctx->undo_stack = ctx->undo_stack->prev;
        free_list(tmp->snapshot);
        free(tmp);
    }

    if (ctx->undo_stack)
    {
        ctx->head = ctx->undo_stack->snapshot;
        ctx->undo_stack->snapshot = NULL;

        undo_node_t *tmp = ctx->undo_stack;
        ctx->undo_stack = ctx->undo_stack->prev;
        free(tmp);
    }
    else
    {
        ctx->head = NULL;
    }

    free_undo(ctx->undo_stack);
    ctx->undo_stack = NULL;
    ctx->n_mods = 0;
    return STATUS_OK;
}

bool matches(const Liver *l, const int choice, const char *value)
{
    if (choice == 1)
        return strcmp(l->surname, value) == 0;
    if (choice == 2)
        return strcmp(l->name, value) == 0;
    if (choice == 3)
        return l->gender == value[0];
    if (choice == 5)
        return strcmp(l->patronymic, value) == 0;
    double val;
    return parse_double(value, &val) == STATUS_OK && l->income - val > EPS;
}

void search_menu(node_t *head)
{
    char buf[LINE];
    int choice = 0;
    printf("1. По фамилии\n2. По имени\n3. По полу\n4. По доходу (>)\n5. По отчеству\n> ");

    if (fgets(buf, sizeof(buf), stdin) == NULL)
        return;
    buf[strcspn(buf, "\n")] = '\0';

    if (parse_int(buf, &choice) != STATUS_OK || choice < 1 || choice > 5)
    {
        printf("Неверный выбор\n");
        return;
    }

    printf("Значение: ");
    fgets(buf, sizeof(buf), stdin);
    buf[strcspn(buf, "\n")] = '\0';

    bool found = false;
    for (node_t *cur = head; cur; cur = cur->next)
    {
        if (matches(&cur->data, choice, buf))
        {
            printf("%s %s %s %02d.%02d.%04d %c %.2f\n",
                   cur->data.surname, cur->data.name, cur->data.patronymic,
                   cur->data.birth.day, cur->data.birth.month, cur->data.birth.year,
                   cur->data.gender, cur->data.income);
            found = true;
        }
    }
    if (!found)
        printf("Не найдено\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Формат: [program] [файл]\n");
        return 1;
    }

    context_t ctx = {0};
    status_t st = read_file(argv[1], &ctx);
    if (st != STATUS_OK)
    {
        print_error(st);
        free_list(ctx.head);
        return 1;
    }

    char buf[LINE];
    char surname[LINE], name[LINE];
    bool done = false;

    while (!done)
    {
        printf("\n1. Поиск\n2. Добавить\n3. Изменить\n4. Удалить\n5. Выгрузить\n6. Undo\n0. Выход\n> ");
        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;
        buf[strcspn(buf, "\n")] = '\0';
        if (buf[0] == '\0')
            continue;

        int choice;
        if (parse_int(buf, &choice) != STATUS_OK)
        {
            print_error(STATUS_INVALID_INT_NUMBER);
            continue;
        }

        if (choice == 0)
            done = true;
        else if (choice == 1)
            search_menu(ctx.head);
        else if (choice == 5)
        {
            printf("Путь к файлу: ");
            fgets(buf, sizeof(buf), stdin);
            buf[strcspn(buf, "\n")] = '\0';
            st = write_file(buf, ctx.head);
            if (st != STATUS_OK)
                print_error(st);
            else
                printf("Записано\n");
        }
        else if (choice == 6)
        {
            int k = 0;
            st = do_undo(&ctx, &k);
            if (st != STATUS_OK)
                print_error(st);
            else if (k == 0)
                printf("Нечего отменять (N=%d)\n", ctx.n_mods);
            else
                printf("Отменено %d модификаций. Осталось N=%d\n", k, ctx.n_mods);
        }
        else if (choice >= 2 && choice <= 4)
        {
            if (choice == 2)
            {
                printf("Данные (фам имя [отч] дд мм гггг пол доход): ");
                fgets(buf, sizeof(buf), stdin);
                buf[strcspn(buf, "\n")] = '\0';
                st = do_add(&ctx, buf);
            }
            else
            {
                printf("Фамилия: ");
                fgets(surname, sizeof(surname), stdin);
                surname[strcspn(surname, "\n")] = '\0';
                printf("Имя: ");
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = '\0';

                if (choice == 3)
                {
                    printf("Новые данные (фам имя [отч] дд мм гггг пол доход): ");
                    fgets(buf, sizeof(buf), stdin);
                    buf[strcspn(buf, "\n")] = '\0';
                    st = do_edit(&ctx, surname, name, buf);
                }
                else if (choice == 4)
                {
                    st = do_delete(&ctx, surname, name);
                }
            }
            if (st != STATUS_OK)
                print_error(st);
            else
                printf("ОК (N=%d)\n", ctx.n_mods);
        }
        else
        {
            printf("Неверный выбор\n");
        }
    }

    free_list(ctx.head);
    free_undo(ctx.undo_stack);
    return 0;
}