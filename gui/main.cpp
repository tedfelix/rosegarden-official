// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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
#include <sys/time.h>
#include "RealTime.h"

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <dcopclient.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kstddirs.h>
#include <ktip.h>
#include <kprocess.h>

#include "constants.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "rosegardengui.h"
#include "rosegardenguidoc.h"
#include "kstartuplogo.h"

#include "rgapplication.h"

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

The Sequencer interfaces directly with \link Rosegarden::AlsaDriver ALSA\endlink
and provides MIDI "play" and "record" ports which can be connected to
other MIDI clients (MIDI IN and OUT hardware ports or ALSA synth devices)
using any ALSA MIDI Connection Manager.  The Sequencer also supports 
playing and recording of Audio sample files using \link Rosegarden::JackDriver Jack\endlink 

The GUI and Sequencer communicate using the KDE DCOP communication framework.
Look in:
 - \link rosegardenguiiface.h gui/rosegardenguiiface.h\endlink
 - \link rosegardensequenceriface.h sequencer/rosegardensequenceriface.h\endlink

for definitions of the DCOP interfaces pertinent to the Sequencer
and GUI.  The main DCOP operations from the GUI involve starting and
stopping the Sequencer, playing and recording, fast forwarding and
rewinding.  Once a play or record cycle is enabled it's the Sequencer
that does most of the hard work.  Events are read from (or written to, when recording)
a set of mmapped files. 

The Sequencer makes use of two libraries libRosegardenSequencer
and libRosegardenSound:

 - libRosegardenSequencer holds everything pertinent to sequencing
    for Rosegarden including the
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
    { "nosequencer", I18N_NOOP("Don't use the sequencer (support editing only)"), 0 },
    { "nosplash", I18N_NOOP("Don't show the splash screen"), 0 },
    { "nofork", I18N_NOOP("Don't automatically run in the background"), 0 },
    { "existingsequencer", I18N_NOOP("Attach to a running sequencer process, if found"), 0 },
    { "ignoreversion", I18N_NOOP("Ignore installed version - for devs only"), 0 },
    { "+[File]", I18N_NOOP("file to open"), 0 },
    { 0, 0, 0 }
};


// -----------------------------------------------------------------

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/SM/SMlib.h>

static int _x_errhandler( Display *dpy, XErrorEvent *err )
{
    char errstr[256];
    XGetErrorText( dpy, err->error_code, errstr, 256 );
    if ( err->error_code != BadWindow )
        kdWarning() << "Rosegarden: detected X Error: " << errstr << " " << err->error_code
		<< "\n  Major opcode:  " << err->request_code << endl;
    return 0;
}
#endif

