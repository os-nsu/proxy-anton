#ifndef SEGMENT_H
#define SEGMENT_H

#include "d_item.h"
#include <unistd.h>
#include <stdlib.h>

typedef struct SegmentHeaderData {
    unsigned int timestap: 32;
    unsigned short int count : 16;
    unsigned int offset : 24;
    unsigned int next : 24;
    unsigned char flags;
} SegmentHeader;




#define FLLINKED(flags) (flags & 0x1)
#define FLDELETE(flags) (flags & (0x1 << 1))
#define FLBIG(flags) (flags & (0x1 << 2)) 

#define FLAGBIG (0x1 << 2)
#define FLAGDELETE (0x1 << 1)

void initSegmentHeader(SegmentHeader *begin, int isBig, int next);

void setSegmentFree(SegmentHeader * begin, int next);

void setSegmentBusy(SegmentHeader * begin, int next);

void addItem(SegmentHeader *segment, int dataSize);

ItemHeader *findSegItem(SegmentHeader *segmentHeader, void *segment, char *key);

int findBigSegItem(SegmentHeader *segmentHeader, int segment, char *key);

ItemHeader *getSegItem(void *segment, int offset);

int getBigSegItem(int segment, int offset);

 


#endif //SEGMENT_H