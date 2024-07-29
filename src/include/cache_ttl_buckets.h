/*!
    \file cache_ttl_buckets.h
    \brief heap for cache
    \version 1.0
    \date 24 Jul 2024

    This file contains interface of TTL Buckets for cache.

    In writing segmentation cache TTL buckets play the main role.
    It is used when cache is written. TTL bucket unite segments
    with close time to live. Segments in TTL bucket are sorted by time
    from the oldest to the newest, that allows garbage collector to find 
    expired segments faster.
    Evict strategy of cache bases on merging principle:
        Algorithm choose n consequence segments in one TTL bucket and 
    merge most frequently used items from these segments to only one
    segment. Other segments will be evicted to free list.

    IDENTIFICATION
        src/include/cache_ttl_buckets.h
*/
#ifndef TTL_BUCKETS_H
#define TTL_BUCKETS_H

#include "cache_hash_table.h"
#include "cache_heap.h"

typedef struct TTLBucketData {
    int headSeg;
    int tailSeg;  
} TTLBucket;

typedef struct TTLGroupData {
    TTLBucket buckets[1024];
    int mergeIdx;
} TTLGroup;

/*!
    \brief Initializes TTL buckets
    \return Pointer to new TTL group if success, -1 else 
*/
TTLGroup *initTTLBuckets(void);

/*!
    \brief Frees TTL buckets
    \param[in] group Pointer to TTL group
*/
void freeTTLGroup(TTLGroup *group);

/*!
    \brief Write cache in TTL group
    \param[in] group Pointer to group
    \param[in] heapIdx Index of heap in hash table array 
    \param[in] table Pointer to hash table
    \param[in] ttl TTL of cache
    \param[in] itemHeader Header of new cache item
    \param[in] value Pointer to value
    \return 0 if success, -1 else
*/
int writeCache(TTLGroup *group, int heap, HashTable *table, int ttl, ItemHeader *itemHeader, void *value);

/*!
    \brief Deletes expired segments from cache
    \param[in] group Pointer to group
    \param[in] heap Pointer to heap
    \param[in] table Pointer to hash table
    \return 0 if success, -1 else
*/
int deleteExpiredSegments(TTLGroup *group, Heap *heap, HashTable *table);

/*!
    \brief Deletes expired segments from cache
    \param[in] group Pointer to group
    \param[in] heap Pointer to heap
    \param[in] table Pointer to hash table
    \return count evicted blocks if success, -1 else
*/
int evictSegments(TTLGroup *group, Heap *heap, HashTable table);



#endif // TTL_BUCKETS
