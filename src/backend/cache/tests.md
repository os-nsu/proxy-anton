TESTS FROM HEAP.C

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



TESTS FROM HASH_TABLE.C


/*
int main() {
    Heap *PMheap = initHeap(8168, 250, "./test");
    Heap *RAMheap = initHeap(512, 1024, NULL);
    Heap *heaps[2] = {RAMheap, PMheap};


    HashTable *table = initTable(1024, heaps, 2);

    int seg = allocateSegment(RAMheap, RAMheap->begin);

    char *greeting = "hello world from cache!\n";
    ItemHeader *item = (ItemHeader *)malloc(sizeof(ItemHeader));
    item->flags = 0;
    item->key = "myval";
    item->keySize = strlen(item->key);
    item->valueSize = strlen(greeting) + 1;

    int offset = addItem(RAMheap->begin, seg, item, greeting);

    int resAdd = addPtr(table, item->key, 0, seg, offset);
    printf("result of adding: %d\n", resAdd);
    memset(item, 0, sizeof(ItemHeader));
    greeting = NULL;
    
    getItem (table, "myval", item, (void **)&greeting);
    printf("item:\nkey size %d\nvalue size %d\nflags %i\nkey %s\nvalue %s\n\n",\
    item->keySize, item->valueSize, item->flags, item->key, greeting);

    printf("searching result: %d\n", getItem (table, "myval", item, (void **)&greeting));

    freeSegment(RAMheap, RAMheap->begin, seg);
    deletePtr(table, item->key, 0, seg, offset);

    freeItemHeader(item);
    freeTable(table);
    freeHeap(RAMheap->begin);
    freeHeap(PMheap->begin);
    return 0;
}*/



TESTS FROM CACHE.C

/*
int main() {

    initCache(1024, 100, 10, 1024 * 1024, 500 * 1024 * 1024, "./test");

    putCache("hello", 10, strlen("hello world!") + 1, "Hello world!");

    char *answer;

    getCache("hello", (void **)&answer);

    printf("answer: %s\n", answer);

    return 0;
} */