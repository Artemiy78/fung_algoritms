#include "post.h"
#include <string.h>

#define LINE 256
#define EPS 1e-9

status_t ask(const char *prompt, char *buf, const size_t size, const bool allow_empty)
{
    if (prompt == NULL || buf == NULL)
        return STATUS_NULL_POINTER;

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
    return (len == 0 && !allow_empty) ? STATUS_INVALID_ARGUMENT : STATUS_OK;
}

status_t ask_string(const char *prompt, const bool allow_empty, String *out)
{
    char buf[LINE];
    status_t st = ask(prompt, buf, sizeof(buf), allow_empty);
    return (st == STATUS_OK) ? string_create(buf, out) : st;
}

status_t ask_digits(const char *prompt, const size_t n, String *out)
{
    char buf[LINE];
    status_t st = ask(prompt, buf, sizeof(buf), false);
    if (st == STATUS_OK && strlen(buf) != n)
        st = STATUS_INVALID_ARGUMENT;
    for (size_t i = 0; st == STATUS_OK && i < n; i++)
        if (!IS_DIG(buf[i]))
            st = STATUS_INVALID_ARGUMENT;
    return (st == STATUS_OK) ? string_create(buf, out) : st;
}

status_t ask_natural(const char *prompt, int *out)
{
    char buf[LINE];
    status_t st = ask(prompt, buf, sizeof(buf), false);
    if (st == STATUS_OK)
        st = parse_int(buf, out);
    if (st == STATUS_OK && *out <= 0)
        st = STATUS_OUT_OF_RANGE;
    return st;
}

status_t ask_weight(const char *prompt, const double eps, double *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0 || eps >= 0.5)
        return STATUS_INVALID_EPSILON;

    char buf[LINE];
    status_t st = ask(prompt, buf, sizeof(buf), false);
    if (st == STATUS_OK)
        st = parse_double(buf, out);
    if (st == STATUS_OK && *out < -eps)
        st = STATUS_OUT_OF_RANGE;
    return st;
}

status_t ask_time(const char *prompt, String *out, ull *key)
{
    char buf[LINE];
    status_t st = ask(prompt, buf, sizeof(buf), false);
    if (st == STATUS_OK)
        st = parse_time(buf, key);
    return (st == STATUS_OK) ? string_create(buf, out) : st;
}

status_t read_mail(Mail *m)
{
    if (m == NULL)
        return STATUS_NULL_POINTER;
    Address *a = &m->to;

    status_t st = ask_string("Город: ", false, &a->city);
    if (st == STATUS_OK)
        st = ask_string("Улица: ", false, &a->street);
    if (st == STATUS_OK)
        st = ask_natural("Дом: ", &a->house);
    if (st == STATUS_OK)
        st = ask_string("Корпус (можно пусто): ", true, &a->building);
    if (st == STATUS_OK)
        st = ask_natural("Квартира: ", &a->apartment);
    if (st == STATUS_OK)
        st = ask_digits("Индекс (6 цифр): ", 6, &a->index);
    if (st == STATUS_OK)
        st = ask_weight("Вес: ", EPS, &m->weight);
    if (st == STATUS_OK)
        st = ask_digits("Идентификатор (14 цифр): ", 14, &m->id);
    if (st == STATUS_OK)
        st = ask_time("Создано (dd:MM:yyyy hh:mm:ss): ", &m->created, &m->created_key);
    if (st == STATUS_OK)
        st = ask_time("Срок доставки (dd:MM:yyyy hh:mm:ss): ", &m->deadline, &m->deadline_key);
    if (st == STATUS_OK && m->deadline_key < m->created_key)
        st = STATUS_INVALID_ARGUMENT;
    return st;
}

void print_mail(const Mail *m)
{
    const Address *a = &m->to;
    printf("Отправление %s (%s)\n  Получатель: %s, %s, д. %d", m->id.str,
           m->delivered ? "вручено" : "не вручено", a->city.str, a->street.str, a->house);
    if (a->building.len > 0)
        printf(", корп. %s", a->building.str);
    printf(", кв. %d, индекс %s\n  Вес: %.2f кг\n  Создано: %s\n  Срок доставки: %s\n",
           a->apartment, a->index.str, m->weight, m->created.str, m->deadline.str);
}

status_t cmd_add(Post *p)
{
    Mail m = {0};
    status_t st = read_mail(&m);
    if (st == STATUS_OK)
        st = post_add(p, &m);
    if (st != STATUS_OK)
        free_mail(&m);
    else
        printf("Добавлено\n");
    return st;
}

