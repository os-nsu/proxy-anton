all: clean_last backend
	mkdir install
	mkdir install/backend
	mkdir install/plugins
	mv ./libconfig.so install/backend
	mv ./liblogger.a install/backend
	gcc -o proxy src/backend/main/main.c src/backend/master/master.c src/backend/master/bgWorker.c src/backend/master/sharedMem.c -export-dynamic -L./install/backend/ -ldl -llogger -lconfig -Wl,-rpath,./install/backend
	mv ./proxy install/backend
	make kernel_plugins
	make clean


backend:
	make -f src/backend/logger/Makefile
	make -f src/backend/config/Makefile

kernel_plugins:
	make -f src/backend/cache/Makefile	

clean:
	rm -rf *.o
	rm -rf *.so
	rm -rf *.a

clean_last:
	rm -rf install