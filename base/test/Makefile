CC	 = g++
CPPFLAGS = -g

LIBCORE  = ../.libs/libcore.a
###LIBCORE  = ../Property.o ../Event.o

INCPATH  = -I..

SRCS	:= test.C

default: test

clean:
	rm -f test test.o

%.o: %.C
	$(CC) $(CPPFLAGS) -c $< $(INCPATH) -o $@

test: test.o
	$(CC) $< $(LIBCORE) -o $@
###test: test.o ../Property.o ../Event.o
###	$(CC) $< $(LIBCORE) -o $@

depend:
	makedepend $(INCPATH) -- $(CPPFLAGS) -- $(SRCS)

# DO NOT DELETE
test.o: ../Event.h ../Property.h ../Track.h ../NotationTypes.h
