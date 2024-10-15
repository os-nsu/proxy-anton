all:
	gcc -c -fPIC plugin.c 
	gcc -shared -o plugin.so plugin.o -ldl -L$(CURDIR)