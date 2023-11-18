
#ifndef __KECCAK256_H_
#define __KECCAK256_H_

#include <stdint.h>
#include <stddef.h>

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


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


void keccak_init(SHA3_CTX *ctx);
void keccak_update(SHA3_CTX *ctx, const unsigned char *msg, uint16_t size);
void keccak_final(SHA3_CTX *ctx, unsigned char* result);
void keccak256(const uint8_t* input, size_t inputLen, uint8_t output[32]);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __KECCAK256_H_ */
