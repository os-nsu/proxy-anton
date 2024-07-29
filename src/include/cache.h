/*!
    \file cache.h
    \brief cache
    \version 1.0
    \date 29 Jul 2024

    This file contains interface of cache.

    Segmentation cache has 3 principles:
    - be proactive, don't be lazy
    - maximize shared meta information for economy
    - perform macro management

    more information: https://www.usenix.org/system/files/nsdi21-yang.pdf

    IDENTIFICATION
        src/include/cache.h
*/
#ifndef CACHE_H
#define CACHE_H

/*!
    \brief Initializes cache 
    \param[in] hashTableSize Size of hash table
    \param[in] countRAMSeg Count segments in RAM
    \param[in] countFileSeg Count segments in FS
    \param[in] ramSegSize Size of segment in RAM
    \param[in] fileSegSize Size of segment in FS
    \param[in] cacheDir Path to cache directory
    \return 0 if success, -1 else
*/
int initCache(int hashTableSize, int countRAMSeg, int countFileSeg, int ramSegSize, int fileSegSize, char *cacheDir);

/*!
    \brief Frees cache
*/
void freeCache(void);

/*!
    \brief Pushes new cache
    \param[in] key Key string for cache
    \param[in] ttl Time to live of cache
    \param[in] valueSize Size of cache data
    \param[in] value Cache value
    \return 0 if success. -1 else
*/
int pushCache(char *key, int ttl, int valueSize, void *value);

/*!
    \brief Gets cache
    \param[in] key Key string of cache
    \param[out] value Pointer to result (Will be NULL if cache not found)
    \return 0 if cache worked correctly, -1 else
*/
int getCache(char *key, void **value);

#endif //CACHE_H