#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* how large the underlying array size should be */
#define UNIT_SIZE 4

/* These are dedicated to UNIT_SIZE == 4 */
#define UTYPE uint32_t
#define UTYPE_TMP uint64_t
#define FORMAT_STRING "%.08x"
#define MAX_VAL ((UTYPE_TMP)0xFFFFFFFF)

#define BN_ARRAY_SIZE (4096 / UNIT_SIZE) /* size of big-numbers in bytes */

/* bn consists of array of TYPE */
struct bn {
    UTYPE array[BN_ARRAY_SIZE];
};
typedef struct bn bn_t;
static inline void bn_init(struct bn *n) {
    for (int i = 0; i < BN_ARRAY_SIZE; ++i)
        n->array[i] = 0;
}

static inline void bn_from_int(struct bn *n, UTYPE_TMP i) {
    bn_init(n);

    /* FIXME: what if machine is not little-endian? */
    n->array[0] = i;
    /* bit-shift with U64 operands to force 64-bit results */
    UTYPE_TMP tmp = i >> 32;
    n->array[1] = tmp;
}

static bool bn_is_zero(struct bn *n) {
    for (int i = 0; i < BN_ARRAY_SIZE; ++i)
        if (n->array[i])
            return false;
    return true;
}

static inline int bn_size(bn_t *a) {
    for (int i = (int)(BN_ARRAY_SIZE - 1); i >= 0; i--) {
        if (a->array[i] != 0) {
            return i + 1;
        }
    }
    return 0;
}

static void bn_to_str(struct bn *n, char *str, int nbytes) {
    /* index into array - reading "MSB" first -> big-endian */
    int j = BN_ARRAY_SIZE - 1;
    int i = 0; /* index into string representation */

    /* reading last array-element "MSB" first -> big endian */
    while ((j >= 0) && (nbytes > (i + 1))) {
        sprintf(&str[i], FORMAT_STRING, n->array[j]);
        i += (2 *
              UNIT_SIZE); /* step UNIT_SIZE hex-byte(s) forward in the string */
        j -= 1;           /* step one element back in the array */
    }

    /* Count leading zeros: */
    for (j = 0; str[j] == '0'; j++)
        ;

    /* Move string j places ahead, effectively skipping leading zeros */
    for (i = 0; i < (nbytes - j); ++i)
        str[i] = str[i + j];

    str[i] = 0;
}

/* Decrement: subtract 1 from n */
static int bn_dec(struct bn *n) {
    if (bn_is_zero(n)) {
        return 0;
    }
    for (int i = 0; i < BN_ARRAY_SIZE; ++i) {
        UTYPE tmp = n->array[i];
        UTYPE res = tmp - 1;
        n->array[i] = res;

        if (!(res > tmp)) break;
    }
    return 1;
}
// c = a + b
static int bn_add(struct bn *a, struct bn *b, struct bn *c) {
    int carry = 0;
    for (int i = 0; i < BN_ARRAY_SIZE; ++i) {
        UTYPE_TMP tmp = (UTYPE_TMP)a->array[i] + b->array[i] + carry;
        carry = (tmp > MAX_VAL);
        c->array[i] = (tmp & MAX_VAL);
    }
    if (carry == 1) {
        return 0;
    } else {
        return 1;
    }
}

static inline void lshift_unit(struct bn *a, int n_units) {
    int i;
    /* Shift whole units */
    for (i = (BN_ARRAY_SIZE - 1); i >= n_units; --i)
        a->array[i] = a->array[i - n_units];
    /* Zero pad shifted units */
    for (; i >= 0; --i)
        a->array[i] = 0;
}

