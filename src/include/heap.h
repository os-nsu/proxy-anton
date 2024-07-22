#ifndef HEAP_H
#define HEAP_H


typedef struct SegHeap {
    int capacity;
    int segmentsCount;
    int bigSegmentsCount;
    int segmentSize;
    int bigSegmentSize;
    void *segHeaders;
    void *RAMsegs;
    int nextFree;
    int nextBigFree;
    int pathSize;
} Heap;


Heap *initHeap(int segmentSize, int bigSegmentSize, int bootSegCount, int bootbigSegCount, char *cacheDir);

void addSpace(Heap *heap, int count, int isBig);

void addNumToFreePool(Heap *heap, int num);

void *getSegmentData(Heap *heap, int num);

int getBigSegmentData(Heap *heap, int num);


#endif   //HEAP_H