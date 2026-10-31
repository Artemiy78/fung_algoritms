#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_OUT_OF_RANGE,
    STATUS_BUFFER_TOO_SMALL,
    STATUS_EMPTY_PATH,
    STATUS_SAME_FILE,
    STATUS_IO_ERROR,
} status_t;

typedef enum
{
    TOKEN_DEFAULT,
    TOKEN_LOWER,
    TOKEN_LOWER_4,
    TOKEN_8,
} token_mode_t;

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
    case STATUS_BUFFER_TOO_SMALL:
        *msg = "буфер слишком мал";
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

status_t to_lower_latin(const int c, int *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (c < 0 || c > 255)
    {
        return STATUS_OUT_OF_RANGE;
    }

    *res = (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
    return STATUS_OK;
}

status_t number_to_base(const unsigned value, const unsigned base, char *buf,
                        const size_t size, size_t *len)
{
    if (buf == NULL || len == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (base < 2 || base > 16)
    {
        return STATUS_OUT_OF_RANGE;
    }

    const char *digits = "0123456789ABCDEF";
    char tmp[sizeof(unsigned) * 8];
    unsigned remainder = value;
    size_t n = 0;

    do
    {
        tmp[n] = digits[remainder % base];
        remainder /= base;
        n++;
    } while (remainder > 0);

    if (n > size)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    for (size_t i = 0; i < n; i++)
    {
        buf[i] = tmp[n - 1 - i];
    }
    *len = n;
    return STATUS_OK;
}

status_t get_token_mode(const size_t index, token_mode_t *mode)
{
    if (mode == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (index < 1)
    {
        return STATUS_OUT_OF_RANGE;
    }

    if (index % 10 == 0)
    {
        *mode = TOKEN_LOWER_4;
    }
    else if (index % 2 == 0)
    {
        *mode = TOKEN_LOWER;
    }
    else if (index % 5 == 0)
    {
        *mode = TOKEN_8;
    }
    else
    {
        *mode = TOKEN_DEFAULT;
    }
    return STATUS_OK;
}

status_t write_token_char(FILE *out, const int c, const token_mode_t mode)
{
    if (out == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    int lower;
    status_t st = to_lower_latin(c, &lower);
    if (st != STATUS_OK)
    {
        return st;
    }

    if (mode == TOKEN_DEFAULT || mode == TOKEN_LOWER)
    {
        int ch = (mode == TOKEN_LOWER) ? lower : c;
        return (fputc(ch, out) == EOF) ? STATUS_IO_ERROR : STATUS_OK;
    }

    char buf[sizeof(unsigned) * 8];
    size_t len;
    if (mode == TOKEN_LOWER_4)
    {
        st = number_to_base((unsigned)lower, 4, buf, sizeof(buf), &len);
    }
    else
    {
        st = number_to_base((unsigned)c, 8, buf, sizeof(buf), &len);
    }
    if (st != STATUS_OK)
    {
        return st;
    }

    for (size_t i = 0; i < len; i++)
    {
        if (fputc(buf[i], out) == EOF)
        {
            return STATUS_IO_ERROR;
        }
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

status_t copy_token(FILE *in, FILE *out, const token_mode_t mode,
                    const bool need_space, bool *got)
{
    if (in == NULL || out == NULL || got == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    int c;
    status_t st = skip_separators(in, &c);
    if (st != STATUS_OK)
    {
        return st;
    }
    if (c == EOF)
    {
        *got = false;
        return ferror(in) ? STATUS_IO_ERROR : STATUS_OK;
    }

    *got = true;
    if (need_space)
    {
        if (fputc(' ', out) == EOF)
            return STATUS_IO_ERROR;
    }

    while (c != EOF)
    {
        bool sep;
        st = is_separator(c, &sep);
        if (st != STATUS_OK)
        {
            return st;
        }
        if (sep)
        {
            break;
        }

        st = write_token_char(out, c, mode);
        if (st != STATUS_OK)
        {
            return st;
        }
        c = fgetc(in);
    }
    return ferror(in) ? STATUS_IO_ERROR : STATUS_OK;
}

status_t combine_files(FILE *in1, FILE *in2, FILE *out)
{
    if (in1 == NULL || in2 == NULL || out == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    bool eof1 = false;
    bool eof2 = false;
    bool need_space = false; // чтобы не ставился пробел перел 1 лексемой
    int turn = 1;

    while (!eof1 || !eof2)
    {
        if (turn == 1 && eof1)
        {
            turn = 2;
        }
        else if (turn == 2 && eof2)
        {
            turn = 1;
        }

        bool got;
        status_t st;
        if (turn == 1)
            st = copy_token(in1, out, TOKEN_DEFAULT, need_space, &got);
        else
            st = copy_token(in2, out, TOKEN_DEFAULT, need_space, &got);

        if (st != STATUS_OK)
        {
            return st;
        }

        if (!got)
        {
            if (turn == 1)
            {
                eof1 = true;
            }
            else
            {
                eof2 = true;
            }
            continue;
        }
        need_space = true;
        turn = (turn == 1) ? 2 : 1;
    }
    return STATUS_OK;
}

status_t transform_file(FILE *in, FILE *out)
{
    if (in == NULL || out == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    size_t index = 1;
    bool need_space = false;
    bool got = true;

    while (got)
    {
        token_mode_t mode;
        status_t st = get_token_mode(index, &mode);
        if (st != STATUS_OK)
        {
            return st;
        }

        st = copy_token(in, out, mode, need_space, &got);
        if (st != STATUS_OK)
        {
            return st;
        }
        if (got)
        {
            need_space = true;
            index++;
        }
    }
    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Формат: [программа] [флаг] [пути к файлам]\n");
        return 1;
    }

    if ((argv[1][0] != '-' && argv[1][0] != '/') ||
        argv[1][1] == '\0' || argv[1][2] != '\0')
    {
        printf("Флаг должен иметь формат -n или /n\n");
        return 1;
    }
    char flag = argv[1][1];

    switch (flag)
    {
    case 'r':
        if (argc != 5)
        {
            printf("Формат для -r: [программа] -r [file1] [file2] [выходной файл]\n");
            return 1;
        }
        break;
    case 'a':
        if (argc != 4)
        {
            printf("Формат для -a: [программа] -a [входной файл] [выходной файл]\n");
            return 1;
        }
        break;
    default:
        printf("Флаги: r, a\n");
        return 1;
    }

    status_t st;
    FILE *in1 = NULL;
    FILE *in2 = NULL;
    FILE *out = NULL;

    switch (flag)
    {
    case 'r':
    {
        st = check_paths(argv[2], argv[4]);
        if (st == STATUS_OK)
        {
            st = check_paths(argv[3], argv[4]);
        }
        if (st != STATUS_OK)
        {
            print_error(st);
            return 1;
        }

        in1 = fopen(argv[2], "r");
        if (in1 == NULL)
        {
            printf("Не удалось открыть файл '%s'\n", argv[2]);
            return 1;
        }
        in2 = fopen(argv[3], "r");
        if (in2 == NULL)
        {
            printf("Не удалось открыть файл '%s'\n", argv[3]);
            fclose(in1);
            return 1;
        }
        out = fopen(argv[4], "w");
        if (out == NULL)
        {
            printf("Не удалось открыть выходной файл '%s'\n", argv[4]);
            fclose(in1);
            fclose(in2);
            return 1;
        }

        st = combine_files(in1, in2, out);
        fclose(in1);
        fclose(in2);
        break;
    }
    case 'a':
    {
        st = check_paths(argv[2], argv[3]);
        if (st != STATUS_OK)
        {
            print_error(st);
            return 1;
        }

        in1 = fopen(argv[2], "r");
        if (in1 == NULL)
        {
            printf("Не удалось открыть файл '%s'\n", argv[2]);
            return 1;
        }
        out = fopen(argv[3], "w");
        if (out == NULL)
        {
            printf("Не удалось открыть выходной файл '%s'\n", argv[3]);
            fclose(in1);
            return 1;
        }

        st = transform_file(in1, out);
        fclose(in1);
        break;
    }
    default:
        printf("Флаги: r, a\n");
        return 1;
    }

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