/*!
    \file ttl_buckets.c
    \brief heap for cache
    \version 1.0
    \date 24 Jul 2024

    This file contains implementation of TTL Buckets for cache.

    In segmentation cache TTL buckets play the main role.
    It is used when cache is written. TTL bucket unite segments
    with close time to live. Segments in TTL bucket are sorted by time
    from the oldest to the newest, that allows garbage collector to find 
    expired segments faster.
    Evict strategy of cache bases on merging principle:
        Algorithm choose n consequence segments in one TTL bucket and 
    merge most frequently used items from these segments to only one
    segment. Other segments will be evicted to free list.

    IDENTIFICATION
        src/backend/cache/ttl_buckets.c
*/

#include <stdlib.h>
#include "../../include/cache_ttl_buckets.h"
#include "../../include/cache_heap.h"
#include "../../include/cache_hash_table.h"


// FUNCTION ADVERTISEMENT

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
int writeCache(TTLGroup *group, int heapIdx, HashTable *table, int ttl, ItemHeader *itemHeader, void *value);

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


/*!
    \brief Gets index of TTL bucket by ttl
    There are 4 ttl subgroups: 256 buckets with range 8s, 256 buckets with range (16*8)s, 256 - (16*16*8)s, 256 - (16*16*16*8)s
    \param[in] ttl ttl of cache
    \return index of ttl bucket if success, -1 else
*/
int getIdxByTTL(int ttl);


// FUNCTION IMPLEMENTATION

int getIdxByTTL(int ttl) {
    if (ttl < 0) {
        return -1;
    }else if (ttl < 2048) {
        return ttl / 8;
    } else if (ttl < 34816) {
        return (ttl - 2048) / 128;
    } else if (ttl < 559104) {
        return (ttl - 34816) / 2048;
    } else if (ttl < 8947712) {
        return (ttl - 559104) / 32768;
    } else {
        return -1;
    }
}

TTLGroup *initTTLBuckets(void) {
    TTLGroup *group = (TTLGroup *)malloc(sizeof(TTLGroup));
    group->mergeIdx = 0;
    for (int j = 0; j < 1024; j++) {
        group->buckets[j].headSeg = -1;
        group->buckets[j].tailSeg = -1;
    }
    return group;
}

void freeTTLGroup(TTLGroup *group) {
    free(group);
}




int writeCache(TTLGroup *group, int heapIdx, HashTable *table, int ttl, ItemHeader *itemHeader, void *value) {
    if (!group || heapIdx < 0 || heapIdx >= table->countHeaps || !table || !value) {
        return -1;
    }
    /*CHOOSE TTL BUCKET*/
    int ttlIdx = getIdxByTTL(ttl);
    /*CHECK AND WRITE TO SEGMENT*/
    int tailNum = group->buckets[ttlIdx].tailSeg;

    Heap *heap = table->heaps[heapIdx];

    SegmentHeader *tailHeader = getSegmentHeader(heap->begin, tailNum); 
    int offset = tailHeader->filledSize;

    int tryResult = addItem(heap->begin, tailNum, itemHeader, value);
    if (tryResult == -1) {
        //ADD SEGMENT
        int num;
        if ((num = allocateSegment(heap, heap->begin)) == -1) {
            return -1;
        }
        tailNum = num;
        group->buckets[ttlIdx].tailSeg = num;

        tailHeader = getSegmentHeader(heap->begin, tailNum); 
        offset = tailHeader->filledSize;

        if (addItem(heap->begin, tailNum, itemHeader, value) == -1) {
            return -1;
        }
        
    } else if (tryResult == -2) {
        return -1;
    }

    /*WRITE POINTER TO HASH TABLE*/
    if (addPtr(table, itemHeader->key, heapIdx, tailNum, offset) == -1) {
        return -1; //unexpected error
    }

    return 0;    
}



int deleteExpiredSegments(TTLGroup *group, Heap *heap, HashTable *table) {
    for(int ttlIdx = 0; ttlIdx < 1024; ttlIdx++) {
        TTLBucket *curBucket = &(group->buckets[ttlIdx]);
        int curNum = curBucket->headSeg;
        int lastNum = -1, nextNum = -1;
        while (curNum != -1) {
            SegmentHeader *curHeader = getSegmentHeader(heap->begin, curNum);
            nextNum = curHeader->next;
            if (SEGDELETE(curHeader->flags) || SEGEXPIRED(curHeader->flags)) {
                /*DELETE FROM LIST AND FREE*/
                if (lastNum == -1) {
                    curBucket->headSeg = curHeader->next;
                } else {
                    SegmentHeader *lastHeader = getSegmentHeader(heap->begin, lastNum);
                    lastHeader->next = curHeader->next;
                    setSegmentHeader(lastHeader);
                }
                freeSegment(heap, heap->begin, curNum);
            }
            curNum = nextNum;
        }
    }
    return 0;
}
