LIB=~/lib/lib.a
CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib

PROPS = \
	banjo \
	tobacco-spitter

all: $(PROPS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

LIBS = $(LIB) -lusb -lrt -lpthread

banjo: banjo.o $(LIB)
	$(CC) banjo.o -o $@ $(LIBS)

tobacco-spitter: tobacco-spitter.o $(LIB)
	$(CC) tobacco-spitter.o -o $@ $(LIBS)

# compile and generate dependency info
%.o: %.c
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o
	@gcc -MM $(CFLAGS) $*.c > $*.d

clean:
	-rm $(LIB) *.o *.d $(PROPS)
