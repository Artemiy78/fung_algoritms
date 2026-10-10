#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_OUT_OF_RANGE,
    STATUS_EMPTY_PATH,
    STATUS_SAME_FILE,
    STATUS_PATH_TOO_LONG,
    STATUS_UNKNOWN_FLAG,
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
    case STATUS_EMPTY_PATH:
        *msg = "путь к файлу не может быть пустым";
        break;
    case STATUS_SAME_FILE:
        *msg = "входной и выходной файлы совпадают";
        break;
    case STATUS_PATH_TOO_LONG:
        *msg = "путь к выходному файлу слишком длинный";
        break;
    case STATUS_UNKNOWN_FLAG:
        *msg = "неизвестный флаг";
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

status_t is_digit(const int c, bool *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (c < 0 || c > 255)
    {
        return STATUS_OUT_OF_RANGE;
    }

    *res = (c >= '0' && c <= '9');
    return STATUS_OK;
}

status_t is_latin(const int c, bool *res)
{
    if (res == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (c < 0 || c > 255)
    {
        return STATUS_OUT_OF_RANGE;
    }

    *res = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    return STATUS_OK;
}

status_t make_out_path(const char *in_path, char *out_path, const size_t size)
{
    if (in_path == NULL || out_path == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    size_t dir_len = 0;
    for (size_t i = 0; in_path[i] != '\0'; i++)
    {
        if (in_path[i] == '/' || in_path[i] == '\\')
        {
            dir_len = i + 1;
        }
    }

    int n = snprintf(out_path, size, "%.*s%s%s", (int)dir_len, in_path,
                     "out_", in_path + dir_len);
    if (n < 0 || (size_t)n >= size)
    {
        return STATUS_PATH_TOO_LONG;
    }
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
    if (strcmp(in_path, out_path) == 0)
    {
        return STATUS_SAME_FILE;
    }
    return STATUS_OK;
}

status_t remove_digits(FILE *in, FILE *out)
{
    if (in == NULL || out == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    int c;
    while ((c = fgetc(in)) != EOF)
    {
        bool digit;
        status_t st = is_digit(c, &digit);
        if (st != STATUS_OK)
        {
            return st;
        }
        if (!digit && fputc(c, out) == EOF)
        {
            return STATUS_IO_ERROR;
        }
    }
    if (ferror(in))
        return STATUS_IO_ERROR;
    return STATUS_OK;
}

status_t count_per_line(FILE *in, FILE *out, const bool need_latins)
{
    if (in == NULL || out == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    size_t count = 0;
    bool line_has_chars = false; // для последней строки без \n
    int c;
    while ((c = fgetc(in)) != EOF)
    {
        if (c == '\n')
        {
            if (fprintf(out, "%zu\n", count) < 0)
            {
                return STATUS_IO_ERROR;
            }
            count = 0;
            line_has_chars = false;
            continue;
        }

        line_has_chars = true;
        bool latin;
        status_t st = is_latin(c, &latin);
        if (st != STATUS_OK)
        {
            return st;
        }

        if (!need_latins)
        {
            bool digit;
            st = is_digit(c, &digit);
            if (st != STATUS_OK)
            {
                return st;
            }
            if (!latin && !digit && c != ' ')
            {
                count++;
            }
        }
        else if (latin)
        {
            count++;
        }
    }
    if (ferror(in))
    {
        return STATUS_IO_ERROR;
    }
    if (line_has_chars && fprintf(out, "%zu\n", count) < 0)
    {
        return STATUS_IO_ERROR;
    }
    return STATUS_OK;
}

status_t replace_non_digits(FILE *in, FILE *out)
{
    if (in == NULL || out == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    int c;
    while ((c = fgetc(in)) != EOF)
    {
        bool digit;
        status_t st = is_digit(c, &digit);
        if (st != STATUS_OK)
        {
            return st;
        }

        int rc;
        if (digit || c == '\n')
        {
            rc = fputc(c, out);
        }
        else
        {
            rc = fprintf(out, "%02X", (unsigned)c);
        }
        if (rc < 0)
        {
            return STATUS_IO_ERROR;
        }
    }
    if (ferror(in))
        return STATUS_IO_ERROR;
    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Число аргументов должно быть не меньше 3\n");
        return 1;
    }

    if ((argv[1][0] != '-' && argv[1][0] != '/') || argv[1][1] == '\0')
    {
        fprintf(stderr, "Формат флага: -nX, /nX или -X, /X\n");
        return 1;
    }

    char flag;
    bool is_n;
    if (argv[1][2] == '\0')
    {
        is_n = false;
        flag = argv[1][1];
    }
    else if (argv[1][3] == '\0' && argv[1][1] == 'n')
    {
        is_n = true;
        flag = argv[1][2];
    }
    else
    {
        fprintf(stderr, "Формат флага: -nX, /nX или -X, /X\n");
        return 1;
    }

    if (flag != 'd' && flag != 'i' && flag != 's' && flag != 'a')
    {
        fprintf(stderr, "Флаги: d, i, s, a\n");
        return 1;
    }

    if (is_n && argc != 4)
    {
        fprintf(stderr, "Верный формат с n: [программа] [флаг с n] [входной файл] [выходной файл]\n");
        return 1;
    }
    if (!is_n && argc != 3)
    {
        fprintf(stderr, "Верный формат без n: [программа] [флаг без n] [входной файл]\n");
        return 1;
    }

    status_t st;
    const char *in_path = argv[2];
    char out_buf[1024];
    const char *out_path;

    if (is_n)
    {
        out_path = argv[3];
    }
    else
    {
        st = make_out_path(in_path, out_buf, sizeof(out_buf));
        if (st != STATUS_OK)
        {
            print_error(st);
            return 1;
        }
        out_path = out_buf;
    }

    st = check_paths(in_path, out_path);
    if (st != STATUS_OK)
    {
        print_error(st);
        return 1;
    }

    FILE *in = fopen(in_path, "r");
    if (in == NULL)
    {
        fprintf(stderr, "Не удалось открыть входной файл '%s'\n", in_path);
        return 1;
    }

    FILE *out = fopen(out_path, "w");
    if (out == NULL)
    {
        fprintf(stderr, "Не удалось открыть выходной файл '%s'\n", out_path);
        fclose(in);
        return 1;
    }

    switch (flag)
    {
    case 'd':
        st = remove_digits(in, out);
        break;
    case 'i':
        st = count_per_line(in, out, true);
        break;
    case 's':
        st = count_per_line(in, out, false);
        break;
    case 'a':
        st = replace_non_digits(in, out);
        break;
    default:
        st = STATUS_UNKNOWN_FLAG;
        break;
    }

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