#include "../../include/d_segment.h"
#include "../../include/d_item.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

void initSegmentHeader(SegmentHeader *begin, int isBig, int next) {
    begin->count = 0;
    begin->flags = FLAGDELETE;
    if(isBig){
        begin->flags |= FLAGBIG;
    }
    begin->offset = 0;
    begin->timestap = time(NULL);
    begin->next = next;
}

void setSegmentFree(SegmentHeader * begin, int next) {
    begin->flags |= FLAGDELETE;
    begin->next = next;
}

void setSegmentBusy(SegmentHeader * begin, int next) {
    begin->flags &= ~FLAGDELETE ;
    begin->next = next;
}

ItemHeader *findSegItem(SegmentHeader *segmentHeader, void *segment, char *key) {
    ItemHeader *curHeader = (ItemHeader *)segment;
    while((char *)curHeader - (char *)segment < segmentHeader->offset) {
        if (strcmp((char *)curHeader + sizeof(ItemHeader), key) == 0) {
            return curHeader;
        } else {
            curHeader = (ItemHeader *)((char *)curHeader + sizeof(ItemHeader) + curHeader->keySize + 1 + curHeader->valueSize);
        }
    }
    return NULL;
}

int findBigSegItem(SegmentHeader *segmentHeader, int segment, char *key) {
    int keyLen = strlen(key);
    ItemHeader candidat;
    int curPos;
    while ((curPos = lseek(segment, 0, SEEK_CUR)) < segmentHeader->offset) {
        read(segment, &candidat, sizeof(ItemHeader));
        if(keyLen == candidat.keySize) {
            char *candidatKey = (char *)calloc(candidat.keySize + 1, sizeof(char));
            read(segment, candidatKey, candidat.keySize + 1);
            if (strcmp(candidatKey, key) == 0) {
                lseek(segment, -(sizeof(ItemHeader) + candidat.keySize + 1), SEEK_CUR);
                free(candidatKey);
                return segment;
            }
            lseek(segment, -(candidat.keySize + 1), SEEK_CUR);
            free(candidatKey);
        }
        lseek(segment, candidat.keySize + 1 + candidat.valueSize, SEEK_CUR);        
    }
    return -1;
}

void addItem(SegmentHeader * segment, int dataSize) {
    segment->count ++;
    segment->offset += dataSize + sizeof(ItemHeader);
}


ItemHeader *getSegItem(void *segment, int offset) {
    return (ItemHeader *)((char *)segment + offset);
}

int getBigSegItem(int segment, int offset) {
    lseek(segment, offset, SEEK_SET);
    return segment;
}