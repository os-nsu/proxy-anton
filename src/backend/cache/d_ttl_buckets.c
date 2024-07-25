#include "../../include/d_heap.h"
#include "../../include/d_segment.h"
#include "../../include/d_item.h"
#include "../../include/d_ttl_buckets.h"
#include "../../include/d_hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



TTLTable *initTTLTable(void) {
    TTLTable *table = (TTLTable *)malloc(sizeof(TTLTable));
    table->mergeIdx = 0;
    int factor = 1;
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 1024; i++) {
            if (j ==0 && i == 0) {
                table->table[0].minTTL = 1;
                table->table[0].maxTTL = 8;
            } else {
                table->table[j * 256 + i].minTTL = table->table[j * 256 + i - 1].maxTTL + 1;
                table->table[j * 256 + i].maxTTL = table->table[j * 256 + i].minTTL + 8 * factor - 1;
            }
            table->table[j * 256 + i].headSeg = -1;
            table->table[j * 256 + i].tailSeg = -1;
        }
        factor *= 16;
    }
    return table;
}

void writeCache(TTLTable *ttlTable, Heap *heap, HashTable *hashTable, int ttl, char *key, int valueSize, void *value) {
    int resIdx = -1, idx = (ttl - 1) / 8;

    /*GET INDEX OF TTL BUCKET*/
    for(int i = 0; i < 3; i++){
        if (idx > 255) {
            idx = (idx - 255) / 16;
        } else {
            resIdx = i * 256 + idx; 
            break;
        }
    }
    if (idx > 255) {
        return;
    }
    if (resIdx == -1) {
        resIdx = 768 + idx; 
    }

    int isBig = valueSize > heap->segmentSize ? 1 : 0;

    /*GET SEGMENT NUMBER*/
    int segNum = -1;
    if(ttlTable->table[resIdx].tailSeg == -1) {
        int segNum = getNum(heap, isBig);

        while (segNum == -1) {
            evictSegments(ttlTable, heap, hashTable);
            segNum = getNum(heap, isBig);
        }

        ttlTable->table[resIdx].headSeg = segNum;
        ttlTable->table[resIdx].tailSeg = segNum;
    } else {
        segNum = ttlTable->table[resIdx].tailSeg;
    }

    /*TRY TO PASTE (AND ADD NEW SEGMENT IF FAIL)*/
    SegmentHeader *segmentHeader = findSegmentHeader(heap, segNum);
    if( FLBIG(segmentHeader->flags) ) {
        
    }
}

void evictSegments(TTLTable *TTLtable, Heap *heap, HashTable *hashTable);

void mergeSegments(TTLTable *table, Heap *heap, HashTable *hashTable);
