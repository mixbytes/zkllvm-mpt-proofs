#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

//////////////////////////////////////////////////////////////////////////////
/// KECCAK
//////////////////////////////////////////////////////////////////////////////

#define sha3_max_permutation_size 25
#define sha3_max_rate_in_qwords 24

typedef struct SHA3_CTX {
    /* 1600 bits algorithm hashing state */
    uint64_t hash[sha3_max_permutation_size];
    /* 1536-bit buffer for leftovers */
    uint64_t message[sha3_max_rate_in_qwords];
    /* count of bytes in the message[] buffer */
    uint16_t rest;
    /* size of a message block processed at once */
    //unsigned block_size;
} SHA3_CTX;

SHA3_CTX ctx;

#define BLOCK_SIZE     ((1600 - 256 * 2) / 8)
#define I64(x) x##LL
#define ROTL64(qword, n) ((qword) << (n) ^ ((qword) >> (64 - (n))))
#define le2me_64(x) (x)
#define IS_ALIGNED_64(p) (0 == (7 & ((const char*)(p) - (const char*)0)))
#define me64_to_le_str(to, from, length) memcpy((to), (from), (length))
const uint8_t constants[]  = {
    1, 26, 94, 112, 31, 33, 121, 85, 14, 12, 53, 38, 63, 79, 93, 83, 82, 72, 22, 102, 121, 88, 33, 116,
    1, 6, 9, 22, 14, 20, 2, 12, 13, 19, 23, 15, 4, 24, 21, 8, 16, 5, 3, 18, 17, 11, 7, 10,
    1, 62, 28, 27, 36, 44, 6, 55, 20, 3, 10, 43, 25, 39, 41, 45, 15, 21, 8, 18, 2, 61, 56, 14,
};
#define TYPE_ROUND_INFO      0
#define TYPE_PI_TRANSFORM   24
#define TYPE_RHO_TRANSFORM  48

uint8_t getConstant(uint8_t type, uint8_t index) {
    return constants[type + index];
}

static uint64_t get_round_constant(uint8_t round) {
    uint64_t result = 0;
        uint8_t roundInfo = getConstant(TYPE_ROUND_INFO, round);
    if (roundInfo & (1 << 6)) { result |= ((uint64_t)1 << 63); }
    if (roundInfo & (1 << 5)) { result |= ((uint64_t)1 << 31); }
    if (roundInfo & (1 << 4)) { result |= ((uint64_t)1 << 15); }
    if (roundInfo & (1 << 3)) { result |= ((uint64_t)1 << 7); }
    if (roundInfo & (1 << 2)) { result |= ((uint64_t)1 << 3); }
    if (roundInfo & (1 << 1)) { result |= ((uint64_t)1 << 1); }
    if (roundInfo & (1 << 0)) { result |= ((uint64_t)1 << 0); }
        return result;
}
unsigned char keccak_msg[1024];
unsigned char keccak_hash[32];

static void keccak_theta() {
    uint64_t C[5], D[5];
    for (uint8_t i = 0; i < 5; i++) {
        C[i] = ctx.hash[i];
        for (uint8_t j = 5; j < 25; j += 5) {
            C[i] ^= ctx.hash[i + j];
        }
    }
    for (uint8_t i = 0; i < 5; i++) {
        D[i] = ROTL64(C[(i + 1) % 5], 1) ^ C[(i + 4) % 5];
    }
    for (uint8_t i = 0; i < 5; i++) {
        for (uint8_t j = 0; j < 25; j += 5) {
            ctx.hash[i + j] ^= D[i];
        }
    }
}

static void keccak_pi() {
    uint64_t A1 = ctx.hash[1];
    for (uint8_t i = 1; i < 24; i++) {
        ctx.hash[getConstant(TYPE_PI_TRANSFORM, i - 1)] = ctx.hash[getConstant(TYPE_PI_TRANSFORM, i)];
    }
    ctx.hash[10] = A1;
}

static void keccak_chi() {
    for (uint8_t i = 0; i < 25; i += 5) {
        uint64_t A0 = ctx.hash[0 + i], A1 = ctx.hash[1 + i];
        ctx.hash[0 + i] ^= ~A1 & ctx.hash[2 + i];
        ctx.hash[1 + i] ^= ~ctx.hash[2 + i] & ctx.hash[3 + i];
        ctx.hash[2 + i] ^= ~ctx.hash[3 + i] & ctx.hash[4 + i];
        ctx.hash[3 + i] ^= ~ctx.hash[4 + i] & A0;
        ctx.hash[4 + i] ^= ~A0 & A1;
    }
}

static void sha3_permutation() {
    for (uint8_t round = 0; round < 24; round++) {
        keccak_theta(ctx.hash);
        
        for (uint8_t i = 1; i < 25; i++) {
            ctx.hash[i] = ROTL64(ctx.hash[i], getConstant(TYPE_RHO_TRANSFORM, i - 1));
        }
        keccak_pi(ctx.hash);
        keccak_chi();
        
        *ctx.hash ^= get_round_constant(round);
    }
}

