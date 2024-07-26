/*!
    \file cache_hash_table.h
    \brief hash table for cache
    \version 1.0
    \date 25 Jul 2024

    This file contains interface of hash table for cache.
    Hash table has bulk chain

    one string of table:
    _________________________________________________________________________________________     _____________________________________________________________________________
    |  Bucket  |   heap   |   heap   |   heap   |   heap   |   heap   |   heap   |   next   |     |   heap   |   heap   |   heap   |   heap   |   heap   |   heap   |   heap   |   heap   |
    |   info   |  pointer |  pointer |  pointer |  pointer |  pointer |  pointer |  pointer-|---->|  pointer |  pointer |  pointer |  pointer |  pointer |  pointer |  pointer |  pointer-|
    |__________|__________|__________|__________|__________|__________|__________|__________|     |__________|__________|__________|__________|__________|__________|__________|__________|
    ...

    bucket info : 64B = |reserved 40B | last use timestap 16B | chain length (count of buses in chain) 8B|

    heap pointer: 64B = |segment idx 20B | offset in segment 20B | frequency counter 8B | additional hash 11 B | heap idx 1B|
    
    IDENTIFICATION
        src/include/cache_hash_table.h
*/
#ifndef HASH_TABLE_H
#define HASH_TABLE_H
#include "cache_heap.h"
#include <stdint.h>


#define PTR_GET_SEG_IDX(ptr) ((ptr & 0xffffff0000000000) >> 40)
#define PTR_SET_SEG_IDX(ptr, idx) (ptr = (ptr | (idx << 40)))

#define PTR_GET_SEG_OFFSET(ptr)((ptr & 0x000000fffff00000) >> 20)
#define PTR_SET_SEG_OFFSET(ptr, offset) (ptr = (ptr | ((offset & 0x00000000000fffff) << 20)))

#define PTR_GET_COUNTER(ptr) ((ptr & 0x00000000000ff000) >> 12)
#define PTR_SET_COUNTER(ptr, counter) (ptr = (ptr | ((counter & 0x00000000000000ff) << 12)))

#define PTR_GET_TAG(ptr) ((ptr & 0x0000000000000ffe) >> 1)
#define PTR_SET_TAG(ptr, tag) (ptr = (ptr | ((tag & 0x00000000000007ff) << 1)))

#define PTR_GET_HEAP(ptr) (ptr & 0x0000000000000001)
#define PTR_SET_HEAP(ptr, heap) (ptr = (ptr | (heap & 0x0000000000000001)))

#define BKT_GET_CHAIN_LEN(bkt) (bkt & 0x00000000000000ff)
#define BKT_SET_CHAIN_LEN(bkt, len) (bkt = (bkt | (len & 0x00000000000000ff)))

#define BKT_GET_TIMESTAMP(bkt) ((bkt & 0x0000000000ffff00) >> 8)
#define BKT_SET_TIMESTAMP(bkt, timestamp) (bkt = (bkt | ((timestamp & 0x000000000000ffff) << 8)))

/*!
    \struct HashBucketData
    \brief Contains bucket of eight pointers
*/
typedef struct HashBucketData {
    uint64_t pointers[8];
} HashBucket;

/*!
    \struct HashTableData
    \brief hash table
*/
typedef struct HashTableData {
    HashBucket *table;
    Heap **heaps; 
    int size;
    int countHeaps;
} HashTable;

/*!
    \brief hash function
*/
uint32_t hashLookup (char *str);

/*!
    \brief Initializes hash table 
    \param[in] size Size of hashTable
    \param[in] heaps Array of pointers to heaps that will be used in hashTable
    \param[in] countHeaps Size of heaps array
    \return pointer to new hash table if success, NULL else
*/
HashTable *initTable(int size, Heap** heaps, int countHeaps);

/*!
    \brief Frees hash table
    \param[in] table Pointer to hash table
*/
void freeTable(HashTable *table);

/*!
    \brief Adds new pointer to table
    \param[in] table Pointer to hash table
    \param[in] key String key of item
    \param[in] heap Index of heap in array 
    \param[in] seg Index of segment
    \param[in] offset offset of item in segment
    \return 0 if success, -1 else
*/
int addPtr(HashTable *table, char *key, int heap, int seg, int offset);

/*!
    \brief Gets ptr in hashtable
    \param[in] table Pointer to hash table
    \param[in] key String key of item in hashtable
    \param[in] heap Heap index in array
    \param[in] seg Index of segment in heap
    \param[in] offset offset of item in segment
    \param[out] pos Position in bus
    \return pointer to pinter to item if success, 0xfffff else
*/
uint64_t *getPtr(HashTable *table, char *key, int heap, int seg, int offset, int *pos);

/*!
    \brief Deletes pointer from hash table
    \param[in] table Pointer to hash table
    \param[in] key String key of item in hash table
    \param[in] heap Heap index in array
    \param[in] seg Index of segment in heap
    \param[in] offset offset of item in segment
    \return 0 if success, -1 else 
*/
int deletePtr(HashTable *table, char *key, int heap, int seg, int offset);

/*!
    \brief Gets value of frequency counter of pointer from hash table
    \param[in] table Pointer to hash table
    \param[in] key String key of item in hash table
    \param[in] heap Heap index in array
    \param[in] seg Index of segment in heap
    \param[in] offset offset of item in segment 
    \return value if frequency counter if success, -1 else 
*/
int getPtrFreq(HashTable *table, char *key, int heap, int seg, int offset);

/*!
    \brief Gets item by pointer from hash table
    \param[in] table Pointer to hash table
    \param[in] key String key of item in hash table
    \param[out] header Pointer to result header of item
    \param[out] item Pointer to result pointer to value of item
    \return 0 if success, -1 else
*/
int getItem(HashTable *table, char *key, ItemHeader *header, void **item);
    


#endif // HASH_TABLE_H