LIB=~/lib/lib.a
CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I.

PROPS = \
	animation \
	banjo \
	deer \
	dropper \
	tobacco-spitter \
	voodoo

all: $(PROPS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

LIBS = $(LIB) -lusb -lrt -lpthread

animation: animation.o $(LIB)
	$(CC) -o $@ animation.o $(LIBS)

banjo: banjo.o $(LIB)
	$(CC) banjo.o -o $@ $(LIBS)

deer: deer.o $(LIB)
	$(CC) deer.o -o $@ $(LIBS)

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
	-rm *.o *.d */*.o */*.d $(PROPS)
