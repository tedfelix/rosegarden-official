# ensure our path is correct
INCLUDES= $(all_includes) -I$(kde_includes)/arts -I../core

COMPILE_FIRST = libRosegardenSound

####### This part is very Rosegarden specific
# you can add here more. This one gets installed 
noinst_LTLIBRARIES = libRosegardenSound.la
libRosegardenSound_la_SOURCES = MidiFile.cpp MidiEvent.cpp MappedEvent.cpp MappedComposition.cpp

lib_LTLIBRARIES = libRosegardenSequencer.la
libRosegardenSequencer_la_SOURCES = MidiRecord.cpp MidiArts.cc Sequencer.cpp

# the library search path. 
libRosegardenSequencer_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -no-undefined

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
libRosegardenSequencer_la_LIBADD   = -lartsmidi -lqt -lsoundserver_idl libRosegardenSound.la ../core/libcore.la

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

bin_PROGRAMS = sound
sound_SOURCES = sound.cpp
sound_LDADD = -lqt libRosegardenSound.la ../core/libcore.la

