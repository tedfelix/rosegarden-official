CC	 = g++
CPPFLAGS = -g
LIBCORE  = ../libcore.a
INCPATH  = -I..

SRCS	:= test.C

default: test

clean:
	rm -f test test.o

%.o: %.C
	$(CC) $(CPPFLAGS) -c $< $(INCPATH) -o $@

test: test.o
	$(CC) $< $(LIBCORE) -o $@

depend:
	makedepend $(INCPATH) -- $(CPPFLAGS) -- $(SRCS)

# DO NOT DELETE
test.o: ../Event.h ../Property.h ../Track.h ../NotationTypes.h
