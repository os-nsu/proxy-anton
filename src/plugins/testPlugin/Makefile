all: test.o
	mkdir ../../../install/plugins/test/
	gcc -shared -o test.so testPlugin.o -ldl -L../../../install/plugins/test/
	mv test.so ../../../install/plugins/test/
	make clean
test.o:
	gcc -c -fPIC testPlugin.c

clean:
	rm -rf *.so
	rm -rf *.a
	rm -rf *.o
	