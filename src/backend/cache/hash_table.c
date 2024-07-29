/*!
    \file hash_table.c
    \brief hash table for cache
    \version 1.0
    \date 25 Jul 2024

    This file contains implementation of bulk-chained hash table.
    
    IDENTIFICATION
        src/backend/cache/hash_table.c
*/
#include "../../include/cache_heap.h"
#include "../../include/cache_hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>




// FUNCTION ADVERTISEMENT

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


/*!
    \brief Gets index by hash
    \param[in] table Pointer to hash table
    \param[in] hash Hash value
    \returns index in table if success, 0xffffffff else
*/
unsigned int getIdx(HashTable *table, uint32_t hash);

/*!
    \brief gets ponter to first pointer after last 
    \param[in] table Pointer to hash table
    \param[in] hash Value of hash
    \param[out] pos Position of pointer in bulk
    \return Pointer to first position after last if last position < 7, else pointer to position after 7th position and *pos = 8, NULL if fail
*/
uint64_t *getEnd(HashTable *table, int hash, int *pos);


/*!
    \brief gets ponter to last pointer in bus chain 
    \param[in] table Pointer to hash table
    \param[in] hash Value of hash
    \param[out] pos Position of pointer in bus
    \return Pointer to last position if sucess, NULL if error or if chain is empty
*/
uint64_t *getLast(HashTable *table, int hash, int *pos);

/*!
    \brief Creates cache pointer
    \param[out] ptr Pointer to result
    \param[in] hash Hash value
    \param[in] heap Index of heap
    \param[in] idx Index of segment
    \param[in] offset Offset in segment in bytes 
*/
void buildCachePtr(uint64_t *ptr, uint64_t hash, uint64_t heap, uint64_t idx, uint64_t offset);

/*!
    \brief Swaps two pointers
    \return 0 if success, -1 else
*/
int swap(uint64_t *first, uint64_t *second);

/*!
    \brief Increments Approximate Smoothed Frequency Counter
    \param[in] ptr Pointer to cache pointer
    \return 0 if success, -1 else
*/
int incrementASFC(uint64_t *ptr);

/*!
    \brief Updates last use time for bucket
    \param[in] ptr Pointer to bucket info
    \return 0 if correctly worked, -1 else
*/
int updateLastUseTime(uint64_t *ptr);


// FUNCTION IMPLEMENTATION


#define ADDITIONAL_HASH(hash) ((hash & 0x07ff0000) >> 16)

#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}



uint32_t hashLookup (char *str) {
        int length = strlen(str);
    int initValue = 31;
    int a, b, c;
    a = b = c = 0xdeadbeef + ((uint32_t)length) + initValue;
    const uint32_t *k = (const uint32_t *)str;
    /* mixing */
    while (length > 12) {
        a += k[0];
        b += k[1];
        c += k[2];
        mix(a,b,c);
        length -= 12;
        k += 3;
    }

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : return c;              /* zero length strings require no mixing */
    }

    /* mix remainder */
    final(a, b, c);
    return c;
}


HashTable *initTable(int size, Heap** heaps, int countHeaps) {
    HashTable *table = (HashTable *)malloc(sizeof(HashTable));
    table->size = size;
    table->countHeaps = countHeaps;
    table->table = (HashBucket *)calloc(size, sizeof(HashBucket));
    for(int i = 0; i < size; i++) {
        BKT_SET_CHAIN_LEN(table->table[i].pointers[0], 1);
    }
    table->heaps = heaps;
    return table;
}


void freeTable(HashTable *table) {
    if (!table) {
        return;
    }

    for (int i = 0; i < table->size; i++) {
        int countBulks =  BKT_GET_CHAIN_LEN(table->table[i].pointers[0]);
        uint64_t *curBucket = (uint64_t *)table->table[i].pointers[7], *delBucket;
        int j = 1;
        while(j < countBulks) {
            j++;
            delBucket = curBucket;
            curBucket = (uint64_t *)*(curBucket + 7);
            free(delBucket);
        }
    }
    free(table->table);
    free(table);
}

unsigned int getIdx(HashTable *table, uint32_t hash) {
    if (!table) {
        return -1;
    }
    return hash & (table->size - 1);
}

