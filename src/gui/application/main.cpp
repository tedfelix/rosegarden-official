/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

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

#include "misc/ConfigGroups.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/widgets/CurrentProgressDialog.h"
#include "document/RosegardenDocument.h"
#include "gui/widgets/StartupLogo.h"
#include "gui/general/ResourceFinder.h"
#include "gui/general/IconLoader.h"
#include "gui/application/RosegardenApplication.h"
#include "base/RealTime.h"

#include <QSettings>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <QStringList>
#include <QRegExp>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTimer>
#include <QApplication>
#include <QtGui>
#include <QPlastiqueStyle>
#include <QPixmapCache>
#include <QStringList>

#include <sys/time.h>

using namespace Rosegarden;


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
RosegardenDocument, which wraps a Composition) can have several views
(class RosegardenMainViewWidget), although at the moment only a single one is
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
   MIDI files) and the MappedEvent and MappedEventList classes (the
   communication class for transferring events back and forth between
   sequencer and GUI).  This library is needed by the GUI as well as
   the Sequencer.
 
The main Sequencer state machine is a good starting point and clearly
visible at the bottom of rosegarden/sequencer/main.cpp.
*/

static QString description =
       RosegardenApplication::tr("Rosegarden - A sequencer and musical notation editor");

// -----------------------------------------------------------------

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/SM/SMlib.h>

static int _x_errhandler(Display *dpy, XErrorEvent *err)
{
    char errstr[256];
    XGetErrorText(dpy, err->error_code, errstr, 256);
    if (err->error_code != BadWindow) {
        std::cerr << "Rosegarden: detected X Error: " << errstr << " " << err->error_code
                  << "\n  Major opcode:  " << err->request_code << std::endl;
    }
    return 0;
}
#endif

void usage()
{
    std::cerr << "Rosegarden: A sequencer and musical notation editor" << std::endl;
    std::cerr << "Usage: rosegarden [--nosplash] [--nosequencer] [file.rg]" << std::endl;
    std::cerr << "       rosegarden --version" << std::endl;
    exit(2);
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--version")) {
            std::cout << "Rosegarden version: " << VERSION << " (\"" << CODENAME << "\")" << std::endl;
            std::cout << "Build key: " << BUILDKEY << std::endl;
            std::cout << "Built against Qt version: " << QT_VERSION_STR << std::endl;
            return 0;
        }
    }

    // Many elements of our GUI looked nauseating on a KDE4 box with the default
    // Oxygen style.  Apparently I was depdending on having Plastique underneath
    // everything to a larger extent than I realized.  To address this, we'll
    // force Plastique.  We might eventually have to take a copy of Plastique
    // and maintain it ourselves, or at least the subtle parts of Plastique we
    // don't already override with our heavily customized UI.
    QApplication::setStyle(new QPlastiqueStyle);

    QPixmapCache::setCacheLimit(8192); // KB

    setsid(); // acquire shiny new process group

    srandom((unsigned int)time(0) * (unsigned int)getpid());

#ifdef Q_WS_X11
#if QT_VERSION >= 0x040500
    bool systemSpecified = false;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-graphicssystem")) {
            systemSpecified = true;
            break;
        }
    }
    if (!systemSpecified) {
        // Set the raster graphics system unless the user has
        // explicitly asked for something else on the command line.
        // This should (?) override the option with which Qt was
        // compiled, if it differs from raster.  The raster renderer
        // is much faster than the usual default "native" renderer
        RG_DEBUG << "Setting raster graphics system for Qt 4.5+" << endl;
        QApplication::setGraphicsSystem("raster");
    }
