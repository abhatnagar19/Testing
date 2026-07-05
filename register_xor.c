/*
 * reg_mismatch — 8-bit register mismatch detector (IP block).
 *
 * Ports:   in  A[7:0], in  B[7:0], out C[7:0]
 * Logic:   C[i] = 1 where A[i] != B[i], else 0  ==>  C = A XOR B
 *
 * Compiles to a single XOR machine instruction; when inlined at a
 * call site it adds zero overhead, like instantiating a hardware
 * XOR block.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static inline uint8_t reg_mismatch(uint8_t A, uint8_t B)
{
    return A ^ B;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <A> <B>   (0-255, decimal or 0x hex)\n",
                argv[0]);
        return 1;
    }

    char *endA, *endB;
    unsigned long a = strtoul(argv[1], &endA, 0);
    unsigned long b = strtoul(argv[2], &endB, 0);
    if (*endA != '\0' || *endB != '\0' || a > 0xFFu || b > 0xFFu) {
        fprintf(stderr, "error: A and B must be integers in 0-255\n");
        return 1;
    }

    uint8_t C = reg_mismatch((uint8_t)a, (uint8_t)b);

    for (int i = 7; i >= 0; i--)
        putchar('0' + ((C >> i) & 1));
    printf("  (0x%02X)\n", C);
    return 0;
}
