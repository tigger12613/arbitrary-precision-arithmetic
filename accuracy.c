
#include <time.h>

#include "q6.c"

int main(int argc, char *argv[]) {

    bn_t a, b, c, d;
    bn_init(&a);
    bn_init(&b);
    bn_init(&c);
    bn_init(&d);
    bn_from_int(&a, 21451234);
    bn_from_int(&d, 234213412);
    int loop = 31;
    for (uint64_t i = 0; i < loop; i++) {
        bn_left_shift(&a,1);
        bn_assign(&b,&a);
        bn_add(&a,&d,&a);

        bn_mul(&a, &b, &c);
        bn_printhex(&c);
        
    }
    bn_from_int(&a, 21451234);

    for (uint64_t i = 0; i < loop; i++) {
        bn_left_shift(&a,1);
        bn_assign(&b,&a);
        bn_add(&a,&d,&a);

        karatsuba_mul(&a, &b, &c);
        bn_printhex(&c);
    }

    return 0;
}