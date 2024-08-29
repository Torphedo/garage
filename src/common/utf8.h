#ifndef UTF8_H
#define UTF8_H
#include <stdbool.h>
#include "int.h"
// This file might get renamed to "unicode.h" later if I add UTF-16/UCS-2
// support for the vehicle strings

enum {
    UNICODE_MISSING_CHARACTER = 0xFFFD,
    UNICODE_MAX = 0x10FFFF,
};

// Checks how many bytes this UTF-8 Unicode character will be. If the byte is
// in the middle of a multi-byte character, a length of zero is returned.
// If the byte is invalid for UTF-8, a length of -1 is returned.
static s8 utf8_byte_len(u8 byte) {
    if (byte >> 7 == 0) {
        return 1;
    }
    else if (byte >> 5 == 0b110) {
        return 2;
    }
    else if (byte >> 4 == 0b1110) {
        return 3;
    }
    else if (byte >> 3 == 0b11110) {
        return 4;
    }
    else if (byte >> 6 == 0b10) {
        // Continuation byte (middle of a multi-byte character).
        return 0;
    }

    // Invalid.
    return -1;
}

// Utility function for utf8_codepoint(), probably not useful to you and might
// be removed.
static u8 utf8_starting_shift(u8 len) {
    if (len == 1) {
        return 0;
    }

    const u8 first_byte_size = (8 - (len + 1));
    const u8 continuation_size = ((len - 1) * 6);
    return first_byte_size + continuation_size;
}

// UTF-8 encoding of a single codepoint. Will usually be null-terminated, but
// don't rely on it since the character could be the whole 4 bytes.
// Recommended to use strncpy() into a UTF-8 buffer.
typedef struct {
    char data[4];
}utf8;

static utf8 codepoint_to_utf8(u32 codepoint) {
    utf8 out = {0};
    if (codepoint > UNICODE_MAX) {
        // Codepoint out of Unicode range, don't even bother
        return out;
    }

    // There's probably a fancy formula for this, but this will work fine.
    u8 bits_in_first_byte = 7;
    if (codepoint > 0x80) {
        bits_in_first_byte = 5;
    }
    if (codepoint > 0x800) {
        bits_in_first_byte = 4;
    }
    if (codepoint > 0x10000) {
        bits_in_first_byte = 3;
    }

    // Mask that only allows the first [bits_in_first_byte] bits
    const u8 first_byte_mask = 0xFF >> (8 - bits_in_first_byte);
    // Indicator for encoding length combined with however many bits we can fit
    const u8 first_byte = (0b11110 << bits_in_first_byte) | (codepoint & first_byte_mask);
    out.data[0] = first_byte;
    codepoint >>= bits_in_first_byte;

    // For every bit fewer in the first byte, 1 continuation byte is added.
    // CLAMP() ensures we don't try to do 6 - 7 and trigger SIGABORT by of
    // overwriting random things on the stack (ask me how I know!)
    const u8 num_continuation_bytes = 6 - CLAMP(0, bits_in_first_byte, 6);

    // Write up to 3 continuation bytes with 6 bits of information each
    for (u8 i = 0; i < num_continuation_bytes; i++) {
        const u8 mask = 0b111111; // Mask to get just the low 6 bits
        // All continuation bytes have "10" as the top 2 bits.
        const u8 byte = (0b10 << 6) | (codepoint & mask);
        codepoint >>= 6; // Move on to the next 6 bits
        out.data[i + 1] = byte;
    }

    return out;
}

// Given a pointer to a UTF-8 string, returns the codepoint and length.
static u32 utf8_codepoint(const char* bytes, u8* length_out) {
    const s8 len = utf8_byte_len(bytes[0]);
    *length_out = len;
    if (len <= 0) {
        *length_out = 1;
        return UNICODE_MISSING_CHARACTER;
    }
    else if (len == 1) {
        // Single-byte character, just fall back to ASCII
        return bytes[0];
    }

    u32 codepoint = 0;
    s8 cur_shift = utf8_starting_shift(len);
    for (u8 i = 0; i < len; i++) {
        u32 data = 0; // Data from this byte. Has to be u32 since we bitshift < 16 bits
        u8 data_bits = 0; // # of bits this byte contributes to the codepoint
        if (i == 0) {
            // First byte of a multi-byte character
            data = bytes[i] & (0xFF >> (len + 1));
            data_bits = (8 - (len + 1));
        }
        // Don't need to handle 1-byte characters here, they have an early exit
        else {
            // Continuation byte
            data = bytes[i] & 0b00111111;
            data_bits = 6;
        }

        // printf("0x%hhX -> %X << %d\n", bytes[i], data, cur_shift);

        // Subtract but clamp to 0
        cur_shift = MAX(0, cur_shift - data_bits);
        codepoint |= (data << cur_shift);
    }

    return codepoint;
}

#endif // UTF8_H

