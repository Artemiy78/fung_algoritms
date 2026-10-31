#include "utils.h"

status_t str_len(const char *str, size_t *out)
{
    if (str == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    size_t i = 0;
    while (str[i] != '\0')
    {
        i++;
    }

    *out = i;
    return STATUS_OK;
}

status_t reverse(const char *str, char **out)
{
    if (str == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    size_t len;
    status_t st = str_len(str, &len);
    if (st != STATUS_OK)
        return st;
    char *res = malloc((len + 1) * sizeof(char));
    if (res == NULL)
        return STATUS_MEMORY_ERROR;
    for (size_t i = 0; i < len; i++)
        res[len - i - 1] = str[i];
    res[len] = '\0';
    *out = res;
    return STATUS_OK;
}

status_t transform_u(const char *str, char **out)
{
    if (str == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    size_t len;
    status_t st = str_len(str, &len);
    if (st != STATUS_OK)
        return st;
    char *res = malloc((len + 1) * sizeof(char));
    if (res == NULL)
        return STATUS_MEMORY_ERROR;
    for (size_t i = 0; i < len; i++)
    {
        if ((i % 2) != 0 && str[i] >= 'a' && str[i] <= 'z')
            res[i] = str[i] - 'a' + 'A';
        else
            res[i] = str[i];
    }
    res[len] = '\0';
    *out = res;
    return STATUS_OK;
}

status_t order_n(const char *str, char **out)
{
    if (str == NULL || out == NULL)
        return STATUS_NULL_POINTER;
    size_t len;
    status_t st = str_len(str, &len);
    if (st != STATUS_OK)
        return st;
    char *res = malloc((len + 1) * sizeof(char));
    if (res == NULL)
        return STATUS_MEMORY_ERROR;

    size_t pos = 0;

    for (size_t j = 0; j < len; j++)
    {
        char c = str[j];
        bool is_d = false;
        st = is_digit(c, &is_d);
        if (st != STATUS_OK)
        {
            free(res);
            return st;
        }
        if (is_d)
            res[pos++] = c;
    }

    for (size_t j = 0; j < len; j++)
    {
        char c = str[j];
        bool is_a = false;
        st = is_alpha(c, &is_a);
        if (st != STATUS_OK)
        {
            free(res);
            return st;
        }
        if (is_a)
            res[pos++] = c;
    }

    for (size_t j = 0; j < len; j++)
    {
        char c = str[j];
        bool is_d = false;
        st = is_digit(c, &is_d);
        if (st != STATUS_OK)
        {
            free(res);
            return st;
        }
        bool is_a = false;
        st = is_alpha(c, &is_a);
        if (st != STATUS_OK)
        {
            free(res);
            return st;
        }
        if (!is_a && !is_d)
            res[pos++] = c;
    }

    res[pos] = '\0';
    *out = res;
    return STATUS_OK;
}

status_t shuffle(const char **arr, size_t count)
{
    if (arr == NULL)
        return STATUS_NULL_POINTER;

    if (count < 2)
        return STATUS_OK;

    for (size_t i = count - 1; i > 0; i--)
    {
        size_t j = (size_t)rand() % (i + 1);
        const char *tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
    return STATUS_OK;
}

status_t add_random(const char **array, size_t n, size_t total_len, char **out)
{
    if (array == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    char *res = malloc(total_len + 1);
    if (res == NULL)
        return STATUS_MEMORY_ERROR;

    size_t counter = 0;
    for (size_t i = 0; i < n; i++)
    {
        for (size_t j = 0; array[i][j] != '\0'; j++)
            res[counter++] = array[i][j];
    }
    res[counter] = '\0';
    *out = res;
    return STATUS_OK;
}

status_t concat_random(const char **array, size_t n, unsigned seed, char **out)
{
    if (array == NULL || out == NULL)
        return STATUS_NULL_POINTER;

    size_t total_len = 0;
    for (size_t i = 0; i < n; i++)
    {
        size_t len;
        status_t st = str_len(array[i], &len);
        if (st != STATUS_OK)
            return st;
        total_len += len;
    }

    srand(seed);
    status_t st = shuffle(array, n);
    if (st != STATUS_OK)
        return st;

    return add_random(array, n, total_len, out);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Число аргументов должно быть больше 2\n");
        return 1;
    }

    char flag;
    if ((argv[1][0] != '-') || (argv[1][1] == '\0' || argv[1][2] != '\0'))
    {
        printf("Флаг должен иметь формат -x\n");
        return 1;
    }
    flag = argv[1][1];
    if (flag != 'c' && argc != 3)
    {
        printf("Формат для l, r, u, n: [program] [flag] [str]\n");
        printf("Формат для c: [program] [flag] [str] [unsigned] [произвольное кол-во str]\n");
        return 1;
    }
    else if (flag == 'c' && argc < 4)
    {
        printf("Формат для c: [program] [flag] [str] [unsigned] [произвольное кол-во str]\n");
        return 1;
    }

    char *str = argv[2];
    char *res = NULL;
    const char **array = NULL;
    size_t len = 0;
    status_t st = STATUS_OK;

    switch (flag)
    {
    case 'l':
        st = str_len(str, &len);
        break;
    case 'r':
        st = reverse(str, &res);
        break;
    case 'u':
        st = transform_u(str, &res);
        break;
    case 'n':
        st = order_n(str, &res);
        break;
    case 'c':
    {
        unsigned seed;
        st = parse_unsigned(argv[3], &seed);
        if (st != STATUS_OK)
            break;
        size_t count = (size_t)(argc - 3);
        array = malloc(count * sizeof(char *));
        if (array == NULL)
        {
            st = STATUS_MEMORY_ERROR;
            break;
        }
        array[0] = argv[2];
        for (size_t k = 1; k < count; k++)
            array[k] = argv[k + 3];
        st = concat_random(array, count, seed, &res);
        break;
    }
    default:
        st = STATUS_INVALID_FLAG;
        break;
    }

    if (st != STATUS_OK)
        print_error(st);
    else if (flag == 'l')
        printf("%zu\n", len);
    else
        printf("%s\n", res);

    free(array);
    free(res);
    return (st == STATUS_OK) ? 0 : 1;
}