#include "../../include/d_item.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


void initItem(ItemHeader *item, char *key, int valueSize, void *value) {
    item->flags = 0;
    item->keySize = strlen(key);
    item->valueSize = valueSize;
    strcpy((char *)item + sizeof(ItemHeader), key);
    for(int i = 0; i < valueSize; i++) {
        *((char *)item + sizeof(ItemHeader) + strlen(key) + 1 + i) = ((char*)value)[i];
    }
}

void initBigSegItem(int fd, char *key, int valueSize, void *value) {
    ItemHeader item = {strlen(key), valueSize, 0};
    write(fd, &item, sizeof(ItemHeader));
    write(fd, key, strlen(key) + 1);
    write(fd, value, valueSize);
}

void *getItemData(ItemHeader *item, int *size) {
    *size = item->valueSize;
    return (char *)item + sizeof(ItemHeader) + item->keySize + 1;
}

int getBigSegItemData(int fd, int *size) {
    int keySize, valueSize;
    read(fd, (char *)(&keySize) + 3, 1);
    read(fd, (char *)(&valueSize) + 1, 3);
    const int bufferSize = 1 + keySize + 1;
    char buffer[bufferSize];
    read(fd, buffer, bufferSize);
    *size = valueSize;
    return fd;
}

int getBigSegItemSize(int fd) {
    ItemHeader itemHeader;
    read(fd, &itemHeader, sizeof(ItemHeader));
    lseek(fd, -sizeof(ItemHeader), SEEK_CUR);
    return sizeof(ItemHeader) + itemHeader.keySize + 1 + itemHeader.valueSize;
}

int getItemSize(ItemHeader *item) {
    return sizeof(ItemHeader) + item->keySize + 1 + item->valueSize;
}

char *getItemKey(ItemHeader *item) {
    char *key = (char *)calloc(item->keySize + 1, sizeof(char));
    strcpy(key, (char *)item + (sizeof(ItemHeader)));
    return key;
}

char *getBigSegItemKey(int fd) {
    ItemHeader header;
    read(fd, &header, sizeof(ItemHeader));
    char *key = (char *)calloc(header.keySize + 1, sizeof(char));
    read(fd, key, header.keySize + 1);
    return key;
}