// NOTE: to get a dump of the stack trace from KDE during program execution:
// std::cerr << kdBacktrace() << std::endl
// (see kdebug.h)

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
                  " (I am %1, but the installed files are for version %2.)\n\n"
                  " This may mean one of the following:\n\n"
                  " 1. This is a new upgrade of Rosegarden, and it has not yet been\n"
                  "     installed.  If you compiled it yourself, check that you have\n"
                  "     run \"scons install\" and that the procedure completed\n"
                  "     successfully.\n\n"
                  " 2. The upgrade was installed in a non-standard directory,\n"
                  "     and an old version was found in a standard directory.  If so,\n"
                  "     you will need to add the correct directory to your KDEDIRS\n"
                  "     environment variable before you can run it.").arg(VERSION).arg(installedVersion),
             i18n("Installation problem"));
        
    } else {

        KMessageBox::detailedError
            (0,
             i18n("Rosegarden does not appear to have been installed."),
             i18n(" One or more of Rosegarden's data files could not be\n"
                  " found in the standard KDE installation directories.\n\n"
                  " This may mean one of the following:\n\n"
                  " 1. Rosegarden has not been correctly installed.  If you compiled\n"
                  "     it yourself, check that you have run \"scons install\" and that\n"
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
    setsid(); // acquire shiny new process group

    srandom((unsigned int)time(0) * (unsigned int)getpid());

    KAboutData aboutData( "rosegarden", I18N_NOOP("Rosegarden"),
                          VERSION, description, KAboutData::License_GPL,
                          I18N_NOOP("Copyright 2000 - 2005 Guillaume Laurent, Chris Cannam, Richard Bown\nParts copyright 1994 - 2004 Chris Cannam, Andy Green, Richard Bown, Guillaume Laurent\nLilypond fonts copyright 1997 - 2005 Han-Wen Nienhuys and Jan Nieuwenhuizen"),
                          0,
                          "http://www.rosegardenmusic.com/",
                          "rosegarden-devel@lists.sourceforge.net");

    aboutData.addAuthor("Guillaume Laurent", 0, "glaurent@telegraph-road.org", "http://telegraph-road.org");
    aboutData.addAuthor("Chris Cannam", 0, "cannam@all-day-breakfast.com", "http://all-day-breakfast.com");
    aboutData.addAuthor("Richard Bown", 0, "bownie@bownie.com", "http://bownie.com");
    aboutData.addAuthor("D. Michael McIntyre (adjunct)", 0, "dmmcintyr@users.sourceforge.net", "http://www.geocities.com/Paris/Rue/5407");

    aboutData.addCredit("Randall Farmer", I18N_NOOP("Chord labelling code"), " rfarme@simons-rock.edu");
    aboutData.addCredit("Hans  Kieserman", I18N_NOOP("Lilypond output\nassorted other patches\ni18n-ization"), "hkieserman@mail.com");
    aboutData.addCredit("Levi Burton", I18N_NOOP("UI improvements\nbug fixes"), "donburton@sbcglobal.net");
    aboutData.addCredit("Mark Hymers", I18N_NOOP("Segment colours\nOther UI and bug fixes"),"<markh@linuxfromscratch.org>");
    aboutData.addCredit("Alexandre Prokoudine", I18N_NOOP("Russian translation\ni18n-ization"), "avp@altlinux.ru");
    aboutData.addCredit("Pedro Lopez-Cabanillas", I18N_NOOP("Spanish translation\nALSA hacking and bug fixes\nmulti-input MIDI recording"), "plcl@users.sourceforge.net");
    aboutData.addCredit("Jörg Schumann", I18N_NOOP("German translation"), "jrschumann@gmx.de");
    aboutData.addCredit("Eckhard Jokisch", I18N_NOOP("German translation"), "e.jokisch@u-code.de");
    aboutData.addCredit("Kevin Donnelly", I18N_NOOP("Welsh translation"));
    aboutData.addCredit("Didier Burli", I18N_NOOP("French translation"), "didierburli@bluewin.ch");
    aboutData.addCredit("Daniele Medri", I18N_NOOP("Italian translation"), "madrid@linuxmeeting.net");
    aboutData.addCredit("Alessandro Musesti", I18N_NOOP("Italian translation"), "a.musesti@dmf.unicatt.it");
    aboutData.addCredit("Stefan Asserhäll", I18N_NOOP("Swedish translation"), "stefan.asserhall@comhem.se");
    aboutData.addCredit("Erik Magnus Johansson", I18N_NOOP("Swedish translation"), "erik.magnus.johansson@telia.com");
    aboutData.addCredit("Hasso Tepper", I18N_NOOP("Estonian translation"), "hasso@estpak.ee");
    aboutData.addCredit("Jelmer Vernooij", I18N_NOOP("Dutch translation"), "jelmer@samba.org");
    aboutData.addCredit("Kevin Liang", I18N_NOOP("HSpinBox class"), "xkliang@rhpcs.mcmaster.ca");
    aboutData.addCredit("Thorsten Wilms", I18N_NOOP("Original designs for rotary controllers"), "t_w_@freenet.de");
    aboutData.addCredit("Oota Toshiya", I18N_NOOP("Japanese translation"), "ribbon@users.sourceforge.net");
    aboutData.addCredit("William", I18N_NOOP("Auto-scroll deceleration\nRests outside staves and other bug fixes"), "rosegarden4p AT orthoset.com");
    aboutData.addCredit("Liu Songhe", I18N_NOOP("Simplified Chinese translation"), "jackliu9999@msn.com");
    aboutData.addCredit("Toni Arnold", I18N_NOOP("LIRC infrared remote-controller support"), "<toni__arnold@bluewin.ch>");
    aboutData.addCredit("Vince Negri", I18N_NOOP("MTC slave timing implementation"), "vince-rg@bulbous.freeserve.co.uk");
    aboutData.addCredit("Jan Bína", I18N_NOOP("Czech translation"), "jbina@sky.cz");
    aboutData.addCredit("Thomas Nagy", I18N_NOOP("SCons/bksys building system"), "tnagy256@yahoo.fr");
    aboutData.addCredit("Vladimir Savic", I18N_NOOP("icons, icons, icons"), "vladimir@vladimirsavic.net");
    aboutData.addCredit("Marcos Germán Guglielmetti", I18N_NOOP("Spanish translation"), "marcospcmusica@yahoo.com.ar");
    aboutData.addCredit("Lisandro Damián Nicanor Pérez Meyer", I18N_NOOP("Spanish translation"), "perezmeyer@infovia.com.ar");
    aboutData.addCredit("Javier Castrillo", I18N_NOOP("Spanish translation"), "riverplatense@gmail.com");
    aboutData.addCredit("Lucas Godoy", I18N_NOOP("Spanish translation"), "godoy.lucas@gmail.com");
    aboutData.addCredit("Feliu Ferrer", I18N_NOOP("Catalan translation"), "mverge2@pie.xtec.es");
    aboutData.addCredit("Quim Perez i Noguer", I18N_NOOP("Catalan translation"), "noguer@osona.com");
    aboutData.addCredit("Carolyn McIntyre", I18N_NOOP("1.2.3 splash screen photo (of Michael's rose garden)\nnew splash screen photo (of Michael McIntyre's\ninstruments along with a rose from the garden of Hassell Arnold Hale, 1916-2006,\nmay he rest in peace)"), "catma@adelphia.net");
    aboutData.addCredit("Heikki Johannes Junes", I18N_NOOP("Finnish translation"), "hjunes@cc.hut.fi");
    aboutData.addCredit("Stephen Torri", I18N_NOOP("guitar chord editor"), "storri@torri.org");

    aboutData.setTranslator(I18N_NOOP("_: NAME OF TRANSLATORS\nYour names") ,I18N_NOOP("_: EMAIL OF TRANSLATORS\nYour emails"));

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
    KUniqueApplication::addCmdLineOptions(); // Add KUniqueApplication options.

    if (!RosegardenApplication::start()) return 0;

    RosegardenApplication app;

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

	if (!args->isSet("ignoreversion")) {
	  // Give up immediately if we haven't been installed or if the
	  // installation is out of date
	  //
	  testInstalledVersion();
	}	

    KConfig *config = kapp->config();
    config->setGroup("KDE Action Restrictions");
    config->writeEntry("action/help_report_bug", false);

    // Show Startup logo
    // (this code borrowed from KDevelop 2.0,
    // (c) The KDevelop Development Team
    //
    config = kapp->config();
    config->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    KStartupLogo* startLogo = 0L;

    // See if the config wants us to control JACK
    //
    if (config->readBoolEntry("Logo",true) && (!kapp->isRestored() && args->isSet("splash")) )
    {
        RG_DEBUG << k_funcinfo << "Showing startup logo\n";
        startLogo = KStartupLogo::getInstance();
        startLogo->show();
    }

    struct timeval logoShowTime;
    gettimeofday(&logoShowTime, 0);

    //
    // Start application
    //
    RosegardenGUIApp *rosegardengui = 0;
 
    if (app.isRestored()) {
        RG_DEBUG << "Restoring from session\n";

        // RESTORE(RosegardenGUIApp);
        int n = 1;
        while (KMainWindow::canBeRestored(n)) {
            // memory leak if more than one can be restored?
            RG_DEBUG << "Restoring from session - restoring app #" << n << endl;
            (rosegardengui = new RosegardenGUIApp)->restore(n);
            n++;
        }

    } else {

#ifndef NO_SOUND
        app.setNoSequencerMode(!args->isSet("sequencer"));
#else
	app.setNoSequencerMode(true);
#endif // NO_SOUND
	
        rosegardengui = new RosegardenGUIApp(!app.noSequencerMode(),
					     args->isSet("existingsequencer"),
                                             startLogo);

        app.setMainWidget(rosegardengui);

        rosegardengui->show();

        // raise start logo
        //
        if (startLogo) {
            startLogo->raise();
            startLogo->setHideEnabled(true);
            QApplication::flushX();
        }

        if (args->count()) {
            rosegardengui->openFile(QFile::decodeName(args->arg(0)), RosegardenGUIApp::ImportCheckType);
        } else {
            // rosegardengui->openDocumentFile();
        }

        args->clear();

    }

    QObject::connect(&app, SIGNAL(aboutToSaveState()),
                     rosegardengui, SLOT(slotDeleteTransport()));


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
        rosegardengui->launchSequencer(args->isSet("existingsequencer"));
    }
    catch(std::string e)
    {
        RG_DEBUG << "RosegardenGUI - " << e << endl;
    }
    catch(QString e)
    {
        RG_DEBUG << "RosegardenGUI - " << e << endl;
    }

    if (startLogo) {
        
        // pause to ensure the logo has been visible for a reasonable
        // length of time, just 'cos it looks a bit silly to show it
        // and remove it immediately

	struct timeval now;
	gettimeofday(&now, 0);

	Rosegarden::RealTime visibleFor =
	    Rosegarden::RealTime(now.tv_sec, now.tv_usec * 1000) -
	    Rosegarden::RealTime(logoShowTime.tv_sec, logoShowTime.tv_usec * 1000);

        if (visibleFor < Rosegarden::RealTime(2, 0)) {
	    int waitTime = visibleFor.sec * 1000 + visibleFor.msec();
            QTimer::singleShot(2500 - waitTime, startLogo, SLOT(close()));
        } else {
            startLogo->close();
        }

    } else {

	// if the start logo is there, it's responsible for showing this;
	// otherwise we have to

        RG_DEBUG << "main: Showing Tips\n";
        KTipDialog::showTip(locate("data", "rosegarden/tips"));
    }


    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);

    // See if the config wants us to load a soundfont
    //
    if (config->readBoolEntry("sfxloadenabled",false)) {
        QString sfxLoadPath = config->readEntry("sfxloadpath", "/bin/sfxload");
        QString soundFontPath = config->readEntry("soundfontpath", "");
        QFileInfo sfxLoadInfo(sfxLoadPath), soundFontInfo(soundFontPath);
        if (sfxLoadInfo.isExecutable() && soundFontInfo.isReadable()) {
            KProcess* sfxLoadProcess = new KProcess;
            (*sfxLoadProcess) << sfxLoadPath << soundFontPath;
            RG_DEBUG << "Starting sfxload : " << sfxLoadPath << " " << soundFontPath << endl;

            QObject::connect(sfxLoadProcess, SIGNAL(processExited(KProcess*)),
                             &app, SLOT(sfxLoadExited(KProcess*)));

            sfxLoadProcess->start();
        } else {
            RG_DEBUG << "sfxload not executable or soundfont not readable : "
                     << sfxLoadPath << " " << soundFontPath << endl;
        }

    } else {
        RG_DEBUG << "sfxload disabled\n";
    }
    

#ifdef Q_WS_X11
    XSetErrorHandler( _x_errhandler );
#endif
    return kapp->exec();
}  