uint64_t *getEnd(HashTable *table, int hash, int *pos) {
    if (!table) {
        return NULL;
    }

    HashBucket *bucket = table->table + getIdx(table, hash);
    int length = BKT_GET_CHAIN_LEN(bucket->pointers[0]);

    for (int i = 1; i < length; i++) {
        bucket = (HashBucket *)bucket->pointers[7];
    }

    uint64_t *last = bucket->pointers;
    int curPos = 0;
    if (length == 1) {
        curPos = 1;
    }
    while(curPos < 8 && *last) {
        last++;
        (curPos)++;
    }
    *pos = curPos;
    return last;
}

uint64_t *getLast(HashTable *table, int hash, int *pos) {
    if (!table) {
        return NULL;
    }

    HashBucket *bucket = table->table + getIdx(table, hash);
    int length = BKT_GET_CHAIN_LEN(bucket->pointers[0]);

    for (int i = 1; i < length; i++) {
        bucket = (HashBucket *)bucket->pointers[7];
    }
    uint64_t *last = bucket->pointers;

    if (length == 1 && last[1] == 0) { // if whole chain is empty
        return NULL;
    }

    int curPos = 0;
    while(curPos < 7 && *(last+1)) {
        last++;
        (curPos)++;
    }
    if (pos) {
        *pos = curPos;
    }
    return last;
}

void buildCachePtr(uint64_t *ptr, uint64_t hash, uint64_t heap, uint64_t idx, uint64_t offset) {
    PTR_SET_TAG((*ptr), ADDITIONAL_HASH(hash));
    PTR_SET_SEG_IDX((*ptr), idx);
    PTR_SET_SEG_OFFSET((*ptr), offset);
    PTR_SET_COUNTER((*ptr), 0);
    PTR_SET_HEAP((*ptr), heap);
}

int swap(uint64_t *first, uint64_t *second) {
    if (!first || !second) {
        return -1;
    }
    if (first == second) {
        return 0;
    }
    uint64_t tmp;
    tmp = *first;
    *first = *second;
    *second = tmp;
    return 0;
}

int addPtr(HashTable *table, char *key, int heap, int seg, int offset) {
    if (!table || !key) {
        return -1;
    }
    if (heap > table->countHeaps) {
        return -1;
    }
    /*FIND POSITION*/
    uint32_t hash = hashLookup(key);
    int pos;
    uint64_t *cachePtr = getEnd(table, hash, &pos);
    if (pos == 8) {
        cachePtr--;
        /*ADD NEW BUS IN CHAIN*/
        HashBucket *next = (HashBucket *)calloc(1, sizeof(HashBucket));
        next->pointers[0] = (uint64_t)next;
        swap(cachePtr, next->pointers);
        cachePtr = next->pointers + 1;
        buildCachePtr(cachePtr, hash, heap, seg, offset);
        /*CHANGE INFO IN BUCKET*/
        HashBucket bkt = table->table[getIdx(table,hash)];
        int length = BKT_GET_CHAIN_LEN(bkt.pointers[0]);
        BKT_SET_CHAIN_LEN(bkt.pointers[0], length + 1);
    } else {
        buildCachePtr(cachePtr, hash, heap, seg, offset);
    }
    return 0;
}


uint64_t *getPtr(HashTable *table, char *key, int heap, int seg, int offset, int *pos) {
    if (!table || !key ) {
        return NULL;
    }

    /*FIND POSITION IN  CHAIN*/
    uint32_t hash = hashLookup(key);
    HashBucket *bucket = table->table + getIdx(table, hash);
    uint64_t *ptr = bucket->pointers;
    int curBus = 0;
    int countBusses = BKT_GET_CHAIN_LEN(ptr[0]);

    if (pos) {
        *pos = -1;
    }
    int mod = 0;
    
    ptr++; //start with bus[1] because bus[0] in first bus is bucket info
    while(curBus < countBusses && *ptr) {
        if (PTR_GET_SEG_IDX((*ptr)) == seg && PTR_GET_SEG_OFFSET((*ptr)) == offset && PTR_GET_HEAP((*ptr))) {
            *pos = mod;
            break;
        }

        if (mod == 7){
            mod = 0;
            ptr = ((HashBucket *)*ptr)->pointers;
            curBus++;
        } else {
            mod++;
            ptr++;
        }   
    }
    return ptr;
}


