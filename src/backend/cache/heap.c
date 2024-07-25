/*!
    \file heap.c
    \brief heap for cache
    \version 1.0
    \date 25 Jul 2024

    This file contains implementation of segment heap.
    Heap consists of segments. Each segment have a header and fix-sized body.
    Each segment contains items. Each item also has header and body.

    Memory work like allocte or free occurres on segment layer.
    Items could be added, be marked as deleted or be read. (couldn't be updated)

    IDENTIFICATION
        src/backend/cache/heap.c
*/

#include "../../include/cache_heap.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>


int fdtest;

//FUNCTION ADVERTISEMENTS

/*!
    \brief initializes heap
    \param[in] segmentSize Size of one segment without header
    \param[in] bootSize Count of segments that will be allocated at start
    \param[in] cacheDir Path to cache directory (NULL if RAM cache heap)
    \return Pointer to new heap if success, NULL else
*/
Heap *initHeap(unsigned int segmentSize,unsigned int bootSize, const char *cacheDir);

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
    \brief Gets pointer to begin of segment Header
    \param[in] begin Pointer to begin of heap memory
    \param[in] num Segment's index
    \return pointer to begin of segment's header if success, NULL else
*/
void *getSegmentHeaderBegin(void *begin, unsigned int num);

/*!
    \brief Gets pointer to begin of segment
    \param[in] begin Pointer to begin of heap memory
    \param[in] num Segment's index
    \return pointer to begin of segment if success, NULL else
*/
void *getRAMSegmentBegin(void *begin, unsigned int num);

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

/*!
    \brief Makes path to segment cache file
    \param[in] cacheDir Path to cache directory
    \param[in] num Number of segment
    \return cstring with path to segment cache file
*/
char *mkCachePath(char *cacheDir, int num);

/*!
    \brief Emphasizes parent folder path from path
    \param[in] path Path
    \return cstring with path to parent folder if success, NULL else
*/
char *getParentFolderPath(const char *path);

/*!
    \brief Initializes segment header
    \param[in] begin Begin ponter to segment header
    \param[in] next Number of next segment (0xffffffff if next is absent)
    \return 0 if success, -1 else
*/
int initSegmentHeader(void *begin, unsigned int next);

/*!
    \brief updates heap managemnet structure
    \param[out] heap Pointer to heap where will be updated state of heap
    \param[in] begin Pointer to begin of heap memory region 
*/
void updateHeapInfo(Heap *heap, void *begin);





///HEAP HEADER 

///< | size 4B | cur count 4B | segment size 4B | next free num 4B | RAM segments ptr 8B | cache path ptr 8B | 

#define getHHSize(begin) (*(uint32_t *)begin)

#define getHHCurCount(begin) (*((uint32_t *)begin + 1))

#define getHHSegSize(begin) (*((uint32_t *)begin + 2))

#define getHHNext(begin) (*((uint32_t *)begin + 3))

#define getHHRAMSegs(begin)  (*((void **)begin + 2))

#define getHHPath(begin)  (*((void **)begin + 3)) 

//SEGMENT HEADERS AREA HEADER

#define getSHAHMinIdx(begin) (*(uint32_t *)begin)

#define getSHAHMaxIdx(begin) (*((uint32_t *)begin + 1))

#define getSHAHNext(begin) (*((void **)begin + 1))

//SEGMENT AREA HEADER 

#define getSAHMinIdx(begin) (*(uint32_t *)begin)

#define getSAHMaxIdx(begin) (*((uint32_t *)begin + 1))

#define getSAHNext(begin) (*((void **)begin + 1))

//SEGMENT HEADER 

///< | next 4B | last use timestamp 4B | filled volume 4B | count items 2B | flags 1B | reserved 1B |

#define getSHNext(begin) (*(uint32_t *)begin)

#define getSHLastUseTime(begin) (*((uint32_t *)begin + 1))

#define getSHFilledSize(begin) (*((uint32_t *)begin + 2))

#define getSHCountItems(begin) (*((uint16_t *)begin + 6))

#define getSHFlags(begin) (*((char *)begin + 14))

//RAM ITEM HEADER 

///< | value size 4B | key size 1B | flags 1B |

