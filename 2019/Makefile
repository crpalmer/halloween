CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I. -I..

PROPS = \
	band \
	earth-invaders earth-invaders-test \
	fogger \
	paperboy \
	scow \


all: $(PROPS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

DEP_LIBS = ../libhalloween.a ~/lib/lib.a
LIBS = $(DEP_LIBS) -lusb -lrt -lpthread ../../tinyalsa/build/libtinyalsa.a -lgpiod

band: band.o $(DEP_LIBS)
	$(CXX) -o $@ band.o $(LIBS)

earth-invaders: earth-invaders.o $(DEP_LIBS)
	$(CXX) -o $@ earth-invaders.o $(LIBS)

earth-invaders-test: earth-invaders-test.o $(DEP_LIBS)
	$(CXX) -o $@ earth-invaders-test.o $(LIBS)

fogger: fogger.o $(DEP_LIBS)
	$(CXX) -o $@ fogger.o $(LIBS)

paperboy: paperboy.o $(DEP_LIBS)
	$(CXX) -o $@ paperboy.o $(LIBS)

scow: scow.o $(DEP_LIBS)
	$(CXX) -o $@ scow.o $(LIBS)

# compile and generate dependency info
%.o: %.c
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o

%.o: %.cpp
	@echo "Building: $*.c"
	@$(CXX) -c $(CFLAGS) $*.cpp -o $*.o

%.d: %.c
	@echo "Depends:  $*.c"
	@gcc -MM $(CFLAGS) $*.c > $*.d

%.d: %.cpp
	@echo "Depends:  $*.c"
	@g++ -MM $(CFLAGS) $*.cpp > $*.d

clean:
	-rm *.o *.d */*.o */*.d $(PROPS)

-include earth-invaders.d
-include earth-invaders-test.d
