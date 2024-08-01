all: parser.o cat.o
	gcc -shared -o libconfig.so cat.o parser.o -L.
parser.o:
	gcc -c -fPIC src/backend/config/parser.c

cat.o:
	gcc -c -fPIC src/backend/config/cat.c