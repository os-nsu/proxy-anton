all: test.o
	gcc -shared -o test.so testPlugin.o -ldl -L$(MAINDIR)/install/plugins/
	mv test.so $(MAINDIR)/install/plugins/
	make clean
test.o:
	gcc -c -fPIC $(MAINDIR)/src/plugins/testPlugin/testPlugin.c

clean:
	rm -rf *.so
	rm -rf *.a
	rm -rf *.o
	