CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I. -I..

PROPS = \
	animation \
	band \
	jousters \
	princess

all: $(PROPS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

DEP_LIBS = ../libhalloween.a ~/lib/lib.a
LIBS = $(DEP_LIBS) -lusb -lrt -lpthread

animation: animation.o $(DEP_LIBS)
	$(CC) -o $@ animation.o $(LIBS)

band: band.o $(DEP_LIBS)
	$(CC) -o $@ band.o $(LIBS)

jousters: jousters.o $(DEP_LIBS)
	$(CC) -o $@ jousters.o $(LIBS)

princess: princess.o $(DEP_LIBS)
	$(CC) -o $@ princess.o $(LIBS)

# compile and generate dependency info
%.o: %.c
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o
	@gcc -MM $(CFLAGS) $*.c > $*.d

clean:
	-rm *.o *.d */*.o */*.d $(PROPS)
