CC	 = g++
CPPFLAGS = -g
LIBCORE  = ../libcore.a
INCPATH  = -I..

default: test

clean:
	rm -f test test.o

%.o: %.C
	$(CC) $(CPPFLAGS) -c $< $(INCPATH) -o $@

test: test.o
	$(CC) $< $(LIBCORE) -o $@
