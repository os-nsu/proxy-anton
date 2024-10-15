export MAINDIR = $(CURDIR)


all: clean_install prepare_dir
	gcc -o proxy  src/master/master.c -Wall
	mv ./proxy install


prepare_dir:
	mkdir install

clean:
	rm -rf *.o
	rm -rf *.so
	rm -rf *.a

clean_install:
	rm -rf install