void keccak_update(uint16_t size) {
    uint16_t idx = (uint16_t)ctx.rest;
    ctx.rest = (unsigned)((ctx.rest + size) % BLOCK_SIZE);
    uint16_t msgIndex = 0; 

    if (idx) {
        uint16_t left = BLOCK_SIZE - idx;
        for (uint16_t k = 0; k < (size < left ? size : left); k++) {
            ((char*)ctx.message)[idx + k] = keccak_msg[msgIndex + k];
        }
        
        if (size < left) return;
        
        for (uint8_t i = 0; i < 17; i++) {
            ctx.hash[i] ^= le2me_64(((uint64_t*)ctx.message)[i]);
        }
        sha3_permutation();
        msgIndex += left;
        size -= left;
    }

    while (size >= BLOCK_SIZE) {
        uint64_t* aligned_message_block;

        if (IS_ALIGNED_64(&keccak_msg[msgIndex])) {

            for (uint8_t i = 0; i < 17; i++) {
                ctx.hash[i] ^= le2me_64(((uint64_t*)(void*)&keccak_msg[msgIndex])[i]);
            }
            sha3_permutation();

        } else {
            for (uint16_t k = 0; k < BLOCK_SIZE; k++) {
                ((char*)ctx.message)[k] = keccak_msg[msgIndex + k];
            }
            for (uint8_t i = 0; i < 17; i++) {
                ctx.hash[i] ^= le2me_64(((uint64_t*)ctx.message)[i]);
            }
            sha3_permutation();
        }

        msgIndex += BLOCK_SIZE;
        size -= BLOCK_SIZE;
    }

    if (size) {
        for (uint16_t k = 0; k < size; k++) {
            ((char*)ctx.message)[k] = keccak_msg[msgIndex + k];
        }
    }
}

void keccak_final() {
    uint16_t digest_length = 100 - BLOCK_SIZE / 2;
    
    memset((char*)ctx.message + ctx.rest, 0, BLOCK_SIZE - ctx.rest);
    ((char*)ctx.message)[ctx.rest] |= 0x01;
    ((char*)ctx.message)[BLOCK_SIZE - 1] |= 0x80;

    for (uint8_t i = 0; i < 17; i++) {
        ctx.hash[i] ^= le2me_64(((uint64_t*)ctx.message)[i]);
    }
    sha3_permutation();
    
    me64_to_le_str(keccak_hash, ctx.hash, digest_length); 
}

//////////////////////////////////////////////////////////////////////////////
/// PROOF VALIDATION
//////////////////////////////////////////////////////////////////////////////


#define MAX_STRINGS 32

