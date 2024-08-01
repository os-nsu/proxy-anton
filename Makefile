all: backend
	gcc -o proxy src/backend/main/main.c src/backend/master/master.c src/backend/master/bgWorker.c src/backend/master/sharedMem.c   -L. -ldl -llogger -lconfig -Wl,-rpath,. 
	
	mkdir install
	mkdir install/backend
	mkdir install/plugins
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