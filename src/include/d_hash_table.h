#ifndef HASH_TABLE_H
#define HASH_TABLE_H
#include "d_heap.h"
#include "d_item.h"
#include <stdint.h>


#define PTR_GET_SEG_IDX(ptr) ((ptr & 0xffffff0000000000) >> 40)
#define PTR_SET_SEG_IDX(ptr, idx) (ptr | (idx << 40))

#define PTR_GET_SEG_OFFSET(ptr)((ptr & 0x000000fffff00000) >> 20)
#define PTR_SET_SEG_OFFSET(ptr, offset) (ptr | ((offset & 0x00000000000fffff) << 20))

#define PTR_GET_COUNTER(ptr) ((ptr & 0x00000000000ff000) >> 12)
#define PTR_SET_COUNTER(ptr, counter) (ptr | ((counter & 0x00000000000000ff) << 12))

#define PTR_GET_TAG(ptr) (ptr & 0x0000000000000fff)
#define PTR_SET_TAG(ptr, tag) (ptr | (tag & 0x0000000000000fff))


#define BKT_GET_CHAIN_LEN(bkt) (bkt & 0x00000000000000ff)
#define BKT_SET_CHAIN_LEN(bkt, len) (bkt | (len & 0x00000000000000ff))

#define BKT_GET_TIMESTAMP(bkt) ((bkt & 0x0000000000ffff00) >> 8)
#define BKT_SET_TIMESTAMP(bkt, timestamp) (bkt | ((timestamp & 0x000000000000ffff) << 8))


typedef struct HashBucketData {
    uint64_t pointers[8];
} HashBucket;

typedef struct HashTableData {
    HashBucket *table;
    int size;
} HashTable;

uint32_t hashLookup (char *str);

HashTable *initTable(int size);

void addPtr(HashTable *table, char *key, int idx, int offset);

uint64_t *getPtr(HashTable *table, char *key, int idx, int offset, int *ptr);

void deletePtr(HashTable *table, char *key, int idx, int offset);

int getPtrFreq(HashTable *table, char *key, int idx, int offset);

ItemHeader *getItem(HashTable *table, Heap *heap, char *key);



#endif // HASH_TABLE_H