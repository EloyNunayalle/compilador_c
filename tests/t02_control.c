#include <stdio.h>

int main() {
    int i;
    int sum = 0;
    for (i = 1; i <= 10; i = i + 1) {
        sum = sum + i;
    }
    printf("%d\n", sum);

    int n = 7;
    if (n > 5) {
        printf("%d\n", 1);
    } else {
        printf("%d\n", 0);
    }

    int k = 0;
    while (k < 3) {
        printf("%d\n", k);
        k = k + 1;
    }

    int m = 0;
    do {
        m = m + 2;
    } while (m < 6);
    printf("%d\n", m);
    return 0;
}
