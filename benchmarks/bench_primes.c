#include <stdio.h>

// Cuenta primos por división de prueba: estresa bucles anidados y módulo.
int is_prime(int n) {
    if (n < 2) return 0;
    int i;
    for (i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int main() {
    int count = 0;
    int n;
    for (n = 2; n < 200000; n++) {
        count += is_prime(n);
    }
    printf("%d\n", count);   // 17984 primos < 200000
    return 0;
}
