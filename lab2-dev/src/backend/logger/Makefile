all: logger.o
	gcc -shared -o liblogger.so logger.o -L $(MAINDIR)/install -Wl,-rpath,$(MAINDIR)/install
logger.o:
	gcc -c -fPIC src/backend/logger/logger.c 