#ifndef RLP_H
#define RLP_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

// Constants
#define STRING_SHORT_START 0x80
#define STRING_LONG_START 0xb8
#define LIST_SHORT_START 0xc0
#define LIST_LONG_START 0xf8
#define WORD_SIZE 32

// RLPItem struct
typedef struct {
    uint32_t len;
    uint8_t* memPtr;
} RLPItem;

// Iterator struct
typedef struct {
    RLPItem item; // Item that's being iterated over
    uint32_t nextPtr; // Position of the next item in the list
} Iterator;

bool hasNext(const Iterator* self);
RLPItem next(Iterator* self);
RLPItem toRlpItem(const uint8_t* item, uint32_t itemLength);
Iterator iterator(const RLPItem* self);
uint32_t rlpLen(const RLPItem* item);
void payloadLocation(const RLPItem* item, uint8_t** memPtr, uint32_t* len);
uint32_t payloadLen(const RLPItem* item);
bool isList(const RLPItem* item);
RLPItem* toList(const RLPItem* item, uint32_t* numItems);
uint8_t* toRlpBytes(const RLPItem* item, uint32_t* bytesLength);
bool toBoolean(const RLPItem* item);
void toAddress(const RLPItem* item, uint8_t address[20]);
uint32_t toUint(const RLPItem* item);
uint32_t toUintStrict(const RLPItem* item);
uint8_t* toBytes(const RLPItem* item, uint32_t* bytesLength);
uint32_t numItems(const RLPItem* item);

// Mock Keccak-256 Hash Function
// This is a placeholder and should be replaced with a proper implementation.
void keccak256(const uint8_t* input, size_t inputLen, uint8_t output[32]);
void rlpBytesKeccak256(const RLPItem* item, uint8_t output[32]);
void _mptHashHash(const RLPItem* item, uint8_t output[32]);

size_t _sharedPrefixLength(size_t xsOffset, const uint8_t* xs, size_t xsLen, const uint8_t* ys, size_t ysLen);
uint8_t* _decodeNibbles(const uint8_t* compact, size_t compactLen, size_t skipNibbles, size_t* nibblesLen);
bool _merklePatriciaCompactDecode(const uint8_t* compact, size_t compactLen, uint8_t** nibbles, size_t* nibblesLen);
void payloadKeccak256(const RLPItem* item, uint8_t output[32]);

#endif // RLP_H