#endif
#endif

    RosegardenApplication theApp(argc, argv);

    std::cerr << "System Locale: " << QLocale::system().name() << std::endl;
    std::cerr << "Qt translations path: " << QLibraryInfo::location(QLibraryInfo::TranslationsPath) << std::endl;

    QTranslator qtTranslator;
    bool qtTranslationsLoaded = 
      qtTranslator.load("qt_" + QLocale::system().name(),
            QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    if (qtTranslationsLoaded) {
        theApp.installTranslator(&qtTranslator);
        std::cerr << "Qt translations loaded successfully." << std::endl;
    } else {
        std::cerr << "Qt translations not loaded." << std::endl;
    }

    QTranslator rgTranslator;
    std::cerr << "RG Translation: trying to load :locale/" << QLocale::system().name() << std::endl;
    bool rgTranslationsLoaded = 
      rgTranslator.load(QLocale::system().name(), ":locale/");
    if (rgTranslationsLoaded) {
        std::cerr << "RG Translations loaded successfully." << std::endl;
        theApp.installTranslator(&rgTranslator);
    } else {
        std::cerr << "RG Translations not loaded." << std::endl;
    }

    theApp.setOrganizationName("rosegardenmusic");
    theApp.setOrganizationDomain("rosegardenmusic.com");
    theApp.setApplicationName(RosegardenApplication::tr("Rosegarden"));

    QStringList args = theApp.arguments();

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
    for (unsigned int i = 0; i < sizeof(sizes)/sizeof(sizes[0]); ++i) {
        QString name = QString("rg-rwb-rose3-%1x%2").arg(sizes[i]).arg(sizes[i]);
        QPixmap pixmap = IconLoader().loadPixmap(name);
        if (!pixmap.isNull()) {
            std::cerr << "Loaded application icon \"" << name << "\"" << std::endl;
            icon.addPixmap(pixmap);
        }
    }
    theApp.setWindowIcon(icon);

    QSettings settings;
    QString stylepath = ResourceFinder().getResourcePath("", "rosegarden.qss");
    if (stylepath != "") {
        std::cerr << "NOTE: Found stylesheet at \"" << stylepath << "\", applying it" << std::endl;
        QFile file(stylepath);
        if (!file.open(QFile::ReadOnly)) {
            std::cerr << "(Failed to open file)" << std::endl;
        } else {
            settings.beginGroup(GeneralOptionsConfigGroup);
            bool Thorn = settings.value("use_thorn_style", true).toBool();
            if (Thorn) {
                QString styleSheet = QLatin1String(file.readAll());
                theApp.setStyleSheet(styleSheet);
            } else {
                std::cerr << "Not loading stylesheet per user request.  Caveat emptor." << std::endl;
            }
            settings.endGroup();
        }
    }

    // Ensure quit on last window close
    //@@@ ???
    //
    QObject::connect(&theApp, SIGNAL(lastWindowClosed()), &theApp, SLOT(quit()));

    settings.beginGroup(GeneralOptionsConfigGroup);

    QString lastVersion = settings.value("lastversion", "").toString();
    bool newVersion = (lastVersion != VERSION);
    if (newVersion) {
        std::cerr << "*** This is the first time running this Rosegarden version" << std::endl;
        settings.setValue("lastversion", VERSION);

    }

    
    // unbundle examples
    QStringList exampleFiles;
    exampleFiles << ResourceFinder().getResourceFiles("examples", "rg");
    for (QStringList::const_iterator i = exampleFiles.begin(); i != exampleFiles.end(); ++i) {
        QString exampleFile(*i);
        QString name = QFileInfo(exampleFile).fileName();
        if (exampleFile.startsWith(":")) {
            ResourceFinder().unbundleResource("examples", name);
            exampleFile = ResourceFinder().getResourcePath("examples", name);
            if (exampleFile.startsWith(":")) { // unbundling failed
                continue;
            }
        }
    }

    // unbundle templates
    QStringList templateFiles;
    templateFiles << ResourceFinder().getResourceFiles("templates", "rgt");
    for (QStringList::const_iterator i = templateFiles.begin(); i != templateFiles.end(); ++i) {
        QString templateFile(*i);
        QString name = QFileInfo(templateFile).fileName();
        if (templateFile.startsWith(":")) {
            ResourceFinder().unbundleResource("templates", name);
            templateFile = ResourceFinder().getResourcePath("templates", name);
            if (templateFile.startsWith(":")) { // unbundling failed
                continue;
            }
        }
    }

    // unbundle libraries
    QStringList libraryFiles;
    libraryFiles << ResourceFinder().getResourceFiles("library", "rgd");
    for (QStringList::const_iterator i = libraryFiles.begin(); i != libraryFiles.end(); ++i) {
        QString libraryFile(*i);
        QString name = QFileInfo(libraryFile).fileName();
        if (libraryFile.startsWith(":")) {
            ResourceFinder().unbundleResource("library", name);
            libraryFile = ResourceFinder().getResourcePath("library", name);
            if (libraryFile.startsWith(":")) { // unbundling failed
                continue;
            }
        }
    }

    // NOTE: We used to have a great heap of code here to calculate a sane
    // default initial size.  When I made RosegardenMainWindow keep track of its
    // own geometry, I originally built in a series of locks to allow the old
    // code here to run one time to establish a known good default, and then
    // that class took over saving and restoring its own geometry afterwards.
    //
    // While testing the locks, I ran several times with the states messed up,
    // where we tried to restore saved settings when there were none to restore.
    // The default seemed to be exactly the same as what we had all that
    // complicated code to set up.  I'm not sure if that code ever did anything
    // in Qt4, and I think our default initial state is rather sane now.
    //
    // If it turns out that it isn't, I think we should set up a sane default in
    // some less complicated way next time, so either way, I have decided to
    // ditch all of the code.

    settings.endGroup();
    settings.beginGroup(GeneralOptionsConfigGroup);

    StartupLogo* startLogo = 0L;

    if (qStrToBool(settings.value("Logo", "true")) && !nosplash) {
        startLogo = StartupLogo::getInstance();
        startLogo->setShowTip(!newVersion);
        startLogo->show();
        startLogo->repaint();
        theApp.processEvents();
        theApp.flushX();
    }

    struct timeval logoShowTime;
    gettimeofday(&logoShowTime, 0);

    //
    // Start application
    //
    RosegardenMainWindow *rosegardengui = 0;

#ifndef NO_SOUND
    theApp.setNoSequencerMode(nosequencer);
#else
    theApp.setNoSequencerMode(true);
#endif // NO_SOUND

    rosegardengui = new RosegardenMainWindow(!theApp.noSequencerMode(), startLogo);

    rosegardengui->setIsFirstRun(newVersion);

    theApp.setMainWidget(rosegardengui);

    rosegardengui->show();

    // raise start logo
    //
    if (startLogo) {
        startLogo->raise();
        startLogo->setHideEnabled(true);
        startLogo->repaint();
        theApp.flushX();
    }

    for (int i = 1; i < args.size(); ++i) {
        if (args[i].startsWith("-")) continue;
        rosegardengui->openFile(args[i], RosegardenMainWindow::ImportCheckType);
        break;
        }

    //@@@???
    QObject::connect(&theApp, SIGNAL(aboutToSaveState()),
                     rosegardengui, SLOT(slotDeleteTransport()));

    // Now that we've started up, raise start logo
    //
    if (startLogo) {
        startLogo->raise();
        startLogo->setHideEnabled(true);
        theApp.flushX();
    }

    settings.endGroup();
    settings.beginGroup(SequencerOptionsConfigGroup);

    // See if the settings wants us to load a soundfont
    //
    if (qStrToBool(settings.value("sfxloadenabled", "false"))) {
        QString sfxLoadPath = settings.value("sfxloadpath", "/usr/bin/asfxload").toString();
        QString soundFontPath = settings.value("soundfontpath", "").toString();
        QFileInfo sfxLoadInfo(sfxLoadPath), soundFontInfo(soundFontPath);
        if (sfxLoadInfo.isExecutable() && soundFontInfo.isReadable()) {
            // setup sfxload Process
            QProcess* sfxLoadProcess = new QProcess;

            RG_DEBUG << "Starting sfxload : " << sfxLoadPath << " " << soundFontPath << endl;

            // NOTE: we used to have a broken connect here to hook to a slot
            // that never existed.  This omission doesn't seem to have ever
            // impacted the functioning of this code, since we pre-test at the
            // head of this if block to see if the elements involved are valid,
            // and I suppose we just go on blind faith that if the elements are
            // valid, then the QProcess will work.

            sfxLoadProcess->start(sfxLoadPath, (QStringList()) << soundFontPath);
        } else {
            RG_DEBUG << "sfxload not executable or soundfont not readable : "
                     << sfxLoadPath << " " << soundFontPath << endl;
        }

    } else {
        RG_DEBUG << "sfxload disabled\n";
    }


#ifdef Q_WS_X11
    XSetErrorHandler(_x_errhandler);
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

        if (!newVersion) {
            RosegardenMainWindow::self()->awaitDialogClearance();
        }
    }

    if (newVersion) {
        StartupLogo::hideIfStillThere();
        CurrentProgressDialog::freeze();

        QDialog *dialog = new QDialog;
        dialog->setModal(true);
        dialog->setWindowTitle(RosegardenApplication::tr("Welcome!"));
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
        label->setText(RosegardenApplication::tr("<h2>Welcome to Rosegarden!</h2><p>Welcome to the Rosegarden audio and MIDI sequencer and musical notation editor.</p><ul><li>If you have not already done so, you may wish to install some DSSI synth plugins, or a separate synth program such as QSynth.  Rosegarden does not synthesize sounds from MIDI on its own, so without these you will hear nothing.</li><li>Rosegarden uses the JACK audio server for recording and playback of audio, and for playback from DSSI synth plugins.  These features will only be available if the JACK server is running.</li><li>Rosegarden has comprehensive documentation: see the Help menu for the handbook, tutorials, and other information!</li></ul><p>Rosegarden was brought to you by a team of volunteers across the world.  To learn more, go to <a style=\"color:gold\" href=\"http://www.rosegardenmusic.com/\">http://www.rosegardenmusic.com/</a>.</p>"));
        label->setWordWrap(true);
        label->setOpenExternalLinks(true);

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

    try {
        rosegardengui->launchSequencer();
    } catch (std::string e) {
        RG_DEBUG << "RosegardenGUI - " << e << endl;
    } catch (QString e) {
        RG_DEBUG << "RosegardenGUI - " << e << endl;
    } catch (Exception e) {
        RG_DEBUG << "RosegardenGUI - " << e.getMessage() << endl;
    }

    return theApp.exec();
}

