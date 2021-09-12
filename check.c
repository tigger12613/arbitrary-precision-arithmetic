
#include "q6.c"


int main(int argc, char *argv[]) {
    struct bn num, result;
    char buf[8192];

    unsigned int n = strtoul(argv[1], NULL, 10);
    if (!n)
        return -2;
    for (int i = 1; i <= n; i++) {
        bn_from_int(&num, i);
        factorial2(&num, &result);
        bn_to_str(&result, buf, sizeof(buf));
        // printf("factorial(%d) = %s\n", i, buf);
        uint32_t string_size = 512;
        char *str = calloc(1, string_size);     
        char *a = bn_get_str(&result, str);
        printf("fac(%d) = %s\n", i, a);
    }

    return 0;
}