#include "../utils.h"
#include <string.h>
#include <ctype.h>
#include <math.h>

#define IS_DIG(c) ((c) >= '0' && (c) <= '9')
#define LINE 1024
#define EPS 1e-9

typedef struct poly_node
{
    int coef;
    int power;
    struct poly_node *next;
} poly_node_t;

typedef struct
{
    poly_node_t *head;
} polynomial_t;

void free_poly(polynomial_t *p)
{
    while (p->head)
    {
        poly_node_t *next = p->head->next;
        free(p->head);
        p->head = next;
    }
}

status_t to_int(const long long v, int *out)
{
    if (v < INT_MIN || v > INT_MAX)
        return STATUS_OUT_OF_RANGE;
    *out = (int)v;
    return STATUS_OK;
}

status_t add_term(polynomial_t *p, const int coef, const int power)
{
    if (coef == 0)
        return STATUS_OK;

    poly_node_t **pp = &p->head;
    while (*pp && (*pp)->power > power)
        pp = &(*pp)->next;

    if (*pp && (*pp)->power == power)
    {
        int sum;
        status_t st = to_int((long long)(*pp)->coef + coef, &sum);
        if (st != STATUS_OK)
            return st;
        (*pp)->coef = sum;
        if (sum == 0)
        {
            poly_node_t *tmp = *pp;
            *pp = tmp->next;
            free(tmp);
        }
        return STATUS_OK;
    }

    poly_node_t *node = malloc(sizeof(poly_node_t));
    if (!node)
        return STATUS_MEMORY_ERROR;
    node->coef = coef;
    node->power = power;
    node->next = *pp;
    *pp = node;
    return STATUS_OK;
}

status_t parse_poly(const char *s, polynomial_t *out)
{
    if (!s || !out)
        return STATUS_NULL_POINTER;
    out->head = NULL;
    if (s[0] == '\0')
        return STATUS_INVALID_ARGUMENT;

    int i = 0;
    int len = strlen(s);
    bool first_term = true;

    while (i < len)
    {
        while (i < len && isspace((unsigned char)s[i]))
        {
            i++;
        }
        if (i >= len)
            break;

        int sign = 1;
        if (s[i] == '+')
        {
            sign = 1;
            i++;
        }
        else if (s[i] == '-')
        {
            sign = -1;
            i++;
        }
        else if (!first_term)
        {
            return STATUS_INVALID_ARGUMENT;
        }
        first_term = false;

        int coef = 1;
        bool has_coef = false;
        if (i < len && isdigit((unsigned char)s[i]))
        {
            has_coef = true;
            long long v = 0;
            while (i < len && isdigit((unsigned char)s[i]))
            {
                v = v * 10 + (s[i] - '0');
                if (v > INT_MAX)
                    return STATUS_OUT_OF_RANGE;
                i++;
            }
            coef = (int)v;
        }

        int power = 0;
        if (i < len && s[i] == 'x')
        {
            i++;
            power = 1;

            if (i < len && s[i] == '^')
            {
                i++;
                if (i >= len || !isdigit((unsigned char)s[i]))
                {
                    return STATUS_INVALID_ARGUMENT;
                }
                long long v = 0;
                while (i < len && isdigit((unsigned char)s[i]))
                {
                    v = v * 10 + (s[i] - '0');
                    if (v > INT_MAX)
                        return STATUS_OUT_OF_RANGE;
                    i++;
                }
                power = (int)v;
            }
        }
        else if (!has_coef)
        {
            return STATUS_INVALID_ARGUMENT;
        }

        status_t st = add_term(out, sign * coef, power);
        if (st != STATUS_OK)
            return st;
    }

    if (out->head == NULL)
        return STATUS_INVALID_ARGUMENT;
    return STATUS_OK;
}

status_t copy_poly(const polynomial_t *src, polynomial_t *dst)
{
    dst->head = NULL;
    for (poly_node_t *cur = src->head; cur; cur = cur->next)
    {
        status_t st = add_term(dst, cur->coef, cur->power);
        if (st != STATUS_OK)
            return st;
    }
    return STATUS_OK;
}

