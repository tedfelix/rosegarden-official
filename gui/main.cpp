// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <qtimer.h>
#include <ctime>

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <dcopclient.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kstddirs.h>

#include "rosestrings.h"
#include "rosedebug.h"
#include "rosegardengui.h"

#include "kstartuplogo.h"

/*! \mainpage Rosegarden-4 global design

Rosegarden is split into 3 main parts:

\section base Base

The base library holds all of the fundamental "music handling"
structures, of which the primary ones are Event, Segment, Track,
Instrument and Composition.  It also contains a selection of utility
and helper classes of a kind that is not specific to any particular
GUI.  Everything here is part of the Rosegarden namespace, and there
are no dependencies on KDE or Qt (although it uses the STL heavily).

The keyword for the basic structures in use is "flexibility".  Our
Event objects can be extended arbitrarily for the convenience of GUI
or performance code without having to change their declaration or
modify anything in the base library.  And most of our assumptions
about the use of the container classes can be violated without
disastrous side-effects.

\subsection musicstructs Music Structures

 - \link Rosegarden::Event Event\endlink is the basic musical element.  It's more or less a
    generalization of the MIDI event.  Each note or rest, each key
    change or tempo change, is an event: there's no "note class" or
    "rest class" as such, they are simply represented by events whose
    type happens to be "note" or "rest".
    Each Event has a type code, absolute time (the moment at which the
    Event starts, relative only to the start of the Composition) and
    duration (usually non-zero only for notes and rests), together
    with an arbitrary set of named and typed properties that can be
    assigned and queried dynamically by other parts of the
    application.  So, for example, a note event is likely to have an
    integer property called "pitch", and probably a "velocity", as
    well as potentially many others -- but this is not fixed anywhere,
    and there's no definition of what exactly a note is: client code
    is simply expected to ignore any unrecognised events or properties
    and to cope if properties that should be there are not.

 - \link Rosegarden::Segment Segment\endlink is a series of consecutive Events found on the same Track,
    automatically ordered by their absolute time.  It's the usual
    container for Events.  A Segment has a starting time that can be
    changed, and a duration that is based solely on the end time of
    the last Event it contains.  Note that in order to facilitate
    musical notation editing, we explicitly store silences as series
    of rest Events; thus a Segment really should contain no gaps
    between its Events.  (This isn't checked anywhere and nothing will
    break very badly if there are gaps, but notation won't quite work
    correctly.)

 - \link Rosegarden::Track Track \endlink is much the same thing as on a mixing table, usually
    assigned to an instrument, a voice, etc.  Although a Track is not
    a container of Events and is not strictly a container of Segments
    either, it is referred to by a set of Segments that are therefore
    mutually associated with the same instruments and parameters.  In
    GUI terms, the Track is a horizontal row on the main Rosegarden
    window, whereas a Segment is a single blue box within that row, of
    which there may be any number.

 - \link Rosegarden::Instrument Instrument \endlink corresponds broadly to a MIDI or Audio channel, and is
    the destination for a performed Event.  Each Track is mapped to a
    single Instrument (although many Tracks may have the same
    Instrument), and the Instrument is indicated in the header at the
    left of the Track's row in the GUI.

 - \link Rosegarden::Composition Composition\endlink is the container for the entire piece of music.  It
    consists of a set of Segments, together with a set of Tracks that
    the Segments may or may not be associated with, a set of
    Instruments, and some information about time signature and tempo
    changes.  (The latter are not stored in Segments; they are only
    stored in the top-level Composition.  You can't have differing
    time signatures or tempos in different Segments.)  Any code that
    wants to know about the locations of bar lines, or request
    real-time calculations based on tempo changes, talks to the
    Composition.


See also docs/data_struct/units.txt for an explanation of the units we
use for time and pitch values.  See docs/discussion/names.txt for some
name-related discussion.  See docs/code/creating_events.txt for an
explanation of how to create new Events and add properties to them.

The base directory also contains various music-related helper classes:

 - The NotationTypes.[Ch] files contain classes that help with
    creating and manipulating events.  It's very important to realise
    that these classes are not the events themselves: although there
    is a Note class in this file, and a TimeSignature class, and Clef
    and Key classes, instances of these are rarely stored anywhere.
    Instead they're created on-the-fly in order to do calculation
    related to note durations or time signatures or whatever, and they
    contain getAsEvent() methods that may be used when an event for
    storage is required.  But the class of a stored event is always
    simply Event.

    The NotationTypes classes also define important constants for the
    names of common properties in Events.  For example, the Note class
    contains Note::EventType, which is the type of a note Event, and
    Note::EventRestType, the type of a rest Event; and Key contains
    Key::EventType, the type of a key change Event, KeyPropertyName,
    the name of the property that defines the key change, and a set
    of the valid strings for key changes.

 - BaseProperties.[Ch] contains a set of "standard"-ish Event
    property names that are not basic enough to go in NotationTypes.

 - \link Rosegarden::SegmentNotationHelper SegmentNotationHelper\endlink
    and \link Rosegarden::SegmentPerformanceHelper SegmentPerformanceHelper\endlink
    do tasks that
    may be useful to notation-type code and performer code
    respectively.  For example, SegmentNotationHelper is used to
    manage rests when inserting and deleting notes in a score editor,
    and to create beamed groups and suchlike; SegmentPerformanceHelper
    generally does calculations involving real performance time of
    notes (taking into account tied notes, tuplets and tempo changes).
    These two lightweight helper classes are also usually constructed
    on-the-fly for use on the events in a given Segment and then
    discarded after use.

 - \link Rosegarden::Quantizer Quantizer\endlink is used to quantize event timings and set quantized
    timing properties on those events.  Note that quantization is
    non-destructive, as it takes advantage of the ability to set new
    Event properties to simply assign the quantized values as separate
    properties from the original absolute time and duration.


\section gui GUI

The GUI directory builds into a KDE/Qt application. Like most KDE
applications, it follows a document/view model. The document (class
RosegardenGUIDoc, which wraps a Composition) can have several views
(class RosegardenGUIView), although at the moment only a single one is
used.

This view is the TrackEditor, which shows all the Composition's
Segments organized in Tracks. Each Segment can be edited in two ways:
notation (score) or matrix (piano roll).

All editor views are derived from EditView. An EditView is the class
dealing with the edition per se of the events. It uses several
components:

 - Layout classes, horizontal and vertical: these are the classes
    which determine the x and y coordinates of the graphic items
    representing the events (notes or piano-roll rectangles).  They
    are derived from the LayoutEngine base-class in the base library.

 - Tools, which implement each editing function at the GUI (such as
    insert, erase, cut and paste). These are the tools which appear on
    the EditView's toolbar.

 - Toolbox, which is a simple string => tool map.

 - Commands, which are the fundamental implementations of editing
    operations (both menu functions and tool operations) subclassed
    from KDE's Command and used for undo and redo.

 - a canvas view.  Although this isn't a part of the EditView's
    definition, both of the existing edit views (notation and matrix)
    use one, because they both use a QCanvas to represent data.

 - LinedStaff, a staff with lines.  Like the canvas view, this isn't
    part of the EditView definition, but both views use one.


There are currently two editor views:

 - NotationView, with accompanying classes NotationHLayout,
    NotationVLayout, NotationStaff, and all the classes in the
    notationtool and notationcommands files.  These are also closely
    associated with the NotePixmapFactory and NoteFont classes, which
    are used to generate notes from component pixmap files.

 - MatrixView, with accompanying classes MatrixHLayout,
    MatrixVLayout, MatrixStaff and other classes in the matrixview
    files.

The editing process works as follows:

[NOTE : in the following, we're talking both about events as UI events
or user events (mouse button clicks, mouse move, keystrokes, etc...)
and Events (our basic music element).  To help lift the ambiguity,
"events" is for UI events, Events is for Rosegarden::Event.]

 -# The canvas view gets the user events (see
    NotationCanvasView::contentsMousePressEvent(QMouseEvent*) for an
    example).  It locates where the event occured in terms of musical
    element: which note or staff line the user clicked on, which pitch
    and time this corresponds to, that kind of stuff.  (In the
    Notation and Matrix views, the LinedStaff calculates mappings
    between coordinates and staff lines: the former is especially
    complicated because of its support for page layout.)\n
 -# The canvas view transmits this kind of info as a signal, which is
 connected to a slot in the parent EditView.
 -# The EditView delegates action to the current tool.\n
 -# The tool performs the actual job (inserting or deleting a note,
    etc...).

Since this action is usually complex (merely inserting a note requires
dealing with the surrounding Events, rests or notes), it does it
through a SegmentHelper (for instance, base/SegmentNotationHelper)
which "wraps" the complexity into simple calls and performs all the
hidden tasks.

The EditView also maintains (obviously) its visual appearance with the
layout classes, applying them when appropriate.

\section sequencer Sequencer

The sequencer directory also builds into a KDE/Qt application, but one
which doesn't have a gui.  The Sequencer can be started automatically
by the main Rosegarden GUI or manually if testing - it's sometimes
more convenient to do the latter as the Sequencer needs to be connected
up to the underlying sound system every time it is started.

The Sequencer interfaces with aRTS (www.arts-project.org) sound system
and provides MIDI "play" and "record" ports which can be connected to
other MIDI clients (MIDI IN and OUT hardware ports or aRTS synth devices)
using the aRTS Midi Manager.  The Sequencer will also eventually  support
playing and recording of Audio sample files.

The GUI and Sequencer communicate using the KDE DCOP communication framework.
Look in:

    rosegarden/gui/rosegardenguiiface.h
    rosegarden/sequencer/rosegardensequenceriface.h

for definitions of the DCOP interfaces pertinent to the Sequencer
and GUI.  The main DCOP operations from the GUI involve starting and
stopping the Sequencer, playing and recording, fast forwarding and
rewinding.  Once a play or record cycle is enabled it's the Sequencer
that does most of the hard work.  To service a play() command the
Sequencer fetches a slice of Events from the Rosegarden Composition
(see getSequencerSlice()) and queues them up with the aRTS MidiEvent
dispatcher.  Interlaced within the main Sequencer loop is a call
which also services pending incoming MIDI events from the Rosegarden
record port and forwards them upwards to the GUI.

The Rosegarden record port is built around a specialisation of the
Arts::MidiPort - see rosegarden/sequencer/MidiArts.idl for the 
interface definition and check out the rosegarden/sequencer/Makefile.am
for how it gets built.  This record port has to be run as part of the aRTS
sound server - check out the documentation in:

    rosegarden/docs/howtos/artsd-mcop-notes

for more information about how to get this working.

The Sequencer makes use of two libraries libRosegardenSequencer
and libRosegardenSound:

 - libRosegardenSequencer holds everything pertinent to sequencing
    for Rosegarden including the aRTS MCOP record interface and the
    Sequencer class itself.  This library is only linked into the
    Rosegarden Sequencer.

 - libRosegardenSound holds the MidiFile class (writing and reading
    MIDI files) and the MappedEvent and MappedComposition classes (the
    communication class for transferring events back and forth across
    DCOP).  This library is needed by the GUI as well as the Sequencer.

The main Sequencer state machine is a good starting point and clearly
visible at the bottom of rosegarden/sequencer/main.cpp.


*/

