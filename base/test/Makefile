CC	 = g++
CPPFLAGS = -g
LIBCORE  = ../libcore.a
INCPATH  = -I..

default: test

clean:
	rm -f test

%.o: %.C
	$(CC) -c $< $(INCPATH) -o $@

test: test.o
	$(CC) $< $(LIBCORE) -o $@
