#include <stdio.h>

int main() {
    int m[3][3];
    int i, j;
    // llena m[i][j] = i*3 + j
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            m[i][j] = i * 3 + j;
        }
    }
    // suma la diagonal: 0 + 4 + 8 = 12
    int diag = 0;
    for (i = 0; i < 3; i++) {
        diag += m[i][i];
    }
    printf("%d\n", diag);      // 12

    // suma total: 0+1+...+8 = 36
    int total = 0;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            total += m[i][j];
        }
    }
    printf("%d\n", total);     // 36
    return 0;
}
