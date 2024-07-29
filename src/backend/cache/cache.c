/*!
    \file cache.c
    \brief cache
    \version 1.0
    \date 29 Jul 2024

    This file contains high-level implementation of cache.

    Segmentation cache has 3 principles:
    - be proactive, don't be lazy
    - maximize shared meta information for economy
    - perform macro management

    more information: https://www.usenix.org/system/files/nsdi21-yang.pdf

    IDENTIFICATION
        src/background/cache/cache.c
*/
#include "../../include/cache.h"
#include "../../include/cache_heap.h"
#include "../../include/cache_hash_table.h"
#include "../../include/cache_ttl_buckets.h"
#include <string.h>


/*FUNCTION ADVERTISEMENT*/

/*!
    \brief Initializes cache 
    \param[in] hashTableSize Size of hash table
    \param[in] countRAMSeg Count segments in RAM
    \param[in] countFileSeg Count segments in FS
    \param[in] ramSegSize Size of segment in RAM
    \param[in] fileSegSize Size of segment in FS
    \param[in] cacheDir Path to cache directory
    \return 0 if success, -1 else
*/
int initCache(int hashTableSize, int countRAMSeg, int countFileSeg, int ramSegSize, int fileSegSize, char *cacheDir);

/*!
    \brief Frees cache
*/
void freeCache(void);

/*!
    \brief Pushes new cache
    \param[in] key Key string for cache
    \param[in] ttl Time to live of cache
    \param[in] valueSize Size of cache data
    \param[in] value Cache value
    \return 0 if success. -1 else
*/
int pushCache(char *key, int ttl, int valueSize, void *value);

/*!
    \brief Gets cache
    \param[in] key Key string of cache
    \param[out] value Pointer to result (Will be NULL if cache not found)
    \return 0 if cache worked correctly, -1 else
*/
int getCache(char *key, void **value);

/*FUNCTION IMPLEMENTATION*/

Heap **heaps;
TTLGroup **groups;
HashTable *table;
int countHeaps;

int initCache(int hashTableSize, int countRAMSeg, int countFileSeg, int ramSegSize, int fileSegSize, char *cacheDir) {
    countHeaps = 2;
    heaps = (Heap **)malloc(sizeof(Heap *) * 2);
    heaps[0] = initHeap(ramSegSize, countRAMSeg, NULL);
    heaps[1] = initHeap(fileSegSize, countFileSeg, cacheDir);
    if (!heaps[0] || !heaps[1]) {
        return -1;
    }
    groups = (TTLGroup **)malloc(sizeof(TTLGroup *) * 2);
    groups[0] = initTTLBuckets();
    groups[1] = initTTLBuckets();
    if (!groups[0] || !groups[1]) {
        return -1;
    }
    table = initTable(hashTableSize, heaps, 2);
    if (!table) {
        return -1;
    }
    return 0;
}


void freeCache(void) {
    for (int i = 0; i < countHeaps; i++) {
        freeHeap(heaps[i]->begin);
        freeTTLGroup(groups[i]);
    }
    free(heaps);
    free(groups);
    freeTable(table);
}

int pushCache(char *key, int ttl, int valueSize, void *value) {
    ItemHeader *itemHeader = (ItemHeader *)malloc(sizeof(ItemHeader));
    itemHeader->flags = 0;
    itemHeader->key = key;
    itemHeader->keySize = strlen(key);
    itemHeader->valueSize = valueSize;
    int isWritten = 0;
    for(int i = 0; i < countHeaps; i++) {
        if (valueSize < heaps[i]->segmentSize) {
            if(writeCache(groups[i], i, table, ttl, itemHeader, value) == 0) {
                isWritten = 1;
            };
            break;
        }
    }
    if (!isWritten) {
        free(itemHeader);
        return -1;    
    }
    free(itemHeader);
    return 0;
}


int getCache(char *key, void **value) {
    ItemHeader resultHeader;
    if (getItem(table, key, &resultHeader, value) == 0) {
        return 0;
    }
    return -1;
}