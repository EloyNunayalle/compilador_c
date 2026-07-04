#include <stdio.h>

// Aproxima pi por serie de Leibniz: estresa aritmética en punto flotante (SSE).
int main() {
    double pi = 0.0;
    double sign = 1.0;
    int i;
    int N = 20000000;
    for (i = 0; i < N; i++) {
        double term = sign / (2.0 * i + 1.0);
        pi = pi + term;
        sign = -sign;
    }
    pi = pi * 4.0;
    printf("%f\n", pi);   // ~3.141592
    return 0;
}
