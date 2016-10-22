LIB=~/lib/lib.a
CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I.

ALL = \
	baxter \
	libhalloween.a \
	lightning \
	fogger \
	talker

all: $(ALL)

LIBS = $(LIB) -lusb -lrt -lpthread

ANIMATION_OBJS= animation-main.o animation-lights.o
TIME_OBJS = ween-time.o

HALLOWEEN_OBJS = \
	$(ANIMATION_OBJS) \
	$(TIME_OBJS)

BAXTER_OBJS = baxter.o
FOGGER_OBJS = fogger.o
LIGHTNING_OBJS = lightning.o
TALKER_OBJS = talker.o

OBJS = $(HALLOWEEN_OBJS) $(BAXTER_OBJS) $(FOGGER_OBJS) $(LIGHTNING_OBJS) $(TALKER_OBJS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

libhalloween.a: $(HALLOWEEN_OBJS) $(LIB)
	@ar r $@ $(HALLOWEEN_OBJS) $(LIB)

baxter: $(BAXTER_OBJS) $(LIB)
	$(CC) -o $@ $(BAXTER_OBJS) $(LIBS)

fogger: $(FOGGER_OBJS) $(LIB)
	$(CC) -o $@ $(FOGGER_OBJS) $(LIBS)

lightning: $(LIGHTNING_OBJS) $(LIB) libhalloween.a
	$(CC) -o $@ $(LIGHTNING_OBJS) libhalloween.a $(LIBS)

talker: $(TALKER_OBJS) $(LIB) libhalloween.a
	$(CC) -o $@ $(TALKER_OBJS) libhalloween.a $(LIBS)

# compile and generate dependency info
%.o: %.c
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o
	@gcc -MM $(CFLAGS) $*.c > $*.d

clean:
	-rm *.o *.d */*.o */*.d $(PROPS)
