#include "libcaesar.h"
#include <string.h>

static unsigned char encryption_key = 0;

void set_key(char key) {
    encryption_key = (unsigned char)key;
}

void caesar(void* src, void* dst, int len) {
    unsigned char *src_bytes = (unsigned char*)src;
    unsigned char *dst_bytes = (unsigned char*)dst;

    for (int i = 0; i < len; i++) {
        dst_bytes[i] = src_bytes[i] ^ encryption_key;
    }
}