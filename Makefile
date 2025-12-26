CC ?= gcc
CFLAGS = -O2 -Wall -Werror=implicit-function-declaration -std=c99

OBJS  = buffer.o
OBJS += request.o
OBJS += test.o
OBJS += test-buffer.o
OBJS += test-request.o

all: test

check: test
	@echo "  CHECK"
	@./test

test: $(OBJS)
	@echo "  LINK    $@"
	@$(CC) $^ -o $@

-include $(patsubst %.o,%.d,$(OBJS))

%.o: %.c
	@echo "  CC      $@"
	@$(CC) $(CFLAGS) -MMD -MF $*.d -c $<

clean:
	@echo "  CLEAN"
	@rm -f *.o test

.PHONY: all check clean