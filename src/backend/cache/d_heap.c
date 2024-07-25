#include "../../include/d_heap.h"
#include "../../include/d_segment.h"
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


/*!
    \brief Makes path to segment cache file
    \param[in] cacheDir Path to cache directory
    \param[in] num Number of segment
    \return cstring with path to segment cache file
*/
char *mkCachePath(char *cacheDir, int num) {
    int fileNameLen = 2, i = 10;
    while(i <= num) {
        i *= 10;
        fileNameLen ++;
    }

    char *fileName = (char *)calloc(fileNameLen + 6, sizeof(char));
    int j = 0;
    while(i / 10 > 0) {
        fileName[j] = (num % i) / (i / 10);
        j++;
        i /= 10;
    }
    strcpy(fileName + fileNameLen - 1, ".cache");

    char *slash = "";
    if (cacheDir[strlen(cacheDir)-1] != '/') {
        slash = "/";
    }

    char *path = calloc(strlen(cacheDir) + strlen(slash) + fileNameLen + 1, sizeof(strlen));
    strcat(strcat(strcat(path, cacheDir), slash), fileName);
    free(fileName);
    return path;
}


/*!
    \brief Maps space for heap and initializes management structures 
    \param[in] segmentSize Size of one segment's payload
    \param[in] bigSegmentSize Size of one big segment's payload
    \param[in] bootSegCount Boot count of segments in heap
    \param[in] bootBigSegCount Boot count of big segments in heap
    \param[in] cacheDir Cstring with path to cache directory
    \return Pointer to heap structure 
*/
Heap *initHeap(int segmentSize, int bigSegmentSize, int bootSegCount, int bootBigSegCount, char *cacheDir) {
    void *start;

    /*PREPARE HEAP'S FIELDS*/
    int cachePathSize;
    if (cacheDir) {
        cachePathSize = strlen(cacheDir);
    } else {
        cachePathSize = 0;
    }

    start = mmap(NULL, sizeof(Heap) + cachePathSize + 1 + 12 + (sizeof(SegmentHeader)) * (bootSegCount + bootBigSegCount), //12 - sizeof(int) + sizeof(pointer) - for list of allocated memory
    PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,  -1, 0);
    printf("init segHeaders %p - %p\n", start, (char *)start + sizeof(Heap) + cachePathSize + (sizeof(SegmentHeader)) * (bootSegCount + bootBigSegCount));

    Heap *heap = (Heap *)start;
    heap->segmentsCount = bootSegCount;
    heap->bigSegmentsCount = bootBigSegCount;
    heap->nextFree = 0;
    heap->nextBigFree = bootSegCount;
    if (cacheDir) {
        heap->pathSize = strlen(cacheDir);
    } else {
        heap->pathSize = 0;
    }
    heap->segHeaders = (char *)heap + sizeof(Heap) + heap->pathSize + 1;
    heap->segmentSize = segmentSize;
    heap->bigSegmentSize = bigSegmentSize;
    /*MAP MEMORY FOR DRAM SEGMENTS*/
    heap->RAMsegs = mmap(NULL, segmentSize * bootSegCount + 16, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("init RAMSegs %p - %p\n", heap->RAMsegs, (char *)heap->RAMsegs + segmentSize * bootSegCount);
    
    for (int i = 0; i <= heap->pathSize; i++) {
        *((char*)heap + sizeof(Heap) + i) = cacheDir[i]; 
    }

    /*SET PARAMETERS FOR LIST OF MAPPED MEMORY*/
    *(int *)((char*)heap + sizeof(Heap) + heap->pathSize + 1) = bootSegCount + bootBigSegCount; // max_index in this area + 1
    *(void **)((char*)heap + sizeof(Heap) + heap->pathSize + 1 + 4) = NULL; //pointer to next area

    *(int *)((char*)heap->RAMsegs) = 0; //minimal index
    *(int *)((char*)heap->RAMsegs + 4) = bootSegCount; //maximal index + 1
    *(void **)((char*)heap->RAMsegs + 8) = NULL; //pointer to next area of DRAM segments
    
    /*INIT SEGMENT HEADERS*/
    for (int i = 0; i < bootSegCount - 1; i++) {
        initSegmentHeader((SegmentHeader *)((char *)heap->segHeaders + 12 + sizeof(SegmentHeader) * i), 0, i + 1 );
    }
    initSegmentHeader((SegmentHeader *)((char *)heap->segHeaders + 12 + sizeof(SegmentHeader) * (bootSegCount - 1)), 0, -1 );
    
    for (int i = 0; i < bootBigSegCount - 1; i++) {
        initSegmentHeader((SegmentHeader *)((char *)heap->segHeaders + 12 + sizeof(SegmentHeader) * (i + bootSegCount)), 1, bootSegCount + i + 1 );
    }
    initSegmentHeader((SegmentHeader *)((char *)heap->segHeaders + 12 + sizeof(SegmentHeader) * (bootSegCount + bootBigSegCount - 1)), 1, -1 );
    
    return heap;
}


/*!
    \brief Maps additional space for cache
    \param[in] heap Pointer to heap
    \param[in] count Count of new segments
    \param[in] isBig Flag that defines type of segment ((0 = std, 1 = big))
*/
void addSpace(Heap *heap, int count, int isBig) {
    /*MAP NEW SPACES*/
    void *nextPartHeaders = mmap((char *)heap + sizeof(Heap) + heap->pathSize + 1 + sizeof(SegmentHeader) * (heap->segmentsCount + heap->bigSegmentsCount),\
    count * sizeof(SegmentHeader) + 12, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("added headers %p - %p\n", nextPartHeaders, (char *)nextPartHeaders + count * sizeof(SegmentHeader));

    /*SET PARAMETERS FOR LIST OF MAPPED MEMORY*/
    *(int *)((char*)nextPartHeaders) = heap->segmentsCount + heap->bigSegmentsCount + count; // max_index in this area + 1
    *(void **)((char*)nextPartHeaders + 4) = NULL; //pointer to next area

    void **nextArea = (void **)((char *)heap + sizeof(Heap) + heap->pathSize + 1 + 4);
    while(*nextArea != NULL) {
        nextArea = (void **)((char *)*nextArea + 4);
    }
    *nextArea = nextPartHeaders; //push new area to list
    


    if (!isBig) {
        void *nextPart = mmap((char *)heap->RAMsegs + heap->segmentsCount * heap->segmentSize,\
        count * heap->segmentSize + 16, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
        printf("added RAMsegs %p - %p\n", nextPart, (char *)nextPart + count * heap->segmentSize);

        /*SET PARAMETERS FOR LIST OF MAPPED MEMORY*/

        *(int *)(nextPart) = heap->segmentsCount; //minimal index
        *(int *)(nextPart + 4) = heap->segmentsCount + count; //maximal index + 1
        *(void **)(nextPart + 8) = NULL; //pointer to next area of DRAM segments

        void **nextSegArea = (void **)((char *)heap->RAMsegs + 8);
        while(*nextSegArea != NULL) {
            nextSegArea = (void **)((char *)*nextSegArea + 8);
        }
        *nextSegArea = nextPart; //push new area to list

    }
    
    /*INIT STRUCTURES*/
    for (int i = 0; i < count - 1; i++) {
        initSegmentHeader(nextPartHeaders + 12 + sizeof(SegmentHeader) * (i),\
        isBig, (heap->segmentsCount + heap->bigSegmentsCount + i + 1));
    }

    if(isBig) {
        initSegmentHeader(nextPartHeaders + sizeof(SegmentHeader) * (count - 1),\
        isBig, (heap->nextBigFree));
        heap->nextBigFree = heap->segmentsCount + heap->bigSegmentsCount;
        heap->bigSegmentsCount += count;
    } else {
        initSegmentHeader(nextPartHeaders + sizeof(SegmentHeader) * (count - 1),\
        isBig, (heap->nextFree));
        heap->nextFree = heap->segmentsCount + heap->bigSegmentsCount;
        heap->segmentsCount += count;
    }
}

SegmentHeader *findSegmentHeader(Heap *heap, int num) {
    int maxIdx = *(int *)((char *)heap + sizeof(Heap) + heap->pathSize + 1);
    int lastIdx = 0;
    void **nextArea = (void **)((char *)heap + sizeof(Heap) + heap->pathSize + 1 + 4);
    while (maxIdx <= num) {
        lastIdx = maxIdx;
        nextArea = (void **)((int *)(*nextArea) + 1);
        maxIdx = *((int *)nextArea - 1);
    }
    return (SegmentHeader *)((char *)nextArea + 8 + (num - lastIdx) * sizeof(SegmentHeader));
}

void addNumToFreePool(Heap *heap, int num) {
    SegmentHeader *segment = findSegmentHeader(heap, num); 
    if (FLBIG(segment->flags)){
        setSegmentFree(segment, heap->nextBigFree);
        heap->nextBigFree = num;
        char *cacheDir = (char *)calloc(heap->pathSize + 1, sizeof(char));
        for (int i = 0; i < heap->pathSize; i++) {
            cacheDir[i] = *((char*)heap + sizeof(Heap) + i);
        }
        char *cachePath = mkCachePath(cacheDir, num);
        remove(cachePath);
        free(cacheDir);
    } else {
        setSegmentFree(segment, heap->nextFree);
        heap->nextFree = num;
    }    
}

int getNum(Heap *heap, int isBig){
    int result;
    if (isBig){
        result = heap->nextBigFree;
        SegmentHeader *segment = findSegmentHeader(heap, heap->nextBigFree);
        heap->nextBigFree = segment->next;
        setSegmentBusy(segment, -1);
    } else {
        result = heap->nextFree;
        SegmentHeader *segment = findSegmentHeader(heap, heap->nextFree);
        heap->nextFree = segment->next;
        setSegmentBusy(segment, -1);
    }
    return result;
}

void *getSegmentData(Heap *heap, int num) {
    int maxIdx = *(int *)((char *)heap->RAMsegs + 4);
    int minIdx = 0;
    void **nextArea = (void **)((char *)heap->RAMsegs + 8);
    while (maxIdx <= num) {
        maxIdx = *(int *)((char *)(*nextArea) + 4);
        nextArea = (void **)((char *)(*nextArea) + 8);
    } 
    minIdx = *((int *)nextArea - 2);  
    return  (char *)nextArea + 8 + (num - minIdx) * heap->segmentSize;
}

int getBigSegmentData(Heap *heap, int num) {
    char *cacheDir = (char *)calloc(heap->pathSize + 1, sizeof(char));
    for (int i = 0; i < heap->pathSize; i++) {
        cacheDir[i] = *((char*)heap + sizeof(Heap) + i);
    }
    char *cachePath = mkCachePath(cacheDir, num);
    int fd = open(cachePath, O_CREAT | O_RDWR);
    free(cachePath);
    free(cacheDir);
    return fd;
}




