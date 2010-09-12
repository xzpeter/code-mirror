CROSS=arm-linux-

all: buttons

buttons: button.c
	$(CROSS)gcc -o buttons button.c -lpthread

clean:
	@rm -vf buttons *.o *~
