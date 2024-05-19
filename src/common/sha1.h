#ifndef SHA1_H
#define SHA1_H

#include <stdbool.h>
#include "int.h"

enum {
    SHA1_HASH_SIZE = 20,
};

typedef struct {
    u8 bytes[SHA1_HASH_SIZE];
}sha1_digest;

sha1_digest SHA1_buf(u8* buf, u64 len);
bool SHA1_equal(sha1_digest x, sha1_digest y);
bool SHA1_blank(sha1_digest x);
void SHA1_print(sha1_digest x);
#endif // SHA1_H