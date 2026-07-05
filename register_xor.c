/*
 * Register bit-mismatch detector.
 *
 * Two 8-bit registers A and B are compared bit by bit. Every bit
 * position where A and B differ is set to 1 in register C; matching
 * positions are 0. This is exactly the bitwise XOR operation:
 *
 *      C = A ^ B
 *
 * XOR maps to a single machine instruction (e.g. XOR on x86,
 * EOR on ARM/AVR), so the computation is as efficient as hand-written
 * machine language -- no loops, branches, or per-bit work.
 */

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uint8_t A, B, C;
    unsigned int a_in, b_in;

    printf("Enter value of register A (0-255): ");
    if (scanf("%u", &a_in) != 1 || a_in > 0xFFu) {
        fprintf(stderr, "Invalid input for register A.\n");
        return 1;
    }

    printf("Enter value of register B (0-255): ");
    if (scanf("%u", &b_in) != 1 || b_in > 0xFFu) {
        fprintf(stderr, "Invalid input for register B.\n");
        return 1;
    }

    A = (uint8_t)a_in;
    B = (uint8_t)b_in;

    C = A ^ B;          /* single XOR instruction */

    printf("\n           b7 b6 b5 b4 b3 b2 b1 b0\n");
    printf("Register A:");
    for (int i = 7; i >= 0; i--)
        printf("  %d", (A >> i) & 1);
    printf("   (0x%02X)\n", A);

    printf("Register B:");
    for (int i = 7; i >= 0; i--)
        printf("  %d", (B >> i) & 1);
    printf("   (0x%02X)\n", B);

    printf("Register C:");
    for (int i = 7; i >= 0; i--)
        printf("  %d", (C >> i) & 1);
    printf("   (0x%02X)\n", C);

    return 0;
}
