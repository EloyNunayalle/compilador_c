#include <stdio.h>

int main() {
    double a = 3.5;
    double b = 5.25;

    if (a < b) {
        printf("%d\n", 1);   // 1
    } else {
        printf("%d\n", 0);
    }

    // suma acumulada en double dentro de un for
    double sum = 0.0;
    int i;
    for (i = 1; i <= 4; i++) {
        sum += i;            // int i promovido a double
    }
    printf("%f\n", sum);     // 10.000000

    double x = 1.0;
    int count = 0;
    while (x < 100.0) {
        x = x * 2.0;
        count++;
    }
    printf("%f\n", x);       // 128.000000
    printf("%d\n", count);   // 7
    return 0;
}
