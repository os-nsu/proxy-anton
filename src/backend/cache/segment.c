#include "../../include/segment.h"
#include "../../include/item.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


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

void addItem(SegmentHeader * segment, int dataSize) {
    segment->count ++;
    segment->offset += dataSize + sizeof(ItemHeader);
}