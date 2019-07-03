
# debug flags need to be consistent with base build
#CPPFLAGS = -O2 
CPPFLAGS = -g 

LIBBASE  = ../../../RGbuild/libRosegardenCommon.a

INCPATH  = -I..

SRCS	:= test.C pitch.C

default: test utf8 colour transpose accidentals realtime

clean:
	rm -f test test.o pitch pitch.o utf8 utf8.o colour colour.o transpose.o transpose accidentals.o accidentals realtime.o realtime

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c $< $(INCPATH) -o $@

test: test.o
	$(CXX) $< $(LIBBASE) -o $@

pitch: pitch.o
	$(CXX) $< $(LIBBASE) -o $@

utf8: utf8.o
	$(CXX) $< $(LIBBASE) -o $@

colour: colour.o
	$(CXX) $< $(LIBBASE) -o $@

transpose: transpose.o
	$(CXX) $< $(LIBBASE) -o $@

accidentals: accidentals.o
	$(CXX) $< $(LIBBASE) -o $@

realtime: realtime.o
	$(CXX) $< $(LIBBASE) -o $@


depend:
	makedepend $(INCPATH) -- $(CPPFLAGS) -- $(SRCS)

# DO NOT DELETE

test.o: ../Event.h ../PropertyMap.h ../Property.h ../RealTime.h
test.o: ../PropertyName.h ../Exception.h ../Segment.h ../Track.h
test.o: ../XmlExportable.h ../Instrument.h ../NotationTypes.h #../StringHash.h
test.o: ../XmlExportable.h ../Instrument.h ../NotationTypes.h 
test.o: ../RefreshStatus.h ../Composition.h
test.o: ../Configuration.h ../ColourMap.h ../Colour.h
test.o: ../SegmentNotationHelper.h ../SegmentPerformanceHelper.h
test.o: ../MidiTypes.h
pitch.o: ../NotationTypes.h ../Event.h ../PropertyMap.h ../Property.h
pitch.o: ../RealTime.h ../PropertyName.h ../Exception.h ../Instrument.h
pitch.o: ../XmlExportable.h #../StringHash.h

transpose.o: ../NotationTypes.h 
accidentals.o: ../NotationTypes.h 
realtime.o: ../RealTime.h ../RealTime.cpp
