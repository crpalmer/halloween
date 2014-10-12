LIB=~/lib/lib.a
CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I.

PROPS = \
	animation/animation \
	banjo \
	dropper \
	tobacco-spitter \
	voodoo

all: $(PROPS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

LIBS = $(LIB) -lusb -lrt -lpthread

ANIMATION_OBJS = animation/main.o animation/lights.o

animation/animation: $(ANIMATION_OBJS) $(LIB)
	$(CC) -o $@ $(ANIMATION_OBJS) $(LIBS)

banjo: banjo.o $(LIB)
	$(CC) banjo.o -o $@ $(LIBS)

dropper: dropper.o $(LIB)
	$(CC) dropper.o -o $@ $(LIBS)

tobacco-spitter: tobacco-spitter.o $(LIB)
	$(CC) tobacco-spitter.o -o $@ $(LIBS)

voodoo: voodoo.o $(LIB)
	$(CC) voodoo.o -o $@ $(LIBS)

# compile and generate dependency info
%.o: %.c
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o
	@gcc -MM $(CFLAGS) $*.c > $*.d

clean:
	-rm $(LIB) *.o *.d $(PROPS)
