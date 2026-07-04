#include <stdio.h>

// Fibonacci recursivo: estresa llamadas a función y recursión.
int fib(int n) {
    if (n < 2) return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    int i;
    int total = 0;
    for (i = 0; i < 5; i++) {
        total = total + fib(30);   // fib(30)=832040
    }
    printf("%d\n", total);         // 4160200
    return 0;
}