unsigned char storage_hash[] = {0x7b, 0x21, 0x70, 0xeb, 0x13, 0x42, 0x08, 0x2d, 0x77, 0x52, 0xa9, 0x67, 0x0e, 0xdb, 0x1c, 0x3f, 0x18, 0xbf, 0x23, 0xb8, 0xaf, 0x1d, 0x23, 0x13, 0x6a, 0x46, 0x27, 0xfe, 0xa3, 0x40, 0xcf, 0x0f};
unsigned char key_hash[] = {0x29, 0x0d, 0xec, 0xd9, 0x54, 0x8b, 0x62, 0xa8, 0xd6, 0x03, 0x45, 0xa9, 0x88, 0x38, 0x6f, 0xc8, 0x4b, 0xa6, 0xbc, 0x95, 0x48, 0x40, 0x08, 0xf6, 0x36, 0x2f, 0x93, 0x16, 0x0e, 0xf3, 0xe5, 0x63};
unsigned char proof0[] = {0xf9, 0x02, 0x11, 0xa0, 0xf4, 0xc5, 0xc0, 0xc8, 0x6a, 0xed, 0x65, 0xa5, 0xeb, 0x36, 0x1f, 0x68, 0x74, 0x74, 0xb7, 0x13, 0x4d, 0xf3, 0x41, 0xbd, 0x55, 0x35, 0xda, 0x26, 0x51, 0x27, 0xf9, 0x41, 0x52, 0xb9, 0xc0, 0xf9, 0xa0, 0xb1, 0x5d, 0x7d, 0xd0, 0xb8, 0x5c, 0x22, 0x7c, 0x0e, 0x2a, 0x16, 0x57, 0x69, 0x71, 0xe2, 0xbf, 0x91, 0x50, 0xcd, 0x30, 0x5e, 0x44, 0xd7, 0xad, 0xd7, 0xac, 0x52, 0xc6, 0xf1, 0x50, 0x53, 0x05, 0xa0, 0xcc, 0xb5, 0xc6, 0xe6, 0xc6, 0xbe, 0x75, 0x42, 0xcb, 0x57, 0x68, 0xed, 0x51, 0x3e, 0xe2, 0x96, 0x88, 0xe6, 0x44, 0x8a, 0xb0, 0x47, 0x1f, 0x7b, 0xee, 0xc0, 0xe4, 0x5b, 0x15, 0x63, 0x50, 0x83, 0xa0, 0x2d, 0x75, 0x63, 0x25, 0x77, 0x9c, 0xe7, 0x15, 0xd5, 0xcf, 0xca, 0xcf, 0xd9, 0xed, 0xb9, 0x6d, 0x5d, 0xc0, 0x03, 0x6b, 0x9d, 0xb0, 0x3d, 0xd5, 0x7d, 0x4e, 0x23, 0x84, 0x5a, 0xe0, 0x71, 0xf5, 0xa0, 0x7c, 0xfd, 0x68, 0xc3, 0x26, 0xcb, 0xb2, 0xd9, 0xcc, 0xf3, 0xac, 0x07, 0x26, 0x72, 0x0c, 0x2c, 0x6e, 0x06, 0x55, 0x40, 0xf5, 0x85, 0xf2, 0xd4, 0x7c, 0x7d, 0x1b, 0xfe, 0x2f, 0x29, 0xec, 0xd5, 0xa0, 0x67, 0x6c, 0x8d, 0x44, 0x9b, 0xbf, 0x42, 0xc9, 0x28, 0xfe, 0xea, 0xef, 0xd6, 0x8d, 0xf2, 0xbc, 0x51, 0x09, 0x40, 0x1a, 0x12, 0xb8, 0xa3, 0x14, 0x6e, 0xb3, 0x76, 0x28, 0xc7, 0x13, 0x1b, 0x7a, 0xa0, 0x62, 0x12, 0xa6, 0x04, 0xab, 0x80, 0x80, 0xff, 0x63, 0x35, 0x24, 0xc4, 0x7c, 0xa1, 0x3b, 0xb8, 0x2d, 0x52, 0x38, 0x25, 0x81, 0x15, 0xf6, 0x92, 0x94, 0x74, 0xb9, 0xcd, 0x72, 0x40, 0x5f, 0x08, 0xa0, 0xbf, 0x7b, 0x2b, 0x79, 0x82, 0x54, 0xef, 0x5e, 0xe1, 0x17, 0x8a, 0xcd, 0x28, 0xb0, 0x50, 0x23, 0xf7, 0x7b, 0xed, 0xd9, 0x79, 0xa0, 0x77, 0x3e, 0x31, 0x25, 0x46, 0x04, 0x8e, 0x77, 0x5d, 0xfb, 0xa0, 0x6e, 0x2a, 0x64, 0x4c, 0xc7, 0x7b, 0xd5, 0x46, 0xcd, 0x87, 0x8d, 0x20, 0xf6, 0x52, 0x89, 0x33, 0xe8, 0xd9, 0xc6, 0xed, 0xf9, 0x94, 0x4d, 0x29, 0x6d, 0x18, 0x1e, 0x56, 0xc0, 0x01, 0x32, 0x85, 0xa0, 0xdc, 0x31, 0xd8, 0x9e, 0xb4, 0x0c, 0x00, 0x4b, 0x2a, 0x1f, 0xd0, 0x70, 0x46, 0x99, 0xdb, 0x24, 0xf0, 0x86, 0x0c, 0x29, 0x55, 0xb8, 0x66, 0xbb, 0x54, 0x9e, 0x6c, 0x20, 0xdd, 0x23, 0xb5, 0x03, 0xa0, 0x39, 0xee, 0xfb, 0x67, 0x8f, 0xb3, 0xb7, 0x01, 0xb3, 0x6e, 0x11, 0x64, 0xc8, 0xa4, 0x1d, 0x2c, 0xe2, 0xb2, 0x55, 0xe3, 0xca, 0x72, 0xfa, 0x61, 0xf8, 0xa3, 0x33, 0xfa, 0xaf, 0x6e, 0xcb, 0x79, 0xa0, 0x46, 0x90, 0x27, 0x24, 0x14, 0xba, 0xf9, 0x06, 0x82, 0x7c, 0x27, 0x76, 0xb3, 0x10, 0x31, 0x77, 0x91, 0xc7, 0xad, 0x60, 0xd8, 0x5e, 0xda, 0xc8, 0x2e, 0x35, 0xc0, 0xfe, 0x75, 0x73, 0xae, 0x1e, 0xa0, 0xed, 0xef, 0x95, 0x06, 0x72, 0x58, 0xd1, 0x70, 0x64, 0xa9, 0xa0, 0xda, 0xcc, 0xc2, 0x1e, 0x17, 0xfa, 0x61, 0x9c, 0xbb, 0x9d, 0x7b, 0x3b, 0xa6, 0xb7, 0xc0, 0x95, 0x58, 0xfd, 0x62, 0x35, 0x35, 0xa0, 0x6b, 0xf6, 0x18, 0x70, 0xe5, 0x08, 0x0b, 0xc2, 0xa2, 0x76, 0x3c, 0x39, 0x34, 0x11, 0x9b, 0x39, 0xc5, 0x8b, 0x5d, 0xcb, 0x53, 0x25, 0x22, 0x32, 0x6c, 0x4c, 0xfe, 0x9b, 0x34, 0x01, 0x96, 0x56, 0xa0, 0xa6, 0xfb, 0x36, 0xe4, 0xec, 0x57, 0x56, 0x15, 0x0c, 0x41, 0xa3, 0xbf, 0x69, 0x55, 0x00, 0x3a, 0xa4, 0xdb, 0x46, 0xd9, 0x73, 0x38, 0x95, 0x08, 0x47, 0x54, 0x29, 0x92, 0x14, 0x1c, 0x78, 0xce, 0xa0, 0xf1, 0x57, 0x13, 0xc1, 0xac, 0x1d, 0x53, 0x3f, 0x18, 0x8e, 0xe9, 0xf5, 0xc2, 0x7f, 0x48, 0x9e, 0x16, 0x76, 0x7e, 0x47, 0x05, 0x94, 0x2f, 0xd0, 0xe8, 0xc0, 0xba, 0x67, 0x82, 0xb0, 0xf2, 0x8b, 0x80};
unsigned char proof1[] = {0xf9, 0x02, 0x11, 0xa0, 0x7f, 0xf1, 0xad, 0x5d, 0x6a, 0x43, 0xbd, 0x41, 0x4f, 0xbd, 0x35, 0xdc, 0x03, 0x0b, 0x19, 0x73, 0x9f, 0xc7, 0xce, 0x61, 0xf7, 0x06, 0x0e, 0x61, 0xd9, 0xd2, 0x54, 0x61, 0xe3, 0xca, 0xd1, 0x31, 0xa0, 0x9f, 0xe9, 0x6e, 0xea, 0xf1, 0x6b, 0xf4, 0xc8, 0x2e, 0x46, 0xc8, 0x5d, 0x91, 0x92, 0xa7, 0xd3, 0xd9, 0xb0, 0x1b, 0xf8, 0xd4, 0xd0, 0xb4, 0xef, 0xed, 0xf1, 0x4d, 0xc1, 0x84, 0x29, 0x26, 0xdd, 0xa0, 0xe4, 0x86, 0xbf, 0xe5, 0x2e, 0x93, 0xde, 0x01, 0x80, 0x05, 0x12, 0x47, 0x31, 0xac, 0x8c, 0x58, 0xe5, 0xb5, 0x35, 0xd4, 0x90, 0x78, 0xfa, 0xc5, 0xb7, 0x15, 0x8d, 0x62, 0xb8, 0x04, 0x95, 0x43, 0xa0, 0xb9, 0x61, 0xb2, 0xa7, 0xa3, 0xd1, 0x43, 0xdd, 0x4d, 0xe2, 0xba, 0xce, 0xaf, 0x38, 0x47, 0xc0, 0x8c, 0x0d, 0x4d, 0xa5, 0xc0, 0x96, 0x8c, 0xe9, 0xcc, 0x3f, 0x95, 0xb5, 0xa6, 0x7b, 0x0a, 0xeb, 0xa0, 0xa9, 0x81, 0xd5, 0x36, 0x42, 0x0b, 0x09, 0xca, 0x2b, 0xd1, 0x3d, 0xda, 0xba, 0x19, 0x8f, 0x7c, 0xf9, 0x6d, 0x29, 0xff, 0xfa, 0x44, 0x8e, 0x78, 0x8f, 0xdf, 0x05, 0xec, 0x81, 0x43, 0x9d, 0xd6, 0xa0, 0x87, 0x87, 0x43, 0x0e, 0x97, 0xb5, 0x6b, 0xfb, 0x38, 0x28, 0x6f, 0xfe, 0xc9, 0x5f, 0x98, 0xc2, 0x6f, 0x01, 0x0a, 0x82, 0xce, 0xeb, 0x35, 0x43, 0x6a, 0xe3, 0x80, 0x2c, 0xf9, 0xe6, 0xe4, 0xf6, 0xa0, 0x5a, 0x01, 0x87, 0x2d, 0x0f, 0x51, 0x41, 0x7a, 0xd6, 0x39, 0x5d, 0xcc, 0xad, 0xcd, 0x13, 0x80, 0xb4, 0xfc, 0xd9, 0x23, 0x74, 0xfe, 0xf3, 0x17, 0x4d, 0x06, 0x1d, 0xe4, 0xf0, 0x98, 0xc7, 0x38, 0xa0, 0x8b, 0xb5, 0x03, 0x9f, 0x2e, 0xc6, 0xdd, 0x49, 0xcc, 0xe2, 0x60, 0x1d, 0x35, 0x7b, 0x9e, 0x44, 0x97, 0x4b, 0x9f, 0xcf, 0xad, 0xea, 0xf5, 0xee, 0xb3, 0x0f, 0x4d, 0x2c, 0x16, 0x4d, 0xf5, 0xa7, 0xa0, 0x52, 0x19, 0x69, 0xc1, 0xa1, 0x4f, 0x57, 0x4b, 0xb2, 0xd2, 0x4f, 0xe0, 0x3b, 0xb6, 0x93, 0xd0, 0x4f, 0xa2, 0xe0, 0x31, 0xb7, 0x95, 0xb1, 0xe1, 0x7b, 0xc8, 0xfd, 0xe5, 0x1a, 0x29, 0xb7, 0xc7, 0xa0, 0x5f, 0xcf, 0xe3, 0x89, 0x59, 0x8c, 0x70, 0x4d, 0x9a, 0x79, 0xd7, 0xe2, 0x74, 0x7e, 0x80, 0xa5, 0x36, 0xd4, 0xac, 0x6b, 0x94, 0x28, 0xb8, 0x42, 0xd6, 0x2f, 0x01, 0x17, 0x04, 0xe5, 0xd5, 0xb3, 0xa0, 0x4d, 0xbe, 0x64, 0xd8, 0x16, 0x60, 0x61, 0x84, 0xf3, 0xcd, 0x89, 0xaf, 0x4b, 0x1c, 0x51, 0x3b, 0x38, 0x95, 0x74, 0x63, 0x51, 0x9a, 0x42, 0x97, 0x5b, 0x47, 0x32, 0x53, 0x0f, 0x99, 0xb6, 0xf9, 0xa0, 0x11, 0xdd, 0x52, 0x77, 0xca, 0x38, 0x90, 0x0f, 0x91, 0x55, 0xc5, 0x5b, 0xda, 0x46, 0x57, 0x2b, 0x93, 0xae, 0xfd, 0x2c, 0xbd, 0x28, 0x2a, 0x6d, 0xe8, 0xd7, 0x96, 0x78, 0x6c, 0xf7, 0x1b, 0x0d, 0xa0, 0x3f, 0x8d, 0x42, 0x7e, 0xf8, 0xd7, 0x22, 0xbe, 0x5c, 0xc7, 0xf7, 0x3c, 0x1e, 0x1e, 0xd6, 0xce, 0xb3, 0x7e, 0x13, 0x1b, 0x0a, 0x4d, 0x61, 0x0c, 0x2f, 0x37, 0x06, 0x34, 0xed, 0xa8, 0x20, 0xfa, 0xa0, 0x6d, 0xa1, 0xa8, 0xb4, 0x40, 0xf2, 0xa0, 0x2e, 0x82, 0x35, 0x3a, 0x83, 0xed, 0xf1, 0xf2, 0xb7, 0xb8, 0xfc, 0xed, 0x70, 0x3f, 0xde, 0x44, 0x21, 0xd8, 0x20, 0x3a, 0x3d, 0xed, 0xc6, 0x3b, 0xd7, 0xa0, 0x46, 0xf0, 0xf0, 0xcc, 0x10, 0x31, 0x7e, 0x99, 0x12, 0x41, 0x5f, 0x3f, 0x63, 0x32, 0xeb, 0xca, 0x49, 0x6a, 0x0d, 0xcf, 0x57, 0xbe, 0x0b, 0x38, 0xb3, 0xa4, 0x66, 0x83, 0x45, 0x50, 0xc6, 0xd1, 0xa0, 0x9b, 0xcb, 0x37, 0xde, 0x0e, 0xb0, 0x91, 0x60, 0xa1, 0x05, 0x2e, 0xc8, 0xc2, 0x8d, 0xe5, 0x1f, 0x69, 0x59, 0xec, 0x78, 0xba, 0xf3, 0x25, 0xd1, 0x98, 0x43, 0x94, 0x1f, 0xb8, 0x2f, 0x0d, 0x96, 0x80};
unsigned char proof2[] = {0xf9, 0x02, 0x11, 0xa0, 0x6b, 0xc8, 0x7a, 0x42, 0xd3, 0x0e, 0x98, 0xf0, 0xf4, 0x98, 0x4f, 0x9e, 0xcc, 0x9a, 0xa8, 0x32, 0x2b, 0x99, 0x9f, 0x4e, 0x50, 0x90, 0x14, 0x45, 0xfe, 0x91, 0x95, 0x05, 0x0e, 0x7e, 0x1e, 0x17, 0xa0, 0xd3, 0x05, 0x67, 0x92, 0x7d, 0x08, 0xea, 0x12, 0x9e, 0xb5, 0x88, 0xe3, 0x8a, 0x58, 0x86, 0x4c, 0x75, 0xb1, 0x31, 0xf7, 0x29, 0x3e, 0x89, 0xd7, 0xfa, 0x9e, 0xc2, 0x39, 0xbc, 0xc0, 0x50, 0xa4, 0xa0, 0x32, 0x86, 0xe4, 0x97, 0x9f, 0xc4, 0xf2, 0xa0, 0x37, 0x7e, 0x3b, 0xb4, 0xd7, 0xa1, 0x54, 0x74, 0x06, 0x9a, 0x13, 0x45, 0x60, 0x1a, 0xa1, 0xdf, 0xa6, 0x90, 0x3a, 0x71, 0xd5, 0xe3, 0xc7, 0xf4, 0xa0, 0xb9, 0x4d, 0x1a, 0x0b, 0x44, 0xfe, 0xde, 0x2c, 0x48, 0xf8, 0x75, 0x2c, 0x3b, 0x5d, 0x78, 0x10, 0xa0, 0x66, 0xdd, 0x2d, 0x0e, 0x96, 0xf3, 0xac, 0x9d, 0xfe, 0x1d, 0x5b, 0xf4, 0x26, 0x5d, 0x00, 0xa0, 0x5e, 0xa7, 0x79, 0x4b, 0x5c, 0x64, 0x14, 0x24, 0x18, 0x9d, 0x0f, 0xca, 0xb8, 0x97, 0x92, 0x21, 0xc2, 0xf5, 0x53, 0xaf, 0x56, 0x9c, 0xdd, 0x02, 0xc7, 0x66, 0xef, 0x11, 0x2d, 0x71, 0x87, 0x81, 0xa0, 0x37, 0x05, 0x50, 0x86, 0x9d, 0xe2, 0x81, 0x8b, 0xf1, 0x9d, 0x20, 0xce, 0xfb, 0x74, 0xdf, 0x7d, 0x0e, 0x88, 0x16, 0x4b, 0x03, 0x2d, 0x57, 0xce, 0xc7, 0x82, 0xda, 0xf4, 0xbe, 0xa3, 0x30, 0x58, 0xa0, 0x3d, 0x00, 0x71, 0xe8, 0xb6, 0x53, 0x39, 0x45, 0xc9, 0x41, 0xcf, 0xd5, 0x95, 0x56, 0xf1, 0x89, 0x52, 0x11, 0x02, 0xfd, 0x70, 0x88, 0x70, 0xee, 0x35, 0xc6, 0xbe, 0x5a, 0x6f, 0xaa, 0x8c, 0x8a, 0xa0, 0x4e, 0xf3, 0x05, 0xc8, 0xf3, 0x37, 0x0f, 0x24, 0xaa, 0x34, 0xc0, 0x6c, 0x25, 0xb6, 0x60, 0x14, 0x69, 0x46, 0xc3, 0xd1, 0xd2, 0x38, 0x95, 0x1a, 0x53, 0xd7, 0x34, 0x54, 0x72, 0xa3, 0xaf, 0xc0, 0xa0, 0x66, 0x9c, 0x57, 0x6f, 0x48, 0xb2, 0xa5, 0x81, 0x50, 0xef, 0x0e, 0x78, 0xf9, 0x15, 0x70, 0x3b, 0x29, 0x65, 0x11, 0x43, 0xf6, 0x73, 0xab, 0x63, 0xc0, 0x5d, 0xdb, 0xa8, 0xc8, 0x9a, 0x62, 0x69, 0xa0, 0x0b, 0xb3, 0xc7, 0xe8, 0x1a, 0xc3, 0x12, 0x93, 0xbf, 0x19, 0x1c, 0xcf, 0x3e, 0x57, 0xd2, 0x02, 0x4f, 0xb5, 0x9d, 0xd4, 0x9a, 0x6d, 0x99, 0x0d, 0x72, 0x6a, 0x14, 0x2c, 0x42, 0x3d, 0x65, 0x90, 0xa0, 0x27, 0x06, 0x60, 0x95, 0x5e, 0x6e, 0xf6, 0x73, 0x01, 0x96, 0x08, 0x00, 0xda, 0x56, 0x90, 0x81, 0x23, 0x32, 0x1e, 0x6a, 0x91, 0xf1, 0xea, 0xf1, 0x4f, 0x10, 0x3d, 0x6a, 0x99, 0x66, 0x64, 0x2e, 0xa0, 0x5f, 0x51, 0x2b, 0x96, 0xe2, 0x98, 0xb7, 0x49, 0x6a, 0xb3, 0xcb, 0xd9, 0x4a, 0x2e, 0x6c, 0x89, 0xb3, 0x95, 0xcb, 0x48, 0x19, 0x7a, 0xb3, 0xc1, 0x78, 0xe2, 0x59, 0xbe, 0x82, 0xa1, 0xe8, 0xd6, 0xa0, 0x29, 0xf5, 0x19, 0x9e, 0x17, 0x0b, 0x94, 0xfe, 0xc7, 0x7a, 0x12, 0xa3, 0xfb, 0xcf, 0xb1, 0x49, 0x98, 0x21, 0x75, 0xb3, 0xc9, 0x6f, 0x06, 0xc3, 0xa9, 0x76, 0xaf, 0x30, 0xe6, 0xbf, 0x2f, 0xa0, 0xa0, 0xd1, 0x53, 0x96, 0x09, 0x17, 0x78, 0xff, 0x22, 0x35, 0xbb, 0x61, 0x5f, 0x69, 0xc6, 0x61, 0x34, 0x08, 0xf6, 0x66, 0x90, 0x40, 0xec, 0xbe, 0xbc, 0x95, 0x34, 0x33, 0xea, 0xe2, 0xea, 0x35, 0x1b, 0xa0, 0x95, 0x71, 0xbb, 0xbb, 0x82, 0x55, 0x0f, 0xfd, 0xd4, 0xd9, 0x50, 0x8a, 0xa7, 0xb0, 0x23, 0xbb, 0xfd, 0x45, 0x2f, 0xb0, 0x17, 0x3d, 0x63, 0xfe, 0x89, 0xd1, 0x02, 0x70, 0x3a, 0xd7, 0xdb, 0x5b, 0xa0, 0xcb, 0xe3, 0xca, 0xb8, 0x5e, 0x12, 0x08, 0xb5, 0xff, 0x3d, 0xec, 0xf2, 0x89, 0x3d, 0xbf, 0x6c, 0x73, 0xf7, 0x70, 0x42, 0x75, 0x70, 0x7a, 0xce, 0xe7, 0x28, 0xb7, 0x75, 0x81, 0xf4, 0xdf, 0x48, 0x80};
unsigned char proof3[] = {0xf9, 0x02, 0x11, 0xa0, 0x89, 0x0a, 0x0a, 0x28, 0xa6, 0x6f, 0x93, 0xfb, 0x32, 0xd6, 0xb0, 0x30, 0x9a, 0x8a, 0xfa, 0x42, 0xa1, 0x19, 0x10, 0x30, 0xdf, 0x21, 0x23, 0x03, 0xd7, 0x95, 0xc2, 0x36, 0xd6, 0xb6, 0xf4, 0x3b, 0xa0, 0x0d, 0xe1, 0xc4, 0x86, 0xb9, 0xe4, 0x04, 0x93, 0x4c, 0xc6, 0xc8, 0xfa, 0x4c, 0x85, 0xec, 0x6a, 0x7c, 0x2f, 0xf0, 0xd8, 0xab, 0x2c, 0x8a, 0x0e, 0x5a, 0xa7, 0x0e, 0x9d, 0x05, 0x07, 0xe4, 0xc7, 0xa0, 0x14, 0x69, 0x2a, 0x00, 0x52, 0x0a, 0xa8, 0xf3, 0x90, 0xf5, 0x41, 0xad, 0x8b, 0xea, 0x6e, 0x26, 0x86, 0xdb, 0x82, 0x7b, 0x07, 0x5b, 0xc2, 0x38, 0xda, 0x73, 0x29, 0x0d, 0x9b, 0x04, 0x18, 0x3e, 0xa0, 0x1b, 0x37, 0xc8, 0x67, 0x01, 0x48, 0x90, 0x00, 0x0f, 0xdb, 0x76, 0x59, 0x1c, 0x89, 0xe2, 0x1a, 0xa0, 0x51, 0xf9, 0xd5, 0xb5, 0x03, 0x68, 0x23, 0x72, 0x79, 0x30, 0x15, 0xde, 0xbc, 0xa9, 0x5a, 0xa0, 0xcb, 0x87, 0xcf, 0x3c, 0xfd, 0x57, 0xbf, 0x94, 0xe9, 0x1c, 0x14, 0x46, 0xfd, 0x95, 0x03, 0xc1, 0xe4, 0x5e, 0xe7, 0x6d, 0x8b, 0xa3, 0xd7, 0x0b, 0xfd, 0x66, 0x63, 0x80, 0x9b, 0x82, 0x7e, 0x40, 0xa0, 0x5e, 0x89, 0x3d, 0xa9, 0x0f, 0xe5, 0x8d, 0xdf, 0xe2, 0x1b, 0xac, 0xe6, 0x56, 0x0a, 0xef, 0xae, 0x08, 0x62, 0x07, 0x9a, 0x83, 0xac, 0x81, 0x3f, 0x1f, 0x61, 0x16, 0x7a, 0x31, 0xd1, 0x21, 0x64, 0xa0, 0xa8, 0x5e, 0x41, 0x37, 0x99, 0xaf, 0x74, 0xee, 0x3b, 0x68, 0xc4, 0xe6, 0xf1, 0xe8, 0xea, 0xc6, 0x40, 0x3a, 0x26, 0x34, 0x1b, 0x7d, 0xe2, 0x0c, 0xcf, 0x3e, 0xf5, 0xfd, 0x17, 0x00, 0xf0, 0x9e, 0xa0, 0xa1, 0x38, 0x47, 0x75, 0x0e, 0x17, 0xaa, 0x18, 0x6e, 0x7d, 0xbf, 0x31, 0x72, 0x60, 0xa5, 0x6c, 0x07, 0xee, 0x39, 0xaf, 0x03, 0x25, 0x56, 0x42, 0x6e, 0x6b, 0x69, 0x7d, 0x09, 0x18, 0xae, 0x49, 0xa0, 0x3a, 0xc4, 0x99, 0x7c, 0xa1, 0xeb, 0xbb, 0xca, 0xc8, 0x4e, 0x9a, 0x6b, 0x34, 0xdc, 0xcf, 0x8f, 0x0c, 0x79, 0xf1, 0xe8, 0x9b, 0x10, 0x94, 0x59, 0xa4, 0xe5, 0x2e, 0x55, 0x5c, 0x83, 0xd2, 0xd6, 0xa0, 0x50, 0x8a, 0x64, 0xc5, 0x15, 0x9f, 0xae, 0xeb, 0xf7, 0xa0, 0x0c, 0x69, 0xd3, 0xe2, 0xe8, 0x0f, 0xce, 0x8b, 0x76, 0x26, 0xdd, 0x35, 0xff, 0xfb, 0x6e, 0x62, 0x16, 0x65, 0x8b, 0x28, 0x3c, 0xd5, 0xa0, 0x8d, 0xd7, 0x8f, 0xbe, 0x96, 0xae, 0xff, 0x73, 0x80, 0x8c, 0x63, 0x7d, 0x2b, 0xf4, 0x6e, 0xdb, 0x81, 0xeb, 0xb3, 0xd3, 0x86, 0x00, 0x69, 0xe8, 0x6a, 0x08, 0xf6, 0x6b, 0xa4, 0x6c, 0x19, 0xae, 0xa0, 0x39, 0x20, 0xbd, 0x60, 0xef, 0x1a, 0x4a, 0x7e, 0xb9, 0x2d, 0xf4, 0x5f, 0x01, 0x5c, 0x42, 0x7a, 0x51, 0x3c, 0x4f, 0x4a, 0x32, 0xb8, 0xf1, 0xa2, 0x7e, 0x8c, 0x84, 0x9b, 0xdc, 0x03, 0xa3, 0x39, 0xa0, 0xbb, 0x10, 0x74, 0x09, 0xcf, 0x01, 0xbf, 0x70, 0x96, 0xf6, 0x27, 0xe2, 0x07, 0x25, 0xfc, 0x5b, 0x26, 0xa6, 0xae, 0x1f, 0x67, 0x4a, 0x2f, 0x6b, 0xb9, 0x65, 0xee, 0xf3, 0x22, 0x96, 0xb9, 0x33, 0xa0, 0xf1, 0x83, 0x9c, 0x6c, 0xac, 0x38, 0xba, 0x36, 0x46, 0xd5, 0x83, 0x90, 0x45, 0x5a, 0xc8, 0x0e, 0xfa, 0x91, 0xb3, 0xb8, 0xe1, 0x3b, 0x6b, 0x7d, 0xac, 0x83, 0x77, 0x61, 0xee, 0xe5, 0xf4, 0xa9, 0xa0, 0x21, 0x7b, 0xaf, 0xc0, 0xc2, 0x18, 0xa9, 0x6c, 0xf1, 0xab, 0x75, 0x7e, 0x9a, 0x5b, 0xae, 0x20, 0x0f, 0x16, 0x34, 0x07, 0x5d, 0xe6, 0x45, 0x36, 0x66, 0x1d, 0xc8, 0x5c, 0x22, 0xc0, 0x2e, 0x3c, 0xa0, 0xe1, 0x9b, 0xac, 0xa6, 0xa0, 0x0e, 0x41, 0xcb, 0xa4, 0xe4, 0x70, 0x33, 0x6c, 0x48, 0xf1, 0xff, 0x11, 0xc6, 0x33, 0xcc, 0x88, 0x84, 0x01, 0xf8, 0x4a, 0xe2, 0xa6, 0xe2, 0xd6, 0x3a, 0xd1, 0xa5, 0x80};
unsigned char proof4[] = {0xf8, 0x71, 0xa0, 0x33, 0xdf, 0x9a, 0xfc, 0xd4, 0x5a, 0xef, 0xdb, 0x90, 0x09, 0xdc, 0x7b, 0xe4, 0xb8, 0x3a, 0x68, 0x0a, 0x7a, 0x45, 0x4a, 0x40, 0xac, 0xef, 0xf0, 0xa8, 0xd9, 0x09, 0x11, 0x7b, 0xdd, 0xee, 0xc7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xa0, 0x94, 0x42, 0x84, 0x87, 0x37, 0xad, 0xa0, 0xc1, 0xab, 0x83, 0xb3, 0x34, 0x93, 0x90, 0x51, 0x1f, 0x8e, 0xb3, 0x3a, 0xe7, 0x28, 0x05, 0x89, 0xd8, 0x6f, 0xb6, 0x11, 0x4f, 0x23, 0x28, 0x59, 0xde, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xa0, 0x95, 0x4e, 0x85, 0x45, 0xcb, 0x42, 0xac, 0x37, 0xf2, 0xef, 0x0f, 0x79, 0xf7, 0x45, 0xa0, 0x3c, 0x66, 0x0d, 0xd1, 0x43, 0xa7, 0xb6, 0x21, 0x13, 0x23, 0x85, 0x06, 0x0a, 0x83, 0x45, 0xe8, 0x21, 0x80, 0x80};
unsigned char proof5[] = {0xe0, 0x9e, 0x3c, 0xd9, 0x54, 0x8b, 0x62, 0xa8, 0xd6, 0x03, 0x45, 0xa9, 0x88, 0x38, 0x6f, 0xc8, 0x4b, 0xa6, 0xbc, 0x95, 0x48, 0x40, 0x08, 0xf6, 0x36, 0x2f, 0x93, 0x16, 0x0e, 0xf3, 0xe5, 0x63, 0x01};

