// main.c
#include "overprintf.h"
#include "utils.h"
#include <stdio.h>

int main(void)
{
    printf("Демонстрация overfprintf\n\n");

    overfprintf(stdout, "1994 в римских: %Ro\n", 1994);
    overfprintf(stdout, "42 в римских: %Ro\n", 42);

    overfprintf(stdout, "\n10 в Цекендорфе: %Zr\n", 10);
    overfprintf(stdout, "100 в Цекендорфе: %Zr\n", 100);

    overfprintf(stdout, "\n255 в base 16 (нижний): %Cv\n", 255, 16);
    overfprintf(stdout, "255 в base 16 (верхний): %CV\n", 255, 16);
    overfprintf(stdout, "100 в base 2: %Cv\n", 100, 2);

    overfprintf(stdout, "\n\"ff\" из base 16: %to\n", "ff", 16);
    overfprintf(stdout, "\"FF\" из base 16: %TO\n", "FF", 16);
    overfprintf(stdout, "\"1010\" из base 2: %to\n", "1010", 2);

    int x = 1;
    overfprintf(stdout, "\nДамп int (1): %mi\n", x);

    unsigned int u = 255;
    overfprintf(stdout, "Дамп unsigned int (255): %mu\n", u);

    double d = 3.14;
    overfprintf(stdout, "Дамп double (3.14): %md\n", d);

    float f = 1.5f;
    overfprintf(stdout, "Дамп float (1.5): %mf\n", f);

    printf("\nДемонстрация oversprintf\n\n");

    char buffer[256];
    oversprintf(buffer, "Римское: %Ro, Base 16: %Cv", 42, 100, 16);
    printf("Buffer: %s\n", buffer);

    oversprintf(buffer, "Цекендорф: %Zr, Decimal: %to", 10, "ff", 16);
    printf("Buffer: %s\n", buffer);

    return 0;
}