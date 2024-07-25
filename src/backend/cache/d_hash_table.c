#include "../../include/heap.h"
#include "../../include/segment.h"
#include "../../include/item.h"
#include "../../include/hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define ADDITIONAL_HASH(hash) ((hash & 0x0fff000000000000) >> 48)

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

HashTable *initTable(int size) {
    HashTable *hashTable = (HashTable *)malloc(sizeof(HashTable));
    hashTable->size = size;
    hashTable->table = (HashBucket *)malloc(sizeof(HashBucket) * size);
    return hashTable;
}

int getIdx(HashTable *table, uint32_t hash) {
    return hash & (table->size - 1);
}

uint64_t *getEnd(HashTable *table, int hash, int *pos) {
    HashBucket *bucket = table->table + getIdx(table, hash);
    int length = BKT_GET_CHAIN_LEN(bucket->pointers[0]);
    for (int i = 1; i < length; i++) {
        bucket = (HashBucket *)bucket->pointers[7];
    }
    uint64_t *last = bucket->pointers;
    *pos = 0;
    while(*pos < 8 && *last) {
        last++;
        (*pos)++;
    }
    return last;
}

uint64_t *getLast(HashTable *table, int hash, int *pos) {
    HashBucket *bucket = table->table + getIdx(table, hash);
    int length = BKT_GET_CHAIN_LEN(bucket->pointers[0]);
    for (int i = 1; i < length; i++) {
        bucket = (HashBucket *)bucket->pointers[7];
    }
    uint64_t *last = bucket->pointers;
    *pos = 0;
    while(*pos < 7 && *(last+1)) {
        last++;
        (*pos)++;
    }
    return last;
}

void buildCachePtr(uint64_t *ptr, uint32_t hash, uint64_t idx, uint64_t offset) {
    PTR_SET_TAG((*ptr), ADDITIONAL_HASH(hash));
    PTR_SET_SEG_IDX((*ptr), idx);
    PTR_SET_SEG_OFFSET((*ptr), offset);
    PTR_SET_COUNTER((*ptr), 0);
}

void swap(uint64_t *first, uint64_t *second) {
    uint64_t tmp;
    tmp = *first;
    *first = *second;
    *second = tmp;
}

void addPtr(HashTable *table, char *key, int idx, int offset) {
    uint32_t hash = hashLookup(key);
    int pos;
    uint64_t *cachePtr = getEnd(table, hash, &pos);
    if (pos == 8) {
        HashBucket *next = (HashBucket *)calloc(1, sizeof(HashBucket));
        next->pointers[0] = (uint64_t)next;
        swap(cachePtr, next->pointers);
        buildCachePtr(next->pointers + 1, hash, idx, offset);
    } else {
        buildCachePtr(cachePtr, hash, idx, offset);
    }
}

uint64_t *getPtr(HashTable *table, char *key, int idx, int offset, int* pos) {
    uint32_t hash = hashLookup(key);
    HashBucket *bucket = table->table + getIdx(table, hash);
    uint64_t *ptr = bucket->pointers;
    *pos = -1;
    int mod = 0;
    while(*ptr) {
        if (PTR_GET_SEG_IDX((*ptr)) == idx && PTR_GET_SEG_OFFSET((*ptr)) == offset) {
            *pos = mod;
            break;
        }

        if (mod == 7){
            mod = 0;
            ptr = ((HashBucket *)*ptr)->pointers;
        } else {
            mod++;
            ptr++;
        }
        
    }
    return ptr;

}

void deletePtr(HashTable *table, char *key, int idx, int offset) {
    int pos;
    uint64_t *delPtr = getPtr(table, key, idx, offset, &pos);
    *delPtr = 0;
    uint64_t *last = getLast(table, hashLookup(key), &pos);
    swap(delPtr, last); 
}

int getPtrFreq(HashTable *table, char *key, int idx, int offset) {
    int pos;
    uint64_t *ptr = getPtr(table, key, idx, offset, &pos);
    return PTR_GET_COUNTER((*ptr));
}

void incrementASFC(uint64_t *ptr) {
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
}

ItemHeader *getItem(HashTable *table, Heap *heap, char *key) {
    uint32_t hash = hashLookup(key);
    int addHash = ADDITIONAL_HASH(hash);
    HashBucket *bucket = table->table + getIdx(table, hash);
    uint64_t *ptr = bucket->pointers;
    int mod = 0;
    while(*ptr) {
        if (PTR_GET_TAG((*ptr)) == addHash) {
            SegmentHeader *segmentHeader = findSegmentHeader(heap, PTR_GET_SEG_IDX((*ptr)));

            void *item = NULL;
            if (FLBIG(segmentHeader->flags)) {
                int fd = getBigSegmentData(heap, PTR_GET_SEG_IDX((*ptr)));
                fd = getBigSegItem(fd, PTR_GET_SEG_OFFSET((*ptr)));
                char * candidatKey = getBigSegItemKey(fd);
                if (strcmp(key, candidatKey) == 0) {
                    int size = getBigSegItemSize(fd);
                    item = malloc(size);
                    read(fd, item, size);
                    close(fd);
                    free(candidatKey);
                    incrementASFC(ptr);
                    return item;
                }
                free(candidatKey);                
            } else {
                void *segment = getSegmentData(heap, PTR_GET_SEG_IDX(*ptr));
                ItemHeader *header = getSegItem(segment, PTR_GET_SEG_OFFSET((*ptr)));
                char *candidatKey = getItemKey(header);
                if(strcmp(key, candidatKey) == 0) {
                    int size = getItemSize(header);
                    item = malloc(size);
                    for(int i = 0; i < size; i++) {
                        *((char *)item + i) = *((char *)header + i);
                    }
                    free(candidatKey);
                    incrementASFC(ptr);
                    return item;
                }
                free(candidatKey);
            }
            

        }

        if (mod == 7){
            mod = 0;
            ptr = ((HashBucket *)*ptr)->pointers;
        } else {
            mod++;
            ptr++;
        }
        
    }
    return NULL;   
}