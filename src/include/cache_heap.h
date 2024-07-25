/*!
    \file cache_heap.h
    \brief heap for cache
    \version 1.0
    \date 24 Jul 2024

    This file contains interface of heap for cache.

    IDENTIFICATION
        src/include/cache_heap.h
*/

#ifndef HEAP_H
#define HEAP_H

#include <unistd.h>
#include <stdlib.h>


/*!
    \struct SegHeap
    \brief Heap of segments 
    This structure is used by user queries, but isn't used in low level.
    Heap contains and manage segmnets - main units in cache space.
    Whole segments in one heap have the same size. Heap have a freePool -
    linked list of available free segments.
*/
typedef struct SegHeap {
    void *begin;
    void *RAMsegs; ///< for segments in DRAM
    char *path; ///< for segments in PMem
    unsigned int size;
    unsigned int curCount;
    unsigned int segmentSize;
    unsigned int nextFree;
} Heap;

/*!
    \struct SegmentHeaderData
    \brief Header of segment
    This structure is user interface in high-level application queries
    for work with segment's header. Segment unites items by ttl  and
    start timestamp. 
*/
typedef struct SegmentHeaderData {
    void *begin;
    unsigned int timestamp;
    unsigned short int count;
    unsigned int filledSize;
    unsigned int next;
    unsigned char flags;
} SegmentHeader;

//SEGMENT HEADER FLAGS
#define SEGDELETE(flags) (flags & (0x1)) 
#define SEGMERGE(flags) (flags & (0x1 << 1))
#define SEGEXPIRED(flags) (flags & (0x1 << 2))

/*!
    \struct ItemHeaderData
    \brief Header for item 
    User interface of header item. Could be used only in high-level.
*/
typedef struct ItemHeaderData {
    char *key;
    unsigned int valueSize;
    unsigned char keySize;
    unsigned char flags;
} ItemHeader;

//ITEM HEADER FLAGS
#define ITEMFLDELETE(flags) (flags & 0x1)


//HEAP INTERFACE FUNCTIONS

/*!
    \brief initializes heap
    \param[in] segmentSize Size of one segment without header
    \param[in] bootSize Count of segments that will be allocated at start
    \param[in] cacheDir Path to cache directory (NULL if RAM cache heap)
    \return Pointer to new heap if success, NULL else
*/
Heap *initHeap(unsigned int segmentSize, unsigned int bootSize, const char *cacheDir);

/*!
    \brief exetends heap space
    Map new space for segments.
    \param[out] heap Pointer to heap where will be updated state of heap
    \param[in] begin Pointer to begin of heap memory region
    \param[in] count Count of segments that will be allocated
    \return 0 if success, -1 else
*/
int extendHeap(Heap *heap, void *begin, unsigned int count);

/*!
    \brief frees heap 
    \param[in] begin Pointer to begin of heap memory region
*/
void freeHeap(void * begin);

/*!
    \brief Takes new segment number
    Pop segment number from free pool
    \param[out] heap Pointer to heap where will be updated state of heap
    \param[in] begin Pointer to begin of heap memory region
    \return number of segment if success, -1 else 
*/
unsigned int allocateSegment(Heap *heap, void *begin);

/*!
    \brief Frees segment
    Push segment number to free pool
    \param[out] heap Pointer to heap where will be updated state of heap
    \param[in] begin Pointer to begin of heap memory region
    \param[in] num Number of segment
    \return 0 if success, -1 else
*/
int freeSegment(Heap *heap, void *begin, unsigned int num);

/*!
    \brief gets segment header with set numer
    \param[in] begin Pointer to begin of heap memory region
    \param[in] num Segment number
    \return Pointer to segment header if success, NULL else
*/
SegmentHeader *getSegmentHeader(void *begin, int num);

/*!
    \brief sets segment header 
    \param[in] header Pointer to header value
    \return 0 if success, -1 else
*/
int setSegmentHeader(SegmentHeader *header);



/*!
    \brief gets Item header 
    \param[in] begin Pointer to begin of heap memory region
    \param[in] num Number of segment
    \param[in] offset Offset in segment
    \return Pointer to ItemHeader if success, NULL else 
*/
ItemHeader *getItemHeader(void *begin, int num, int offset);

/*!
    \brief sets item header
    \param[in] begin Pointer to begin of heap memory region
    \param[in] num Number of segment
    \param[in] offset Offset in segment
    \param[in] heder Pointer to item header value
    \return 0 if success, -1 else
*/
int setItemHeader(void *begin, int num, int offset, ItemHeader *header);

/*!
    \brief Adds new item in segment
    \param[in] begin Pointer to begin of heap memory region
    \param[in] num Number of segment
    \param[in] header Pointer to item header value
    \param[in] value Pointer to item value
    \return offset if success, -1 if memory not enough, -2 else
*/
unsigned int addItem(void *begin, int num, ItemHeader *header, void * value);


/*!
    \brief Reads item from segment
    \param[in] begin Pointer to begin of heap memory region
    \param[in] num Number of segment
    \param[in] offset Offset in segment
    \param[out] header Pointer to result header
    \param[out] value Pointer to result pointer to value
    \return 0 ifsuccess, -1 else
*/
int readItem(void *begin, int num, int offset, ItemHeader *header, void **value);


///< \todo merge() зависит от реализации поэтому надо его реализовать эффективно внутри


#endif   //HEAP_H