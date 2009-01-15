// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <QDesktopWidget>
#include <Q3Canvas>
#include <QTimer>
#include <QApplication>
#include <sys/time.h>
#include "base/RealTime.h"

//@@@ required ? :
//#include <kcmdlineargs.h>
//#include <kaboutdata.h>
//#include <ktip.h>
//#include <kglobalsettings.h>

#include <QSettings>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QProcess>

#include <QStringList>
#include <QRegExp>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>

#include "document/ConfigGroups.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "gui/application/RosegardenGUIApp.h"
#include "gui/widgets/CurrentProgressDialog.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/widgets/StartupLogo.h"
#include "gui/general/ResourceFinder.h"
#include "gui/general/IconLoader.h"

#include "gui/application/RosegardenApplication.h"

using namespace Rosegarden;

using std::cerr;
using std::endl;

/*! \mainpage Rosegarden global design
 
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
 
 - \link Event Event\endlink is the basic musical element.  It's more or less a
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
 
 - \link Segment Segment\endlink is a series of consecutive Events found on the same Track,
    automatically ordered by their absolute time.  It's the usual
    container for Events.  A Segment has a starting time that can be
    changed, and a duration that is based solely on the end time of
    the last Event it contains.  Note that in order to facilitate
    musical notation editing, we explicitly store silences as series
    of rest Events; thus a Segment really should contain no gaps
    between its Events.  (This isn't checked anywhere and nothing will
    break very badly if there are gaps, but notation won't quite work
    correctly.)
 
 - \link Track Track \endlink is much the same thing as on a mixing table, usually
    assigned to an instrument, a voice, etc.  Although a Track is not
    a container of Events and is not strictly a container of Segments
    either, it is referred to by a set of Segments that are therefore
    mutually associated with the same instruments and parameters.  In
    GUI terms, the Track is a horizontal row on the main Rosegarden
    window, whereas a Segment is a single blue box within that row, of
    which there may be any number.
 
 - \link Instrument Instrument \endlink corresponds broadly to a MIDI or Audio channel, and is
    the destination for a performed Event.  Each Track is mapped to a
    single Instrument (although many Tracks may have the same
    Instrument), and the Instrument is indicated in the header at the
    left of the Track's row in the GUI.
 
 - \link Composition Composition\endlink is the container for the entire piece of music.  It
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
 
 - \link SegmentNotationHelper SegmentNotationHelper\endlink
    and \link SegmentPerformanceHelper SegmentPerformanceHelper\endlink
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
 
 - \link Quantizer Quantizer\endlink is used to quantize event timings and set quantized
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
    use one, because they both use a Q3Canvas to represent data.
 
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
"events" is for UI events, Events is for Event.]
 
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
 
The Sequencer interfaces directly with \link AlsaDriver ALSA\endlink
and provides MIDI "play" and "record" ports which can be connected to
other MIDI clients (MIDI IN and OUT hardware ports or ALSA synth devices)
using any ALSA MIDI Connection Manager.  The Sequencer also supports 
playing and recording of Audio sample files using \link JackDriver Jack\endlink 
 
The GUI and Sequencer were originally implemented as separate processes
communicating using the KDE DCOP communication framework, but they have
now been restructured into separate threads of a single process.  The
original design still explains some of the structure of these classes,
however.  Generally, the DCOP functions that the GUI used to call in
the sequencer are now simple public functions of RosegardenSequencer
that are described in the RosegardenSequencerIface parent class (this
class is retained purely for descriptive purposes); calls that the
sequencer used to make back to the GUI have mostly been replaced by
polling from the GUI to sequencer.

The main operations invoked from the GUI involve starting and
stopping the Sequencer, playing and recording, fast forwarding and
rewinding.  Once a play or record cycle is enabled it's the Sequencer
that does most of the hard work.  Events are read from (or written to,
when recording) a set of mmapped files shared between the threads.
 
The Sequencer makes use of two libraries libRosegardenSequencer
and libRosegardenSound:
 
 - libRosegardenSequencer holds everything pertinent to sequencing
   for Rosegarden including the
   Sequencer class itself.  This library is only linked into the
   Rosegarden Sequencer.
 
 - libRosegardenSound holds the MidiFile class (writing and reading
   MIDI files) and the MappedEvent and MappedComposition classes (the
   communication class for transferring events back and forth between
   sequencer and GUI).  This library is needed by the GUI as well as
   the Sequencer.
 
The main Sequencer state machine is a good starting point and clearly
visible at the bottom of rosegarden/sequencer/main.cpp.
*/

