#include <time.h>

#include "q6.c"
#define NANOSEC 1e9
#define UINT32MAXVAL 0xffffffff

int bn_bit_count(bn_t *a) {
    for (int i = (int)(BN_ARRAY_SIZE - 1); i >= 0; i--) {
        if (a->array[i] != 0) {
            unsigned int b = __builtin_clz(a->array[i]);
            return (i)*32 + (32 - b);
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    struct timespec t_start, t_end;
    long long k = 500;
    const int size = 1000;
    long long normal_time[size], karatsuba_time[size];
    int normal_bits[size], karatsuba_bits[size];
    bn_t a, b, c, d;
    bn_init(&a);
    bn_init(&b);
    bn_init(&c);
    bn_init(&d);

    srand(time(NULL));

    FILE *fp1, *fp2;
    fp1 = fopen("origin.txt", "a");
    fp2 = fopen("karatsuba.txt","a");

    for (int k = 0; k < 50; k++) {
        uint32_t rand_n = rand();
        bn_from_int(&a, rand_n);
        bn_from_int(&d, rand_n);
        // printf("%d ",rand_n);
        // printf("%d\n",bn_bit_count(&a));
        int loop = 31;
        for (uint64_t i = 0; i < loop; i++) {
            
            normal_bits[i] = bn_bit_count(&a);
            clock_gettime(CLOCK_MONOTONIC, &t_start);
            for (uint64_t j = 0; j < 10; j++) {
                bn_mul(&a, &b, &c);
                //bn_printhex(&c);
            }
            clock_gettime(CLOCK_MONOTONIC, &t_end);
            normal_time[i] = (t_end.tv_sec * NANOSEC + t_end.tv_nsec) - (t_start.tv_sec * NANOSEC + t_start.tv_nsec);
            bn_left_shift(&a, 1);
            bn_assign(&b, &a);
            bn_add(&a, &d, &a);
        }
        for (int i = 0; i < loop; i++) {
            fprintf(fp1, "%d ", normal_bits[i]);
            fprintf(fp1, "%lld\n", normal_time[i]);
        }
        //printf("\n");
        bn_from_int(&a, rand_n);

        for (uint64_t i = 0; i < loop; i++) {
            
            karatsuba_bits[i] = bn_bit_count(&a);
            clock_gettime(CLOCK_MONOTONIC, &t_start);
            for (uint64_t j = 0; j < 10; j++) {
                karatsuba_mul(&a, &b, &c);
                //bn_printhex(&c);
            }
            clock_gettime(CLOCK_MONOTONIC, &t_end);
            karatsuba_time[i] = (t_end.tv_sec * NANOSEC + t_end.tv_nsec) - (t_start.tv_sec * NANOSEC + t_start.tv_nsec);
            bn_left_shift(&a, 1);
            bn_assign(&b, &a);
            bn_add(&a, &d, &a);
        }
        for (int i = 0; i < loop; i++) {
            fprintf(fp2, "%d ", karatsuba_bits[i]);
            fprintf(fp2, "%lld\n", karatsuba_time[i]);
        }
    }
    //printf("\n");
    fclose(fp1);
    fclose(fp2);
    return 0;
}
