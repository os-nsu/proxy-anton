/*!
    \file sharedMem.c
    \brief shared memory
    \version 1.0
    \date 30 Jul 2024

    This file contains implementation of shared memory system.

    IDENTIFICATION
        src/backend/master/sharedMem.c
*/

#include "../../include/config.h"
#include "../../include/master.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

//FUNCTION ADVERTISEMENT


/*!
    \brief Initializes hash table of regions
    \return Pointer to hash table if success, -1 else
*/
RegionTable *initRegionTable(int size);

/*!
    \brief hash string
    Now algorithm is sdbm because this was used
    in real data base and worked fine
    \param[in] str value
    \return hash that is unsigned long
*/
unsigned long hashStringSh(const char* str);

/*!
    \brief Adds pair of name and region into hahsh table
    \param[in] table Pointer to hash table
    \param[in] name Name of region
    \param[in] begin Begin of region
    \param[in] size Size of region
    \return 0 if success, -1 else
*/
int addRegion(RegionTable *table, const char *name, void *begin, int size);

/*!
    \brief Finds region in hash table by name
    \param[in] table Pointer to hash table
    \param[in] name Name of region
    \return Pointer to region node if success, NULL else
*/
RegionNode *findRegion(RegionTable *table, const char *name);


/*!
    \brief Frees regionNode
    \param[in] node Pointer to node
*/
void freeRegionNode(RegionNode *node);


/*!
    \brief Frees table 
    \param[in] table Pointer to hash table
*/
void freeRegionTable(RegionTable *table);


/*!
    \brief Requests for extend shared memory
    \param[in] size Size of memory extension
    \return 0 if success, -1 else
*/
int requestSharedMemory(int size);


/*!
    \brief Start up shared memory
    \return Pointer to management structure of mapped area if success, NULL else
*/
SharedAreaManager *mapSharedMemory(void);

/*!
    \brief Find or register new shared area
    \param[in] manager Pointer to shared memory management structure
    \param[in] table Pointer to table of regions
    \param[in] name Name of area
    \param[in] size Size of area
    \param[out] found Result flag, 1 if region was found, 0 else
    \return Pointer to begin of area if success, NULL else
*/
void *registerSharedArea(SharedAreaManager *manager, RegionTable *table, const char *name, int size, int *found);


//FUNCTION IMPLEMENTATION


unsigned long hashStringSh(const char* str) {
    unsigned long hash = 0;
    int c;

    while ((c = *str++)) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

RegionTable *initRegionTable(int size) {
    RegionTable *table = (RegionTable *)malloc(sizeof(RegionTable));

    table->size = size;
    table->heads = (RegionNode **)calloc(size, sizeof(RegionNode *));
    return table;
}

int addRegion(RegionTable *table, const char *name, void *begin, int size) {
    if (!table || !name || !begin) {
        return -1;
    }
    int regionIdx = hashStringSh((char *)name) % table->size;
    RegionNode *node = (RegionNode *)malloc(sizeof(RegionNode));
    node->begin = begin;
    node->name = (char *)calloc(strlen(name) + 1, sizeof(char));
    strcpy(node->name, name);
    node->begin = begin;
    node->size = size;
    node->next = table->heads[regionIdx];
    table->heads[regionIdx] = node;
    return 0;
}

RegionNode *findRegion(RegionTable *table, const char *name) {
    if (!table || !name ) {
        return NULL;
    }
    int regionIdx = hashStringSh(name) % table->size;
    RegionNode *node = table->heads[regionIdx];
    while (node != NULL)
    {
        if (strcmp(node->name, name) == 0) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

void freeRegionNode(RegionNode *node) {
    if (!node) {
        return;
    }
    free(node->name);
    free(node);
}

void freeRegionTable(RegionTable *table) {
    for(int i = 0; i < table->size; i++) {
        RegionNode *curNode = table->heads[i];
        RegionNode *delNode = NULL;

        while (curNode) {
            delNode = curNode;
            curNode = curNode->next;
            freeRegionNode(delNode);
        }    
    }
    free(table->heads);
    free(table);
}


int requestSharedMemory(int size) {
    CATFollower shmem;
    if(addFollowerToCAT("kernel", "shmem_size", &shmem) == -1) {
        union Value bootSize = (union Value)(long long)size; 
        
        if (createCATParameter("kernel", "shmem_size", T_LONG,\
        1, &bootSize, &shmem, "size of shared memory") == -1) {
            return -1; //unexpected error
        }
    } else {
        union Value updatedSize = (union Value)((long long)size + shmem.data[0].lngf);
        if (updateCATParameter("kernel", "shmem_size", 1, &updatedSize) == -1) {
            return -1; //unexpected error
        }
    }
    return 0;
}

SharedAreaManager *mapSharedMemory(void) {
    CATFollower shmem;
    if(addFollowerToCAT("kernel", "shmem_size", &shmem) == -1) {
        return NULL;
    }
    if (shmem.data[0].lngf < 0) {
        return NULL;
    }
    
    /*MMAP SHARED AREA*/
    void *beginArea = mmap(NULL, shmem.data[0].lngf, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    SharedAreaManager *manager = (SharedAreaManager *)malloc(sizeof(SharedAreaManager));
    manager->begin = beginArea;
    manager->capacity = shmem.data[0].lngf;
    manager->filledSize = 0;
    return manager;
}

void *registerSharedArea(SharedAreaManager *manager, RegionTable *table, const char *name, int size, int *found) {
    if (!manager || !table || !name) {
        return NULL;
    }

    /*CHECK THAT REGION ALREADY EXISTS*/
    RegionNode *goalNode = findRegion(table, name);
    if (goalNode) {
        if (found) {
            *found = 1;
        }
        return goalNode->begin;
    }

    /*CHECK REST SIZE*/
    if (manager->capacity - manager->filledSize < size) {
        return NULL;
    }
    /*TRY TO ALLOCATE NEW AREA*/
    void *resultPtr = (char *)manager->begin + manager->filledSize;
    if (addRegion(table, name, resultPtr, size) == -1) {
        return NULL;
    }
    manager->filledSize+=size;
    if (found){
        *found = 0;
    }
    
    return resultPtr;
}

