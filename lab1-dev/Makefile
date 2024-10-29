export MAINDIR = $(CURDIR)

all: clean_install prepare_dir backend
	mv ./libdynamichello.so install
	mv ./libstatichello.a install
	gcc -o proxy  src/backend/master/master.c -Wall -export-dynamic -L./install/ -ldynamichello -lstatichello -Wl,-rpath,$(MAINDIR)/install/plugins
	mv ./proxy install
	make custom_plugins

prepare_dir:
	mkdir install
	mkdir install/plugins

backend:
	make -f src/backend/libs/dynamic/Makefile
	make -f src/backend/libs/static/Makefile

custom_plugins:
	make -f contrib/Makefile

clean:
	rm -rf *.o
	rm -rf *.so
	rm -rf *.a

clean_install:
	rm -rf install