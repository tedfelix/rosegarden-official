
default: libcore.a

all:	libcore.a

CC	:= g++
CPPFLAGS	:= -g
SRCS	:= Property.C Event.C ViewElement.C Track.C Composition.C NotationTypes.C
OBJS	:= Property.o Event.o ViewElement.o Track.o Composition.o NotationTypes.o

libcore.a:	$(OBJS)
	ar rc libcore.a $(OBJS)

depend:
	makedepend -- $(CPPFLAGS) -- $(SRCS)

clean:
	rm -f *.o 

distclean:	clean
	rm -f libcore.a

# DO NOT DELETE
