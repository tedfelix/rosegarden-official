
all:	test speedtrial libcore.a

CC	:= g++
CPPFLAGS	:= -g

test:	test.o Element.o Element2.o

libcore.a:	Element2.o
	ar rc libcore.a $<

clean:
	rm *.o test speedtrial

# DO NOT DELETE