status_t add_poly(const polynomial_t *a, const polynomial_t *b, polynomial_t *result)
{
    status_t st = copy_poly(a, result);
    for (const poly_node_t *cur = b->head; st == STATUS_OK && cur; cur = cur->next)
        st = add_term(result, cur->coef, cur->power);
    return st;
}

status_t sub_poly(const polynomial_t *a, const polynomial_t *b, polynomial_t *result)
{
    status_t st = copy_poly(a, result);
    for (const poly_node_t *cur = b->head; st == STATUS_OK && cur; cur = cur->next)
    {
        int neg;
        st = to_int(-(long long)cur->coef, &neg);
        if (st == STATUS_OK)
            st = add_term(result, neg, cur->power);
    }
    return st;
}

status_t mult_poly(const polynomial_t *a, const polynomial_t *b, polynomial_t *result)
{
    result->head = NULL;
    for (poly_node_t *ca = a->head; ca; ca = ca->next)
    {
        for (poly_node_t *cb = b->head; cb; cb = cb->next)
        {
            int c, e;
            status_t st = to_int((long long)ca->coef * cb->coef, &c);
            if (st == STATUS_OK)
                st = to_int((long long)ca->power + cb->power, &e);
            if (st == STATUS_OK)
                st = add_term(result, c, e);
            if (st != STATUS_OK)
                return st;
        }
    }
    return STATUS_OK;
}

status_t div_poly(const polynomial_t *a, const polynomial_t *b, polynomial_t *quotient, polynomial_t *remainder)
{
    if (!b->head)
        return STATUS_INVALID_ARGUMENT;

    quotient->head = NULL;
    remainder->head = NULL;
    status_t st = copy_poly(a, remainder);
    if (st != STATUS_OK)
        return st;

    while (remainder->head && remainder->head->power >= b->head->power)
    {
        const long long rc = remainder->head->coef, bc = b->head->coef;
        if (rc % bc != 0)
            return STATUS_INVALID_ARGUMENT;
        int q_coef;
        st = to_int(rc / bc, &q_coef);
        if (st != STATUS_OK)
            return st;
        int q_power = remainder->head->power - b->head->power;

        st = add_term(quotient, q_coef, q_power);
        if (st != STATUS_OK)
            return st;

        polynomial_t term = {0};
        st = add_term(&term, q_coef, q_power);
        if (st != STATUS_OK)
        {
            free_poly(&term);
            return st;
        }

        polynomial_t product = {0};
        st = mult_poly(&term, b, &product);
        free_poly(&term);
        if (st != STATUS_OK)
        {
            free_poly(&product);
            return st;
        }

        polynomial_t new_rem = {0};
        st = sub_poly(remainder, &product, &new_rem);
        free_poly(&product);

        if (st != STATUS_OK)
        {
            free_poly(&new_rem);
            free_poly(remainder);
            return st;
        }

        free_poly(remainder);
        *remainder = new_rem;
    }
    return STATUS_OK;
}

status_t eval_poly(const polynomial_t *p, const int x, long long *result)
{
    if (!p || !result)
        return STATUS_NULL_POINTER;

    if (!p->head)
    {
        *result = 0;
        return STATUS_OK;
    }

    int max_power = p->head->power;

    long long acc = 0;
    for (int e = max_power; e >= 0; e--)
    {
        int coef = 0;
        for (const poly_node_t *cur = p->head; cur; cur = cur->next)
        {
            if (cur->power == e)
            {
                coef = cur->coef;
                break;
            }
            if (cur->power < e)
                break;
        }
        if (fabs((double)acc * (double)x) > 9.0e18)
            return STATUS_OUT_OF_RANGE;
        acc = acc * x + coef;
    }

    *result = acc;
    return STATUS_OK;
}

status_t diff_poly(const polynomial_t *p, polynomial_t *result)
{
    result->head = NULL;
    for (poly_node_t *cur = p->head; cur; cur = cur->next)
    {
        if (cur->power > 0)
        {
            int c;
            status_t st = to_int((long long)cur->coef * cur->power, &c);
            if (st == STATUS_OK)
                st = add_term(result, c, cur->power - 1);
            if (st != STATUS_OK)
                return st;
        }
    }
    return STATUS_OK;
}