static const char *description =
I18N_NOOP("Rosegarden - A sequencer and musical notation editor");

static KCmdLineOptions options[] =
{
    { "nosplash", I18N_NOOP("don't show splash screen"), 0 },
    { "nosequencer", I18N_NOOP("don't use an external sequencer"), 0 },
    { "+[File]", I18N_NOOP("file to open"), 0 },
    { 0, 0, 0 }
};

void testInstalledVersion()
{
    QString versionLocation = locate("appdata", "version.txt");
    QString installedVersion;

    if (versionLocation) {
	QFile versionFile(versionLocation);
	if (versionFile.open(IO_ReadOnly)) {
	    QTextStream text(&versionFile);
	    QString s = text.readLine().stripWhiteSpace();
	    versionFile.close();
	    if (s) {
		if (s == VERSION) return;
		installedVersion = s;
	    }
	}
    }

    if (installedVersion) {

	KMessageBox::detailedError
	    (0,
	     i18n("Installation contains the wrong version of Rosegarden."),
	     i18n(" The wrong versions of Rosegarden's data files were\n"
		  " found in the standard KDE installation directories.\n"
		  " (I am " + QString(VERSION) + ", but the installed files are for version " + installedVersion + ".)\n\n"
		  " This may mean one of the following:\n\n"
		  " 1. This is a new upgrade of Rosegarden, and it has not yet been\n"
		  "     installed.  If you compiled it yourself, check that you have\n"
		  "     run \"make install\" and that the procedure completed\n"
		  "     successfully.\n\n"
		  " 2. The upgrade was installed in a non-standard directory,\n"
		  "     and an old version was found in a standard directory.  If so,\n"
		  "     you will need to add the correct directory to your KDEDIRS\n"
		  "     environment variable before you can run it."),
	     i18n("Installation problem"));
	
    } else {

	KMessageBox::detailedError
	    (0,
	     i18n("Rosegarden does not appear to have been installed."),
	     i18n(" One or more of Rosegarden's data files could not be\n"
		  " found in the standard KDE installation directories.\n\n"
		  " This may mean one of the following:\n\n"
		  " 1. Rosegarden has not been correctly installed.  If you compiled\n"
		  "     it yourself, check that you have run \"make install\" and that\n"
		  "     the procedure completed successfully.\n\n"
		  " 2. Rosegarden has been installed in a non-standard directory,\n"
		  "     and you need to add this directory to your KDEDIRS environment\n"
		  "     variable before you can run it.  This may be the case if you\n"
		  "     installed into $HOME or a local third-party package directory\n"
		  "     like /usr/local or /opt."),
	     i18n("Installation problem"));
    }

    exit(1);
}

