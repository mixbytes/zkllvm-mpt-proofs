#ifdef __ZKLLVM__
#include <nil/crypto3/hash/algorithm/hash.hpp>
#include <nil/crypto3/hash/sha2.hpp>
using namespace nil::crypto3;
#endif



/////////////     RLP PART

#include "rlp.h"
// [WARNING] code was generated with the help of ChatGPT, [TODO] - recheck&optimize

bool hasNext(const Iterator* self) {
    return (uintptr_t)self->nextPtr < (uintptr_t)(self->item.memPtr + self->item.len);
}

uint32_t _itemLength(const uint8_t *ptr) {
    uint8_t firstByte = *ptr;
    if (firstByte < STRING_SHORT_START) {
        return 1;
    } else if (firstByte < STRING_LONG_START) {
        return firstByte - STRING_SHORT_START + 1;
    } else if (firstByte < LIST_SHORT_START) {
        uint8_t lengthOfLength = firstByte - STRING_LONG_START + 1;
        uint32_t dataLength = 0;
        for (uint8_t i = 0; i < lengthOfLength; ++i) {
            dataLength = (dataLength << 8) + ptr[i + 1];
        }
        return 1 + lengthOfLength + dataLength;
    } else if (firstByte < LIST_LONG_START) {
        return firstByte - LIST_SHORT_START + 1;
    } else {
        uint8_t lengthOfLength = firstByte - LIST_LONG_START + 1;
        uint32_t dataLength = 0;
        for (uint8_t i = 0; i < lengthOfLength; ++i) {
            dataLength = (dataLength << 8) + ptr[i + 1];
        }
        return 1 + lengthOfLength + dataLength;
    }
}


uint32_t _payloadOffset(const uint8_t *ptr) {
    uint8_t firstByte = *ptr;
    if (firstByte < STRING_SHORT_START) {
        return 0;
    } else if (firstByte < STRING_LONG_START || (firstByte >= LIST_SHORT_START && firstByte < LIST_LONG_START)) {
        return 1;
    } else if (firstByte < LIST_SHORT_START) {
        return firstByte - (STRING_LONG_START - 1) + 1;
    } else {
        return firstByte - (LIST_LONG_START - 1) + 1;
    }
}


RLPItem next(Iterator* self) {
    if (!hasNext(self)) {
        return (RLPItem){0, NULL};
    }

    const uint8_t *ptr = self->item.memPtr + self->nextPtr;
    uint32_t itemLength = _itemLength(ptr);
    self->nextPtr += itemLength;

    return (RLPItem){itemLength, (uint8_t*)ptr};
}

RLPItem toRlpItem(const uint8_t* item, uint32_t itemLength) {
    RLPItem rlpItem;
    rlpItem.len = itemLength;
    rlpItem.memPtr = (uint8_t*)malloc(itemLength);
    if (rlpItem.memPtr != NULL) {
        memcpy(rlpItem.memPtr, item, itemLength);
    }
    return rlpItem;
}

Iterator iterator(const RLPItem* self) {
    Iterator it;
    it.item = *self;
    it.nextPtr = _payloadOffset(self->memPtr);
    return it;
}

uint32_t rlpLen(const RLPItem* item) {
    return item->len;
}

void payloadLocation(const RLPItem* item, uint8_t** memPtr, uint32_t* len) {
    uint32_t offset = _payloadOffset(item->memPtr);
    *memPtr = item->memPtr + offset;
    *len = item->len - offset;
}

uint32_t payloadLen(const RLPItem* item) {
    uint32_t offset = _payloadOffset(item->memPtr);
    return item->len - offset;
}

bool isList(const RLPItem* item) {
    if (item->len == 0) return false;

    uint8_t byte0 = item->memPtr[0];
    return byte0 >= LIST_SHORT_START;
}

