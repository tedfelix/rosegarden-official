
default: libcore.a

all:	libcore.a

CC	:= g++
CPPFLAGS	:= -g -Wall
SRCS	:= Property.C Event.C ViewElement.C Track.C Composition.C NotationTypes.C Equation.C
OBJS	:= Property.o Event.o ViewElement.o Track.o Composition.o NotationTypes.o Equation.o

libcore.a:	$(OBJS)
	ar rc libcore.a $(OBJS)

depend:
	makedepend -- $(CPPFLAGS) -- $(SRCS)

clean:
	rm -f *.o 

distclean:	clean
	rm -f libcore.a *~

# DO NOT DELETE

Property.o: Property.h
Event.o: Event.h Property.h
ViewElement.o: ViewElement.h Event.h Property.h
Track.o: Track.h Event.h Property.h
Composition.o: Composition.h Track.h Event.h Property.h
NotationTypes.o: NotationTypes.h Event.h Property.h
Equation.o: Equation.h
