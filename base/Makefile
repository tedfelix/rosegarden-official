
default: libcore.a

all:	test speedtrial libcore.a

CC	:= g++
CPPFLAGS	:= -g

test:	test.o Element.o Event.o

libcore.a:	Event.o NotationTypes.o
	ar rc libcore.a Event.o NotationTypes.o

NotationTypes.o:	NotationTypes.h

clean:
	rm -f *.o test speedtrial

distclean:	clean
	rm -f libcore.a

# DO NOT DELETE
