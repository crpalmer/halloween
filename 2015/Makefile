LIB=~/lib/lib.a
CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I.

PROPS = \
	animation \
	band \
	talker

all: $(PROPS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

LIBS = $(LIB) -lusb -lrt -lpthread

animation: animation.o $(LIB)
	$(CC) -o $@ animation.o $(LIBS)

band: band.o $(LIB)
	$(CC) -o $@ band.o $(LIBS)

talker: talker.o $(LIB)
	$(CC) -o $@ talker.o $(LIBS)

# compile and generate dependency info
%.o: %.c
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o
	@gcc -MM $(CFLAGS) $*.c > $*.d

clean:
	-rm *.o *.d */*.o */*.d $(PROPS)
