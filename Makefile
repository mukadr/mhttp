CC ?= gcc
CFLAGS = -O2 -Wall -Werror=implicit-function-declaration -std=c99

.PHONY: all check clean

all: test

check: test
	./test

test: test.o test-buffer.o test-request.o buffer.o request.o
	$(CC) $^ -o test

test.o: test.c
	$(CC) $^ -c $(CFLAGS)

test-buffer.o: test-buffer.c
	$(CC) $^ -c $(CFLAGS)

test-request.o: test-request.c
	$(CC) $^ -c $(CFLAGS)

buffer.o: buffer.c
	$(CC) $^ -c $(CFLAGS)

request.o: request.c
	$(CC) $^ -c $(CFLAGS)

clean:
	rm -f *.o test
