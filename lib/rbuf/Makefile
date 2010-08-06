PWD_RET = $(shell pwd)
EXEC = $(shell basename $(PWD_RET))
#CROSS = arm-linux-
CC = $(CROSS)gcc
CUR_sources = $(wildcard *.c)
SOURCES = $(CUR_sources)
OBJS = $(SOURCES:.c=.o)
DEPS = $(SOURCES:.c=.d)

ifdef CROSS
TEST="asdf"
endif

CFLAGS = -DTEST_STRING=$(shell date +%s) -g -Wall
LDFLAGS = -lpthread

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
	@rm -f $(OBJS) $(DEPS) $(EXEC)

sinclude $(DEPS)
