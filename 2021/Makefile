CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I. -I..
CPPFLAGS=$(CFLAGS)

PROPS = \
	alien \
	coffin \
	jacob \

all: $(PROPS)

# pull in dependency info for *existing* .o files
DEP_LIBS = ../libhalloween.a ~/lib/lib.a
LIBS = $(DEP_LIBS) -lusb -lrt -lpthread

alien: alien.o $(DEP_LIBS)
	$(CXX) -o $@ alien.o $(LIBS)

coffin: coffin.o $(DEP_LIBS)
	$(CXX) -o $@ coffin.o $(LIBS)

jacob: jacob.o $(DEP_LIBS)
	$(CXX) -o $@ jacob.o $(LIBS)

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
