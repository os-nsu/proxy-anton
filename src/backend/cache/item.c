#include "../../include/item.h"
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

void initFileItem(int fd, char *key, int valueSize, void *value) {
    ItemHeader item = {strlen(key), valueSize, 0};
    write(fd, &item, sizeof(ItemHeader));
    write(fd, key, strlen(key) + 1);
    write(fd, value, valueSize);
}

void *getItemData(ItemHeader *item, int *size) {
    *size = item->valueSize;
    return (char *)item + sizeof(ItemHeader) + item->keySize + 1;
}

int getFileItemData(int fd, int *size) {
    int keySize, valueSize;
    read(fd, (char *)(&keySize) + 3, 1);
    read(fd, (char *)(&valueSize) + 1, 3);
    const int bufferSize = 1 + keySize + 1;
    char buffer[bufferSize];
    read(fd, buffer, bufferSize);
    *size = valueSize;
    return fd;
}