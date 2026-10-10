#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_OUT_OF_RANGE,
    STATUS_INVALID_FORMAT,
    STATUS_MEMORY_ERROR,
    STATUS_OVERFLOW,
    STATUS_INVALID_NUMBER,
    STATUS_IO_ERROR,
} status_t;

status_t status_message(const status_t status, const char **msg)
{
    if (msg == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    switch (status)
    {
    case STATUS_OK:
        *msg = "";
        break;
    case STATUS_NULL_POINTER:
        *msg = "нулевой указатель";
        break;
    case STATUS_OUT_OF_RANGE:
        *msg = "значение вне допустимого диапазона";
        break;
    case STATUS_MEMORY_ERROR:
        *msg = "ошибка выделения памяти";
        break;
    case STATUS_OVERFLOW:
        *msg = "переполнение при вычислении";
        break;
    case STATUS_INVALID_NUMBER:
        *msg = "аргумент не является числом";
        break;
    case STATUS_IO_ERROR:
        *msg = "ошибка чтения или записи файла";
        break;
    default:
        *msg = "неизвестная ошибка";
        break;
    }
    return STATUS_OK;
}

status_t print_error(const status_t st)
{
    const char *msg;
    status_t rc = status_message(st, &msg);
    if (rc != STATUS_OK)
    {
        return rc;
    }
    fprintf(stderr, "Ошибка: %s\n", msg);
    return STATUS_OK;
}

status_t parse_int(const char *str, int *res)
{
    if (str == NULL || res == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    int x;
    char extra;
    if (sscanf(str, "%d%c", &x, &extra) != 1)
    {
        return STATUS_INVALID_NUMBER;
    }

    *res = x;
    return STATUS_OK;
}

status_t rand_range(const int min, const int max, int *const result)
{
    if (result == NULL)
        return STATUS_NULL_POINTER;
    if (min > max)
        return STATUS_OUT_OF_RANGE;

    *result = min + rand() % (max - min + 1);
    return STATUS_OK;
}

status_t print_array(const int *const arr, const size_t size)
{
    if (arr == NULL)
        return STATUS_NULL_POINTER;

    if (size == 0)
    {
        if (printf("(пустой массив)\n") < 0)
            return STATUS_IO_ERROR;
        return STATUS_OK;
    }

    for (size_t i = 0; i < size; i++)
    {
        const int written = printf("%d%s", arr[i], (i + 1 == size) ? "" : " ");
        if (written < 0)
            return STATUS_IO_ERROR;
    }
    if (printf("\n") < 0)
        return STATUS_IO_ERROR;

    return STATUS_OK;
}

status_t fill_array_random(int *const arr, const size_t size,
                           const int min_val, const int max_val)
{
    if (arr == NULL)
        return STATUS_NULL_POINTER;
    if (size == 0)
        return STATUS_OUT_OF_RANGE;
    if (min_val > max_val)
        return STATUS_OUT_OF_RANGE;

    for (size_t i = 0; i < size; i++)
    {
        int val = 0;
        const status_t st = rand_range(min_val, max_val, &val);
        if (st != STATUS_OK)
            return st;
        arr[i] = val;
    }
    return STATUS_OK;
}

status_t swap_min_max(int *const arr, const size_t size)
{
    if (arr == NULL)
        return STATUS_NULL_POINTER;
    if (size == 0)
        return STATUS_OUT_OF_RANGE;

    size_t min_idx = 0;
    size_t max_idx = 0;

    for (size_t i = 1; i < size; i++)
    {
        if (arr[i] < arr[min_idx])
            min_idx = i;
        if (arr[i] > arr[max_idx])
            max_idx = i;
    }

    const int temp = arr[min_idx];
    arr[min_idx] = arr[max_idx];
    arr[max_idx] = temp;

    return STATUS_OK;
}

status_t run_part1(const int a, const int b)
{
    const size_t fixed_size = 10;
    int arr[10];

    if (printf("1. Исходный массив (диапазон [%d..%d]):\n", a, b) < 0)
        return STATUS_IO_ERROR;

    status_t st = fill_array_random(arr, fixed_size, a, b);
    if (st != STATUS_OK)
        return st;

    st = print_array(arr, fixed_size);
    if (st != STATUS_OK)
        return st;

    st = swap_min_max(arr, fixed_size);
    if (st != STATUS_OK)
        return st;

    if (printf("После обмена минимального и максимального элементов:\n") < 0)
        return STATUS_IO_ERROR;

    st = print_array(arr, fixed_size);
    return st;
}

status_t find_closest(const int *const b_arr, const size_t b_size,
                      const int target, int *const result)
{
    if (b_arr == NULL || result == NULL)
        return STATUS_NULL_POINTER;
    if (b_size == 0)
        return STATUS_OUT_OF_RANGE;

    int closest = b_arr[0];
    int min_diff = abs(target - b_arr[0]);

    for (size_t i = 1; i < b_size; i++)
    {
        const int diff = abs(target - b_arr[i]);
        if (diff < min_diff)
        {
            min_diff = diff;
            closest = b_arr[i];
        }
    }

    *result = closest;
    return STATUS_OK;
}

status_t build_array_c(const int *const a_arr, const size_t a_size,
                       const int *const b_arr, const size_t b_size,
                       int **const c_arr)
{
    if (a_arr == NULL || b_arr == NULL || c_arr == NULL)
        return STATUS_NULL_POINTER;
    if (a_size == 0 || b_size == 0)
        return STATUS_OUT_OF_RANGE;

    *c_arr = (int *)malloc(a_size * sizeof(int));
    if (*c_arr == NULL)
        return STATUS_MEMORY_ERROR;

    status_t st = STATUS_OK;
    for (size_t i = 0; i < a_size; i++)
    {
        int closest = 0;
        st = find_closest(b_arr, b_size, a_arr[i], &closest);
        if (st != STATUS_OK)
        {
            free(*c_arr);
            *c_arr = NULL;
            return st;
        }

        const long long sum = (long long)a_arr[i] + (long long)closest;
        if (sum < INT_MIN || sum > INT_MAX)
        {
            free(*c_arr);
            *c_arr = NULL;
            return STATUS_OVERFLOW;
        }

        (*c_arr)[i] = (int)sum;
    }

    return STATUS_OK;
}

status_t run_part2(void)
{
    int size_a_val = 0, size_b_val = 0;
    status_t st = rand_range(10, 10000, &size_a_val);
    if (st != STATUS_OK)
        return st;
    st = rand_range(10, 10000, &size_b_val);
    if (st != STATUS_OK)
        return st;

    const size_t size_a = (size_t)size_a_val;
    const size_t size_b = (size_t)size_b_val;

    if (printf("2. Динамические массивы:\n") < 0)
        return STATUS_IO_ERROR;
    if (printf("Размер A = %zu, Размер B = %zu\n", size_a, size_b) < 0)
        return STATUS_IO_ERROR;

    int *a_arr = (int *)malloc(size_a * sizeof(int));
    if (a_arr == NULL)
        return STATUS_MEMORY_ERROR;

    int *b_arr = (int *)malloc(size_b * sizeof(int));
    if (b_arr == NULL)
    {
        free(a_arr);
        return STATUS_MEMORY_ERROR;
    }

    for (size_t i = 0; i < size_a; i++)
    {
        int val = 0;
        st = rand_range(-1000, 1000, &val);
        if (st != STATUS_OK)
        {
            free(a_arr);
            free(b_arr);
            return st;
        }
        a_arr[i] = val;
    }
    for (size_t i = 0; i < size_b; i++)
    {
        int val = 0;
        st = rand_range(-1000, 1000, &val);
        if (st != STATUS_OK)
        {
            free(a_arr);
            free(b_arr);
            return st;
        }
        b_arr[i] = val;
    }

    int *c_arr = NULL;
    st = build_array_c(a_arr, size_a, b_arr, size_b, &c_arr);

    if (st == STATUS_OK)
    {
        if (printf("Первые 10 элементов массива C (C[i] = A[i] + ближайший из B):\n") < 0)
            st = STATUS_IO_ERROR;
        else
        {
            const size_t show = (size_a < 10) ? size_a : 10;
            for (size_t i = 0; i < show; i++)
            {
                if (printf("C[%zu] = %d\n", i, c_arr[i]) < 0)
                {
                    st = STATUS_IO_ERROR;
                    break;
                }
            }
        }
    }
    else
    {
        free(a_arr);
        free(b_arr);
        return st;
    }

    free(c_arr);
    free(b_arr);
    free(a_arr);

    return st;
}

int main(int argc, char *argv[])
{
    srand((unsigned)time(NULL));

    if (argc == 3)
    {
        int a = 0, b = 0;
        status_t st = parse_int(argv[1], &a);
        if (st != STATUS_OK)
        {
            print_error(st);
            return 1;
        }

        st = parse_int(argv[2], &b);
        if (st != STATUS_OK)
        {
            print_error(st);
            return 1;
        }

        if (a > b)
        {
            print_error(STATUS_OUT_OF_RANGE);
            return 1;
        }

        st = run_part1(a, b);
        if (st != STATUS_OK)
        {
            print_error(st);
            return 1;
        }
    }
    else if (argc == 1)
    {
        status_t st = run_part2();
        if (st != STATUS_OK)
        {
            print_error(st);
            return 1;
        }
    }
    else
    {
        fprintf(stderr, "Неверный формат\n");
        return 1;
    }
}