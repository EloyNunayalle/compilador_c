#include <stdio.h>

// Multiplicación de matrices repetida: estresa arreglos 2D e índices.
int A[60][60];
int B[60][60];
int C[60][60];

int main() {
    int N = 60;
    int i, j, k, rep;
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            A[i][j] = (i + j) % 7;
            B[i][j] = (i * j) % 5;
        }
    }
    for (rep = 0; rep < 40; rep++) {
        for (i = 0; i < N; i++) {
            for (j = 0; j < N; j++) {
                int sum = 0;
                for (k = 0; k < N; k++) {
                    sum += A[i][k] * B[k][j];
                }
                C[i][j] = sum;
            }
        }
    }
    // checksum de la diagonal
    int total = 0;
    for (i = 0; i < N; i++) total += C[i][i];
    printf("%d\n", total);
    return 0;
}
