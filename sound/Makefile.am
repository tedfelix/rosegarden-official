KDE_CXXFLAGS = $(USE_EXCEPTIONS)

if NO_SOUND 
else
if HAVE_ALSA
else
arts_includes= -I$(kde_includes)/arts 
endif
endif

# over stated path
INCLUDES= $(all_includes) $(arts_includes) -I$(top_srcdir)/gui -I$(top_srcdir)/base @JACK_CFLAGS@

COMPILE_FIRST = libRosegardenSound

# The two libraries built from this directory are libRosegardenSound
# and libRosegardenSequencer
#
noinst_LTLIBRARIES = libRosegardenSound.la
libRosegardenSound_la_SOURCES = AudioCache.cpp \
				AudioFile.cpp \
				AudioFileManager.cpp \
				AudioPlayQueue.cpp \
				AudioProcess.cpp \
				Audit.cpp \
                                BWFAudioFile.cpp \
				ControlBlock.cpp \
                                DSSIPluginFactory.cpp \
                                DSSIPluginInstance.cpp \
                                LADSPAPluginFactory.cpp \
                                LADSPAPluginInstance.cpp \
				MappedComposition.cpp \
                                MappedDevice.cpp \
                                MappedEvent.cpp \
                                MappedInstrument.cpp \
                                MappedRealTime.cpp \
                                MappedStudio.cpp \
                                MidiEvent.cpp \
                                MidiFile.cpp \
                                MP3AudioFile.cpp \
                                PeakFile.cpp \
                                PeakFileManager.cpp \
				PluginFactory.cpp \
				PluginIdentifier.cpp \
                                RIFFAudioFile.cpp \
				RunnablePluginInstance.cpp \
				SequencerDataBlock.cpp \
                                SF2PatchExtractor.cpp \
                                SoundFile.cpp \
                                WAVAudioFile.cpp

lib_LTLIBRARIES = libRosegardenSequencer.la


EXTRA_libRosegardenSequencer_la_SOURCES = MidiArts.cc \
                                          ArtsDriver.cpp \
                                          AlsaDriver.cpp \
					  JackDriver.cpp \
                                          MidiRecord.cpp \
					  PlayableAudioFile.cpp \
                                          SoundDriver.cpp \
                                          SoundDriverFactory.cpp


# Conditional compile for the different types of sound support.
#
#

# No sound support (use DummyDriver)
if NO_SOUND
libRosegardenSequencer_la_SOURCES = PlayableAudioFile.cpp \
				    SoundDriver.cpp \
				    SoundDriverFactory.cpp
else

if HAVE_ALSA
libRosegardenSequencer_la_SOURCES = AlsaDriver.cpp \
                                    AlsaPort.cpp \
				    JackDriver.cpp \
				    PlayableAudioFile.cpp \
                                    SoundDriver.cpp \
                                    SoundDriverFactory.cpp

else

arts_libs=-lsoundserver_idl -lartsmidi -lartsflow -lartsmodules -lartskde 

libRosegardenSequencer_la_SOURCES = MidiArts.cc \
                                    ArtsDriver.cpp \
                                    MidiRecord.cpp \
				    PlayableAudioFile.cpp \
                                    SoundDriver.cpp \
                                    SoundDriverFactory.cpp
endif

endif

libRosegardenSequencer_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -no-undefined
libRosegardenSequencer_la_LIBADD   = $(LIB_QT) $(arts_libs) libRosegardenSound.la ../base/libbase.la @ALSA_LIBS@  @JACK_LIBS@

libRosegardenSequencer_la_CFLAGS = @ALSA_CFLAGS@

# aRts stubs generated from idl
#
MidiArts.h: $(srcdir)/MidiArts.idl
	$(MCOPIDL) -I$(kde_includes) -I$(kde_includes)/arts -t $(srcdir)/MidiArts.idl

MidiArts.cc: MidiArts.h
MidiArts.mcopclass: MidiArts.h
MidiArts.mcoptype: MidiArts.h
MidiRecord.cpp: MidiArts.h


libRosegardenSound_la_METASOURCES = AUTO

# when cleaning
#
CLEANFILES = MidiArts.h MidiArts.cc
DISTCLEANFILES = $(sound_METASOURCES) MidiArts.h MidiArts.cc


# some test programs
#
#
#bin_PROGRAMS = showtax
#showtax_SOURCES = showtaxonomy.cpp
#miditest_LDADD = -lasound -lqt libRosegardenSound.la libRosegardenSequencer.la ./base/libbase.la

#audiotest_SOURCES = audiotest.cpp
#audiotest_LDADD = -lartsmidi -lqt -lartsflow -lartsmodules -lsoundserver_idl libRosegardenSound.la libRosegardenSequencer.la -lsndfile

