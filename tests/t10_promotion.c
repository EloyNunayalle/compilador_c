#include <stdio.h>

// Promoción automática int -> double y conversión (cast) double -> int
double half(double x) {
    return x / 2.0;
}

int main() {
    int n = 7;
    double d = n;          // promoción int -> double
    printf("%f\n", d);     // 7.000000

    double avg = (3 + 4 + 5) / 3.0;   // 3 se promueve; 12/3.0
    printf("%f\n", avg);   // 4.000000

    double h = half(9);    // 9 (int) promovido a double
    printf("%f\n", h);     // 4.500000

    int truncated = (int)3.9;   // conversión explícita (trunca)
    printf("%d\n", truncated);  // 3

    double mix = 2 + 1.5;  // int + double => double
    printf("%f\n", mix);   // 3.500000
    return 0;
}
