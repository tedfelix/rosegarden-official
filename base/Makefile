
all:	test speedtrial libcore.a

CC	:= g++
CPPFLAGS	:= -g

test:	test.o Element.o Element2.o

libcore.a:	Element2.o NotationTypes.o
	ar rc libcore.a Element2.o NotationTypes.o

NotationTypes.o:	NotationTypes.h

clean:
	rm -f *.o test speedtrial

# DO NOT DELETE
