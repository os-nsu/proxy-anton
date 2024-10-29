all: parser.o cat.o

	ar -rcs libconfig.a cat.o parser.o
parser.o:
	gcc -c src/backend/config/parser.c

cat.o:
	gcc -c src/backend/config/cat.c