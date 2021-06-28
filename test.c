#include <time.h>

#include "q6.c"
#define NANOSEC 1e9
#define UINT32MAXVAL 0xffffffff

int main(int argc, char *argv[]) {
    struct bn num, result;
    char buf[8192];
    struct timespec t_start, t_end;
    long long k = 500;
    const int size = 1000;
    long long normal_time[size], karatsuba_time[size];
    bn_t a, b, c, d;
    bn_init(&a);
    bn_init(&b);
    bn_init(&c);
    bn_init(&d);
    bn_from_int(&a, 1567579508);
    bn_from_int(&d, 1567579508);
    int loop = 31;
    for (uint64_t i = 0; i < loop; i++) {
        bn_left_shift(&a,1);
        bn_assign(&b,&a);
        bn_add(&a,&d,&a);

        clock_gettime(CLOCK_MONOTONIC, &t_start);
        for (uint64_t j = 0; j < 10; j++) {
            bn_mul(&a, &b, &c);
            //bn_printhex(&c);
        }
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        normal_time[i] = (t_end.tv_sec * NANOSEC + t_end.tv_nsec) - (t_start.tv_sec * NANOSEC + t_start.tv_nsec);
    }
    for (int i = 0; i < loop; i++) {
        printf("%lld ", normal_time[i]);
    }
    printf("\n");
    bn_from_int(&a, 1567579508);

    for (uint64_t i = 0; i < loop; i++) {
        bn_left_shift(&a,1);
        bn_assign(&b,&a);
        bn_add(&a,&d,&a);

        clock_gettime(CLOCK_MONOTONIC, &t_start);
        for (uint64_t j = 0; j < 10; j++) {
            karatsuba_mul(&a, &b, &c);
            //bn_printhex(&c);
        }
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        karatsuba_time[i] = (t_end.tv_sec * NANOSEC + t_end.tv_nsec) - (t_start.tv_sec * NANOSEC + t_start.tv_nsec);
    }
    for (int i = 0; i < loop; i++) {
        printf("%lld ", karatsuba_time[i]);
    }
    printf("\n");

    return 0;
}
