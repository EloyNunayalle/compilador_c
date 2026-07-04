#include <stdio.h>

int add(int a, int b) { return a + b; }
int mul(int a, int b) { return a * b; }
int sub(int a, int b) { return a - b; }

// Aplica una operación pasada como puntero a función (estilo callback/lambda).
int apply(int (*op)(int, int), int x, int y) {
    return op(x, y);
}

int main() {
    int (*f)(int, int);
    f = add;
    printf("%d\n", f(3, 4));       // 7
    f = mul;
    printf("%d\n", f(3, 4));       // 12

    printf("%d\n", apply(add, 10, 5));  // 15
    printf("%d\n", apply(sub, 10, 5));  // 5
    printf("%d\n", apply(mul, 10, 5));  // 50
    return 0;
}
