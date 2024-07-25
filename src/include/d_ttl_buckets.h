
#ifndef TTL_BUCKETS_H
#define TTL_BUCKETS_H

#include "d_hash_table.h"
#include "d_heap.h"

typedef struct TTLBucketData {
    int headSeg;
    int tailSeg;
    int minTTL;
    int maxTTL;    
} TTLBucket;

typedef struct TTLTableData {
    TTLBucket table[1024];
    int mergeIdx;
} TTLTable;



TTLTable *initTTLTable(void);

void writeCache(TTLTable *table, Heap *heap, HashTable *hashTable, int ttl, char *key, int valueSize, void *value);

void evictSegments(TTLTable *TTLtable, Heap *heap, HashTable *hashTable);

void mergeSegments(TTLTable *table, Heap *heap, HashTable *hashTable);



#endif // TTL_BUCKETS

