/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[main]"

#include "misc/ConfigGroups.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "gui/application/RosegardenMainWindow.h"
#include "document/RosegardenDocument.h"
#include "gui/widgets/StartupLogo.h"
#include "gui/general/ResourceFinder.h"
#include "gui/general/IconLoader.h"
#include "gui/general/ThornStyle.h"
#include "gui/application/RosegardenApplication.h"
#include "base/RealTime.h"

#include "sound/audiostream/WavFileReadStream.h"
#include "sound/audiostream/WavFileWriteStream.h"
#include "sound/audiostream/OggVorbisReadStream.h"
#include "sound/audiostream/SimpleWavFileWriteStream.h"

#include <svnversion.h> // generated file
#include "rosegarden-version.h"

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
#include <QPixmapCache>
#include <QStringList>

#include <sound/SoundDriverFactory.h>
#include <sys/time.h>
#include <unistd.h>

using namespace Rosegarden;


/*! \mainpage Rosegarden global design
 
Rosegarden is split into 3 main parts:
 
\section base Base
 
The base library holds all of the fundamental "music handling"
structures, of which the primary ones are Event, Segment, Track,
Instrument and Composition.  It also contains a selection of utility
and helper classes of a kind that is not specific to any particular
GUI.

This design came about at a time when Rosegarden had been through several
toolkit experiments, and did not want to chain itself to any one GUI toolkit.
We wanted to be able to take the core of Rosegarden and build a new application
around it simply and easily, and so the base library is built around the STL,
and Qt and KDE classes were not allowed here.  In practice, we and Qt share the
same fate now, and we have been allowing Qt classes in the base library whenever
that represented the most pragmatic and expedient solution to a problem.
 
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
 
 
See also http://rosegardenmusic.com/wiki/dev:units.txt for an explanation of the
units we use for time and pitch values.  See
http://rosegardenmusic.com/wiki/dev:creating_events.txt for an explanation of
how to create new Events and add properties to them.
 
The base directory also contains various music-related helper classes:
 
 - The NotationTypes.[c|h] files contain classes that help with
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
 
 - BaseProperties.[c|h] contains a set of "standard"-ish Event
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
 
The GUI directory builds into a Qt application that follows a document/view model. The document (class
RosegardenDocument, which wraps a Composition (along with several other related classes)) can have several views
(class RosegardenMainViewWidget), although at the moment only a single one is
used.
 
This view is the TrackEditor, which shows all the Composition's Segments
organized in Tracks. Each Segment can be edited in several ways, as notation, on
a piano roll matrix, or via the raw event list.

All editor views are derived from EditViewBase. EditViewBase is the class
dealing with the edition per se of the events. It uses several
components:

\remarks LayoutEngine no longer seems to be relevant.  The following
documentation needs to be updated by someone who really understands how
everything works on the far side of the Thorn restructuring.  Readers
should understand this documentation might not reflect reality very well. 
 
 - Layout classes, horizontal and vertical: these are the classes
    which determine the x and y coordinates of the graphic items
    representing the events (notes or piano-roll rectangles).  They
    are derived from the LayoutEngine base-class in the base library.
 
 - Tools, which implement each editing function at the GUI (such as
    insert, erase, cut and paste). These are the tools which appear on
    the EditView's toolbar.
 
 - Toolbox, which is a simple string => tool map.
 
 - Commands, which are the fundamental implementations of editing
    operations (both menu functions and tool operations).  Originally a 
    KDE subclass, these are our own implementation now, likely
    borrowed from Sonic Visualiser.
 
 - a QGraphicsScene and QGraphicsView, no longer actually from a shared base
   class, I don't think
 
 - LinedStaff, a staff with lines.  Like the canvas view, this isn't
    part of the EditView definition, but both views use one. (Probably
    different implementations now, and no longer shared.  Author not sure.)
 
 
There are currently two editor views:

\remarks some of this is still true, some of it isn't
 
 - NotationView, with accompanying classes NotationHLayout,
    NotationVLayout, NotationStaff, and all the classes in the
    notationtool and notationcommands files.  These are also closely
    associated with the NotePixmapFactory and NoteFont classes, which
    are used to generate notes from component pixmap files.
 
 - MatrixView, with accompanying classes MatrixHLayout,
    MatrixVLayout, and other classes in the matrixview
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
   for Rosegarden including the Sequencer class itself.
 
 - libRosegardenSound holds the MidiFile class (writing and reading
   MIDI files) and the MappedEvent and MappedEventList classes (the
   communication class for transferring events back and forth between
   sequencer and GUI).  This library is needed by the GUI as well as
   the Sequencer.
 
The main Sequencer state machine is a good starting point and clearly
visible at the bottom of rosegarden/sequencer/main.cpp.
*/

