all:
	gcc -c -fPIC plugin.c 
	gcc -shared -o plugin.so plugin.o -ldl -L$(CURDIR)

clean:
	rm -rf *.o
	rm -rf *.so
	rm -rf *.a
