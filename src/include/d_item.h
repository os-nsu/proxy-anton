#ifndef ITEM_H
#define ITEM_H


typedef struct ItemHeaderData {
    unsigned char keySize;
    unsigned int valueSize : 24;
    unsigned char flags;
} ItemHeader;


#define ITEMFLDELETE(flags) (flags & 0x1)
#define ITEMFLMERGE(flags) (flags & 0x1 << 1)

void initItem(ItemHeader *item, char *key, int valueSize, void *value);

void initBigSegItem(int fd, char *key, int valueSize, void *value);

void *getItemData(ItemHeader *item, int *size);

int getBigSegItemData(int fd, int *size);

int getBigSegItemSize(int fd);

int getItemSize(ItemHeader *item);

char *getItemKey(ItemHeader *item);

char *getBigSegItemKey(int fd);

#endif //ITEM_H