#include "../utils.h"
#include <string.h>

#define EXAMS 5
#define FIELDS (EXAMS + 4)
#define LINE 512
#define WS " \t\r\n"
#define EPS 1e-9

typedef unsigned long long ull;

typedef struct
{
    int id;
    char *name;
    char *surname;
    char *group;
    unsigned char *grades;
} Student;

typedef bool (*match_fn)(const Student *, const void *);
typedef int (*cmp_fn)(const void *, const void *);

void free_student(Student *s)
{
    free(s->name);
    free(s->surname);
    free(s->group);
    free(s->grades);
}

void free_students(Student *arr, const size_t n)
{
    for (size_t i = 0; i < n; i++)
        free_student(&arr[i]);
    free(arr);
}

status_t dup_str(const char *src, char **out)
{
    if (src == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    size_t len = strlen(src) + 1;
    *out = malloc(len);
    if (*out == NULL)
        return STATUS_MEMORY_ERROR;
    memcpy(*out, src, len);
    return STATUS_OK;
}

status_t parse_id(const char *s, int *out)
{
    status_t st = parse_int(s, out);
    if (st == STATUS_OK && *out < 0)
        st = STATUS_OUT_OF_RANGE;
    return st;
}

status_t parse_student(char *line, Student *s)
{
    if (line == NULL || s == NULL)
        return STATUS_NULL_POINTER;

    char *tok[FIELDS];
    size_t cnt = 0;
    for (char *t = strtok(line, WS); t != NULL; t = strtok(NULL, WS))
    {
        if (cnt == FIELDS)
            return STATUS_INVALID_ARGUMENT;
        tok[cnt++] = t;
    }
    if (cnt != FIELDS)
        return STATUS_INVALID_ARGUMENT;

    status_t st = parse_id(tok[0], &s->id);
    if (st == STATUS_OK)
        st = check_latin(tok[1]);
    if (st == STATUS_OK)
        st = check_latin(tok[2]);
    if (st == STATUS_OK)
        st = dup_str(tok[1], &s->name);
    if (st == STATUS_OK)
        st = dup_str(tok[2], &s->surname);
    if (st == STATUS_OK)
        st = dup_str(tok[3], &s->group);
    if (st == STATUS_OK)
    {
        s->grades = malloc(EXAMS);
        if (s->grades == NULL)
            st = STATUS_MEMORY_ERROR;
    }
    for (int i = 0; st == STATUS_OK && i < EXAMS; i++)
    {
        int g = 0;
        st = parse_int(tok[4 + i], &g);
        if (st == STATUS_OK && (g < 0 || g > 5))
            st = STATUS_OUT_OF_RANGE;
        if (st == STATUS_OK)
            s->grades[i] = (unsigned char)g;
    }
    return st;
}

status_t read_students(const char *path, Student **out, size_t *n)
{
    if (path == NULL || out == NULL || n == NULL)
        return STATUS_NULL_POINTER;

    FILE *f = fopen(path, "r");
    if (f == NULL)
        return STATUS_FILE_OPEN_ERROR;

    Student *arr = NULL;
    size_t size = 0, cap = 0;
    char line[LINE];
    status_t st = STATUS_OK;

    while (st == STATUS_OK && fgets(line, sizeof(line), f) != NULL)
    {
        if (strchr(line, '\n') == NULL && !feof(f))
        {
            st = STATUS_INVALID_ARGUMENT;
            continue;
        }
        if (line[strspn(line, WS)] == '\0')
            continue;

        if (size == cap)
        {
            size_t new_cap = (cap == 0) ? 4 : cap * 2;
            Student *tmp = realloc(arr, new_cap * sizeof(Student));
            if (tmp == NULL)
            {
                st = STATUS_MEMORY_ERROR;
                continue;
            }
            arr = tmp;
            cap = new_cap;
        }

        Student s = {0};
        st = parse_student(line, &s);
        if (st == STATUS_OK)
            arr[size++] = s;
        else
            free_student(&s);
    }
    if (st == STATUS_OK && ferror(f))
        st = STATUS_FILE_READ_ERROR;
    fclose(f);

    if (st != STATUS_OK)
    {
        free_students(arr, size);
        return st;
    }
    *out = arr;
    *n = size;
    return STATUS_OK;
}

bool match_id(const Student *s, const void *key)
{
    return s->id == *(const int *)key;
}
bool match_surname(const Student *s, const void *key)
{
    return strcmp(s->surname, (const char *)key) == 0;
}
bool match_name(const Student *s, const void *key)
{
    return strcmp(s->name, (const char *)key) == 0;
}
bool match_group(const Student *s, const void *key)
{
    return strcmp(s->group, (const char *)key) == 0;
}

status_t find_all(const Student *arr, const size_t n, const match_fn match, const void *key,
                  const Student ***found, size_t *count)
{
    if (arr == NULL || match == NULL || key == NULL || found == NULL || count == NULL)
        return STATUS_NULL_POINTER;
    if (n == 0)
        return STATUS_INVALID_COUNT;

    const Student **res = malloc(n * sizeof(*res));
    if (res == NULL)
        return STATUS_MEMORY_ERROR;

    size_t c = 0;
    for (size_t i = 0; i < n; i++)
        if (match(&arr[i], key))
            res[c++] = &arr[i];

    *found = res;
    *count = c;
    return STATUS_OK;
}

int cmp_id(const void *a, const void *b)
{
    const Student *x = a, *y = b;
    return (x->id > y->id) - (x->id < y->id);
}
int cmp_surname(const void *a, const void *b)
{
    return strcmp(((const Student *)a)->surname, ((const Student *)b)->surname);
}
int cmp_name(const void *a, const void *b)
{
    return strcmp(((const Student *)a)->name, ((const Student *)b)->name);
}
int cmp_group(const void *a, const void *b)
{
    return strcmp(((const Student *)a)->group, ((const Student *)b)->group);
}

status_t sort_students(Student *arr, const size_t n, const cmp_fn cmp)
{
    if (arr == NULL || cmp == NULL)
        return STATUS_NULL_POINTER;
    qsort(arr, n, sizeof(Student), cmp);
    return STATUS_OK;
}

status_t average(const Student *s, double *out)
{
    if (s == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    unsigned sum = 0;
    for (int i = 0; i < EXAMS; i++)
        sum += s->grades[i];
    *out = (double)sum / EXAMS;
    return STATUS_OK;
}

status_t total_average(const Student *arr, const size_t n, double *out)
{
    if (arr == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    if (n == 0)
        return STATUS_INVALID_COUNT;
    ull sum = 0;
    for (size_t i = 0; i < n; i++)
        for (int j = 0; j < EXAMS; j++)
            sum += arr[i].grades[j];
    *out = (double)sum / (double)(n * EXAMS);
    return STATUS_OK;
}

status_t find_above_average(const Student *arr, const size_t n, const double eps,
                            const Student ***found, size_t *count)
{
    if (arr == NULL || found == NULL || count == NULL)
        return STATUS_NULL_POINTER;
    if (eps <= 0 || eps >= 0.5)
        return STATUS_INVALID_EPSILON;

    double total;
    status_t st = total_average(arr, n, &total);
    if (st != STATUS_OK)
        return st;

    const Student **res = malloc(n * sizeof(*res));
    if (res == NULL)
        return STATUS_MEMORY_ERROR;

    size_t c = 0;
    for (size_t i = 0; i < n; i++)
    {
        double avg;
        st = average(&arr[i], &avg);
        if (st != STATUS_OK)
        {
            free(res);
            return st;
        }
        if (avg - total > eps)
            res[c++] = &arr[i];
    }
    *found = res;
    *count = c;
    return STATUS_OK;
}

status_t print_student(FILE *out, const Student *s)
{
    if (out == NULL || s == NULL)
        return STATUS_NULL_POINTER;
    fprintf(out, "%d %s %s %s |", s->id, s->name, s->surname, s->group);
    for (int i = 0; i < EXAMS; i++)
        fprintf(out, " %d", s->grades[i]);
    fprintf(out, "\n");
    return ferror(out) ? STATUS_FILE_WRITE_ERROR : STATUS_OK;
}

status_t print_info(FILE *out, const Student *s)
{
    if (out == NULL || s == NULL)
        return STATUS_NULL_POINTER;
    double avg;
    status_t st = average(s, &avg);
    if (st != STATUS_OK)
        return st;
    fprintf(out, "%s %s, группа %s, средняя оценка %.2f\n", s->surname, s->name, s->group, avg);
    return ferror(out) ? STATUS_FILE_WRITE_ERROR : STATUS_OK;
}

status_t print_name(FILE *out, const Student *s)
{
    if (out == NULL || s == NULL)
        return STATUS_NULL_POINTER;
    fprintf(out, "%s %s\n", s->surname, s->name);
    return ferror(out) ? STATUS_FILE_WRITE_ERROR : STATUS_OK;
}

status_t read_line(char *buf, const size_t size, bool *eof)
{
    if (buf == NULL || eof == NULL)
        return STATUS_NULL_POINTER;

    *eof = false;
    if (fgets(buf, (int)size, stdin) == NULL)
    {
        *eof = true;
        return ferror(stdin) ? STATUS_FILE_READ_ERROR : STATUS_OK;
    }
    size_t len = strcspn(buf, "\n");
    if (buf[len] == '\0' && !feof(stdin))
    {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
        return STATUS_INVALID_ARGUMENT;
    }
    buf[len] = '\0';
    return STATUS_OK;
}

status_t ask(const char *prompt, char *buf, const size_t size)
{
    bool eof = false;
    printf("%s", prompt);
    status_t st = read_line(buf, size, &eof);
    if (st == STATUS_OK && (eof || buf[0] == '\0'))
        st = STATUS_INVALID_ARGUMENT;
    return st;
}

status_t search(const Student *arr, const size_t n, const int choice)
{
    char buf[LINE];
    status_t st = ask("Введите значение: ", buf, sizeof(buf));
    if (st != STATUS_OK)
        return st;

    int id = 0;
    match_fn match = match_group;
    const void *key = buf;
    switch (choice)
    {
    case 1:
        st = parse_id(buf, &id);
        match = match_id;
        key = &id;
        break;
    case 2:
        match = match_surname;
        break;
    case 3:
        match = match_name;
        break;
    default:
        break;
    }
    if (st != STATUS_OK)
        return st;

    const Student **found = NULL;
    size_t count = 0;
    st = find_all(arr, n, match, key, &found, &count);
    if (st == STATUS_OK && count == 0)
        printf("Не найдено\n");
    for (size_t i = 0; st == STATUS_OK && i < count; i++)
        st = print_student(stdout, found[i]);
    free(found);
    return st;
}

status_t sort_by(Student *arr, const size_t n, const int choice)
{
    cmp_fn cmp = cmp_group;
    if (choice == 5)
        cmp = cmp_id;
    else if (choice == 6)
        cmp = cmp_surname;
    else if (choice == 7)
        cmp = cmp_name;

    status_t st = sort_students(arr, n, cmp);
    for (size_t i = 0; st == STATUS_OK && i < n; i++)
        st = print_student(stdout, &arr[i]);
    return st;
}

status_t trace_by_id(const Student *arr, const size_t n, FILE *trace)
{
    char buf[LINE];
    int id = 0;
    status_t st = ask("Введите id: ", buf, sizeof(buf));
    if (st == STATUS_OK)
        st = parse_id(buf, &id);
    if (st != STATUS_OK)
        return st;

    const Student **found = NULL;
    size_t count = 0;
    st = find_all(arr, n, match_id, &id, &found, &count);
    for (size_t i = 0; st == STATUS_OK && i < count; i++)
        st = print_info(trace, found[i]);
    if (st == STATUS_OK && fflush(trace) != 0)
        st = STATUS_FILE_WRITE_ERROR;
    if (st == STATUS_OK && count == 0)
        printf("Не найдено\n");
    else if (st == STATUS_OK)
        printf("Записано в трассировочный файл: %zu\n", count);
    free(found);
    return st;
}

status_t trace_above_average(const Student *arr, const size_t n, FILE *trace)
{
    const Student **found = NULL;
    size_t count = 0;
    status_t st = find_above_average(arr, n, EPS, &found, &count);
    for (size_t i = 0; st == STATUS_OK && i < count; i++)
        st = print_name(trace, found[i]);
    if (st == STATUS_OK && fflush(trace) != 0)
        st = STATUS_FILE_WRITE_ERROR;
    if (st == STATUS_OK)
        printf("Записано в трассировочный файл: %zu\n", count);
    free(found);
    return st;
}

status_t handle(Student *arr, const size_t n, FILE *trace, const int choice, bool *done)
{
    switch (choice)
    {
    case 0:
        *done = true;
        return STATUS_OK;
    case 1:
    case 2:
    case 3:
    case 4:
        return search(arr, n, choice);
    case 5:
    case 6:
    case 7:
    case 8:
        return sort_by(arr, n, choice);
    case 9:
        return trace_by_id(arr, n, trace);
    case 10:
        return trace_above_average(arr, n, trace);
    default:
        return STATUS_INVALID_ARGUMENT;
    }
}

void run_menu(Student *arr, const size_t n, FILE *trace)
{
    char buf[LINE];
    bool done = false;
    while (!done)
    {
        printf("\n1-4: поиск по id / фамилии / имени / группе\n"
               "5-8: сортировка по id / фамилии / имени / группе\n"
               "9: в файл данные студента по id\n"
               "10: в файл студенты с баллом выше среднего\n"
               "0: выход\n> ");

        int choice = 0;
        status_t st = read_line(buf, sizeof(buf), &done);
        if (st == STATUS_OK && !done)
            st = parse_int(buf, &choice);
        if (st == STATUS_OK && !done)
            st = handle(arr, n, trace, choice, &done);
        if (st != STATUS_OK)
            print_error(st);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Формат: [program] [файл студентов] [трассировочный файл]\n");
        return 1;
    }
    if (strcmp(argv[1], argv[2]) == 0)
    {
        print_error(STATUS_SAME_FILE);
        return 1;
    }

    Student *arr = NULL;
    size_t n = 0;
    FILE *trace = NULL;

    status_t st = read_students(argv[1], &arr, &n);
    if (st == STATUS_OK)
    {
        trace = fopen(argv[2], "w");
        if (trace == NULL)
            st = STATUS_FILE_OPEN_ERROR;
    }
    if (st == STATUS_OK && n == 0)
        printf("В файле нет студентов\n");
    else if (st == STATUS_OK)
        run_menu(arr, n, trace);

    if (st != STATUS_OK)
        print_error(st);

    if (trace != NULL)
        fclose(trace);
    free_students(arr, n);
    return (st == STATUS_OK) ? 0 : 1;
}