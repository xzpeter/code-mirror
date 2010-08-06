PWD_RET = $(shell pwd)
EXEC = $(shell basename $(PWD_RET))
#CROSS = arm-linux-
CC = $(CROSS)gcc
CUR_sources = $(wildcard *.c)
SOURCES = $(CUR_sources)
OBJS = $(SOURCES:.c=.o)
DEPS = $(SOURCES:.c=.d)

CFLAGS = -g -DTEST_STRING=$(shell date +%s) -I/home/peter/workspace/include
LDFLAGS = -lcurl
ifdef CROSS
LDFLAGS = $(LDFLAGS) -L/home/peter/workspace/lib
endif

all : $(EXEC)

%.d: %.c
	@echo updating dependency file $@ ...
	@set -e; rm -f $@;\
	$(CC) -MM  $< > $@.$$$$;\
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$;

$(EXEC) : $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^

clean:
	@rm -f $(OBJS) $(DEPS) $(EXEC) *.d.*

sinclude $(DEPS)
