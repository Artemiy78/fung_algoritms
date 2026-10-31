#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#define START_CAPACITY 16

typedef unsigned long long ull;

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_OUT_OF_RANGE,
    STATUS_INVALID_CHAR,
    STATUS_OVERFLOW,
    STATUS_MEMORY_ERROR,
    STATUS_EMPTY_PATH,
    STATUS_SAME_FILE,
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
    case STATUS_INVALID_CHAR:
        *msg = "в числе недопустимый символ (разрешены цифры и латинские буквы)";
        break;
    case STATUS_OVERFLOW:
        *msg = "десятичное значение числа слишком велико";
        break;
    case STATUS_MEMORY_ERROR:
        *msg = "не удалось выделить память";
        break;
    case STATUS_EMPTY_PATH:
        *msg = "путь к файлу не может быть пустым";
        break;
    case STATUS_SAME_FILE:
        *msg = "выходной файл совпадает с входным";
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
    printf("Ошибка: %s\n", msg);
    return STATUS_OK;
}

status_t get_file_name(const char *path, const char **name)
{
    if (path == NULL || name == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    const char *result = path;
    for (const char *p = path; *p != '\0'; p++)
    {
        if (*p == '/' || *p == '\\')
        {
            result = p + 1;
        }
    }

    *name = result;
    return STATUS_OK;
}

status_t check_paths(const char *in_path, const char *out_path)
{
    if (in_path == NULL || out_path == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (in_path[0] == '\0' || out_path[0] == '\0')
    {
        return STATUS_EMPTY_PATH;
    }

    const char *in_name;
    const char *out_name;
    status_t st = get_file_name(in_path, &in_name);
    if (st == STATUS_OK)
    {
        st = get_file_name(out_path, &out_name);
    }
    if (st != STATUS_OK)
    {
        return st;
    }

    if (strcmp(in_name, out_name) == 0)
    {
        return STATUS_SAME_FILE;
    }
    return STATUS_OK;
}

status_t is_separator(const int c, bool *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (c < 0 || c > 255)
    {
        return STATUS_OUT_OF_RANGE;
    }

    *res = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    return STATUS_OK;
}

status_t digit_value(const int c, unsigned *value)
{
    if (value == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (c < 0 || c > 255)
    {
        return STATUS_OUT_OF_RANGE;
    }

    if (c >= '0' && c <= '9')
    {
        *value = (unsigned)(c - '0');
    }
    else if (c >= 'A' && c <= 'Z')
    {
        *value = (unsigned)(c - 'A') + 10;
    }
    else if (c >= 'a' && c <= 'z')
    {
        *value = (unsigned)(c - 'a') + 10;
    }
    else
    {
        return STATUS_INVALID_CHAR;
    }
    return STATUS_OK;
}

status_t skip_separators(FILE *in, int *first)
{
    if (in == NULL || first == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    int c;
    while ((c = fgetc(in)) != EOF)
    {
        bool sep;
        status_t st = is_separator(c, &sep);
        if (st != STATUS_OK)
        {
            return st;
        }
        if (!sep)
        {
            break;
        }
    }

    *first = c;
    return STATUS_OK;
}

status_t read_token(FILE *in, char **token, size_t *len)
{
    if (in == NULL || token == NULL || len == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    *token = NULL;

    int c;
    status_t st = skip_separators(in, &c);
    if (st != STATUS_OK)
    {
        return st;
    }

    size_t capacity = START_CAPACITY;
    char *buf = (char *)malloc(capacity);
    if (buf == NULL)
    {
        return STATUS_MEMORY_ERROR;
    }

    size_t n = 0;
    while (c != EOF)
    {
        bool sep;
        st = is_separator(c, &sep);
        if (st != STATUS_OK)
        {
            free(buf);
            return st;
        }
        if (sep)
        {
            break;
        }

        if (n + 1 >= capacity)
        {
            if (capacity > SIZE_MAX / 2)
            {
                free(buf);
                return STATUS_MEMORY_ERROR;
            }
            capacity *= 2;
            char *bigger = realloc(buf, capacity);
            if (bigger == NULL)
            {
                free(buf);
                return STATUS_MEMORY_ERROR;
            }
            buf = bigger;
        }
        buf[n] = (char)c;
        n++;
        c = fgetc(in);
    }
    if (ferror(in))
    {
        free(buf);
        return STATUS_IO_ERROR;
    }

    buf[n] = '\0';
    *token = buf;
    *len = n;
    return STATUS_OK;
}

status_t token_to_upper(char *token, const size_t len)
{
    if (token == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    for (size_t i = 0; i < len; i++)
    {
        if (token[i] >= 'a' && token[i] <= 'z')
        {
            token[i] = (char)(token[i] - 'a' + 'A');
        }
    }
    return STATUS_OK;
}

status_t min_base(const char *token, const size_t len, unsigned *base)
{
    if (token == NULL || base == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    unsigned max_digit = 0;
    for (size_t i = 0; i < len; i++)
    {
        unsigned d;
        status_t st = digit_value((unsigned char)token[i], &d);
        if (st != STATUS_OK)
        {
            return st;
        }
        if (d > max_digit)
        {
            max_digit = d;
        }
    }

    *base = (max_digit + 1 < 2) ? 2 : max_digit + 1;
    return STATUS_OK;
}

status_t to_decimal(const char *token, const size_t len, const unsigned base, ull *res)
{
    if (token == NULL || res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (base < 2 || base > 36)
    {
        return STATUS_OUT_OF_RANGE;
    }

    ull value = 0;
    for (size_t i = 0; i < len; i++)
    {
        unsigned d;
        status_t st = digit_value((unsigned char)token[i], &d);
        if (st != STATUS_OK)
        {
            return st;
        }
        if (d >= base)
        {
            return STATUS_OUT_OF_RANGE;
        }

        if (value > (ULLONG_MAX - d) / base)
        {
            return STATUS_OVERFLOW;
        }
        value = value * base + d;
    }

    *res = value;
    return STATUS_OK;
}

status_t first_important(const char *token, const size_t len, size_t *start)
{
    if (token == NULL || start == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (len == 0)
    {
        return STATUS_OUT_OF_RANGE;
    }

    size_t i = 0;
    while (i + 1 < len && token[i] == '0')
    {
        i++;
    }
    *start = i;
    return STATUS_OK;
}

status_t convert_file(FILE *in, FILE *out)
{
    if (in == NULL || out == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    while (1)
    {
        char *token;
        size_t len;
        status_t st = read_token(in, &token, &len);
        if (st != STATUS_OK)
        {
            return st;
        }
        if (len == 0)
        {
            free(token);
            break;
        }

        unsigned base;
        ull value;
        size_t start;

        st = token_to_upper(token, len);
        if (st == STATUS_OK)
        {
            st = min_base(token, len, &base);
        }
        if (st == STATUS_OK)
        {
            st = to_decimal(token, len, base, &value);
        }
        if (st == STATUS_OK)
        {
            st = first_important(token, len, &start);
        }
        if (st == STATUS_OK && fprintf(out, "%s %u %llu\n", token + start, base, value) < 0)
        {
            st = STATUS_IO_ERROR;
        }

        free(token);
        if (st != STATUS_OK)
        {
            return st;
        }
    }
    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Формат: [программа] [входной файл] [выходной файл]\n");
        return 1;
    }

    status_t st = check_paths(argv[1], argv[2]);
    if (st != STATUS_OK)
    {
        print_error(st);
        return 1;
    }

    FILE *in = fopen(argv[1], "r");
    if (in == NULL)
    {
        printf("Не удалось открыть входной файл '%s'\n", argv[1]);
        return 1;
    }

    FILE *out = fopen(argv[2], "w");
    if (out == NULL)
    {
        printf("Не удалось открыть выходной файл '%s'\n", argv[2]);
        fclose(in);
        return 1;
    }

    st = convert_file(in, out);

    fclose(in);
    if (fclose(out) != 0 && st == STATUS_OK)
    {
        st = STATUS_IO_ERROR;
    }

    if (st != STATUS_OK)
    {
        print_error(st);
        return 1;
    }
    return 0;
}