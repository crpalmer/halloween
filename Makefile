LIB=~/lib/lib.a
CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I.

PROPS = \
	lightning

all: $(PROPS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

LIBS = $(LIB) -lusb -lrt -lpthread

lightning: lightning.o $(LIB)
	$(CC) -o $@ lightning.o $(LIBS)

# compile and generate dependency info
%.o: %.c
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o
	@gcc -MM $(CFLAGS) $*.c > $*.d

clean:
	-rm *.o *.d */*.o */*.d $(PROPS)
