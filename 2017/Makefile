CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I. -I..

PROPS = \
	animation \
	band \
	coffin \
	dude \
	fortune-fish \
	sparky

all: $(PROPS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

DEP_LIBS = ../libhalloween.a ~/lib/lib.a
LIBS = $(DEP_LIBS) -lusb -lrt -lpthread

animation: animation.o $(DEP_LIBS)
	$(CC) -o $@ animation.o $(LIBS)

band: band.o $(DEP_LIBS)
	$(CC) -o $@ band.o $(LIBS)

coffin: coffin.o $(DEP_LIBS)
	$(CC) -o $@ coffin.o $(LIBS)

dude: dude.o $(DEP_LIBS)
	$(CC) -o $@ dude.o $(LIBS)

fortune-fish: fortune-fish.o $(DEP_LIBS)
	$(CC) -o $@ fortune-fish.o $(LIBS)

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
