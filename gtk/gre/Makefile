prj = gre
flags = -Wall -g `pkg-config --cflags --libs gtk+-2.0`

default: $(prj)

$(prj) : gre.c
	gcc -o $@ $(flags) $^

run : 
	./$(prj)

clean :
	@rm $(prj)
