.PHONY: all clean

all: mhttp

mhttp: main.o
	gcc main.o -o mhttp

main.o: main.c
	gcc -c -O2 -Wall main.c

clean:
	rm -f *.o mhttp
