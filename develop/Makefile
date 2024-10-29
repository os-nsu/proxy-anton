export MAINDIR = $(CURDIR)

COPT := ${COPT} 
CFLAGS := ${CFLAGS} -export-dynamic
LDFLAGS := ${LDFLAGS} -L./install/ -ldl -llogger -lconfig
CPPFLAGS := ${CPPFLAGS}

CC = gcc

all:clean_install prepare_dir backend
	mv ./libconfig.so install
	mv ./liblogger.a install
	$(CC) $(CFLAGS) $(COPT) $(CPPFLAGS) -o proxy src/backend/main/main.c src/backend/master/master.c src/backend/master/bgWorker.c src/backend/master/sharedMem.c  $(LDFLAGS) -Wl,-rpath,$(MAINDIR)/install
	mv ./proxy install
	make kernel_plugins
	make custom_plugins
	make clean


prepare_dir:
	mkdir install
	mkdir install/plugins

backend:
	make -f src/backend/logger/Makefile
	make -f src/backend/config/Makefile

kernel_plugins:
	make -f src/backend/cache/Makefile	

clean:
	rm -rf *.o
	rm -rf *.so
	rm -rf *.a

clean_install:
	rm -rf install

custom_plugins:
	make -f src/plugins/Makefile