unsigned int proof_lens[] = {532, 532, 532, 532, 115, 33};
unsigned char *proofs[] = {proof0, proof1, proof2, proof3, proof4, proof5};
unsigned char value_data[] = {0x01};
unsigned int value_len = 1;
int proof_length = 6;

unsigned char nibbles[64];
unsigned char decoded_data[MAX_STRINGS][64]; // Assuming a max length for each string
unsigned int decoded_lens[MAX_STRINGS];

int main() {
    int nibble_index = 0;

    for (int i = 0; i < 32; i++) {
        nibbles[i * 2] = key_hash[i] >> 4;
        nibbles[i * 2 + 1] = key_hash[i] & 0x0F;
    }

    for (int i = 0; i < proof_length; i++) {
        memset(&ctx, 0, sizeof(SHA3_CTX));

        for (int k = 0; k < proof_lens[i]; k++) {
            keccak_msg[k] = proofs[i][k];
        }
        keccak_update(proof_lens[i]);
        keccak_final();

        if (memcmp(storage_hash, keccak_hash, 32) != 0) {
            return 0;
        }

        unsigned char *input_data = proofs[i];
        unsigned int input_len = proof_lens[i];
        unsigned int idx = 1;
        unsigned int num_strings = 0;

        if (input_data[0] > 0xc0) {
            if (0xf7 < input_data[0]) {
                idx += input_data[0] - 0xf7;
            }

            while (idx < input_len) {
                if (num_strings >= MAX_STRINGS) {
                    return 0;
                }

                if (input_data[idx] <= 0x7f) {
                    decoded_lens[num_strings] = 1;
                    memcpy(decoded_data[num_strings], &input_data[idx], 1);
                    idx++;
                } else if (input_data[idx] <= 0xb7) {
                    unsigned int n = input_data[idx] - 0x80;
                    decoded_lens[num_strings] = n;
                    memcpy(decoded_data[num_strings], &input_data[idx + 1], n);
                    idx += 1 + n;
                } else {
                    return 0;
                }

                num_strings++;
            }

            if (num_strings == 0 || num_strings > MAX_STRINGS) {
                return 0;
            }

            if (num_strings == 17) {
                for (int k = 0; k < 32; k++) {
                    storage_hash[k] = decoded_data[nibbles[nibble_index]][k];
                }
                nibble_index++;
            } else {
                if (value_len == decoded_lens[1] && memcmp(value_data, decoded_data[1], value_len) == 0) {
                    return 111;
                } else {
                    return 0;
                }
            }
        } else {
            return 0;
        }
    }
    return 0;
}
