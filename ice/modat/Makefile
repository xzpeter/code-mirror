sinclude cross.make
PRJ = modat 
CC = $(CROSS)gcc 
AR = $(CROSS)ar
CFLAGS = -g -Wall -Werror
LDFLAGS = -lpthread 
MODAT_OBJ = rbuf.o utils.o modat_common.o modat_daemon.o modat.o T35i.o

default: modat.a

modat.a: $(MODAT_OBJ)
	$(AR) r $@ $^

modat: main.o modat.a
	$(CC) -o $@ $(LDFLAGS) $^ 

clean:
	@rm -f modat *.o cscope.* modat *.a
