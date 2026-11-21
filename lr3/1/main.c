#include "../utils.h"

#define BUF_SIZE 34

status_t add(const unsigned a, const unsigned b, unsigned *out)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;

    unsigned x = a;
    unsigned y = b;
    while (y != 0)
    {
        unsigned carry = (x & y) << 1;
        x ^= y;
        y = carry;
    }
    *out = x;
    return STATUS_OK;
}

status_t to_base_2r(const int number, const unsigned r, char *out, const size_t size)
{
    if (out == NULL)
        return STATUS_NULL_POINTER;
    if (r < 1 || r > 5)
        return STATUS_INVALID_R;
    if (size < BUF_SIZE)
        return STATUS_SMALL_BUFFER;

    const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
    const unsigned mask = ~(~0u << r);
    status_t st;

    const bool negative = number < 0;
    unsigned n = (unsigned)number;
    if (negative)
    {
        st = add(~n, 1, &n);
        if (st != STATUS_OK)
            return st;
    }

    unsigned len = 0;
    do
    {
        out[len] = digits[n & mask];
        st = add(len, 1, &len);
        if (st != STATUS_OK)
            return st;
        n >>= r;
    } while (n != 0);

    if (negative)
    {
        out[len] = '-';
        st = add(len, 1, &len);
        if (st != STATUS_OK)
            return st;
    }
    out[len] = '\0';

    unsigned i = 0;
    unsigned j;
    st = add(len, ~0u, &j);
    if (st != STATUS_OK)
        return st;

    while (i < j)
    {
        char tmp = out[i];
        out[i] = out[j];
        out[j] = tmp;

        st = add(i, 1, &i);
        if (st != STATUS_OK)
            return st;
        st = add(j, ~0u, &j);
        if (st != STATUS_OK)
            return st;
    }

    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Формат: [program] [число]\n");
        return 1;
    }

    int number;
    status_t st = parse_int(argv[1], &number);

    char res[BUF_SIZE];
    unsigned r = 1;
    while (st == STATUS_OK && r <= 5)
    {
        st = to_base_2r(number, r, res, sizeof(res));
        if (st == STATUS_OK)
        {
            printf("r = %u (основание %u): %s\n", r, 1u << r, res);
            st = add(r, 1, &r);
        }
    }

    if (st != STATUS_OK)
        print_error(st);

    return (st == STATUS_OK) ? 0 : 1;
}