LIB=~/lib/lib.a
CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I.

PROPS = \
	animation \
	lightning \
	fogger

all: $(PROPS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

LIBS = $(LIB) -lusb -lrt -lpthread

ANIMATION_OBJS= animation.o animation-actions.o animation-lights.o

animation: $(ANIMATION_OBJS) $(LIB)
	$(CC) -o $@ $(ANIMATION_OBJS) $(LIBS)

fogger: fogger.o $(LIB)
	$(CC) -o $@ fogger.o $(LIBS)

lightning: lightning.o $(LIB)
	$(CC) -o $@ lightning.o $(LIBS)

# compile and generate dependency info
%.o: %.c
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o
	@gcc -MM $(CFLAGS) $*.c > $*.d

clean:
	-rm *.o *.d */*.o */*.d $(PROPS)
