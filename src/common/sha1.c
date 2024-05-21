// Modified from the sha1.c in RFC 3171.
// SHA-1 produces a 20-byte message digest for any byte stream.

#include <stdbool.h>
#include <stdio.h>

#include "endian.h"
#include "sha1.h"

#define SHA1CircularShift(bits,word) (((word) << (bits)) | ((word) >> (32-(bits))))

typedef struct SHA1Context {
    u32 intermediate_hash[SHA1_HASH_SIZE / 4]; /* Message Digest  */
    u64 length;

    u8 msg_block_idx;
    u8 msg_block[64]; // 512-bit message block

    bool computed;  // Is the digest computed?
    bool corrupted; // Is the message digest corrupted?
}SHA1Context;

enum {
    shaSuccess = 0,
    shaNull,         // Null pointer parameter
    shaInputTooLong,
    shaStateError    // You called Input after Result
};

void SHA1PadMessage(SHA1Context *);
void SHA1ProcessMessageBlock(SHA1Context *);

bool SHA1_equal(sha1_digest x, sha1_digest y) {
    return (memcmp(&x, &y, sizeof(x)) == 0);
}

bool SHA1_blank(sha1_digest x) {
    sha1_digest empty = {0};
    return (memcmp(&x, &empty, sizeof(x)) == 0);
}

void SHA1_print(sha1_digest x) {
    for (u32 i = 0; i < SHA1_HASH_SIZE; i++) {
        printf("%02x", x.bytes[i]);
    }
}

SHA1Context SHA1_Init() {
    return (SHA1Context) {
        .intermediate_hash = {
            0x67452301,
            0xEFCDAB89,
            0x98BADCFE,
            0x10325476,
            0xC3D2E1F0,
        }
    };
}

// Writes the 160-bit digest into the digest array provided by the caller.
int SHA1Result(SHA1Context* context, sha1_digest* digest) {
    if (!context || !digest) {
        return shaNull;
    }

    if (context->corrupted) {
        return shaStateError;
    }

    if (!context->computed) {
        SHA1PadMessage(context);
        // Message may be sensitive, clear it out
        memset(context->msg_block, 0x00, sizeof(context->msg_block));
        context->length = 0; // and clear length
        context->computed = true;
    }

    for(u32 i = 0; i < SHA1_HASH_SIZE; ++i) {
        digest->bytes[i] = context->intermediate_hash[i >> 2]
                >> 8 * ( 3 - ( i & 0x03 ) );
    }

    return shaSuccess;
}

// Adds an array of bytes as the next portion of the message.
int SHA1Input(SHA1Context* context, const u8* message_array, u32 length) {
    if (!length) {
        return shaSuccess;
    }

    if (!context || !message_array) {
        return shaNull;
    }

    if (context->computed) {
        context->corrupted = true;
        return shaStateError;
    }

    if (context->corrupted) {
        return context->corrupted;
    }
    while(length-- && !context->corrupted) {
        context->msg_block[context->msg_block_idx++] = *message_array;

        if (context->length >= UINT64_MAX - 8) {
            context->corrupted = true;
        }
        context->length += 8;

        if (context->msg_block_idx == 64) {
            SHA1ProcessMessageBlock(context);
        }

        message_array++;
    }

    return shaSuccess;
}

// Process the next 512 bits of the message in msg_block
void SHA1ProcessMessageBlock(SHA1Context *context) {
    const uint32_t K[] = { // Constants defined in SHA-1
            0x5A827999,
            0x6ED9EBA1,
            0x8F1BBCDC,
            0xCA62C1D6
    };
    u32 W[80];         // Word sequence
    u32 A, B, C, D, E; // Word buffers

    // Initialize the first 16 words in the array W
    for(u8 t = 0; t < 16; t++) {
        W[t] = context->msg_block[t * 4] << 24;
        W[t] |= context->msg_block[t * 4 + 1] << 16;
        W[t] |= context->msg_block[t * 4 + 2] << 8;
        W[t] |= context->msg_block[t * 4 + 3];
    }

    for(u8 t = 16; t < 80; t++) {
        W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = context->intermediate_hash[0];
    B = context->intermediate_hash[1];
    C = context->intermediate_hash[2];
    D = context->intermediate_hash[3];
    E = context->intermediate_hash[4];

    // TODO: Look into SIMD acceleration on newer CPUs. See https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sha-extensions.html
    for(u8 t = 0; t < 20; t++) {
        u32 temp =  SHA1CircularShift(5,A) +
                ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);

        B = A;
        A = temp;
    }

    for(u8 t = 20; t < 40; t++) {
        u32 temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(u8 t = 40; t < 60; t++) {
        u32 temp = SHA1CircularShift(5,A) +
               ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(u8 t = 60; t < 80; t++) {
        u32 temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    context->intermediate_hash[0] += A;
    context->intermediate_hash[1] += B;
    context->intermediate_hash[2] += C;
    context->intermediate_hash[3] += D;
    context->intermediate_hash[4] += E;

    context->msg_block_idx = 0;
}

// Pads the message to 512 bits in accordance with the standard. The last 8
// bytes store the original message's length. It will also call the
// ProcessMessageBlock() function provided appropriately. When it returns, you
// can assume the message digest has been computed.
void SHA1PadMessage(SHA1Context *context) {
    // Check to see if the current message block is too small to hold the
    // initial padding bits and length. If so, we will pad the block, process
    // it, and then continue padding into a second block.
    if (context->msg_block_idx > 55) {
        context->msg_block[context->msg_block_idx++] = 0x80;
        while(context->msg_block_idx < 64) {
            context->msg_block[context->msg_block_idx++] = 0;
        }

        SHA1ProcessMessageBlock(context);

        while(context->msg_block_idx < 56) {
            context->msg_block[context->msg_block_idx++] = 0;
        }
    }
    else {
        context->msg_block[context->msg_block_idx++] = 0x80;
        while(context->msg_block_idx < 56) {
            context->msg_block[context->msg_block_idx++] = 0;
        }
    }

    // Store the message length in the last 8 bytes
    ENDIAN_FLIP(u64, context->length);
    u64* len = (u64*)&context->msg_block[56];
    *len = context->length;
    ENDIAN_FLIP(u64, context->length);

    SHA1ProcessMessageBlock(context);
}

sha1_digest SHA1_buf(u8* buf, u64 len) {
    SHA1Context ctx = SHA1_Init();
    sha1_digest out = {0};
    SHA1Input(&ctx, buf, len);
    SHA1Result(&ctx, &out);

    return out;
}