int deletePtr(HashTable *table, char *key, int heap, int seg, int offset) {
    if (!table || !key) {
        return -1;
    }
    /*FIND AND SWAP CHOOSED ELEMENT WITH LAST*/
    int pos, lastPos;
    uint64_t *delPtr = getPtr(table, key, heap, seg, offset, &pos);
    if (!delPtr) {
        return -1;
    }
    *delPtr = 0;

    uint32_t hash = hashLookup(key);

    uint64_t *last = getLast(table, hash, &lastPos);
    if (!last) {
        return 0;
    }

    swap(delPtr, last);


    /*DELETE EXCESS BUS AND UPDATE BUCKET INFO*/
    if(lastPos == 0) {
        free(last);
        uint32_t idx = getIdx(table, hash);
        int length = BKT_GET_CHAIN_LEN(table->table[idx].pointers[0]); 
        BKT_SET_CHAIN_LEN(table->table[idx].pointers[0], length);
        uint64_t *delLink = getLast(table, hash, NULL);
        *delLink = 0;
    }
    
    return 0; 
}


int getPtrFreq(HashTable *table, char *key, int heap, int seg, int offset) {
    if (!table || !key) {
        return -1;
    }
    int pos;
    uint64_t *ptr = getPtr(table, key, heap, seg, offset, &pos);
    if (!ptr) {
        return -1;
    }

    return PTR_GET_COUNTER((*ptr));
}

int incrementASFC(uint64_t *ptr) {
    if (!ptr) {
        return -1;
    }
    int counter = PTR_GET_COUNTER((*ptr));
    if(counter < 16) {
        counter++;
    } else if (counter < 128) {
        double probability = 1.0 / (double)counter * 10000;
        srand(time(NULL));
        if(rand() % 10000 < probability) {
            counter++;
        } 
    }
    PTR_SET_COUNTER((*ptr), counter);
    return 0;
}

int updateLastUseTime(uint64_t *ptr) {
    if (!ptr) {
        return -1;
    }
    long int curTime = time(NULL);

    long int lastTime = BKT_GET_TIMESTAMP((*ptr));
    if(curTime > lastTime) {
        BKT_SET_TIMESTAMP((*ptr),curTime);
    }
    return 0;
}

int getItem(HashTable *table, char *key, ItemHeader *header, void **item) {
    if (!table || !key || !header || !item) {
        return -1;
    }
    /*FIND BUCKET*/
    uint32_t hash = hashLookup(key);
    int addHash = ADDITIONAL_HASH(hash);
    HashBucket *bucket = table->table + getIdx(table, hash);
    uint64_t *ptr = bucket->pointers + 1;

    int curBus = 0, countBusses = BKT_GET_CHAIN_LEN(ptr[0]);    
    int mod = 0;
    while(curBus < countBusses && *ptr) {
        if (PTR_GET_TAG((*ptr)) == addHash) {
            /*COMPARE ITEM HEADER FROM HEAP*/
            int heapIdx = PTR_GET_HEAP((*ptr));
            int segIdx = PTR_GET_SEG_IDX((*ptr));
            int offset = PTR_GET_SEG_OFFSET((*ptr));

            Heap *heap = table->heaps[heapIdx];
            ItemHeader *candidat = getItemHeader(heap->begin, segIdx, offset);
    
            if(!candidat) {
                return -1; //unexpected error
            }

            if (strcmp(candidat->key, key) == 0) {
                readItem(heap->begin, segIdx, offset, header, item);
                updateLastUseTime(bucket->pointers);
                printf("new time %lu\n", BKT_GET_TIMESTAMP((*bucket->pointers)));
                freeItemHeader(candidat);
                return 0;
            }
            freeItemHeader(candidat);
        }

        if (mod == 7){
            mod = 0;
            curBus++;
            ptr = ((HashBucket *)*ptr)->pointers;
        } else {
            mod++;
            ptr++;
        }
        
    }
    return -1;
}