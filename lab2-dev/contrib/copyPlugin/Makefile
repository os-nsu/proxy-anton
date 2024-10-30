all: copy.o
	gcc -shared -o copy.so copyPlugin.o -ldl -L$(MAINDIR)/install/plugins/
	mv copy.so $(MAINDIR)/install/plugins/
	make clean
copy.o:
	gcc -c -fPIC $(MAINDIR)/contrib/copyPlugin/copyPlugin.c

clean:
	rm -rf *.so
	rm -rf *.a
	rm -rf *.o
	