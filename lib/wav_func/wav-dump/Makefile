OUT=wavdump
CC=gcc

all : $(OUT)

$(OUT) : wavdump.c ../wav_func.o
	$(CC) -g -o $@ $^

clean:
	@rm $(OUT)
