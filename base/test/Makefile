CC	 = g++
# debug flags need to be consistent with base build
#CPPFLAGS = -O2 
CPPFLAGS = -g 

LIBBASE  = ../.libs/libbase.a

INCPATH  = -I..

SRCS	:= test.C thread.C pitch.C

default: test thread pitch

clean:
	rm -f test test.o thread thread.o pitch pitch.o

%.o: %.C
	$(CC) $(CPPFLAGS) -c $< $(INCPATH) -o $@

test: test.o
	$(CC) $< $(LIBBASE) -o $@

pitch: pitch.o
	$(CC) $< $(LIBBASE) -o $@

thread: thread.o
	$(CC) $< $(LIBBASE) -o $@ -lpthread

depend:
	makedepend $(INCPATH) -- $(CPPFLAGS) -- $(SRCS)

# DO NOT DELETE

test.o: ../Event.h ../PropertyMap.h ../Property.h ../RealTime.h
test.o: ../PropertyName.h ../Exception.h ../Segment.h ../Track.h
test.o: ../XmlExportable.h ../Instrument.h ../NotationTypes.h ../StringHash.h
test.o: ../RefreshStatus.h ../Composition.h ../FastVector.h
test.o: ../Configuration.h ../SegmentNotationHelper.h
test.o: ../SegmentPerformanceHelper.h ../MidiTypes.h 
thread.o: ../Lock.h ../Composition.h ../FastVector.h ../RealTime.h
thread.o: ../Segment.h ../Track.h ../XmlExportable.h ../Instrument.h
thread.o: ../Event.h ../PropertyMap.h ../Property.h ../PropertyName.h
thread.o: ../Exception.h ../NotationTypes.h ../StringHash.h
thread.o: ../RefreshStatus.h ../Configuration.h 
pitch.o: ../NotationTypes.h ../Event.h ../PropertyMap.h ../Property.h
pitch.o: ../RealTime.h ../PropertyName.h ../Exception.h ../Instrument.h
pitch.o: ../XmlExportable.h ../StringHash.h
