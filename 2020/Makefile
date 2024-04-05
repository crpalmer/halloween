CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I. -I..
CPPFLAGS=$(CFLAGS)

PROPS = \
	animation \
	band \
	fogger \
	hypnopops \
	witch \

NO_LONGER_SUPPORTED = \
	guitar.ops \
	piano.ops \
	vocals.ops \


all: $(PROPS) $(DATA)

# pull in dependency info for *existing* .o files
DEP_LIBS = ../libhalloween.a ~/lib/lib.a
LIBS = $(DEP_LIBS) -lusb -lrt -lpthread ../../tinyalsa/build/libtinyalsa.a -lgpiod

animation: animation.o $(DEP_LIBS)
	$(CXX) -o $@ animation.o $(LIBS)

band: band.o $(DEP_LIBS)
	$(CXX) -o $@ band.o $(LIBS)

fogger: fogger.o $(DEP_LIBS)
	$(CXX) -o $@ fogger.o $(LIBS)

hypnopops: hypnopops.o $(DEP_LIBS)
	$(CXX) -o $@ hypnopops.o $(LIBS)

witch: witch.o $(DEP_LIBS)
	$(CXX) -o $@ witch.o $(LIBS)

guitar.ops: ~/halloween-media/guitar.wav
	~/lib/utils/talking-skull-prepare `basename $<` > $@

piano.ops: ~/halloween-media/piano.wav
	~/lib/utils/talking-skull-prepare `basename $<` > $@

vocals.ops: ~/halloween-media/vocals.wav
	~/lib/utils/talking-skull-prepare `basename $<` > $@

# compile and generate dependency info
%.o: %.c %.d
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o

%.o: %.cpp %.d
	@echo "Building: $*.cpp"
	@(CXX) -c $(CPPFLAGS) $*.cpp -o $*.o

%.d: %.c
	@echo "Depends:  $*.c"
	@gcc -MM $(CFLAGS) $*.c > $*.d

clean:
	-rm *.o *.d */*.o */*.d $(PROPS)

include ${glob *.d}
