kde_includes=/opt/arts-0.5.4/include

# we define it here but make sometimes ignores it
LIBTOOL=libtool
MCOPIDL=mcopidl

# set the include path for X, qt and KDE
INCLUDES= $(all_includes) -I$(kde_includes)/arts -I../core
#INCLUDES= $(all_includes) -I/usr/include/arts


####### This part is very Rosegarden specific
# you can add here more. This one gets installed 
lib_LTLIBRARIES = libMidiArts.la

libMidiArts_la_SOURCES = MidiRecord.cpp MidiArts.cc

# the library search path. 
libMidiArts_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -no-undefined

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
libMidiArts_la_LIBADD   = -lartsmidi

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

#mcoptypedir = $(libdir)/mcop
#mcoptype_DATA = MidiArts.mcoptype MidiArts.mcopclass MidiRecord.mcopclass

# our executable

bin_PROGRAMS = sound
sound_METASOURCES = AUTO
sound_SOURCES = MidiFile.cpp MidiEvent.cpp Sequencer.cpp sound.cpp
sound_LDFLAGS =
sound_LDADD = libMidiArts.la -lsoundserver_idl ../core/libcore.a

DISTCLEANFILES = $(sound_METASOURCES) MidiArts.h MidiArts.cc
