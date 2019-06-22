CFLAGS=-Wall -Werror -g -I/home/crpalmer/lib -I. -I..

PROPS = \
	earth-invaders \


all: $(PROPS)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

DEP_LIBS = ../libhalloween.a ~/lib/lib.a
LIBS = $(DEP_LIBS) -lusb -lrt -lpthread

earth-invaders: earth-invaders.o $(DEP_LIBS)
	$(CXX) -o $@ earth-invaders.o $(LIBS)

# compile and generate dependency info
%.o: %.c %.d
	@echo "Building: $*.c"
	@gcc -c $(CFLAGS) $*.c -o $*.o

%.o: %.cpp %.d
	@echo "Building: $*.c"
	@$(CXX) -c $(CFLAGS) $*.cpp -o $*.o

%.d: %.c
	@echo "Depends:  $*.c"
	@gcc -MM $(CFLAGS) $*.c > $*.d

clean:
	-rm *.o *.d */*.o */*.d $(PROPS)

-include earth-invaders.d