// -----------------------------------------------------------------

static void usage()
{
    std::cerr << "Rosegarden: A sequencer and musical notation editor" << std::endl;
    std::cerr << "Usage: rosegarden [--nosplash] [--nosound] [file.rg]" << std::endl;
    std::cerr << "       rosegarden --version" << std::endl;
    exit(2);
}

int main(int argc, char *argv[])
{

    // Initialization of static objects related to read and write of audio
    // files.
    // This fixes bug #1503 (Audio files can't be read when RG is built in
    // release mode).

#ifdef HAVE_LIBSNDFILE
    WavFileReadStream::initStaticObjects();
    WavFileWriteStream::initStaticObjects();
#endif

#ifdef HAVE_OGGZ
#ifdef HAVE_FISHSOUND
    OggVorbisReadStream::initStaticObjects();
#endif
#endif

#ifndef HAVE_LIBSNDFILE
    SimpleWavFileWriteStream::initStaticObjects();
#endif



    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--version")) {
            std::cout << "Rosegarden version: " << VERSION << " (\"" << CODENAME << "\")" << std::endl;
            std::cout << "Build key: " << BUILDKEY << std::endl;
            std::cout << "Built against Qt version: " << QT_VERSION_STR << std::endl;
            return 0;
        }
    }

    QPixmapCache::setCacheLimit(8192); // KB

    setsid(); // acquire shiny new process group

    srandom((unsigned int)time(nullptr) * (unsigned int)getpid());

    bool styleSpecified = false;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-style")) {
            styleSpecified = true;
            break;
        }
    }

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    RosegardenApplication theApp(argc, argv);

    theApp.setOrganizationName("rosegardenmusic");
    theApp.setOrganizationDomain("rosegardenmusic.com");
    theApp.setApplicationName(QObject::tr("Rosegarden"));

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    bool Thorn = settings.value("use_thorn_style", true).toBool();

    // If the option was turned on in settings, but the user has specified a
    // style on the command line (obnoxious user!) then we must turn this option
    // _off_ in settings as though the user had un-checked it on the config
    // page, or else mayhem and chaos will reign.
    if (Thorn && styleSpecified) {
        settings.setValue("use_thorn_style", false);
        Thorn = false;
    }

    settings.endGroup();

    ThornStyle::setEnabled(Thorn);

    // This allows icons to appear in the instrument popup menu in
    // TrackButtons.
    theApp.setAttribute(Qt::AA_DontShowIconsInMenus, false);
    QStringList args = theApp.arguments();

    // enable to load resources from rcc file (if not compiled in)
#ifdef RESOURCE_FILE_NOT_COMPILED_IN
    std::cerr << "Loading resource file ./data/data.rcc..." << std::endl;
    if (!QResource::registerResource("./data/data.rcc")) {
        std::cerr << "Failed to register resource file!" << std::endl;
    }
