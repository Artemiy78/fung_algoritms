#include <stdio.h>
#include "overscan.h"

int main(void)
{
    printf("=== Демонстрация oversscanf ===\n\n");

    int roman;
    if (oversscanf("MCMXCIV", "%Ro", &roman) == STATUS_OK)
    {
        printf("Римское 'MCMXCIV' = %d\n", roman);
    }

    unsigned int zeck;
    if (oversscanf("010011", "%Zr", &zeck) == STATUS_OK)
    {
        printf("Цекендорф '010011' = %u\n", zeck);
    }

    int num;
    if (oversscanf("ff", "%Cv", &num, 16) == STATUS_OK)
    {
        printf("'ff' в base 16 = %d\n", num);
    }

    if (oversscanf("FF", "%CV", &num, 16) == STATUS_OK)
    {
        printf("'FF' в base 16 = %d\n", num);
    }

    if (oversscanf("1100100", "%Cv", &num, 2) == STATUS_OK)
    {
        printf("'1100100' в base 2 = %d\n", num);
    }

    int d;
    unsigned int u;
    float f;
    char s[64];
    if (oversscanf("42 255 3.14 hello", "%d %u %f %s", &d, &u, &f, s) == STATUS_OK)
    {
        printf("Стандартные: d=%d, u=%u, f=%.2f, s=%s\n", d, u, f, s);
    }

    printf("\n=== Демонстрация overfscanf ===\n\n");

    printf("Введите римское число (например, XLII): ");
    if (overfscanf(stdin, "%Ro", &roman) == STATUS_OK)
    {
        printf("Считано: %d\n\n", roman);
    }
    else
    {
        printf("Ошибка парсинга римского числа\n\n");
    }

    printf("Введите число в base 16 (нижний регистр, например, ff): ");
    if (overfscanf(stdin, "%Cv", &num, 16) == STATUS_OK)
    {
        printf("Считано: %d\n\n", num);
    }
    else
    {
        printf("Ошибка парсинга числа\n\n");
    }

    printf("Введите строку и число: ");
    char buf[64];
    int val;
    if (overfscanf(stdin, "%s %d", buf, &val) == STATUS_OK)
    {
        printf("Строка: %s, число: %d\n", buf, val);
    }
    else
    {
        printf("Ошибка парсинга\n");
    }

    return 0;
}