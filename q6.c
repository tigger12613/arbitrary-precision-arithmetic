#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* how large the underlying array size should be */
#define UNIT_SIZE 4

/* These are dedicated to UNIT_SIZE == 4 */
#define UTYPE uint32_t
#define UTYPE_TMP uint64_t
#define FORMAT_STRING "%.08x"
#define MAX_VAL ((UTYPE_TMP)0xFFFFFFFF)

#define BN_ARRAY_SIZE (128 / UNIT_SIZE) /* size of big-numbers in bytes */

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
    if (bn_is_zero) {
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

    for (int i = 0; i < BN_ARRAY_SIZE; ++i) {
        bn_init(&row);

        for (int j = 0; j < BN_ARRAY_SIZE; ++j) {
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

int main(int argc, char *argv[]) {
    struct bn num, result;
    char buf[8192];

    unsigned int n = strtoul(argv[1], NULL, 10);
    if (!n)
        return -2;
    for (int i = 1; i <= n; i++) {
        bn_from_int(&num, i);
        factorial(&num, &result);
        bn_to_str(&result, buf, sizeof(buf));
        // printf("factorial(%d) = %s\n", i, buf);
        uint32_t string_size = 512;
        char *str = calloc(1, string_size);
        char *a = bn_get_str(&result, str);
        printf("fac(%d) = %s\n", i, a);
    }

    return 0;
}