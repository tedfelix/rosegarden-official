KDE_CXXFLAGS = $(USE_EXCEPTIONS)

# over stated path
INCLUDES= $(all_includes) -I$(kde_includes)/arts -I$(top_srcdir)/base


COMPILE_FIRST = libRosegardenSound

# The two libraries built from this directory are libRosegardenSound
# and libRosegardenSequencer
#
noinst_LTLIBRARIES = libRosegardenSound.la
libRosegardenSound_la_SOURCES = AudioFile.cpp \
                                AudioFileManager.cpp \
                                BWFAudioFile.cpp \
                                MidiFile.cpp \
                                MidiEvent.cpp \
                                MappedDevice.cpp \
                                MappedEvent.cpp \
                                MappedComposition.cpp \
                                MappedInstrument.cpp \
                                MappedRealTime.cpp \
                                MappedStudio.cpp \
                                PeakFile.cpp \
                                PeakFileManager.cpp \
                                PluginManager.cpp \
                                RIFFAudioFile.cpp \
                                SoundFile.cpp \
                                WAVAudioFile.cpp

lib_LTLIBRARIES = libRosegardenSequencer.la


EXTRA_libRosegardenSequencer_la_SOURCES = MidiArts.cc \
                                          ArtsDriver.cpp \
                                          AlsaDriver.cpp \
                                          MidiRecord.cpp \
                                          SoundDriver.cpp \
                                          Sequencer.cpp

# Conditional compile for the different types of sound support.
#
#

# No sound support (use DummyDriver)
if NO_SOUND
libRosegardenSequencer_la_SOURCES = SoundDriver.cpp \
                                    Sequencer.cpp
else

if HAVE_ALSA
libRosegardenSequencer_la_SOURCES = AlsaDriver.cpp \
                                    Sequencer.cpp \
                                    SoundDriver.cpp

else

libRosegardenSequencer_la_SOURCES = MidiArts.cc \
                                    ArtsDriver.cpp \
                                    MidiRecord.cpp \
                                    Sequencer.cpp \
                                    SoundDriver.cpp
endif

endif

libRosegardenSequencer_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -no-undefined
libRosegardenSequencer_la_LIBADD   = -lartsmidi $(LIB_QT) -lsoundserver_idl libRosegardenSound.la ../base/libbase.la -lartsflow -lartsmodules @ALSA_LIBS@

libRosegardenSequencer_la_CFLAGS = @ALSA_CFLAGS@

# aRts stubs generated from idl
#
MidiArts.h: $(srcdir)/MidiArts.idl
	$(MCOPIDL) -I$(kde_includes) -I$(kde_includes)/arts -t $(srcdir)/MidiArts.idl

MidiArts.cc: MidiArts.h
MidiArts.mcopclass: MidiArts.h
MidiArts.mcoptype: MidiArts.h
MidiRecord.cpp: MidiArts.h

# when cleaning
#
CLEANFILES = MidiArts.h MidiArts.cc
DISTCLEANFILES = $(sound_METASOURCES) MidiArts.h MidiArts.cc


# some test programs
#
#
#bin_PROGRAMS = audiotest
#miditest_SOURCES = miditest.cpp
#miditest_LDADD = -lasound -lqt libRosegardenSound.la libRosegardenSequencer.la ./base/libbase.la

#audiotest_SOURCES = audiotest.cpp
#audiotest_LDADD = -lartsmidi -lqt -lartsflow -lartsmodules -lsoundserver_idl libRosegardenSound.la libRosegardenSequencer.la -lsndfile

