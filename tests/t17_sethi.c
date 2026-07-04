#include <stdio.h>

int main() {
    int a = 1, b = 2, c = 3, d = 4, e = 5, f = 6;

    // Right-heavy: a - (b - (c - (d - (e - f))))
    int r1 = a - (b - (c - (d - (e - f))));

    // Left-heavy: ((((a - b) - c) - d) - e) - f
    int r2 = ((((a - b) - c) - d) - e) - f;

    // Balanced: (a+b)*(c-d) + (e*f)
    int r3 = (a + b) * (c - d) + (e * f);

    // Division right-heavy: 100 / (a + b + c)
    int r4 = 100 / (a + b + c);

    // Mixed: ((a - b) - (c - (d - e))) * f
    int r5 = ((a - b) - (c - (d - e))) * f;

    printf("%d\n", r1);
    printf("%d\n", r2);
    printf("%d\n", r3);
    printf("%d\n", r4);
    printf("%d\n", r5);
    return 0;
}