static void bn_mul(struct bn *a, struct bn *b, struct bn *c) {
    struct bn row, tmp;
    bn_init(c);
    int a_size = bn_size(a);
    int b_size = bn_size(b);
    for (int i = 0; i < a_size; ++i) {
        bn_init(&row);

        for (int j = 0; j < b_size; ++j) {
            if (i + j < BN_ARRAY_SIZE) {
                bn_init(&tmp);
                UTYPE_TMP intermediate = a->array[i] * (UTYPE_TMP)b->array[j];
                bn_from_int(&tmp, intermediate);
                lshift_unit(&tmp, i + j);
                bn_add(&tmp, &row, &row);
            }
        }
        bn_add(c, &row, c);
    }
}

/* Copy src into dst. i.e. dst := src */
static void bn_assign(struct bn *dst, struct bn *src) {
    for (int i = 0; i < BN_ARRAY_SIZE; ++i)
        dst->array[i] = src->array[i];
}

/*
 * factorial(N) := N * (N-1) * (N-2) * ... * 1
 */
static void factorial(struct bn *n, struct bn *res) {
    struct bn tmp;
    bn_assign(&tmp, n);
    bn_dec(n);

    while (!bn_is_zero(n)) {
        bn_mul(&tmp, n, res); /* res = tmp * n */
        bn_dec(n);            /* n -= 1 */
        bn_assign(&tmp, res); /* tmp = res */
    }
    bn_assign(res, &tmp);
}
static bn_t *bn_tmp_copy(bn_t *n) {
    bn_t *tmp = calloc(1, sizeof(bn_t));
    for (int i = 0; i < BN_ARRAY_SIZE; ++i)
        tmp->array[i] = n->array[i];
    return tmp;
}

#define digit_div(n1, n0, d, q, r) \
    __asm__("divl %4"              \
            : "=a"(q), "=d"(r)     \
            : "0"(n0), "1"(n1), "g"(d))

#define BN_NORMALIZE(u, usize)         \
    while ((usize) && !(u)[(usize)-1]) \
        --(usize);

/* Set u[size] = u[usize] / v, and return the remainder. */
static uint32_t bn_ddivi(uint32_t *u, uint32_t size, uint32_t v) {
    assert(u != NULL);
    assert(v != 0);

    if (v == 1)
        return 0;

    // APM_NORMALIZE(u, size);
    // if (!size)
    //     return 0;

    // if ((v & (v - 1)) == 0)
    //     return apm_rshifti(u, size, apm_digit_lsb_shift(v));

    uint32_t s1 = 0;
    //uint32_t size = BN_ARRAY_SIZE;
    //uint32_t *uu = u->array;
    u += size;

    do {
        uint32_t s0 = *--u;
        //printf("s0 = %u\n", s0);
        uint32_t q, r;
        if (s1 == 0) {
            q = s0 / v;
            r = s0 % v;
        } else {
            digit_div(s1, s0, v, q, r);
        }
        *u = q;
        s1 = r;
        //printf("s1 = %u\n", s1);
    } while (--size);
    return s1;
}

static const char radix_chars[37] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

#ifndef SWAP
#define SWAP(x, y)           \
    do {                     \
        typeof(x) __tmp = x; \
        x = y;               \
        y = __tmp;           \
    } while (0)
#endif

