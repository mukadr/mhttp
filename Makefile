CC ?= gcc
CFLAGS = -O2 -Wall

.PHONY: all check clean

all: test

check: test
	./test

test: test.o test-buffer.o buffer.o
	$(CC) $^ -o test

test.o: test.c
	$(CC) $^ -c $(CFLAGS)

test-buffer.o: test-buffer.c
	$(CC) $^ -c $(CFLAGS)

buffer.o: buffer.c
	$(CC) $^ -c $(CFLAGS)

clean:
	rm -f *.o test
