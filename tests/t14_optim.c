#include <stdio.h>

int main() {
    // Expresiones con muchas constantes: el optimizador las pliega en compilación.
    int a = 2 + 3 * 4;          // -> 14
    int b = (10 - 2) * (1 + 1); // -> 16
    int c = 100 / 5 / 2;        // -> 10
    int d = a + 0;              // identidad -> a
    int e = b * 1;              // identidad -> b
    int x = 7;
    int f = x * 2;              // reducción de fuerza -> x + x
    printf("%d\n", a);   // 14
    printf("%d\n", b);   // 16
    printf("%d\n", c);   // 10
    printf("%d\n", d);   // 14
    printf("%d\n", e);   // 16
    printf("%d\n", f);   // 14
    return 0;
}
