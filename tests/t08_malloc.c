#include <stdio.h>
#include <stdlib.h>

int main() {
    int n = 5;
    int *arr = malloc(n * sizeof(int));
    int i;
    for (i = 0; i < n; i++) {
        arr[i] = i * 10;
    }
    int sum = 0;
    for (i = 0; i < n; i++) {
        sum += arr[i];
    }
    printf("%d\n", sum);   // 0+10+20+30+40 = 100
    free(arr);
    return 0;
}
