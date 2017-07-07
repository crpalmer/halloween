CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I. -I..

PROPS = \
	sparky

all: $(PROPS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

DEP_LIBS = ../libhalloween.a ~/lib/lib.a
LIBS = $(DEP_LIBS) -lusb -lrt -lpthread

sparky: sparky.o $(DEP_LIBS)
	$(CC) -o $@ sparky.o $(LIBS)

# James class demo

107-band: 107-band.o 107-utils.o $(DEP_LIBS)
	$(CC) -o $@ 107-band.o 107-utils.o $(LIBS)

107-talker: 107-talker.o 107-utils.o $(DEP_LIBS)
	$(CC) -o $@ 107-talker.o 107-utils.o $(LIBS)

# compile and generate dependency info
%.o: %.c
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o
	@gcc -MM $(CFLAGS) $*.c > $*.d

clean:
	-rm *.o *.d */*.o */*.d $(PROPS)
