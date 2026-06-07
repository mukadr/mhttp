CC ?= gcc
CFLAGS = -O2 -Wall -Werror=implicit-function-declaration -std=c99

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
	@$(MAKE) CFLAGS="-std=c99 -g -fsanitize=address,undefined -Wall -Werror=implicit-function-declaration" check

leaks: clean
	@$(MAKE) CFLAGS="-std=c99 -g -Wall -Werror=implicit-function-declaration" test
	@echo "  LEAKS"
	@leaks -quiet -atExit -- ./test

.PHONY: all check clean sanitize leaks
