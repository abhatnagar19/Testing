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
    /*
     * Single validity gate for the whole program. The argument-count
     * check is folded into the parse checks: `in` selects between the
     * real argv slots and a pair of empty strings — both sides of the
     * ?: are plain addresses, so it compiles to a conditional move,
     * not a branch — and empty strings then fail the nothing-parsed
     * check below. `bad` is built from pure bitwise ops:
     *   (a | b) >> 8      nonzero iff either value exceeds 255
     *   *endA | *endB     nonzero iff trailing garbage after digits
     *   endX == in[X]     nonzero iff nothing parsed (empty/bad arg,
     *                     or argc != 3 via the empty-string select)
     * One test-and-branch decides success or failure for everything.
     */
    static const char *const none[2] = { "", "" };
    const char *const *in =
        (argc == 3) ? (const char *const *)(argv + 1) : none;

    char *endA, *endB;
    unsigned long a = strtoul(in[0], &endA, 0);
    unsigned long b = strtoul(in[1], &endB, 0);

    unsigned long bad = ((a | b) >> 8)
                      | (unsigned char)(*endA | *endB)
                      | (unsigned long)(endA == in[0])
                      | (unsigned long)(endB == in[1]);

    uint8_t C = reg_mismatch((uint8_t)a, (uint8_t)b);

    /*
     * No if at all: both outcomes are prepared, and `fail` drives a
     * 2:1 mux — conditional moves select the message, its length,
     * and the stream. The flag doubles as the exit status. The
     * result line is formatted branch-free too (hex via table
     * lookup), so main's own logic contains zero conditional jumps.
     */
    char line[17];
    reg_to_binary(C, line);
    memcpy(line + 8, "  (0x", 5);
    static const char hexd[16] = "0123456789ABCDEF";
    line[13] = hexd[C >> 4];
    line[14] = hexd[C & 0x0F];
    line[15] = ')';
    line[16] = '\n';

    static const char usage[] =
        "usage: register_xor <A> <B>   (integers 0-255, decimal or 0x hex)\n";

    /* m = all-ones on failure, all-zeros on success; each select is
       (x & m) | (y & ~m) — AND/OR gates, nothing for the compiler to
       turn back into a jump. */
    int fail = (bad != 0);
    uintptr_t m = -(uintptr_t)fail;
    const char *msg = (const char *)
        (((uintptr_t)usage & m) | ((uintptr_t)line & ~m));
    size_t len  = ((sizeof usage - 1) & m) | (sizeof line & ~m);
    FILE *strm  = (FILE *)
        (((uintptr_t)stderr & m) | ((uintptr_t)stdout & ~m));
    fwrite(msg, 1, len, strm);
    return fail;
}
