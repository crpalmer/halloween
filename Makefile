LIB=~/lib/lib.a
CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I.

PROPS = \
	libanimation.a \
	lightning \
	fogger

all: $(PROPS)

LIBS = $(LIB) -lusb -lrt -lpthread

ANIMATION_OBJS= animation-main.o animation-lights.o
FOGGER_OBJS = fogger.o
LIGHTNING_OBJS = lightning.o

OBJS = $(ANIMATION_OBJS) $(FOGGER_OBJS) $(LIGHTNING_OBJS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

libanimation.a: $(ANIMATION_OBJS) $(LIB)
	@ar r $@ $(ANIMATION_OBJS) $(LIB)

fogger: $(FOGGER_OBJS) $(LIB)
	$(CC) -o $@ $(FOGGER_OBJS) $(LIBS)

lightning: $(LIGHTNING_OBJS) $(LIB)
	$(CC) -o $@ $(LIGHTNING_OBJS) $(LIBS)

# compile and generate dependency info
%.o: %.c
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o
	@gcc -MM $(CFLAGS) $*.c > $*.d

clean:
	-rm *.o *.d */*.o */*.d $(PROPS)
