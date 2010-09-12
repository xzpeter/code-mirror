sinclude cross.make
PRJ = ice
CC = $(CROSS)gcc
ifdef CROSS
	EXTRA = -DPLATFORM_ARM=1
	SUFFIX = arm
else
	EXTRA = -DPLATFORM_PC=1
	SUFFIX = i386
endif
CFLAGS = -g -Wall -Werror $(EXTRA) -DDEBUG_LEVEL=1
LDFLAGS = -lpthread

default: ice

ice: main.o ads321.o modat/modat.a
	$(CC) $(LDFLAGS) -o $@.$(SUFFIX) $^

modat/modat.a:
	make -C modat/

clean:
	rm -rf cscope.* *.o *.log *.txt report.latest
	make clean -C modat/
