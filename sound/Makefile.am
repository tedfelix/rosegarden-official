KDE_CXXFLAGS = $(USE_EXCEPTIONS)

# ensure our path is correct
INCLUDES= $(all_includes) -I$(kde_includes)/arts -I../base

COMPILE_FIRST = libRosegardenSound

####### This part is very Rosegarden specific
# you can add here more. This one gets installed 

noinst_LTLIBRARIES = libRosegardenSound.la
libRosegardenSound_la_SOURCES = AudioFile.cpp \
                                AudioFileManager.cpp \
                                MidiFile.cpp \
                                MidiEvent.cpp \
                                MappedDevice.cpp \
                                MappedEvent.cpp \
                                MappedComposition.cpp \
                                MappedInstrument.cpp \
                                SoundFile.cpp

lib_LTLIBRARIES = libRosegardenSequencer.la
libRosegardenSequencer_la_SOURCES = ArtsDriver.cpp\
				    AlsaDriver.cpp \
                                    MidiArts.cc \
                                    MidiRecord.cpp \
                                    Sequencer.cpp \
                                    SoundDriver.cpp

# the library search path. 
libRosegardenSequencer_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -no-undefined

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
libRosegardenSequencer_la_LIBADD   = -lartsmidi -lqt -lsoundserver_idl libRosegardenSound.la ../base/libbase.la -lartsflow -lartsmodules -lasound

# make messages.po. Move this one to ../po/ and "make merge" in po
# the -x is for skipping messages already translated in kdelibs
#messages:
#	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(kless_SOURCES) && mv messages.po ../po/kless.pot

MidiArts.h: $(srcdir)/MidiArts.idl
	$(MCOPIDL) -I$(kde_includes) -t $(srcdir)/MidiArts.idl

MidiArts.cc: MidiArts.h
MidiArts.mcopclass: MidiArts.h
MidiArts.mcoptype: MidiArts.h
MidiRecord.cpp: MidiArts.h

DISTCLEANFILES = $(sound_METASOURCES) MidiArts.h MidiArts.cc

# Variable names are broken up because otherwise the Makefile.cvs
# scanning picks them up
#
#bin_PROGRAMS = miditest
#miditest_SOURCES = miditest.cpp
#miditest_LDADD = -lasound -lqt libRosegardenSound.la libRosegardenSequencer.la ./base/libbase.la

#audiotest_SOURCES = audiotest.cpp
#audiotest_LDADD = -lartsmidi -lqt -lartsflow -lartsmodules -lsoundserver_idl libRosegardenSound.la libRosegardenSequencer.la
