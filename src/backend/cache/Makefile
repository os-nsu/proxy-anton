all: heap.o hash_table.o ttl_buckets.o
	mkdir ./install/plugins/cache/
	-gcc -shared -o cache.so heap.o hash_table.o ttl_buckets.o -ldl -L./install/plugins/cache/
	mv cache.so install/plugins/cache
heap.o:
	-gcc -c -fPIC src/backend/cache/heap.c

hash_table.o:
	-gcc -c -fPIC src/backend/cache/hash_table.c

ttl_buckets.o:
	-gcc -c -fPIC src/backend/cache/ttl_buckets.c