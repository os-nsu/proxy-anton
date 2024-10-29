all: plugin.o
	gcc -shared -o plugin.so plugin.o -ldl -L$(MAINDIR)/install/plugins/
	mv plugin.so $(MAINDIR)/install/plugins/
	make clean

plugin.o:
	gcc -c -fPIC $(MAINDIR)/contrib/plugin/plugin.c

clean:
	rm -rf *.o
	rm -rf *.so
	rm -rf *.a