static char *bn_get_str(bn_t *n, char *out) {
    unsigned int radix = 10;
    const uint32_t max_radix = 0x3B9ACA00U;
    const unsigned int max_power = 9;

    if (!out)
        out = calloc(256, (sizeof(char)));

    char *outp = out;
    bn_t *tmp = bn_tmp_copy(n);

    uint32_t size;
    uint32_t *tmp_u = tmp->array;
    for (int i = 32; i >= 0; i--) {
        if (tmp->array[i] != 0) {
            size = i + 1;
            break;
        }
    }

    BN_NORMALIZE(n->array, size);
    if (size == 0 || (size == 1 && tmp_u[0] < radix)) {
        if (!out)
            out = malloc(2);
        out[0] = size ? radix_chars[tmp_u[0]] : '0';
        out[1] = '\0';
        return out;
    }

    uint32_t tsize = size;
    do {
        /* Multi-precision: divide U by largest power of RADIX to fit in
            * one apm_digit and extract remainder.
            */
        uint32_t r = bn_ddivi(tmp_u, size, max_radix);
        tsize -= (tmp_u[tsize - 1] == 0);
        //printf("r = %u\n", r);
        /* Single-precision: extract K remainders from that remainder,
            * where K is the largest integer such that RADIX^K < 2^BITS.
            */
        unsigned int i = 0;
        do {
            uint32_t rq = r / radix;
            uint32_t rr = r % radix;
            *outp++ = radix_chars[rr];
            r = rq;
            if (tsize == 0 && r == 0) /* Eliminate any leading zeroes */
                break;
        } while (++i < max_power);
        assert(r == 0);
        /* Loop until TMP = 0. */
    } while (tsize != 0);
    free(tmp);
    // printf("%s\n", out);
    char *f = outp - 1;
    /* Eliminate leading (trailing) zeroes */
    while (*f == '0')
        --f;
    /* NULL terminate */
    f[1] = '\0';
    /* Reverse digits */
    for (char *s = out; s < f; ++s, --f)
        SWAP(*s, *f);

    return out;
}


static inline void bn_print(bn_t *a){
    uint32_t string_size = 512;
    char *str = calloc(1, string_size);
    char *str1 = bn_get_str(a, str);
    printf("%s\n", str1);
    free(str1);
}
static inline void bn_printhex(bn_t *a){
    char buf[8192];
    bn_to_str(a, buf, sizeof(buf));
    printf("%s\n", buf);
}