#endif

    RG_DEBUG << "System Locale:" << QLocale::system().name();
    RG_DEBUG << "Qt translations path: " << QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    QTranslator qtTranslator;
    bool qtTranslationsLoaded = 
      qtTranslator.load("qt_" + QLocale::system().name(),
            QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    if (qtTranslationsLoaded) {
        theApp.installTranslator(&qtTranslator);
        RG_DEBUG << "Qt translations loaded successfully.";
    } else {
        RG_WARNING << "Qt translations not loaded.";
    }

    QTranslator rgTranslator;
    RG_DEBUG << "RG Translation: trying to load :locale/" << QLocale::system().name();
    bool rgTranslationsLoaded = 
      rgTranslator.load(QLocale::system().name(), ":locale/");
    if (rgTranslationsLoaded) {
        RG_DEBUG << "RG Translations loaded successfully.";
        theApp.installTranslator(&rgTranslator);
    } else {
        RG_WARNING << "RG Translations not loaded.";
    }

    bool nosplash = false;
    bool nosound = false;
    int nonOptArgs = 0;

    for (int i = 1; i < args.size(); ++i) {
        if (args[i].startsWith("-")) {
            if (args[i] == "--nosplash") nosplash = true;
            else if (args[i] == "--nosound") nosound = true;
            else usage();
        } else {
            ++nonOptArgs;
        }
    }
    if (nonOptArgs > 1) usage();

    QIcon icon;
    static const int sizes[] = { 16, 22, 24, 32, 48, 64, 128 };
    for (size_t i = 0; i < sizeof(sizes)/sizeof(sizes[0]); ++i) {
        QString name = QString("rg-rwb-rose3-%1x%2").arg(sizes[i]).arg(sizes[i]);
        QPixmap pixmap = IconLoader().loadPixmap(name);
        if (!pixmap.isNull()) {
            RG_DEBUG << "Loaded application icon \"" << name << "\"";
            icon.addPixmap(pixmap);
        }
    }
    theApp.setWindowIcon(icon);

    settings.beginGroup(GeneralOptionsConfigGroup);

    QString lastVersion = settings.value("lastversion", "").toString();
    bool newVersion = (lastVersion != VERSION);
    if (newVersion) {
        RG_WARNING << "*** This is the first time running this Rosegarden version";
        settings.setValue("lastversion", VERSION);

    }

    RG_INFO << "Unbundling examples...";
    
    // unbundle examples
    const QStringList exampleFiles = ResourceFinder().getResourceFiles("examples", "rg");
    for (QStringList::const_iterator i = exampleFiles.constBegin(); i != exampleFiles.constEnd(); ++i) {
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

    RG_INFO << "Unbundling templates...";

    // unbundle templates
    const QStringList templateFiles = ResourceFinder().getResourceFiles("templates", "rgt");
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

    RG_INFO << "Unbundling libraries (device files)...";

    // unbundle libraries
    const QStringList libraryFiles = ResourceFinder().getResourceFiles("library", "rgd");
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

    StartupLogo* startLogo = nullptr;

    if (qStrToBool(settings.value("Logo", "true")) && !nosplash) {
        startLogo = StartupLogo::getInstance();
        startLogo->setShowTip(!newVersion);
        startLogo->show();
        startLogo->repaint();
        theApp.processEvents();
        theApp.flush();
    }

    struct timeval logoShowTime;
    gettimeofday(&logoShowTime, nullptr);

    SoundDriverFactory::setSoundEnabled(!nosound);

    RG_INFO << "Creating RosegardenMainWindow instance...";

    RosegardenMainWindow *mainWindow =
            new RosegardenMainWindow(!nosound, startLogo);

    mainWindow->setIsFirstRun(newVersion);

    // This parentless/shown window will become the main window when
    // QApplication::exec() is called.
    mainWindow->show();

    // raise start logo
    //
    if (startLogo) {
        startLogo->raise();
        startLogo->setHideEnabled(true);
        startLogo->repaint();
        theApp.flush();
    }

    for (int i = 1; i < args.size(); ++i) {
        if (args[i].startsWith("-")) continue;
        mainWindow->openFile(args[i], RosegardenMainWindow::ImportCheckType);
        break;
    }

    //@@@???
    QObject::connect(&theApp, &RosegardenApplication::aboutToSaveState,
                     mainWindow, &RosegardenMainWindow::slotDeleteTransport);

    // Now that we've started up, raise start logo
    //
    if (startLogo) {
        startLogo->raise();
        startLogo->setHideEnabled(true);
        theApp.flush();
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

            RG_DEBUG << "Starting sfxload : " << sfxLoadPath << " " << soundFontPath;

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


    if (startLogo) {

        // pause to ensure the logo has been visible for a reasonable
        // length of time, just 'cos it looks a bit silly to show it
        // and remove it immediately

        struct timeval now;
        gettimeofday(&now, nullptr);

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
        QString manualURL(QObject::tr("http://rosegardenmusic.com/wiki/doc:manual-en"));
        label->setText(QObject::tr("<h2>Welcome to Rosegarden!</h2><p>Welcome to the Rosegarden audio and MIDI sequencer and musical notation editor.</p><ul><li>If you have not already done so, you may wish to install some DSSI synth plugins, or a separate synth program such as QSynth.  Rosegarden does not synthesize sounds from MIDI on its own, so without these you will hear nothing.</li><li>Rosegarden uses the JACK audio server for recording and playback of audio, and for playback from DSSI synth plugins.  These features will only be available if the JACK server is running.</li><li>Rosegarden has comprehensive documentation: see the <a style=\"color:gold\" href=\"http://rosegardenmusic.com\">Rosegarden website</a> for the <a style=\"color:gold\" href=\"%1\">manual</a>, <a style=\"color:gold\" href=\"http://www.rosegardenmusic.com/tutorials/\">tutorials</a>, and other information!</li></ul><p>Rosegarden was brought to you by a team of volunteers across the world.  To learn more, go to the <a style=\"color:gold\" href=\"http://www.rosegardenmusic.com/\">Rosegarden website</a>.</p>").arg(manualURL));
        label->setWordWrap(true);
        label->setOpenExternalLinks(true);

        hb->setLayout(hbLayout);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
        metagrid->addWidget(buttonBox, 1, 0);
        metagrid->setRowStretch(0, 10);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

        mainWindow->awaitDialogClearance();
        dialog->exec();
    }
    settings.endGroup();

    RG_INFO << "Launching the sequencer...";

    try {
        mainWindow->launchSequencer();
    } catch (std::string e) {
        RG_DEBUG << "mainWindow->launchSequencer() - " << e;
    } catch (QString e) {
        RG_DEBUG << "mainWindow->launchSequencer() - " << e;
    } catch (Exception e) {
        RG_DEBUG << "mainWindow->launchSequencer() - " << e.getMessage();
    }

//#define STYLE_TEST
#ifdef STYLE_TEST
    QProgressDialog dialog;
    dialog.setMaximum(500);
    for (int i = 0; i <= 500; ++i) {
        dialog.setValue(i);
        QThread::msleep(1);
        qApp->processEvents();
    }

    QMessageBox::information(0, "Rosegarden", "Information.", QMessageBox::Ok, QMessageBox::Ok);
    QMessageBox::critical(0, "Rosegarden", "Critical!", QMessageBox::Ok, QMessageBox::Ok);
    QMessageBox::question(0, "Rosegarden", "Question?", QMessageBox::Ok, QMessageBox::Ok);
    QMessageBox::warning(0, "Rosegarden", "Warning!", QMessageBox::Ok, QMessageBox::Ok);
#endif

    RG_INFO << "Starting the app...";

    int returnCode = theApp.exec();

    // Announce end of run so that we can tell if we have crashed on
    // the way down.
    RG_INFO << "Rosegarden main() exiting with rc:" << returnCode;

    return returnCode;
}

