CC = arm-linux-gcc
CFLAGS = -g

default: test-serial

test-serial: test-serial.o utils.o
	$(CC) -o $@ $^ -lpthread

utils.o: ../modat/utils.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@rm *.o test-serial