status_t pow_poly(const polynomial_t *base, const int e, polynomial_t *result)
{
    result->head = NULL;
    status_t st = add_term(result, 1, 0);
    for (int i = 0; st == STATUS_OK && i < e; i++)
    {
        polynomial_t next = {0};
        st = mult_poly(result, base, &next);
        free_poly(result);
        *result = next;
    }
    return st;
}

status_t cmps_poly(const polynomial_t *outer, const polynomial_t *inner, polynomial_t *result)
{
    result->head = NULL;
    for (const poly_node_t *cur = outer->head; cur; cur = cur->next)
    {
        polynomial_t pw = {0};
        status_t st = pow_poly(inner, cur->power, &pw);
        for (const poly_node_t *t = pw.head; st == STATUS_OK && t; t = t->next)
        {
            int c;
            st = to_int((long long)t->coef * cur->coef, &c);
            if (st == STATUS_OK)
                st = add_term(result, c, t->power);
        }
        free_poly(&pw);
        if (st != STATUS_OK)
            return st;
    }
    return STATUS_OK;
}

void print_poly(const polynomial_t *p)
{
    if (!p->head)
    {
        printf("0");
        return;
    }
    int first = 1;
    for (poly_node_t *cur = p->head; cur; cur = cur->next)
    {
        if (!first && cur->coef > 0)
            printf("+");
        if (cur->power == 0)
        {
            printf("%d", cur->coef);
        }
        else if (cur->power == 1)
        {
            if (cur->coef == 1)
                printf("x");
            else if (cur->coef == -1)
                printf("-x");
            else
                printf("%dx", cur->coef);
        }
        else
        {
            if (cur->coef == 1)
                printf("x^%d", cur->power);
            else if (cur->coef == -1)
                printf("-x^%d", cur->power);
            else
                printf("%dx^%d", cur->coef, cur->power);
        }
        first = 0;
    }
}

status_t execute_instruction(char *instr, polynomial_t *adder)
{
    if (instr[0] == '\0')
        return STATUS_OK;

    char *open = strchr(instr, '(');
    const size_t len = strlen(instr);
    if (open == NULL || instr[len - 1] != ')')
        return STATUS_INVALID_ARGUMENT;

    *open = '\0';
    instr[len - 1] = '\0';

    const char *name = instr;
    char *args = open + 1;

    char *comma = strchr(args, ',');
    char *arg1 = NULL, *arg2 = NULL;

    if (comma)
    {
        *comma = '\0';
        arg1 = args;
        arg2 = comma + 1;
        while (*arg2 && isspace((unsigned char)*arg2))
            arg2++;
    }
    else
    {
        arg1 = args;
        if (arg1[0] == '\0')
            arg1 = NULL;
    }

    if (arg1)
    {
        while (*arg1 && isspace((unsigned char)*arg1))
            arg1++;
    }

    const bool is_eval = (strcmp(name, "Eval") == 0);
    const bool is_diff = (strcmp(name, "Diff") == 0);
    polynomial_t p1 = {0}, p2 = {0}, res = {0};
    long long value = 0;
    status_t st = STATUS_OK;

    if (is_diff)
    {
        if (arg2)
            st = STATUS_INVALID_ARGUMENT;
        else if (arg1)
            st = parse_poly(arg1, &p1);
        else
            st = copy_poly(adder, &p1);
    }
    else if (is_eval)
    {
        if (arg2)
        {
            st = parse_poly(arg1, &p1);
            if (st == STATUS_OK)
            {
                int x;
                st = parse_int(arg2, &x);
                if (st == STATUS_OK)
                    st = eval_poly(&p1, x, &value);
            }
        }
        else if (arg1)
        {
            st = copy_poly(adder, &p1);
            if (st == STATUS_OK)
            {
                int x;
                st = parse_int(arg1, &x);
                if (st == STATUS_OK)
                    st = eval_poly(&p1, x, &value);
            }
        }
        else
        {
            st = STATUS_INVALID_ARGUMENT;
        }
    }
    else
    {
        if (arg2)
        {
            st = parse_poly(arg1, &p1);
            if (st == STATUS_OK)
                st = parse_poly(arg2, &p2);
        }
        else if (arg1)
        {
            st = copy_poly(adder, &p1);
            if (st == STATUS_OK)
                st = parse_poly(arg1, &p2);
        }
        else
        {
            st = STATUS_INVALID_ARGUMENT;
        }
    }

    if (st == STATUS_OK && !is_eval)
    {
        if (strcmp(name, "Add") == 0)
            st = add_poly(&p1, &p2, &res);
        else if (strcmp(name, "Sub") == 0)
            st = sub_poly(&p1, &p2, &res);
        else if (strcmp(name, "Mult") == 0)
            st = mult_poly(&p1, &p2, &res);
        else if (strcmp(name, "Div") == 0)
        {
            polynomial_t rem = {0};
            st = div_poly(&p1, &p2, &res, &rem);
            free_poly(&rem);
        }
        else if (strcmp(name, "Mod") == 0)
        {
            polynomial_t quot = {0};
            st = div_poly(&p1, &p2, &quot, &res);
            free_poly(&quot);
        }
        else if (is_diff)
        {
            st = diff_poly(&p1, &res);
        }
        else if (strcmp(name, "Cmps") == 0)
        {
            st = cmps_poly(&p1, &p2, &res);
        }
        else
        {
            st = STATUS_INVALID_ARGUMENT;
        }
    }

    if (st == STATUS_OK && is_eval)
    {
        printf("%lld\n", value);
    }
    else if (st == STATUS_OK)
    {
        free_poly(adder);
        *adder = res;
        res.head = NULL;
        printf("Сумматор: ");
        print_poly(adder);
        printf("\n");
    }

    free_poly(&p1);
    free_poly(&p2);
    free_poly(&res);
    return st;
}

