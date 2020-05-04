# g++ options
CXXFLAGS = -std=c++11 -Wall -Wextra -Wundef

all : widestring

# At the very least, we need to explain to "make" how to make the
# binary from the object files.  Via implicit rules, it knows
# how to make a ".o" from what it finds in the directory.
widestring : widestring.o
	g++ -o widestring widestring.o

.PHONY : clean
clean :
	rm widestring widestring.o

