#include <stdio.h>

int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int mul(int a, int b) { return a * b; }
int half(int x) { return x / 2; }

int compute(int x) { return sub(mul(x, x), x); }

int main() {
    int a = add(10, 20);
    int b = sub(50, 12);
    int c = mul(add(3, 4), sub(9, 2));
    int d = half(100);
    int e = add(mul(6, 7), sub(100, 58));
    int f = compute(9);
    int x = 10, y = 5;
    int g = sub(100, add(x, add(y, 3)));
    printf("%d\n", a);
    printf("%d\n", b);
    printf("%d\n", c);
    printf("%d\n", d);
    printf("%d\n", e);
    printf("%d\n", f);
    printf("%d\n", g);
    return 0;
}
