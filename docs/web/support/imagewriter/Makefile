
#CFLAGS	:=	-g -I../libpng-1.0.5 -I../zlib-1.1.3 -I../gd-1.7.3
CFLAGS	:=	-g -I/usr/local/include -I/usr/local/include/freetype2
#LIBS	:=	-L../gd-1.7.3 -lgd -L../freetype-1.2/lib -lttf -L../libpng-1.0.5 -lpng -L../zlib-1.1.3 -lz
LIBS	:=	-L/usr/local/lib -lgd -lfreetype -lpng -ljpeg -lz

%.o:		%.C
		c++ -c $(CFLAGS) $<

write:		ImageWriter.o write.o
		c++ ImageWriter.o write.o -o imagewriter $(LIBS)

clean:
		rm -f ImageWriter.o write.o

distclean:	clean
		rm -f imagewriter
