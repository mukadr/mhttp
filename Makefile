CFLAGS = -O2 -Wall

.PHONY: all check clean

all: check

check: test
	./test

test: test.o test-buffer.o buffer.o
	gcc $^ -o test

test.o: test.c
	gcc $^ -c $(CFLAGS)

test-buffer.o: test-buffer.c
	gcc $^ -c $(CFLAGS)

buffer.o: buffer.c
	gcc $^ -c $(CFLAGS)

clean:
	rm -f *.o test
