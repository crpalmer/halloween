CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I. -I..
CPPFLAGS=$(CFLAGS)

PROPS = \
	alien \
	animation \
	band \
	coffin \
	jacob \
	talker-and-fogger \

DATA = \
	pour-some-sugar.wav \
	pour-some-sugar-drums.wav \
	pour-some-sugar-guitar.wav \
	pour-some-sugar-vocals.wav \
	drums.ops \
	guitar.ops \
	vocals.ops \

all: $(PROPS) $(DATA)

# pull in dependency info for *existing* .o files
DEP_LIBS = ../libhalloween.a ~/lib/lib.a
LIBS = $(DEP_LIBS) -lusb -lrt -lpthread ../../tinyalsa/build/libtinyalsa.a

alien: alien.o $(DEP_LIBS)
	$(CXX) -o $@ alien.o $(LIBS)

animation: animation.o $(DEP_LIBS)
	$(CXX) -o $@ animation.o $(LIBS)

band: band.o $(DEP_LIBS)
	$(CXX) -o $@ band.o $(LIBS)

coffin: coffin.o $(DEP_LIBS)
	$(CXX) -o $@ coffin.o $(LIBS)

talker-and-fogger: talker-and-fogger.o $(DEP_LIBS)
	$(CXX) -o $@ talker-and-fogger.o $(LIBS)

jacob: jacob.o $(DEP_LIBS)
	$(CXX) -o $@ jacob.o $(LIBS)

pour-some-sugar.wav: ~/halloween-media/pour-some-sugar.mp3
	ffmpeg -y -i $< $@

pour-some-sugar-drums.wav: ~/halloween-media/pour-some-sugar-drums.mp3
	ffmpeg -y -i $< $@

pour-some-sugar-guitar.wav: ~/halloween-media/pour-some-sugar-guitar.mp3
	ffmpeg -y -i $< $@

pour-some-sugar-vocals.wav: ~/halloween-media/pour-some-sugar-vocals.mp3
	ffmpeg -y -i $< $@

drums.ops: pour-some-sugar-drums.wav
	~/lib/utils/talking-skull-prepare $< > $@

guitar.ops: pour-some-sugar-guitar.wav
	~/lib/utils/talking-skull-prepare $< > $@

vocals.ops: pour-some-sugar-vocals.wav
	~/lib/utils/talking-skull-prepare $< > $@

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
	-rm *.o *.d */*.o */*.d $(PROPS) $(DATA) 

include ${glob *.d}
