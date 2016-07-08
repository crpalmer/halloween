LIB=~/lib/lib.a
CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I. -I..

PROPS = \
	animation \
	jousters \

all: $(PROPS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

LIBS = $(LIB) -lusb -lrt -lpthread
LIBS_ANIMATION = ../libanimation.a

animation: animation.o $(LIB) $(LIBS_ANIMATION)
	$(CC) -o $@ animation.o $(LIBS_ANIMATION) $(LIBS)

jousters: jousters.o $(LIB) $(LIBS_ANIMATION)
	$(CC) -o $@ jousters.o $(LIBS_ANIMATION) $(LIBS)

# compile and generate dependency info
%.o: %.c
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o
	@gcc -MM $(CFLAGS) $*.c > $*.d

clean:
	-rm *.o *.d */*.o */*.d $(PROPS)
