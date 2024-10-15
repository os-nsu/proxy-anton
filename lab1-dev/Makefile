export MAINDIR = $(CURDIR)


all: clean_install prepare_dir
	make -f src/libs/dynamic/Makefile
	mv ./libdynamichello.so install
	gcc -o proxy  src/master/master.c -Wall -L./install/ -ldynamichello -Wl,-rpath,$(MAINDIR)/install
	mv ./proxy install

prepare_dir:
	mkdir install

clean:
	rm -rf *.o
	rm -rf *.so
	rm -rf *.a

clean_install:
	rm -rf install