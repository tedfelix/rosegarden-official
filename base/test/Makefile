CC	 = g++
# debug flags need to be consistent with base build
#CPPFLAGS = -O2 
CPPFLAGS = -g 

LIBBASE  = ../.libs/libbase.a
###LIBBASE  = ../Property.o ../Event.o ../NotationTypes.o ../Track.o ../Quantizer.o

INCPATH  = -I..

SRCS	:= test.C

default: test thread

clean:
	rm -f test test.o

%.o: %.C
	$(CC) $(CPPFLAGS) -c $< $(INCPATH) -o $@

test: test.o
	$(CC) $< $(LIBBASE) -o $@

thread: thread.o
	$(CC) $< $(LIBBASE) -o $@ -lpthread

###test: test.o ../Property.o ../Event.o ../NotationTypes.o ../Track.o ../Quantizer.o
###	$(CC) $< $(LIBBASE) -o $@

depend:
	makedepend $(INCPATH) -- $(CPPFLAGS) -- $(SRCS)

# DO NOT DELETE
test.o: ../Event.h ../Property.h ../Segment.h ../NotationTypes.h
