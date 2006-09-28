
# debug flags need to be consistent with base build
#CPPFLAGS = -O2 
CPPFLAGS = -g 

LIBBASE  = ../.libs/libbase.a

INCPATH  = -I..

SRCS	:= test.C pitch.C

default: test utf8 colour

clean:
	rm -f test test.o pitch pitch.o utf8 utf8.o colour colour.o

%.o: %.C
	$(CC) $(CPPFLAGS) -c $< $(INCPATH) -o $@

test: test.o
	$(CC) $< $(LIBBASE) -o $@

pitch: pitch.o
	$(CC) $< $(LIBBASE) -o $@

utf8: utf8.o
	$(CC) $< $(LIBBASE) -o $@

colour: colour.o
	$(CC) $< $(LIBBASE) -o $@

depend:
	makedepend $(INCPATH) -- $(CPPFLAGS) -- $(SRCS)

# DO NOT DELETE

test.o: ../Event.h ../PropertyMap.h ../Property.h ../RealTime.h
test.o: ../PropertyName.h ../Exception.h ../Segment.h ../Track.h
test.o: ../XmlExportable.h ../Instrument.h ../NotationTypes.h ../StringHash.h
test.o: ../RefreshStatus.h ../Composition.h ../FastVector.h
test.o: ../Configuration.h ../ColourMap.h ../Colour.h
test.o: ../SegmentNotationHelper.h ../SegmentPerformanceHelper.h
test.o: ../MidiTypes.h
pitch.o: ../NotationTypes.h ../Event.h ../PropertyMap.h ../Property.h
pitch.o: ../RealTime.h ../PropertyName.h ../Exception.h ../Instrument.h
pitch.o: ../XmlExportable.h ../StringHash.h
