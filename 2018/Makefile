CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I. -I..

PROPS = \
	animation \
	animation2 \
	band \
	factory \
	molar \
	orc \


all: $(PROPS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

DEP_LIBS = ../libhalloween.a ~/lib/lib.a
LIBS = $(DEP_LIBS) -lusb -lrt -lpthread

animation: animation.o $(DEP_LIBS)
	$(CXX) -o $@ animation.o $(LIBS)

animation2: animation2.o $(DEP_LIBS)
	$(CXX) -o $@ animation2.o $(LIBS)

band: band.o $(DEP_LIBS)
	$(CXX) -o $@ band.o $(LIBS)

factory: factory.o $(DEP_LIBS)
	$(CXX) -o $@ factory.o $(LIBS)

molar: molar.o $(DEP_LIBS)
	$(CXX) -o $@ molar.o $(LIBS)

orc: orc.o $(DEP_LIBS)
	$(CXX) -o $@ orc.o $(LIBS)

# compile and generate dependency info
%.o: %.c %.d
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o

%.d: %.c
	@echo "Depends:  $*.c"
	@gcc -MM $(CFLAGS) $*.c > $*.d

clean:
	-rm *.o *.d */*.o */*.d $(PROPS)

include animation.d
include animation2.d
include band.d
include factory.d
include molar.d
include orc.d