#define getRIHValueSize(begin) (*(uint32_t *)begin)

#define getRIHKeySize(begin) (*((char *)begin + 4))

#define getRIHFlags(begin) (*((char *)begin + 5))

char *getRIHKey(void *begin, int keyLength);

//PM ITEM HEADER GETTERS AND SETTERS

unsigned int getPIHValueSize(int fd);

int setPIHValueSize(int fd, unsigned int size);

unsigned char getPIHKeySize(int fd);

int setPIHKeySize(int fd, unsigned char size);

unsigned char getPIHFlags(int fd);

int setPIHFlags(int fd, unsigned char flags);

char *getPIHKey(int fd, int keyLength);




//FUNCTION IMPLEMENTATION

unsigned int getPIHValueSize(int fd) {
    if (fd < 0) {
        return -1;
    }
    int flags = fcntl(fd, F_GETFL);
    if (!((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR )) {
        return -1;
    }
    unsigned int result;
    int readSize = read(fd, &result, 4);
    lseek(fd, -(int32_t)readSize, SEEK_CUR);
    if(readSize != 4) {
        return -1;
    }
    return result;
}

int setPIHValueSize(int fd, unsigned int size) {
    if (fd < 0) {
        return -1;
    }
    int flags = fcntl(fd, F_GETFL);
    if (!((flags & O_ACCMODE) == O_WRONLY || (flags & O_ACCMODE) == O_RDWR )) {
        return -1;
    }
    int writeSize = write(fd, &size, 4);
    lseek(fd, -(int32_t)writeSize, SEEK_CUR);
    if(writeSize != 4) {
        return -1;
    }
    return 0;
}

unsigned char getPIHKeySize(int fd) {
    if (fd < 0) {
        return -1;
    }
    int flags = fcntl(fd, F_GETFL);
    if (!((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR )) {
        return -1;
    }
    lseek(fd, 4, SEEK_CUR);
    unsigned char result;
    int readSize = read(fd, &result, 1);
    lseek(fd, -(int32_t)readSize - 4, SEEK_CUR);
    if(readSize != 1) {
        return -1;
    }
    return result;
}

int setPIHKeySize(int fd, unsigned char size) {
    if (fd < 0) {
        return -1;
    }
    int flags = fcntl(fd, F_GETFL);
    if (!((flags & O_ACCMODE) == O_WRONLY || (flags & O_ACCMODE) ==O_RDWR )) {
        return -1;
    }
    lseek(fd, 4, SEEK_CUR);
    int writeSize = write(fd, &size, 1);
    lseek(fd, -(int32_t)writeSize - 4, SEEK_CUR);
    if(writeSize != 1) {
        return -1;
    }
    return 0;
}

unsigned char getPIHFlags(int fd) {
    if (fd < 0) {
        return -1;
    }
    int flags = fcntl(fd, F_GETFL);
    if (!((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR )) {
        return -1;
    }
    lseek(fd, 5, SEEK_CUR);
    unsigned char result;
    int readSize = read(fd, &result, 1);
    lseek(fd, -(int32_t)readSize - 5, SEEK_CUR);
    if(readSize != 1) {
        return -1;
    }
    return result;
}

int setPIHFlags(int fd, unsigned char flags) {
    if (fd < 0) {
        return -1;
    }
    int fflags = fcntl(fd, F_GETFL);
    if (!((fflags & O_ACCMODE) == O_WRONLY || (fflags & O_ACCMODE) ==O_RDWR )) {
        return -1;
    }
    lseek(fd, 5, SEEK_CUR);
    int writeSize = write(fd, &flags, 1);
    lseek(fd, -(int32_t)writeSize - 5, SEEK_CUR);
    if(writeSize != 1) {
        return -1;
    }
    return 0;
}

char *getPIHKey(int fd, int keyLength) {
    if (fd < 0 || keyLength < 1) {
        return NULL;
    }
    int fflags = fcntl(fd, F_GETFL);
    if (!((fflags & O_ACCMODE) == O_RDONLY || (fflags & O_ACCMODE) == O_RDWR )) {
        return NULL;
    }
    lseek(fd, 6, SEEK_CUR);
    char *key = (char *)calloc(keyLength + 1, sizeof(char));
    read(fd, key, keyLength + 1);
    lseek(fd, - 1 - (int32_t)keyLength - 6, SEEK_CUR);
    return key;
}

char *getRIHKey(void *begin, int keyLength) {
    if (!begin || keyLength < 1) {
        return NULL;
    }
    char *key = (char *)calloc(keyLength + 1, sizeof(char));
    strcpy(key, (char *)begin + 6);
    return key;
}


void updateHeapInfo(Heap *heap, void *begin) {
    if (!heap || !begin) {
        return;
    }
    heap->begin = begin;
    heap->nextFree = *((int *)begin  + 3);
    heap->size = *((int *)begin);
    heap->path = *((char **)begin + 3);
    heap->RAMsegs = *((void **)begin + 2);
    heap->segmentSize = *((int *)begin + 2);
    heap->curCount = *((int *)begin + 1);
}

char *mkCachePath(char *cacheDir, int num){
    int fileNameLen = 2, i = 10;
    while(i <= num) {
        i *= 10;
        fileNameLen ++;
    }

    char *fileName = (char *)calloc(fileNameLen + 6, sizeof(char));
    int j = 0;
    while(i / 10 > 0) {
        fileName[j] = '0' + (num % i) / (i / 10);
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

char *getParentFolderPath(const char *path) {
    if (!path) {
        return NULL;
    }
    int pathLen = strlen(path);
    char *ppath = (char *)calloc(pathLen + 1, sizeof(char));
    strcpy(ppath, path);
    if (ppath[pathLen - 1] == '/') {
        ppath[pathLen - 1] = 0;
    }
    if (strcmp(ppath, ".") == 0) {
        return strcpy(ppath, "../");
    } else if (strlen(ppath) == 0) {
        return NULL; //path is root
    }
    char *c = ppath + strlen(ppath) - 1;
    while (c != ppath - 1 && *c != '/' ) {
        *(c--) = 0;
    }
    if (c == ppath - 1) {
        return NULL;
    }
    char *result = (char *)calloc(strlen(ppath) + 1, sizeof(char));
    strcpy(result, ppath);
    free(ppath);
    return result;
}

int initSegmentHeader(void *begin, unsigned int next) {
    // segment header is | next 4B | last use timestamp 4B | filled volume 4B | count items 2B | flags 1B | reserved 1B |
    *((int *)begin) = next;
    *((int *)begin + 1) = 0;
    *((int *)begin + 2) = 0;
    *((short *)begin + 6) = 0;
    *((char *)begin + 14) = SEGDELETE(0xff);
    return 0;
}




Heap *initHeap(unsigned int segmentSize, unsigned int bootSize, const char *cacheDir) {
    /*MAP REGIONS*/
    int heapHeaderSize = 32; ///< | size 4B | cur count 4B | segment size 4B | next free num 4B | RAM segments ptr 8B | cache path ptr 8B | 
    int segmentHeaderSize = 16; ///< | next 4B | last use timestamp 4B | filled volume 4B | count items 2B | flags 1B | reserved 1B | 
    int segHeadAreaHeadSize = 16;
    void *heapRegion = mmap(NULL, heapHeaderSize + segHeadAreaHeadSize + bootSize * segmentHeaderSize,\
    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fdtest, 0);

    if (!heapRegion) {
        return NULL;
    }

    *((uint32_t *)heapRegion) = bootSize;
    *((uint32_t *)heapRegion + 1) = 0;
    *((uint32_t *)heapRegion + 2) = segmentSize;
    *((uint32_t *)heapRegion + 3) = 0;
    *((void **)heapRegion + 2) = NULL;
    *((void **)heapRegion + 3) = NULL;

    if (cacheDir){
        /*USE PERSISTANCE MEMORY*/

        // check that path exists at list
        if (access(cacheDir, F_OK)){
            if (errno == ENOTDIR) {
                munmap(heapRegion, heapHeaderSize + bootSize * segmentHeaderSize);
                return NULL;
            }
            //try to make dir
            char *parent = getParentFolderPath(cacheDir);
            if (!parent) {
                munmap(heapRegion, heapHeaderSize + bootSize * segmentHeaderSize);
                return NULL; //incorrect path
            }
            if (mkdir(cacheDir, 0777)) {
                munmap(heapRegion, heapHeaderSize + bootSize * segmentHeaderSize);
                return NULL; //cannot create directory
            }

        } else {
            // check that file is folder
            struct stat cacheDirStat;
            memset(&cacheDirStat, 0, sizeof(struct stat));
            stat(cacheDir, &cacheDirStat);
            if((cacheDirStat.st_mode & S_IFMT) != S_IFDIR) {
                munmap(heapRegion, heapHeaderSize + bootSize * segmentHeaderSize);
                return NULL; // not a directory
            }
        }

        char *myCacheDir = (char *)calloc(strlen(cacheDir) + 1, sizeof(char));
        strcpy(myCacheDir, cacheDir);
        *((void **)heapRegion + 3) = myCacheDir; ///< \todo Don't forget to free path when free heap;
        
    } else {
        /*USE DRAM*/
        int areaHeaderSize = 16;
        void *segmentArea = mmap(NULL, areaHeaderSize + bootSize * segmentSize,\
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fdtest, 16 * 512);
        *((void **)heapRegion + 2) = segmentArea;
        if (*(long long *)((void **)heapRegion + 2) == -1LL) {
            munmap(heapRegion, heapHeaderSize + bootSize * segmentHeaderSize);
            return NULL;
        }
        *((int *)segmentArea) = 0; //min index
        *((int *)segmentArea + 1) = bootSize - 1; //max index
        *((void **)segmentArea + 1) = NULL; //next
    }
    /*INIT SEGMENT HEADERS*/
    *((uint32_t *)heapRegion + 8) = 0; // min index
    *((uint32_t *)heapRegion + 9) = bootSize - 1; // max index
    *((void **)heapRegion + 5) = NULL; //next
    void *curHeader = ((uint32_t *)heapRegion + 12);
    for (int i = 0; i < bootSize - 1; i++) {
        initSegmentHeader(curHeader, i + 1);
        curHeader = (char *)curHeader + segmentHeaderSize; 
    }
    initSegmentHeader(curHeader, -1);

    Heap *heap = (Heap *)malloc(sizeof(Heap));
    updateHeapInfo(heap, heapRegion);
    return heap;                                         
}


int extendHeap(Heap *heap, void *begin, unsigned int count) {
    /*MAP NEW MEMORY*/
    int segmentHeaderSize = 16, areaHeaderSize = 16;
    void *nextHeaderArea = mmap(NULL, areaHeaderSize + (segmentHeaderSize * count),\
    PROT_WRITE | PROT_READ, MAP_SHARED | MAP_FILE,  fdtest, 16 * 256);
    
    if((long long)nextHeaderArea == -1LL) {
        return -1;
    }

    if (!*((char **)begin + 3)) {
        /*MAP MEMORY FOR SEGMENTS IF DRAM SEGMENTS*/
        void *segmentArea = mmap(NULL, areaHeaderSize + *((int *)begin + 2) * count,\
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fdtest, 16 * 768);

        if ((long long)(segmentArea) == -1LL) {
            munmap(nextHeaderArea, areaHeaderSize + *((int *)begin + 2) * count);
            return -1;
        }
        *((int *)segmentArea) = *((int *)begin); //min index
        *((int *)segmentArea + 1) = *((int *)begin) + count - 1; //max index
        *((void **)segmentArea + 1) = NULL; //next

        /*PUSH SEGMENTS AREA TO LIST*/
        void **curPtr = (void **)begin + 2;
        while(*(curPtr) != NULL) {
            curPtr = (void **)*curPtr + 1;
        }
        *(curPtr) = segmentArea;
    }

    /*INITIALIZE HEADERS AREA*/
    *((int *)nextHeaderArea) = *(unsigned int *)begin;
    *((int *)nextHeaderArea + 1) = *(unsigned int *)begin + count - 1;
    *((void **)nextHeaderArea + 1) = NULL;

    void *curHeader = (void **)nextHeaderArea + 2;
    for (int i = 0; i < count - 1; i++) {
        initSegmentHeader(curHeader, *(unsigned int *)begin + i + 1);
        curHeader = (char *)curHeader + segmentHeaderSize;
    }

    /*PUSH HEADERS AREA TO LIST*/
    initSegmentHeader(curHeader, *((int *)begin + 3));

    void **curPtr = (void **)begin + 5;
    while(*(curPtr) != NULL) {
        curPtr = (void **)*curPtr + 1; // *ptr have type void *, so void * + 1 adds only 1 byte or UB
    }
    *(curPtr) = nextHeaderArea;
    *((int *)begin + 3) = *((int *)begin);
    *((int *)begin) += count;
    
    updateHeapInfo(heap, begin);
    return 0;
}


void freeHeap(void * begin) {
    void **curHeadAreaPtr, **delHeadAreaPtr;
    int segmentAreaHeadSize = 16, headAreaHeadSize = 16, headerSize = 16, heapHeaderSize = 32;
    char *cacheDir = *((char **)begin + 3);
    if(cacheDir) {
        /*DELETE ALL FILES IN CACHE DIRECTORY*/
        if (!access (cacheDir, F_OK)) {
            struct stat cacheDirStat;
            memset(&cacheDirStat, 0, sizeof(struct stat));
            stat(cacheDir, &cacheDirStat);
            //check that file is directory
            if((cacheDirStat.st_mode & S_IFMT) == S_IFDIR) {
                //if we have access to file we'll delete it
                for(int i = 0; i < *((unsigned int *)begin); i++) {
                    char *cacheName = mkCachePath(cacheDir, i);
                    if (!access(cacheName, F_OK)) {
                        remove(cacheName);
                    }
                    free(cacheName);
                }
            }
        }
        //free path to cache directory
        free(cacheDir);
    } else {
        /*UNMAP AREAS IN LOOP*/
        void **curSegAreaPtr, **delSegAreaPtr;
        int areaLength = 0;
        curSegAreaPtr = *((void **)begin + 2);
        delSegAreaPtr = NULL;
        int i = 0;
        while (curSegAreaPtr != NULL) {
            delSegAreaPtr = curSegAreaPtr;
            if (*curSegAreaPtr) {
                curSegAreaPtr = *(curSegAreaPtr + 1);
            } else {
                curSegAreaPtr = NULL;
            }
            
            areaLength = segmentAreaHeadSize + *((int *)begin + 2) * (*((int *)delSegAreaPtr + 1) + 1 - *(int *)delSegAreaPtr);
            munmap(delSegAreaPtr, areaLength);
        }
    }

    /*UNMAP AREAS IN LOOP*/
    int areaLength = 0;
    curHeadAreaPtr = *((void **)begin + 5);
    delHeadAreaPtr = NULL;
    while (curHeadAreaPtr != NULL) {
            delHeadAreaPtr = curHeadAreaPtr;
            if (*curHeadAreaPtr) {
                curHeadAreaPtr = *(curHeadAreaPtr + 1);
            } else {
                curHeadAreaPtr = NULL;
            }
            areaLength = headAreaHeadSize + headerSize * (*((int *)delHeadAreaPtr + 1) + 1 - *(int *)delHeadAreaPtr);
            munmap(delHeadAreaPtr, areaLength);
        }

    /*UNMAP FIRST REGION*/

    munmap(begin, heapHeaderSize + headAreaHeadSize + headerSize * (*((int *)begin + 9) + 1 - *((int *)begin + 8)));
}

void *getSegmentHeaderBegin(void *begin, unsigned int num) {
    int segmentHeaderSize = 16, SHAHSize = 16;
    if (!begin || (int32_t)num == -1) {
        return NULL;
    }
    /*FIND NEEDED AREA IN LINKED LIST*/
    void **curPtr = (void **)begin + 4;
    int max = getSHAHMaxIdx(curPtr), min = getSHAHMinIdx(curPtr);
    while (curPtr && max < num) {
        curPtr = getSHAHNext(curPtr);
        if(curPtr) {
            max = getSHAHMaxIdx(curPtr);
            min = getSAHMinIdx(curPtr);
        }
    } 
    if (!curPtr) {
        return NULL;
    }
    /*GET OFFSET IN AREA*/
    return (char *)curPtr + SHAHSize + segmentHeaderSize * (num - min);
}

void *getRAMSegmentBegin(void *begin, unsigned int num) {
    int SAHSize = 16;
    if (!begin || (int32_t)num == -1) {
        return NULL;
    }
    /*FIND NEEDED AREA IN LINKED LIST*/
    void **curPtr = getHHRAMSegs(begin);
    int max = getSAHMaxIdx(curPtr), min = getSAHMinIdx(curPtr);
    while (curPtr && max < num) {
        curPtr = getSAHNext(curPtr);
        if(curPtr) {
            max = getSAHMaxIdx(curPtr);
            min = getSAHMinIdx(curPtr);
        }
    } 
    if (!curPtr) {
        return NULL;
    }
    /*GET OFFSET IN AREA*/
    return (char *)curPtr + SAHSize + getHHSegSize(begin) * (num - min);
}


unsigned int allocateSegment(Heap *heap, void *begin) {
    unsigned int num = getHHNext(begin);
    if ((int32_t)num == -1) {
        return -1;
    }
    void *segment = getSegmentHeaderBegin(begin, num); 
    if (!segment) {
        return -1;
    }
    /*UPADATE FREE LIST*/
    getHHNext(begin) = getSHNext(segment);
    getSHNext(segment) = -1;
    getSHFlags(segment) &= ~SEGDELETE(0xff);
    updateHeapInfo(heap, begin);

    /*MAKE FILE IF USING PMEM*/
    if (getHHPath(begin)) {
        char *segmentName = mkCachePath(getHHPath(begin), num);
        if (access(getHHPath(begin), F_OK)) {
            return -1;
        }
        printf("file name: %s\n", segmentName);
        FILE *file = fopen(segmentName,"wb+");
        fclose(file);
    }
    return num;
}


int freeSegment(Heap *heap, void *begin, unsigned int num) {
    if ((int32_t)num == -1) {
        return -1;
    }
    void *segment = getSegmentHeaderBegin(begin, num);
    if (!segment) {
        return -1;
    }
    /*UPADATE FREE LIST*/
    getSHNext(segment) = getHHNext(begin);
    getHHNext(begin) = num;
    updateHeapInfo(heap, begin);

    /*DELETE FILE IF USING PMEM*/
    if (getHHPath(begin) && !access (getHHPath(begin), F_OK)) {
        struct stat cacheDirStat;
        memset(&cacheDirStat, 0, sizeof(struct stat));
        stat(getHHPath(begin), &cacheDirStat);
        //check that file is directory
        if((cacheDirStat.st_mode & S_IFMT) == S_IFDIR) {
            //if we have access to file we'll delete it
            char *cacheName = mkCachePath(getHHPath(begin), num);
            if (!access(cacheName, F_OK)) {
                remove(cacheName);
            }
            free(cacheName);
        }
    }
    return 0;
}

SegmentHeader *getSegmentHeader(void *begin, int num) {
    if (!begin || (int32_t)num == -1) {
        return NULL;
    }
    void *segment = getSegmentHeaderBegin(begin, num);
    if (!segment) {
        return NULL;
    }
    SegmentHeader *segHead = (SegmentHeader *)malloc(sizeof(SegmentHeader));
    segHead->begin = segment;
    segHead->next = getSHNext(segment);
    segHead->timestamp = getSHLastUseTime(segment);
    segHead->filledSize = getSHFilledSize(segment);
    segHead->count = getSHCountItems(segment);
    segHead->flags = getSHFlags(segment);
    return segHead;
}

int setSegmentHeader(SegmentHeader *header) {
    if (!header || !(header->begin)) {
        return -1;
    }
    getSHNext(header->begin) = header->next;
    getSHLastUseTime(header->begin) = header->timestamp;
    getSHFilledSize(header->begin) = header->filledSize;
    getSHCountItems(header->begin) = header->count;
    getSHFlags(header->begin) = header->flags;
    return 0;
}


ItemHeader *getItemHeader(void *begin, int num, int offset) {
    if (!begin || (uint32_t)num == -1 || (uint32_t)offset == -1) {
        return NULL;
    }
    void *segmentHeader = getSegmentHeaderBegin(begin, num);
    if (!segmentHeader) {
        return NULL;
    }
    if (SEGDELETE(getSHFlags(segmentHeader))) {
        return NULL;
    }
    ItemHeader *header = (ItemHeader *)malloc(sizeof(ItemHeader));
    if (getHHPath(begin)) {
        /*READ ITEM HEADER IN FILE*/
        char *segmentName = mkCachePath(getHHPath(begin), num);
        if (access(segmentName, R_OK)) {
            return NULL;
        }
        int segment = open(segmentName, O_RDONLY);
        lseek(segment, offset, SEEK_SET);
        header->valueSize = getPIHValueSize(segment);
        header->keySize = getPIHKeySize(segment);
        header->flags = getPIHFlags(segment);
        header->key = getPIHKey(segment, header->keySize);
        close(segment);
    }  else {
        /*READ ITEM HEADER IN DRAM*/
        void *segment = getRAMSegmentBegin(begin, num);
        if (!segment) {
            return NULL;
        }
        segment = (char*)segment + offset;
        header->valueSize = getRIHValueSize(segment);
        header->keySize = getRIHKeySize(segment);
        header->flags = getRIHFlags(segment);
        header->key = getRIHKey(segment, header->keySize);
    }
    return header;
}


int setItemHeader(void *begin, int num, int offset, ItemHeader *header) {
    if (!header || !begin || (uint32_t)num == -1 || (uint32_t)offset == -1) {
        return -1;
    }
    void *segmentHeader = getSegmentHeaderBegin(begin, num);
    if (!segmentHeader) {
        return -1;
    }
    if (SEGDELETE(getSHFlags(segmentHeader))) {
        return -1;
    }

    if (getHHPath(begin)) {
        /*WRITE ITEM HEADER FLAGS IN FILE*/
        char *segmentName = mkCachePath(getHHPath(begin), num);
        if (access(segmentName, W_OK | R_OK)) {
            return -1;
        }
        int segment = open(segmentName, O_RDWR);
        lseek(segment, offset, SEEK_SET);
        setPIHFlags(segment, header->flags);
        close(segment);
    }  else {
        /*WRITE ITEM HEADER FLAGS IN DRAM*/
        void *segment = getRAMSegmentBegin(begin, num);
        if (!segment) {
            return -1;
        }
        segment = (char*)segment + offset;
        getRIHFlags(segment) = header->flags;
    }
    return 0;
}

unsigned int addItem(void *begin, int num, ItemHeader *header, void * value) {
    int itemHeaderSize = 6;
    if (!begin || (int32_t)num == -1 || !header || !value) {
        return -2;
    }
    void *segmentHeader = getSegmentHeaderBegin(begin, num);
    if (!segmentHeader) {
        return -2;
    }
    if (SEGDELETE(getSHFlags(segmentHeader))) {
        return -2;
    }
    int stop;
    // check that segment have enough space for item
    unsigned int itemSize = itemHeaderSize + header->keySize + 1 + header->valueSize;
    if (getHHSegSize(begin) - getSHFilledSize(segmentHeader) < itemSize) {
        return -1;
    }

    if (getHHPath(begin)) {
        /*ADD ITEM TO SEGMNENT IN PMEM*/
        char *segmentName = mkCachePath(getHHPath(begin), num);
        if (access(segmentName, W_OK | R_OK)) {
            return -2;
        }
        int segment = open(segmentName, O_RDWR);
        lseek(segment, getSHFilledSize(segmentHeader), SEEK_SET);
        char* filler = (char *)calloc(itemSize, sizeof(char));
        lseek(segment, -(int32_t)itemSize, SEEK_CUR);

        setPIHValueSize(segment, header->valueSize);
        setPIHKeySize(segment, header->keySize);
        setPIHFlags(segment,header->flags);

        lseek(segment, itemHeaderSize, SEEK_CUR);
        write(segment, header->key, header->keySize + 1);
        write(segment, value, header->valueSize);

        close(segment);
    } else {
        /*ADD ITEM TO SEGMENT IN DRAM*/
        void *segment = getRAMSegmentBegin(begin, num);
        if (!segment) {
            return -1;
        }
        segment = (char*)segment + getSHFilledSize(segmentHeader);
        getRIHKeySize(segment) = header->keySize;
        getRIHValueSize(segment) = header->valueSize;
        getRIHFlags(segment) = header->flags;
        segment = (char *)segment + itemHeaderSize;
        strcpy(segment, header->key);
        segment = (char *)segment + header->keySize;
        *(char *)segment = 0;
        segment = (char *)segment + 1;
        for (int i = 0; i < header->valueSize; i++) {
            *((char *)segment + i) = *((char *)value + i);
        }
    }

    /*CHANGE INFO IN SEGMENT HEADER*/
    getSHFilledSize(segmentHeader) += itemSize;
    getSHCountItems(segmentHeader) += 1;

    return getSHFilledSize(segmentHeader) - itemSize;
}


int readItem(void *begin, int num, int offset, ItemHeader *header, void **value) {
    if (!begin || (uint32_t)num == -1 || (uint32_t)offset == -1) {
        return -1;
    }
    void *segmentHeader = getSegmentHeaderBegin(begin, num);
    if (!segmentHeader) {
        return -1;
    }
    if (SEGDELETE(getSHFlags(segmentHeader))) {
        return -1;
    }
    if (getHHPath(begin)) {
        /*READ ITEM HEADER IN FILE*/
        char *segmentName = mkCachePath(getHHPath(begin), num);
        if (access(segmentName, R_OK)) {
            return -1;
        }
        int segment = open(segmentName, O_RDONLY);
        lseek(segment, offset, SEEK_SET);
        printf("valueSize %d\n", getPIHValueSize(segment));
        header->valueSize = getPIHValueSize(segment);
        header->keySize = getPIHKeySize(segment);
        header->flags = getPIHFlags(segment);
        header->key = getPIHKey(segment, header->keySize);
        *value = malloc(header->valueSize);
        lseek(segment, 7 + header->keySize, SEEK_CUR);
        read(segment, *value, header->valueSize);
        close(segment);
    }  else {
        /*READ ITEM HEADER IN DRAM*/
        void *segment = getRAMSegmentBegin(begin, num);
        if (!segment) {
            return -1;
        }
        segment = (char*)segment + offset;
        printf("segment ptr %p\n",segment);
        header->valueSize = getRIHValueSize(segment);
        header->keySize = getRIHKeySize(segment);
        header->flags = getRIHFlags(segment);
        header->key = getRIHKey(segment, header->keySize);
        printf("'%d %d %d'\n%p\n%s\n", *(int *)segment, *((char*)segment + 4), *((char*)segment + 5), segment,(char*)segment + 6);
        *value = (char *)segment + header->keySize + 7;
    }
    return 0;
}





/* test test test
int main() {
    fdtest = open("fileWithHeap.bin", O_RDWR);
    Heap *heap = initHeap(1024, 250, "./test");
    //printf("extend: %d\n", extendHeap(heap, heap->begin, 250));
    int c;
    scanf("%d",&c);
    int num = allocateSegment(heap, heap->begin);
    scanf("%d",&c);
    char *greeting = "hello world from cache!\n";
    ItemHeader *item = (ItemHeader *)malloc(sizeof(ItemHeader));
    item->flags = 0;
    item->key = "myval";
    item->keySize = strlen(item->key);
    item->valueSize = strlen(greeting) + 1; 
    unsigned int offset = addItem(heap->begin, num, item, greeting);
    if ((int32_t)offset == -2) {
        printf("mem not enough!\n");
        return 0;
    } else if ((int32_t)offset == -1) {
        printf("err while added item\n");
        return 0;
    }
    char *res;
    memset(item,0,sizeof(ItemHeader));
    readItem(heap->begin,num, offset, item, (void **)&res);
    printf("item:\nkeysize: %d\nvalSize %d\nflags %d\nkey %s\nvalue %s\n",\
    item->keySize, item->valueSize, item->flags, item->key, res);
    free(item);
    scanf("%d",&c);
    printf("%d\n",freeSegment(heap, heap->begin, num));
    freeHeap(heap->begin);
    return 0;
}*/
