#include <stdio.h>

int main() {
    int a[5];
    int i;
    for (i = 0; i < 5; i++) {
        a[i] = i * i;
    }
    int sum = 0;
    for (i = 0; i < 5; i++) {
        sum += a[i];
    }
    printf("%d\n", sum);   // 0+1+4+9+16 = 30
    return 0;
}