int main(int argc, char *argv[])
{
    KAboutData aboutData( "rosegarden", I18N_NOOP("Rosegarden"),
                          VERSION, description, KAboutData::License_GPL,
                          "Copyright 2000 - 2002 Guillaume Laurent, Chris Cannam, Richard Bown\nParts copyright 1994 - 2001 Chris Cannam, Andy Green, Richard Bown, Guillaume Laurent\nLilypond fonts copyright 1997 - 2001 Han-Wen Nienhuys and Jan Nieuwenhuizen");
    aboutData.addAuthor("Guillaume Laurent, Chris Cannam, Richard Bown",0,
                        "glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

    KApplication app;

    // Give up immediately if we haven't been installed or if the
    // installation is out of date
    //
    testInstalledVersion();

    //
    // Ensure quit on last window close
    // Register main DCOP interface
    //
    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
    app.dcopClient()->registerAs(app.name(), false);
    app.dcopClient()->setDefaultObject(ROSEGARDEN_GUI_IFACE_NAME);

    // Parse cmd line args
    //
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    // Show Startup logo
    // (this code borrowed from KDevelop 2.0,
    // (c) The KDevelop Development Team
    //
    KConfig* config = KGlobal::config();
    config->setGroup("General Options");
    KStartupLogo* startLogo = 0L;


    if (config->readBoolEntry("Logo",true) && (!kapp->isRestored() && args->isSet("splash")) )
    {
	RG_DEBUG << "main: Showing startup logo\n";
	startLogo = KStartupLogo::getInstance();
	startLogo->show();
    }

    clock_t logoShowTime = clock();

    //
    // Start application
    //
    RosegardenGUIApp *rosegardengui = 0;
 
    if (app.isRestored()) {

        // RESTORE(RosegardenGUIApp);
        int n = 1;
        while (KMainWindow::canBeRestored(n)) {
            (new RosegardenGUIApp)->restore(n);
            n++;
        }

    } else {

        rosegardengui = new RosegardenGUIApp(args->isSet("sequencer"),
					     startLogo);
        rosegardengui->show();

	// raise start logo
	//
	if (startLogo) {
	    startLogo->raise();
	    startLogo->setHideEnabled(true);
	    QApplication::flushX();
	}

        if (args->count()) {
            rosegardengui->openFile(args->arg(0));
        } else {
            // rosegardengui->openDocumentFile();
        }

        args->clear();

    }

    // Now that we've started up, raise start logo
    //
    if (startLogo) {
        startLogo->raise();
	startLogo->setHideEnabled(true);
        QApplication::flushX();
    }

    // Check for sequencer and launch if needed
    //
    try
    {
        rosegardengui->launchSequencer();
    }
    catch(std::string e)
    {
        std::cout << "RosegardenGUI - " << e << std::endl;
    }
    catch(QString e)
    {
        std::cout << "RosegardenGUI - " << e << std::endl;
    }



    if (startLogo) {
	
	// pause to ensure the logo has been visible for a reasonable
	// length of time, just 'cos it looks a bit silly to show it
	// and remove it immediately

	int visibleFor = (clock() - logoShowTime) * 1000 / CLOCKS_PER_SEC;

	if (visibleFor < 2000) {
	    QTimer::singleShot(2500 - visibleFor, startLogo, SLOT(close()));
	} else {
	    startLogo->close();
	}
    }

    return app.exec();
}  

