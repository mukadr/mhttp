CC ?= gcc
CFLAGS = -std=c99 -Wall -Werror=implicit-function-declaration

ifdef SANITIZE
DEBUG = true
CFLAGS += -fsanitize=address,undefined
endif

ifdef DEBUG
CFLAGS += -O0 -g
else
CFLAGS += -O2
endif

OBJS  = buffer.o
OBJS += request.o
OBJS += response.o
OBJS += slice.o
OBJS += test.o
OBJS += test-buffer.o
OBJS += test-request.o
OBJS += test-response.o

all: test

check: test
	@echo "  CHECK"
	@./test

test: $(OBJS)
	@echo "  LINK    $@"
	@$(CC) $(CFLAGS) $^ -o $@

-include $(patsubst %.o,%.d,$(OBJS))

%.o: %.c
	@echo "  CC      $@"
	@$(CC) $(CFLAGS) -MMD -MF $*.d -c $<

clean:
	@echo "  CLEAN"
	@rm -f *.o *.d test

sanitize: clean
	@$(MAKE) SANITIZE=1 check

leaks: clean
	@$(MAKE) DEBUG=1 test
	@echo "  LEAKS"
	@leaks -quiet -atExit -- ./test

.PHONY: all check clean sanitize leaks
