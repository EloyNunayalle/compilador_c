#include <stdio.h>

struct Point {
    int x;
    int y;
};

int main() {
    struct Point p;
    p.x = 3;
    p.y = 4;

    struct Point *pp = &p;
    pp->x = 10;          // modifica p.x vía puntero

    printf("%d %d\n", p.x, p.y);   // 10 4
    printf("%d\n", pp->y);          // 4
    return 0;
}
