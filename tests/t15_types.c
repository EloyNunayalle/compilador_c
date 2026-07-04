#include <stdio.h>
#include <stdbool.h>

// Prueba los tipos básicos de MiniC: int, char, double, bool.
struct Varied {
    int a;
    char b;
    double c;
    bool d;
};

struct Inner {
    int x;
    char y;
};
struct Outer {
    struct Inner inner;
    double z;
    bool flag;
};

int main() {
    int i = 42;
    char c = 'A';
    double d = 3.14;
    bool b = true;
    bool f = false;

    printf("%d\n", i);      // 42
    printf("%c\n", c);      // A
    printf("%d\n", (int)c); // 65
    printf("%f\n", d);      // 3.140000
    printf("%d\n", b);      // 1
    printf("%d\n", f);      // 0

    bool cmp = (i > 10);
    printf("%d\n", cmp);    // 1

    char c2 = c + 1;
    printf("%c\n", c2);     // B

    int sizeInt = sizeof(int);
    int sizeChar = sizeof(char);
    int sizeDouble = sizeof(double);
    printf("%d\n", sizeInt);    // 4
    printf("%d\n", sizeChar);   // 1
    printf("%d\n", sizeDouble); // 8

    // Struct con tipos variados
    struct Varied v;
    v.a = 123;
    v.b = 'X';
    v.c = 2.718;
    v.d = true;
    printf("%d %c %.3f %d\n", v.a, v.b, v.c, v.d); // 123 X 2.718 1

    struct Varied *pv = &v;
    pv->a = 456;
    pv->c = 1.618;
    printf("%d %.3f\n", pv->a, pv->c); // 456 1.618

    // Anidación de struct
    struct Outer o;
    o.inner.x = 99;
    o.inner.y = 'Z';
    o.z = 3.1415;
    o.flag = false;
    printf("%d %c %.4f %d\n", o.inner.x, o.inner.y, o.z, o.flag); // 99 Z 3.1415 0

    // Casting explícito entre tipos
    double cast_d = 3.14;
    int cast_i = (int)cast_d;           // double → int
    printf("%d\n", cast_i);              // 3

    char cast_c = (char)cast_i;         // int → char
    printf("%d\n", (int)cast_c);        // 3

    double cast_d2 = (double)cast_i;    // int → double
    printf("%.6f\n", cast_d2);          // 3.000000

    bool cast_b = (bool)cast_i;         // int (nonzero) → bool
    printf("%d\n", cast_b);             // 1

    int cast_zero = 0;
    bool cast_b2 = (bool)cast_zero;     // int 0 → bool
    printf("%d\n", cast_b2);            // 0

    char cast_ch = 'A';
    int cast_i2 = (int)cast_ch;         // char → int
    printf("%d\n", cast_i2);            // 65

    return 0;
}
