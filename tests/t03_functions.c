#include <stdio.h>

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int fib(int n) {
    if (n < 2) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int add(int a, int b) {
    return a + b;
}

int main() {
    printf("%d\n", factorial(5));
    printf("%d\n", fib(10));
    printf("%d\n", add(20, 22));
    return 0;
}
