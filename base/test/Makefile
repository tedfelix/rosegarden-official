CC	 = g++
# debug flags need to be consistent with base build
#CPPFLAGS = -O2 
CPPFLAGS = -g 

LIBBASE  = ../.libs/libbase.a

INCPATH  = -I..

SRCS	:= test.C thread.C pitch.C

default: test thread pitch

clean:
	rm -f test test.o thread thread.o pitch pitch.o

%.o: %.C
	$(CC) $(CPPFLAGS) -c $< $(INCPATH) -o $@

test: test.o
	$(CC) $< $(LIBBASE) -o $@

pitch: pitch.o
	$(CC) $< $(LIBBASE) -o $@

thread: thread.o
	$(CC) $< $(LIBBASE) -o $@ -lpthread

depend:
	makedepend $(INCPATH) -- $(CPPFLAGS) -- $(SRCS)

# DO NOT DELETE