RLPItem* toList(const RLPItem* item, uint32_t* numItems) {
    if (!isList(item)) {
        *numItems = 0;
        return NULL;
    }

    uint32_t count = 0;
    uintptr_t currPtr = (uintptr_t)(item->memPtr) + _payloadOffset(item->memPtr);
    uintptr_t endPtr = (uintptr_t)(item->memPtr) + item->len;
    while (currPtr < endPtr) {
        currPtr += _itemLength((uint8_t*)currPtr);
        count++;
    }

    RLPItem* list = (RLPItem*)malloc(count * sizeof(RLPItem));
    if (list == NULL) {
        *numItems = 0;
        return NULL;
    }

    currPtr = _payloadOffset(item->memPtr);
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t len = _itemLength(item->memPtr + currPtr);
        list[i] = (RLPItem){len, item->memPtr + currPtr};
        currPtr += len;
    }

    *numItems = count;
    return list;
}


uint8_t* toRlpBytes(const RLPItem* item, uint32_t* bytesLength) {
    *bytesLength = item->len;
    uint8_t* bytes = (uint8_t*)malloc(*bytesLength);
    if (bytes != NULL) {
        memcpy(bytes, item->memPtr, *bytesLength);
    }
    return bytes;
}

bool toBoolean(const RLPItem* item) {
    if (item->len != 1) {
        return false;
    }
    return item->memPtr[0] != 0 && item->memPtr[0] != STRING_SHORT_START;
}

void toAddress(const RLPItem* item, uint8_t address[20]) {
    if (item->len != 21) {
        return;
    }
    memcpy(address, item->memPtr + 1, 20);
}

uint32_t toUint(const RLPItem* item) {
    if (item->len == 0 || item->len > 33) {
        return 0;
    }

    uint32_t value = 0;
    uint32_t len = item->len;
    if (len == 33) {
        len--;
    }

    for (uint32_t i = 0; i < len; ++i) {
        value = (value << 8) | item->memPtr[item->len - len + i];
    }

    return value;
}

uint32_t toUintStrict(const RLPItem* item) {
    if (item->len != 33) {
        return 0;
    }

    uint32_t value = 0;
    for (uint32_t i = 1; i < item->len; ++i) {
        value = (value << 8) | item->memPtr[i];
    }

    return value;
}

uint8_t* toBytes(const RLPItem* item, uint32_t* bytesLength) {
    *bytesLength = item->len;
    uint8_t* bytes = (uint8_t*)malloc(*bytesLength);
    if (bytes != NULL) {
        memcpy(bytes, item->memPtr, *bytesLength);
    }
    return bytes;
}

uint32_t numItems(const RLPItem* item) {
    if (item->len == 0) return 0;

    uint32_t count = 0;
    uintptr_t currPtr = (uintptr_t)(item->memPtr) + _payloadOffset(item->memPtr);
    uintptr_t endPtr = (uintptr_t)(item->memPtr) + item->len;
    while (currPtr < endPtr) {
        currPtr += _itemLength((uint8_t*)currPtr);
        count++;
    }

    return count;
}

void copy(uint32_t src, uint32_t dest, uint32_t len) {
    if (len == 0) return;

    uintptr_t srcAddr = (uintptr_t)src;
    uintptr_t destAddr = (uintptr_t)dest;

    const uint8_t* srcPtr = (const uint8_t*)srcAddr;
    uint8_t* destPtr = (uint8_t*)destAddr;

    while (len >= WORD_SIZE) {
        *((uint32_t*)destPtr) = *((const uint32_t*)srcPtr);
        srcPtr += WORD_SIZE;
        destPtr += WORD_SIZE;
        len -= WORD_SIZE;
    }

    if (len > 0) {
        uint32_t mask = 0xFFFFFFFF << (WORD_SIZE - len) * 8;
        *((uint32_t*)destPtr) = (*((uint32_t*)destPtr) & mask) | (*((uint32_t*)srcPtr) & ~mask);
    }
}
// Mock Keccak-256 Hash Function
// This is a placeholder and should be replaced with a proper implementation.
void keccak256(const uint8_t* input, size_t inputLen, uint8_t output[32]) {
    // Mock hashing: Just filling the output with some data for demonstration.
    memset(output, 0, 32);
    for (size_t i = 0; i < inputLen && i < 32; i++) {
        output[i] = input[i] ^ (uint8_t)i;  // Simple XOR operation for demonstration
    }
}

