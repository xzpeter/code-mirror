PRJ = modat 
#CROSS = arm-linux-
CC = $(CROSS)gcc 
AR = $(CROSS)ar
CFLAGS = -g -Wall -Werror -DDEBUG_LEVEL=2 
LDFLAGS = -lpthread 
MODAT_OBJ = rbuf.o modat_misc.o modat_common.o modat_daemon.o modat.o T35i.o

default: modat

modat.a: $(MODAT_OBJ)
	$(AR) r $@ $^

modat: main.o modat.a
	$(CC) -o $@ $(LDFLAGS) $^ 

clean:
	@rm -f modat *.o cscope.*