static QString description =
       QObject::tr("Rosegarden - A sequencer and musical notation editor");

/*&&& removed options -- we'll want a different set anyway 

static KCmdLineOptions options[] =
    {
        { "nosequencer", I18N_NOOP("Don't use the sequencer (support editing only)"), 0 },
        { "nosplash", I18N_NOOP("Don't show the splash screen"), 0 },
        { "nofork", I18N_NOOP("Don't automatically run in the background"), 0 },
        { "ignoreversion", I18N_NOOP("Ignore installed version - for devs only"), 0 },
        { "+[File]", I18N_NOOP("file to open"), 0 },
        { 0, 0, 0 }
    };
*/

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
	cerr << "Rosegarden: detected X Error: " << errstr << " " << err->error_code
		  << "\n  Major opcode:  " << err->request_code << endl;
    return 0;
}
#endif

void usage()
{
    cerr << "Rosegarden: A sequencer and musical notation editor" << endl;
    cerr << "Version " << VERSION << endl;
    cerr << endl;
    cerr << "Usage: rosegarden [--nosplash] [--nosequencer] [file.rg]" << endl;
    cerr << endl;
    exit(2);
}

int main(int argc, char *argv[])
{
    setsid(); // acquire shiny new process group

    srandom((unsigned int)time(0) * (unsigned int)getpid());

/*&&& Leaving this here for reference; we have to decide what to do with
      the About box.  To be honest I think a single big bit of HTML in a
      suitable window should be fine, but let's see

    KAboutData aboutData( "rosegarden", "", QObject::tr("Rosegarden"),
                          VERSION, description, KAboutData::License_GPL,
                          QObject::tr("Copyright 2000 - 2009 Guillaume Laurent, Chris Cannam, Richard Bown\nParts copyright 1994 - 2004 Chris Cannam, Andy Green, Richard Bown, Guillaume Laurent\nLilyPond fonts copyright 1997 - 2005 Han-Wen Nienhuys and Jan Nieuwenhuizen"),
                          QString(),
                          "http://www.rosegardenmusic.com/",
                          "rosegarden-devel@lists.sourceforge.net");

    aboutData.addAuthor(QObject::tr("Guillaume Laurent (lead)"), QString(), "glaurent@telegraph-road.org", "http://telegraph-road.org");
    aboutData.addAuthor(QObject::tr("Chris Cannam (lead)"), QString(), "cannam@all-day-breakfast.com", "http://all-day-breakfast.com");
    aboutData.addAuthor(QObject::tr("Richard Bown (lead)"), QString(), "richard.bown@ferventsoftware.com");
    aboutData.addAuthor(QObject::tr("D. Michael McIntyre"), QString(), "dmmcintyr@users.sourceforge.net");
    aboutData.addAuthor(QObject::tr("Pedro Lopez-Cabanillas"), QString(), "plcl@users.sourceforge.net");
    aboutData.addAuthor(QObject::tr("Heikki Johannes Junes"), QString(), "hjunes@users.sourceforge.net");

    aboutData.addCredit(QObject::tr("Randall Farmer"), QObject::tr("Chord labelling code"), " rfarme@simons-rock.edu");
    aboutData.addCredit(QObject::tr("Hans  Kieserman"), QObject::tr("LilyPond output\nassorted other patches\ni18n-ization"), "hkieserman@mail.com");
    aboutData.addCredit(QObject::tr("Levi Burton"), QObject::tr("UI improvements\nbug fixes"), "donburton@sbcglobal.net");
    aboutData.addCredit(QObject::tr("Mark Hymers"), QObject::tr("Segment colours\nOther UI and bug fixes"), "<markh@linuxfromscratch.org>");
    aboutData.addCredit(QObject::tr("Alexandre Prokoudine"), QObject::tr("Russian translation\ni18n-ization"), "avp@altlinux.ru");
    aboutData.addCredit(QObject::tr("Jörg Schumann"), QObject::tr("German translation"), "jrschumann@gmx.de");
    aboutData.addCredit(QObject::tr("Eckhard Jokisch"), QObject::tr("German translation"), "e.jokisch@u-code.de");
    aboutData.addCredit(QObject::tr("Kevin Donnelly"), QObject::tr("Welsh translation"));
    aboutData.addCredit(QObject::tr("Didier Burli"), QObject::tr("French translation"), "didierburli@bluewin.ch");
    aboutData.addCredit(QObject::tr("Yves Guillemot"), QObject::tr("French translation\nBug fixes"), "yc.guillemot@wanadoo.fr");
    aboutData.addCredit(QObject::tr("Daniele Medri"), QObject::tr("Italian translation"), "madrid@linuxmeeting.net");
    aboutData.addCredit(QObject::tr("Alessandro Musesti"), QObject::tr("Italian translation"), "a.musesti@dmf.unicatt.it");
    aboutData.addCredit(QObject::tr("Stefan Asserhäll"), QObject::tr("Swedish translation"), "stefan.asserhall@comhem.se");
    aboutData.addCredit(QObject::tr("Erik Magnus Johansson"), QObject::tr("Swedish translation"), "erik.magnus.johansson@telia.com");
    aboutData.addCredit(QObject::tr("Hasso Tepper"), QObject::tr("Estonian translation"), "hasso@estpak.ee");
    aboutData.addCredit(QObject::tr("Jelmer Vernooij"), QObject::tr("Dutch translation"), "jelmer@samba.org");
    aboutData.addCredit(QObject::tr("Jasper Stein"), QObject::tr("Dutch translation"), "jasper.stein@12move.nl");
    aboutData.addCredit(QObject::tr("Arnout Engelen"), QObject::tr("Transposition by interval"));
    aboutData.addCredit(QObject::tr("Thorsten Wilms"), QObject::tr("Original designs for rotary controllers"), "t_w_@freenet.de");
    aboutData.addCredit(QObject::tr("Oota Toshiya"), QObject::tr("Japanese translation"), "ribbon@users.sourceforge.net");
    aboutData.addCredit(QObject::tr("William"), QObject::tr("Auto-scroll deceleration\nRests outside staves and other bug fixes"), "rosegarden4p AT orthoset.com");
    aboutData.addCredit(QObject::tr("Liu Songhe"), QObject::tr("Simplified Chinese translation"), "jackliu9999@msn.com");
    aboutData.addCredit(QObject::tr("Toni Arnold"), QObject::tr("LIRC infrared remote-controller support"), "<toni__arnold@bluewin.ch>");
    aboutData.addCredit(QObject::tr("Vince Negri"), QObject::tr("MTC slave timing implementation"), "vince.negri@gmail.com");
    aboutData.addCredit(QObject::tr("Jan Bína"), QObject::tr("Czech translation"), "jbina@sky.cz");
    aboutData.addCredit(QObject::tr("Thomas Nagy"), QObject::tr("SCons/bksys building system"), "tnagy256@yahoo.fr");
    aboutData.addCredit(QObject::tr("Vladimir Savic"), QObject::tr("icons, icons, icons"), "vladimir@vladimirsavic.net");
    aboutData.addCredit(QObject::tr("Marcos Germán Guglielmetti"), QObject::tr("Spanish translation"), "marcospcmusica@yahoo.com.ar");
    aboutData.addCredit(QObject::tr("Lisandro Damián Nicanor Pérez Meyer"), QObject::tr("Spanish translation"), "perezmeyer@infovia.com.ar");
    aboutData.addCredit(QObject::tr("Javier Castrillo"), QObject::tr("Spanish translation"), "riverplatense@gmail.com");
    aboutData.addCredit(QObject::tr("Lucas Godoy"), QObject::tr("Spanish translation"), "godoy.lucas@gmail.com");
    aboutData.addCredit(QObject::tr("Feliu Ferrer"), QObject::tr("Catalan translation"), "mverge2@pie.xtec.es");
    aboutData.addCredit(QObject::tr("Quim Perez i Noguer"), QObject::tr("Catalan translation"), "noguer@osona.com");
    aboutData.addCredit(QObject::tr("Carolyn McIntyre"), QObject::tr("1.2.3 splash screen photo\nGave birth to D. Michael McIntyre, bought him a good flute once\nupon a time, and always humored him when he came over to play her\nsome new instrument, even though she really hated his playing.\nBorn October 19, 1951, died September 21, 2007, R. I. P."), "DECEASED");
    aboutData.addCredit(QObject::tr("Stephen Torri"), QObject::tr("Initial guitar chord editing code"), "storri@torri.org");
    aboutData.addCredit(QObject::tr("Piotr Sawicki"), QObject::tr("Polish translation"), "pelle@plusnet.pl");
    aboutData.addCredit(QObject::tr("David García-Abad"), QObject::tr("Basque translation"), "davidgarciabad@telefonica.net");

    aboutData.setTranslator(QObject::tr("NAME OF TRANSLATORS", "Your names"),
			    QObject::tr("EMAIL OF TRANSLATORS", "Your emails"));
*/

    RosegardenApplication app(argc, argv);

    app.setOrganizationName("rosegardenmusic");
    app.setOrganizationDomain("rosegardenmusic.org");
    app.setApplicationName(QObject::tr("Rosegarden"));

    QStringList args = app.arguments();

    bool nosplash = false;
    bool nosequencer = false;
    int nonOptArgs = 0;

    for (int i = 1; i < args.size(); ++i) {
	if (args[i].startsWith("-")) {
	    if (args[i] == "--nosplash") nosplash = true;
	    else if (args[i] == "--nosequencer") nosequencer = true;
	    else usage();
	} else {
	    ++nonOptArgs;
	}
    }
    if (nonOptArgs > 1) usage();

    QIcon icon;
    int sizes[] = { 16, 22, 24, 32, 48, 64, 128 };
    for (int i = 0; i < sizeof(sizes)/sizeof(sizes[0]); ++i) {
	QString name = QString("rg-rwb-rose3-%1x%2").arg(sizes[i]).arg(sizes[i]);
	QPixmap pixmap = IconLoader().loadPixmap(name);
	if (!pixmap.isNull()) {
	    cerr << "Loaded application icon \"" << name << "\"" << endl;
	    icon.addPixmap(pixmap);
	}
    }
    app.setWindowIcon(icon);

    QString stylepath = ResourceFinder().getResourcePath("", "rosegarden.qss");
    if (stylepath != "") {
	cerr << "NOTE: Found stylesheet at \"" << stylepath << "\", applying it" << endl;
	QFile file(stylepath);
	if (!file.open(QFile::ReadOnly)) {
	    cerr << "(Failed to open file)" << endl;
	} else {
	    QString styleSheet = QLatin1String(file.readAll());
	    app.setStyleSheet(styleSheet);
	}
    }

    // Ensure quit on last window close
    //@@@ ???
    //
    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    QString lastVersion = settings.value("lastversion", "").toString();
    bool newVersion = (lastVersion != VERSION);
    if (newVersion) {
	cerr << "*** This is the first time running this Rosegarden version" << endl;
	settings.setValue("lastversion", VERSION);
    }

    // If there is no config setting for the startup window size, set
    // one now.  But base the default on the appropriate desktop size
    // (i.e. not the entire desktop, if Xinerama is in use).  This is
    // obtained from KGlobalSettings::desktopGeometry(), but we can't
    // give it a meaningful point to measure from at this stage so we
    // always use the "leftmost" display (point 0,0).

    // The config keys are "Height X" and "Width Y" where X and Y are
    // the sizes of the available desktop (i.e. the whole shebang if
    // under Xinerama).  These are obtained from QDesktopWidget.

    settings.endGroup();
    settings.beginGroup( "MainView" );

    int windowWidth = 0, windowHeight = 0;

    QDesktopWidget *desktop = app.desktop();
    if (desktop) {
	QRect totalRect(desktop->screenGeometry());
	QRect desktopRect = desktop->availableGeometry();
	QSize startupSize;
	if (desktopRect.height() <= 800) {
	    startupSize = QSize((desktopRect.width() * 6) / 7,
				(desktopRect.height() * 6) / 7);
	} else {
	    startupSize = QSize((desktopRect.width() * 4) / 5,
				(desktopRect.height() * 4) / 5);
	}
	QString widthKey = QString("Width %1").arg(totalRect.width());
	QString heightKey = QString("Height %1").arg(totalRect.height());
	windowWidth = settings.value
	    (widthKey, startupSize.width()).toInt();
	windowHeight = settings.value
	    (heightKey, startupSize.height()).toInt();
    }

    settings.endGroup();
    settings.beginGroup(GeneralOptionsConfigGroup);

    StartupLogo* startLogo = 0L;

    if (qStrToBool(settings.value("Logo", "true")) && !nosplash) {
        startLogo = StartupLogo::getInstance();
	startLogo->setShowTip(!newVersion);
        startLogo->show();
        app.processEvents();	
    }

    struct timeval logoShowTime;
    gettimeofday(&logoShowTime, 0);

    //
    // Start application
    //
    RosegardenGUIApp *rosegardengui = 0;

/*&&& worry about this later
    if (app.isSessionRestored()) {
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
*/

#ifndef NO_SOUND
        app.setNoSequencerMode(nosequencer);
#else
        app.setNoSequencerMode(true);
#endif // NO_SOUND

        rosegardengui = new RosegardenGUIApp(!app.noSequencerMode(), startLogo);

	rosegardengui->setIsFirstRun(newVersion);

        app.setMainWidget(rosegardengui);

	if (windowWidth != 0 && windowHeight != 0) {
	    rosegardengui->resize(windowWidth, windowHeight);
	}

        rosegardengui->show();

        // raise start logo
        //
        if (startLogo) {
            startLogo->raise();
            startLogo->setHideEnabled(true);
            app.flushX();
        }

	for (int i = 1; i < args.size(); ++i) {
	    if (args[i].startsWith("-")) continue;
	    rosegardengui->openFile(args[i], RosegardenGUIApp::ImportCheckType);
	    break;
        }

	//@@@???
    QObject::connect(&app, SIGNAL(aboutToSaveState()),
                     rosegardengui, SLOT(slotDeleteTransport()));

    // Now that we've started up, raise start logo
    //
    if (startLogo) {
        startLogo->raise();
        startLogo->setHideEnabled(true);
        app.flushX();
    }

    // Check for sequencer and launch if needed
    //
    try {
        rosegardengui->launchSequencer();
    } catch (std::string e) {
        RG_DEBUG << "RosegardenGUI - " << e << endl;
    } catch (QString e) {
        RG_DEBUG << "RosegardenGUI - " << e << endl;
    } catch (Exception e) {
        RG_DEBUG << "RosegardenGUI - " << e.getMessage() << endl;
    }

    settings.endGroup();
    settings.beginGroup( SequencerOptionsConfigGroup );

    // See if the settings wants us to load a soundfont
    //
    if ( qStrToBool( settings.value("sfxloadenabled", "false" ) ) ) {
        QString sfxLoadPath = settings.value("sfxloadpath", "/bin/sfxload").toString();
        QString soundFontPath = settings.value("soundfontpath", "").toString();
        QFileInfo sfxLoadInfo(sfxLoadPath), soundFontInfo(soundFontPath);
        if (sfxLoadInfo.isExecutable() && soundFontInfo.isReadable()) {
            // setup sfxload Process
            QProcess* sfxLoadProcess = new QProcess;

            RG_DEBUG << "Starting sfxload : " << sfxLoadPath << " " << soundFontPath << endl;

            QObject::connect(sfxLoadProcess, SIGNAL(processExited(QProcess*)),
                             &app, SLOT(sfxLoadExited(QProcess*)));

            sfxLoadProcess->start(sfxLoadPath, (QStringList()) << soundFontPath);
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

    if (startLogo) {

        // pause to ensure the logo has been visible for a reasonable
        // length of time, just 'cos it looks a bit silly to show it
        // and remove it immediately

        struct timeval now;
        gettimeofday(&now, 0);

        RealTime visibleFor =
            RealTime(now.tv_sec, now.tv_usec * 1000) -
            RealTime(logoShowTime.tv_sec, logoShowTime.tv_usec * 1000);

        if (visibleFor < RealTime(2, 0)) {
            int waitTime = visibleFor.sec * 1000 + visibleFor.msec();
            QTimer::singleShot(2500 - waitTime, startLogo, SLOT(close()));
        } else {
            startLogo->close();
        }

    } else {

        // if the start logo is there, it's responsible for showing this;
        // otherwise we have to

//&&& We lack startup tips! Do we want to restore them?
	if (!newVersion) {
	    RosegardenGUIApp::self()->awaitDialogClearance();
	    QString tipResource = ResourceFinder().getResourcePath("", "tips");
	    if (tipResource != "") {
// 			KTipDialog::showTip(tipResource);	//&&&
	    }
	}
    }

    if (newVersion) {
        StartupLogo::hideIfStillThere();
        CurrentProgressDialog::freeze();

        QDialog *dialog = new QDialog;
        dialog->setModal(true);
        dialog->setWindowTitle(QObject::tr("Welcome!"));
        QGridLayout *metagrid = new QGridLayout;
        dialog->setLayout(metagrid);

        QWidget *hb = new QWidget;
        QHBoxLayout *hbLayout = new QHBoxLayout;
        metagrid->addWidget(hb, 0, 0);

        QLabel *image = new QLabel;
        hbLayout->addWidget(image);
        image->setAlignment(Qt::AlignTop);

	image->setPixmap(IconLoader().loadPixmap("welcome-icon"));

        QLabel *label = new QLabel;
        hbLayout->addWidget(label);
        label->setText(QObject::tr("<h2>Welcome to Rosegarden!</h2><p>Welcome to the Rosegarden audio and MIDI sequencer and musical notation editor.</p><ul><li>If you have not already done so, you may wish to install some DSSI synth plugins, or a separate synth program such as QSynth.  Rosegarden does not synthesize sounds from MIDI on its own, so without these you will hear nothing.</li><li>Rosegarden uses the JACK audio server for recording and playback of audio, and for playback from DSSI synth plugins.  These features will only be available if the JACK server is running.</li><li>Rosegarden has comprehensive documentation: see the Help menu for the handbook, tutorials, and other information!</li></ul><p>Rosegarden was brought to you by a team of volunteers across the world.  To learn more, go to <a href=\"http://www.rosegardenmusic.com/\">http://www.rosegardenmusic.com/</a>.</p>"));
        label->setWordWrap(true);

        hb->setLayout(hbLayout);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
        metagrid->addWidget(buttonBox, 1, 0);
        metagrid->setRowStretch(0, 10);
	QObject::connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
	QObject::connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

        rosegardengui->awaitDialogClearance();
        dialog->exec();

	CurrentProgressDialog::thaw();
    }
    settings.endGroup();

    return app.exec();
}

