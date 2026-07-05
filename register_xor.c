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
#include <string.h>

static inline uint8_t reg_mismatch(uint8_t A, uint8_t B)
{
    return A ^ B;
}

/*
 * Render all 8 bits of r as ASCII '0'/'1' at once, MSB first —
 * branchless, loop-free, ~6 ALU instructions:
 *   1. replicate r into all 8 bytes of a 64-bit word,
 *   2. mask a different bit in each byte (byte written first tests b7),
 *   3. collapse each byte to 0/1: nonzero + 0x7F carries into bit 7,
 *   4. OR with '0' (0x30) to get ASCII.
 */
static inline void reg_to_binary(uint8_t r, char out[8])
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    const uint64_t sel = 0x8040201008040201ULL;
#else
    const uint64_t sel = 0x0102040810204080ULL;
#endif
    uint64_t t = (r * 0x0101010101010101ULL) & sel;
    t = ((t + 0x7F7F7F7F7F7F7F7FULL) >> 7) & 0x0101010101010101ULL;
    t |= 0x3030303030303030ULL;
    memcpy(out, &t, 8);
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

    char bits[8];
    reg_to_binary(C, bits);
    fwrite(bits, 1, sizeof bits, stdout);
    printf("  (0x%02X)\n", C);
    return 0;
}
