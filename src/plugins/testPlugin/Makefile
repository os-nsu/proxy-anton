all: test.o
	gcc -shared -o test.so testPlugin.o -ldl -L../../../install/plugins/
	mv test.so ../../../install/plugins/
	make clean
test.o:
	gcc -c -fPIC testPlugin.c

clean:
	rm -rf *.so
	rm -rf *.a
	rm -rf *.o
	