status_t process_file(const char *path, polynomial_t *adder)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return STATUS_FILE_OPEN_ERROR;

    char line[LINE];
    bool in_block_comment = false;
    status_t st = STATUS_OK;

    while (st == STATUS_OK && fgets(line, sizeof(line), f))
    {
        char *pct = strchr(line, '%');
        if (pct)
            *pct = '\0';

        for (int i = 0; line[i] != '\0'; i++)
        {
            if (in_block_comment)
            {
                if (line[i] == '[')
                {
                    st = STATUS_INVALID_ARGUMENT;
                    break;
                }
                if (line[i] == ']')
                {
                    in_block_comment = false;
                    line[i] = ' ';
                }
                else
                {
                    line[i] = ' ';
                }
            }
            else
            {
                if (line[i] == '[')
                {
                    in_block_comment = true;
                    line[i] = ' ';
                }
                else if (line[i] == ']')
                {
                    st = STATUS_INVALID_ARGUMENT;
                    break;
                }
            }
        }
        if (st != STATUS_OK)
            break;
        if (in_block_comment)
            continue;

        char *token = strtok(line, ";");
        while (token != NULL)
        {
            while (isspace((unsigned char)*token))
                token++;
            char *end = token + strlen(token) - 1;
            while (end > token && isspace((unsigned char)*end))
            {
                *end = '\0';
                end--;
            }

            if (strlen(token) > 0)
            {
                st = execute_instruction(token, adder);
                if (st != STATUS_OK)
                    break;
            }
            token = strtok(NULL, ";");
        }
    }

    if (st == STATUS_OK && ferror(f))
        st = STATUS_FILE_READ_ERROR;
    if (st == STATUS_OK && in_block_comment)
        st = STATUS_INVALID_ARGUMENT;

    fclose(f);
    return st;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Формат: [program] [файл]\n");
        return 1;
    }

    polynomial_t adder = {0};
    status_t st = process_file(argv[1], &adder);

    if (st != STATUS_OK)
    {
        print_error(st);
        free_poly(&adder);
        return 1;
    }

    free_poly(&adder);
    return 0;
}