
CFLAGS	:=	-g -I/usr/local/include -I/usr/local/include/freetype2
LIBS	:=	-L/usr/local/lib -lgd -lfreetype -lpng -ljpeg -lz

%.o:		%.C
		c++ -c $(CFLAGS) $<

write:		ImageWriter.o write.o
		c++ ImageWriter.o write.o -o imagewriter $(LIBS)

clean:
		rm -f ImageWriter.o write.o

distclean:	clean
		rm -f imagewriter