void rlpBytesKeccak256(const RLPItem* item, uint8_t output[32]) {
    keccak256(item->memPtr, item->len, output);
}

void _mptHashHash(const RLPItem* item, uint8_t output[32]) {
    uint8_t tempHash[32];

    if (item->len < 32) {
        rlpBytesKeccak256(item, output);
    } else {
        rlpBytesKeccak256(item, tempHash);

        // Normally, we would use keccak256 again on the output of the first hash.
        // However, for the mock version, we'll just pass the tempHash directly.
        keccak256(tempHash, 32, output);
    }
}


size_t _sharedPrefixLength(size_t xsOffset, const uint8_t* xs, size_t xsLen, const uint8_t* ys, size_t ysLen) {
    size_t i;
    for (i = 0; i + xsOffset < xsLen && i < ysLen; i++) {
        if (xs[i + xsOffset] != ys[i]) {
            return i;
        }
    }
    return i;
}

uint8_t* _decodeNibbles(const uint8_t* compact, size_t compactLen, size_t skipNibbles, size_t* nibblesLen) {
    if (compactLen == 0) return NULL;

    size_t length = compactLen * 2;
    if (skipNibbles > length) return NULL;
    length -= skipNibbles;

    uint8_t* nibbles = (uint8_t *) malloc(length * sizeof(uint8_t));
    if (nibbles == NULL) return NULL;

    size_t nibblesLength = 0;
    for (size_t i = skipNibbles; i < skipNibbles + length; i++) {
        nibbles[nibblesLength] = (i % 2 == 0) ? (compact[i / 2] >> 4) & 0xF : compact[i / 2] & 0xF;
        nibblesLength++;
    }

    *nibblesLen = nibblesLength;
    return nibbles;
}

bool _merklePatriciaCompactDecode(const uint8_t* compact, size_t compactLen, uint8_t** nibbles, size_t* nibblesLen) {
    if (compactLen == 0) return false;

    uint8_t first_nibble = compact[0] >> 4 & 0xF;
    size_t skipNibbles;
    bool isLeaf;

    switch (first_nibble) {
        case 0: skipNibbles = 2; isLeaf = false; break;
        case 1: skipNibbles = 1; isLeaf = false; break;
        case 2: skipNibbles = 2; isLeaf = true; break;
        case 3: skipNibbles = 1; isLeaf = true; break;
        default:
            // Handle error
            return false;
    }

    *nibbles = _decodeNibbles(compact, compactLen, skipNibbles, nibblesLen);
    return isLeaf;
}

void payloadKeccak256(const RLPItem* item, uint8_t output[32]) {
    uint8_t* memPtr;
    uint32_t len;
    payloadLocation(item, &memPtr, &len);
    keccak256(memPtr, len, output);  // Using the keccak256 function from the provided header
}

////////////    // EOF RLP PART












#ifdef __ZKLLVM__
bool is_same(typename hashes::sha2<256>::block_type block0,
    typename hashes::sha2<256>::block_type block1){

    return block0[0] == block1[0] && block0[1] == block1[1];
}

[[circuit]] bool
    validate_path(std::array<typename hashes::sha2<256>::block_type, 0x05> merkle_path,
        typename hashes::sha2<256>::block_type leave,
        typename hashes::sha2<256>::block_type root) {
    
    typename hashes::sha2<256>::block_type subroot = leave;

    for (int i = 0; i < 0x05; i++) {
        subroot = hash<hashes::sha2<256>>(subroot, merkle_path[i]);
    }

    return is_same(subroot, root);
}
#endif






// PART for debuggging
#ifndef __ZKLLVM__

#include <stdio.h>
int main() {
    printf("AAAAAAAAAAAAAAAAAAAAAA\n");
    return 0;
}
#endif