status_t cmd_id(Post *p, const int choice)
{
    String id = {NULL, 0};
    size_t idx = 0;
    bool found = false;

    status_t st = ask_digits("Идентификатор (14 цифр): ", 14, &id);
    if (st == STATUS_OK)
        st = find_index(p, &id, &idx, &found);
    if (st == STATUS_OK && !found)
        printf("Не найдено\n");
    else if (st == STATUS_OK && choice == 2)
    {
        st = post_remove(p, idx);
        if (st == STATUS_OK)
            printf("Удалено\n");
    }
    else if (st == STATUS_OK && choice == 3)
        print_mail(&p->mails[idx]);
    else if (st == STATUS_OK)
    {
        p->mails[idx].delivered = true;
        printf("Отмечено как вручённое\n");
    }
    string_free(&id);
    return st;
}

status_t cmd_list(const Post *p, const bool delivered)
{
    const Mail **list = NULL;
    size_t count = 0;
    ull now = 0;

    status_t st = now_key(&now);
    if (st == STATUS_OK)
        st = post_select(p, now, delivered, &list, &count);
    if (st == STATUS_OK && count == 0)
        printf("Нет отправлений\n");
    for (size_t i = 0; st == STATUS_OK && i < count; i++)
        print_mail(list[i]);
    free(list);
    return st;
}

status_t run_menu(Post *p)
{
    if (p == NULL || p->office == NULL)
        return STATUS_NULL_POINTER;

    char buf[LINE];
    while (true)
    {
        printf("\nОтделение %s: %s, %s, %d\n", p->office->index.str, p->office->city.str,
               p->office->street.str, p->office->house);
        printf("1 добавить   2 удалить   3 найти   4 отметить вручённым\n"
               "5 доставленные   6 просроченные   7 все   0 выход\n");

        int choice = 0;
        status_t st = ask("> ", buf, sizeof(buf), false);
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
                st = cmd_add(p);
                break;
            case 2:
            case 3:
            case 4:
                st = cmd_id(p, choice);
                break;
            case 5:
            case 6:
                st = cmd_list(p, choice == 5);
                break;
            case 7:
                if (p->size == 0)
                    printf("Пусто\n");
                for (size_t i = 0; i < p->size; i++)
                    print_mail(&p->mails[i]);
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

int main(void)
{
    String a = {NULL, 0}, b = {NULL, 0}, c = {NULL, 0};
    String *d = NULL;
    int order = 0, lex = 0;
    bool same = false;

    status_t ds = string_create("zz", &a);
    if (ds == STATUS_OK)
        ds = string_create("aaa", &b);
    if (ds == STATUS_OK)
        ds = string_cmp(&a, &b, &order);
    if (ds == STATUS_OK)
        ds = string_lex(&a, &b, &lex);
    if (ds == STATUS_OK)
        printf("\"%s\" и \"%s\": порядок %d (по длине), лексикографически %d\n",
               a.str, b.str, order, lex);

    if (ds == STATUS_OK)
        ds = string_copy(&b, &c);
    if (ds == STATUS_OK)
        ds = string_eq(&b, &c, &same);
    if (ds == STATUS_OK)
        printf("копия: \"%s\", равна оригиналу: %s\n", c.str, same ? "да" : "нет");

    if (ds == STATUS_OK)
        ds = string_clone(&a, &d);
    if (ds == STATUS_OK)
        ds = string_concat(d, &b);
    if (ds == STATUS_OK)
        printf("клон a + b: \"%s\" (длина %d), сам a: \"%s\"\n", d->str, d->len, a.str);

    if (ds != STATUS_OK)
        print_error(ds);

    string_free(&a);
    string_free(&b);
    string_free(&c);
    if (d != NULL)
    {
        string_free(d);
        free(d);
    }

    Address office = {0};
    Post post = {&office, NULL, 0, 0};

    status_t st = string_create("Moscow", &office.city);
    if (st == STATUS_OK)
        st = string_create("Tverskaya", &office.street);
    if (st == STATUS_OK)
        st = string_create("", &office.building);
    if (st == STATUS_OK)
        st = string_create("101000", &office.index);
    office.house = 1;
    office.apartment = 1;

    if (st == STATUS_OK)
        st = run_menu(&post);
    if (st != STATUS_OK)
        print_error(st);

    free_post(&post);
    free_address(&office);
    return (st == STATUS_OK) ? 0 : 1;
}