// a = b concat c
static inline void bn_split_shift(bn_t *a, bn_t *b, bn_t *c, const int offset) {
    bn_init(b);
    bn_init(c);
    for (int i = 0; i < offset; i++) {
        c->array[i] = a->array[i];
    }
    for (int i = 0; i < BN_ARRAY_SIZE - offset; i++) {
        b->array[i] = a->array[i + offset];
    }
}
// c = a - b
static inline int bn_sub(bn_t *a, bn_t *b, bn_t *c) {
    //printf("sub\n");
    UTYPE_TMP carry = 0;
    for (int i = 0; i < BN_ARRAY_SIZE; i++) {
        if (a->array[i] == 0 && carry == 1) {
            c->array[i] = MAX_VAL - (UTYPE_TMP)b->array[i] + 1;
            carry = 1;
        } else {
            if (a->array[i] - carry < b->array[i]) {
                c->array[i] = (MAX_VAL - (UTYPE_TMP)b->array[i])+ 1 + (UTYPE_TMP)a->array[i] - carry;
                carry = 1;
            } else {
                c->array[i] = (UTYPE_TMP)a->array[i] - carry - (UTYPE_TMP)b->array[i];
                carry = 0;
            }
        }
    }
    if (carry == 1) {
        return 0;
    } else {
        return 1;
    }
}
static inline void bn_left_shift(bn_t *a, int offset) {
    for (int i = (int)(BN_ARRAY_SIZE - 1); i >= offset; i--) {
        a->array[i] = a->array[i - offset];
    }
    for (int i = 0; i < offset; i++) {
        a->array[i] = 0;
    }
}
static void karatsuba_mul(bn_t *a, bn_t *b, bn_t *c) {
    int m1 = bn_size(a);
    int m2 = bn_size(b);
    bn_init(c);
    // printf("m1=%d,m2=%d\n",m1,m2);
    if (m1 <= 1 && m2 <= 1) {
        UTYPE_TMP intermediate = a->array[0] * (UTYPE_TMP)b->array[0];
        bn_from_int(c, intermediate);
        return;
    }else if(m1 == 1){
        bn_t row ,tmp;
        bn_init(&row);
        for (int j = 0; j < m2; ++j) {
            bn_init(&tmp);
            UTYPE_TMP intermediate = a->array[0] * (UTYPE_TMP)b->array[j];
            bn_from_int(&tmp, intermediate);
            lshift_unit(&tmp, j);
            bn_add(&tmp, &row, &row);
        }
        bn_add(c, &row, c);
        //bn_mul(a,b,c);
        return;
    }else if( m2 == 1){
        bn_t row ,tmp;
        bn_init(&row);
        for (int j = 0; j < m1; ++j) {
            bn_init(&tmp);
            UTYPE_TMP intermediate = a->array[j] * (UTYPE_TMP)b->array[0];
            bn_from_int(&tmp, intermediate);
            lshift_unit(&tmp, j);
            bn_add(&tmp, &row, &row);
        }
        bn_add(c, &row, c);
        //bn_mul(a,b,c);
        return;
    }else if(m1 == 0 || m2 == 0){
        return;
    }

    //min
    int m = (m1 > m2) ? m2 : m1;
    int mm = m / 2;
    bn_t high1, low1, high2, low2, z0, z1, z2, tmp1, tmp2;

    bn_init(&high1);
    bn_init(&low1);
    bn_init(&high2);
    bn_init(&low2);
    bn_init(&z0);
    bn_init(&z1);
    bn_init(&z2);
    bn_init(&tmp1);
    bn_init(&tmp2);

    bn_split_shift(a, &high1, &low1, mm);
    // bn_printhex(a);
    // bn_printhex(&high1);
    // bn_printhex(&low1);

    bn_split_shift(b, &high2, &low2, mm);
    // printf("b\n");
    // bn_printhex(b);
    // bn_printhex(&high2);
    // bn_printhex(&low2);

    //printf("shift\n");
    karatsuba_mul(&low1, &low2, &z0);
    // printf("z0=");
    // bn_printhex(&z0);
    bn_add(&low1, &high1, &tmp1);
    bn_add(&low2, &high2, &tmp2);
    karatsuba_mul(&tmp1, &tmp2, &z1);
    // printf("z1=");
    // bn_printhex(&z1);
    karatsuba_mul(&high1, &high2, &z2);
    // printf("z2=");
    // bn_printhex(&z2);
    
    //printf("sub\n");
    bn_sub(&z1, &z2, &tmp1);
    // printf("tmp1=");
    // bn_printhex(&tmp1);
    bn_sub(&tmp1, &z0, &tmp1);
    // printf("tmp1=");
    // bn_printhex(&tmp1);

    bn_left_shift(&tmp1, mm);
    bn_left_shift(&z2, 2*mm);
    bn_add(&tmp1, &z2, &tmp2);
    bn_add(&tmp2, &z0, c);

    return;
}

static void factorial2(struct bn *n, struct bn *res) {
    struct bn tmp;
    bn_assign(&tmp, n);
    bn_dec(n);

    while (!bn_is_zero(n)) {
        karatsuba_mul(&tmp, n, res); /* res = tmp * n */

        // char *str = calloc(1, 512);
        // char *a = bn_get_str(res, str);
        // printf("%s\n", a);

        bn_dec(n);            /* n -= 1 */
        bn_assign(&tmp, res); /* tmp = res */
    }
    bn_assign(res, &tmp);
}
static char* bn_str(bn_t *a){
    uint32_t string_size = 512;
    char *str = calloc(1, string_size);
    return bn_get_str(a, str);
}

// int main(int argc, char *argv[]) {
//     struct bn num, result;
//     char buf[8192];

//     unsigned int n = strtoul(argv[1], NULL, 10);
//     if (!n)
//         return -2;
//     for (int i = 1; i <= n; i++) {
//         bn_from_int(&num, i);
//         factorial2(&num, &result);
//         bn_to_str(&result, buf, sizeof(buf));
//         // printf("factorial(%d) = %s\n", i, buf);
//         uint32_t string_size = 512;
//         char *str = calloc(1, string_size);     
//         char *a = bn_get_str(&result, str);
//         printf("fac(%d) = %s\n", i, a);
//     }

//     return 0;
// }