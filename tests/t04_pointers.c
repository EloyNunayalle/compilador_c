#include <stdio.h>

void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int main() {
    int x = 3;
    int y = 7;
    swap(&x, &y);
    printf("%d %d\n", x, y);   // 7 3

    int *p = &x;
    *p = 100;
    printf("%d\n", x);         // 100
    return 0;
}
