// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

// include files for QT
#include <qdragobject.h>

// include files for KDE
#include <kstdaccel.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kprocess.h>
#include <kprinter.h>
#include <dcopclient.h>
#include <qiconset.h>
#include <kstddirs.h>

#include <kaction.h>
#include <kstdaction.h>

// application specific includes
#include "rosegardengui.h"
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "MidiFile.h"
#include "rg21io.h"
#include "rosegardendcop.h"
#include "ktmpstatusmsg.h"
#include "SegmentPerformanceHelper.h"
#include "NotationTypes.h"
#include "sequencemanager.h"
#include "trackbuttons.h"
#include "trackeditor.h"

#define ID_STATUS_MSG 1

using std::cerr;
using std::endl;
using std::cout;
using Rosegarden::timeT;

static Rosegarden::MappedComposition mappComp;

RosegardenGUIApp::RosegardenGUIApp()
    : KMainWindow(0), RosegardenIface(this), DCOPObject("RosegardenIface"),
      m_config(kapp->config()),
      m_fileRecent(0),
      m_view(0),
      m_doc(0),
      m_sequencerProcess(0),
      m_seqManager(0),
      m_transport(0),
      m_originatingJump(false),
      m_storedLoopStart(0),
      m_storedLoopEnd(0)
{
    // accept dnd
    setAcceptDrops(true);

    ///////////////////////////////////////////////////////////////////
    // call inits to invoke all other construction parts
    
    initStatusBar();
    initDocument();
    setupActions();

    initView();

    // Create a sequence manager
    m_seqManager = new Rosegarden::SequenceManager(m_doc, m_transport);

    connect(m_doc, SIGNAL(pointerPositionChanged(Rosegarden::timeT)),
            this,   SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    connect(m_doc, SIGNAL(loopChanged(Rosegarden::timeT, Rosegarden::timeT)),
            this,  SLOT(slotSetLoop(Rosegarden::timeT, Rosegarden::timeT)));

    readOptions();

    actionCollection()->action("draw")->activate();

//     ///////////////////////////////////////////////////////////////////
//     // disable menu and toolbar items at startup
//     disableCommand(ID_FILE_SAVE);
//     disableCommand(ID_FILE_SAVE_AS);
//     disableCommand(ID_FILE_PRINT);
 
//     disableCommand(ID_EDIT_CUT);
//     disableCommand(ID_EDIT_COPY);
//     disableCommand(ID_EDIT_PASTE);


    // Now autoload
    //
    performAutoload();
}

RosegardenGUIApp::~RosegardenGUIApp()
{
    kdDebug(KDEBUG_AREA) << "~RosegardenGUIApp()\n";

    if (m_sequencerProcess) {
        m_sequencerProcess->blockSignals(true);
        delete m_sequencerProcess;
    }
}


void RosegardenGUIApp::setupActions()
{
    // setup File menu
    // New Window ?
    KStdAction::openNew (this, SLOT(slotFileNew()),     actionCollection());
    KStdAction::open    (this, SLOT(slotFileOpen()),    actionCollection());
    m_fileRecent = KStdAction::openRecent(this,
                                          SLOT(slotFileOpenRecent(const KURL&)),
                                          actionCollection());
    KStdAction::save  (this, SLOT(slotFileSave()),          actionCollection());
    KStdAction::saveAs(this, SLOT(slotFileSaveAs()),        actionCollection());
    KStdAction::close (this, SLOT(slotFileClose()),         actionCollection());
    KStdAction::print (this, SLOT(slotFilePrint()),         actionCollection());

    new KAction(i18n("Import &MIDI file..."), 0, 0, this,
                SLOT(slotImportMIDI()), actionCollection(),
                "file_import_midi");

    new KAction(i18n("Import &Rosegarden 2.1 file..."), 0, 0, this,
                SLOT(slotImportRG21()), actionCollection(),
                "file_import_rg21");

    new KAction(i18n("Export as &MIDI file..."), 0, 0, this,
                SLOT(slotExportMIDI()), actionCollection(),
                "file_export_midi");

    KStdAction::quit  (this, SLOT(slotQuit()),              actionCollection());

    // setup edit menu
    KStdAction::cut      (this, SLOT(slotEditCut()),        actionCollection());
    KStdAction::copy     (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste    (this, SLOT(slotEditPaste()),      actionCollection());

    // setup Settings menu
    //
    m_viewToolBar = KStdAction::showToolbar  (this, SLOT(slotToggleToolBar()),   actionCollection());
    m_viewTracksToolBar = new KToggleAction(i18n("Show T&racks Toolbar"), 0, this,
                                            SLOT(slotToggleTracksToolBar()), actionCollection(),
                                            "show_tracks_toolbar");
    m_viewStatusBar = KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()), actionCollection());

    m_viewTransport = new KToggleAction(i18n("Show Tra&nsport"), 0, this,
                                        SLOT(slotToggleTransport()),
                                        actionCollection(),
                                        "show_transport");

    m_viewTrackLabels = new KToggleAction(i18n("Show Track &Labels"), 0, this,
                                             SLOT(slotToggleTrackLabels()),
                                             actionCollection(),
                                             "show_tracklabels");

    m_viewSegmentParameters = new KToggleAction(i18n("Show &Segment Parameters"), 0, this,
                                                SLOT(slotToggleSegmentParameters()),
                                                actionCollection(),
                                                "show_segment_parameters");

    m_viewInstrumentParameters = new KToggleAction(i18n("Show &Instrument Parameters"), 0, this,
                                                   SLOT(slotToggleInstrumentParameters()),
                                                   actionCollection(),
                                                   "show_instrument_parameters");

    m_viewRulers = new KToggleAction(i18n("Show &Rulers"), 0, this,
                                     SLOT(slotToggleRulers()),
                                     actionCollection(),
                                     "show_rulers");

    KStdAction::saveOptions(this, SLOT(save_options()), actionCollection());
    KStdAction::preferences(this, SLOT(customize()),    actionCollection());

    KStdAction::keyBindings      (this, SLOT(editKeys()),     actionCollection());
    KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());

    KRadioAction *action = 0;
    
    // Create the select icon
    //
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QIconSet icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/select.xpm"));

    // TODO : add some shortcuts here
    action = new KRadioAction(i18n("&Select"), icon,
                              0,
                              this, SLOT(slotPointerSelected()),
                              actionCollection(), "select");

    action->setExclusiveGroup("segmenttools");
                             
    action = new KRadioAction(i18n("&Erase"), "eraser",
                              0,
                              this, SLOT(slotEraseSelected()),
                              actionCollection(), "erase");
    action->setExclusiveGroup("segmenttools");

    action = new KRadioAction(i18n("&Draw"), "pencil",
                              0,
                              this, SLOT(slotDrawSelected()),
                              actionCollection(), "draw");
    action->setExclusiveGroup("segmenttools");

    action = new KRadioAction(i18n("&Move"), "move",
                              0,
                              this, SLOT(slotMoveSelected()),
                              actionCollection(), "move");
    action->setExclusiveGroup("segmenttools");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/resize.xpm"));
    action = new KRadioAction(i18n("&Resize"), icon,
                              0,
                              this, SLOT(slotResizeSelected()),
                              actionCollection(), "resize");
    action->setExclusiveGroup("segmenttools");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/split.xpm"));
    action = new KRadioAction(i18n("&Split"), icon,
                              0,
                              this, SLOT(slotSplitSelected()),
                              actionCollection(), "split");
    action->setExclusiveGroup("segmenttools");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/join.xpm"));
    action = new KRadioAction(i18n("&Join"), icon,
                              0,
                              this, SLOT(slotJoinSelected()),
                              actionCollection(), "join");
    action->setExclusiveGroup("segmenttools");

    new KAction(i18n("Change &Time Resolution..."), 
                0,
                this, SLOT(slotChangeTimeResolution()),
                actionCollection(), "change_time_res");

    new KAction(i18n("&Score Editor"),
                0,
                this, SLOT(slotEditAllTracks()),
                actionCollection(), "edit_all_tracks");

    // Transport controls [rwb]
    //
    // We set some default key bindings - with numlock off
    // use 1 (End) and 3 (Page Down) for Rwd and Ffwd and
    // 0 (insert) and Enter for Play and Stop 
    //
    m_playTransport = new KAction(i18n("&Play"), 0, Key_Enter, this,
                                  SLOT(slotPlay()), actionCollection(),
                                  "play");
    m_playTransport->setGroup("transportcontrols");

    m_stopTransport = new KAction(i18n("&Stop"), 0, Key_Insert, this,
                                  SLOT(slotStop()), actionCollection(),
                                  "stop");
    m_stopTransport->setGroup("transportcontrols");

    m_ffwdTransport = new KAction(i18n("&Fast Forward"), 0, Key_PageDown,
                                  this,
                                  SLOT(slotFastforward()), actionCollection(),
                                  "fast_forward");
    m_ffwdTransport->setGroup("transportcontrols");

    m_rewindTransport = new KAction(i18n("Re&wind"), 0, Key_End, this,
                                  SLOT(slotRewind()), actionCollection(),
                                  "rewind");
    m_rewindTransport->setGroup("transportcontrols");

    m_recordTransport = new KAction(i18n("&Record"), 0, Key_End, this,
                                  SLOT(slotRecord()), actionCollection(),
                                  "record");

    m_recordTransport->setGroup("transportcontrols");

    m_rewindEndTransport = new KAction(i18n("Rewind to Beginning"), 0, 0, this,
                                  SLOT(slotRewindToBeginning()), actionCollection(),
                                  "rewindtobeginning");

    m_rewindEndTransport->setGroup("transportcontrols");

    m_ffwdEndTransport = new KAction(i18n("Fast Forward to End"), 0, 0, this,
                                  SLOT(slotFastForwardToEnd()), actionCollection(),
                                   "fastforwardtoend");

    m_ffwdEndTransport->setGroup("transportcontrols");

    // create the Transport GUI and add the callbacks
    //
    m_transport = new Rosegarden::RosegardenTransportDialog(this, "");

    // We should be plugging the actions into the buttons
    // on the transport - but this doesn't seem to be working
    // as expected (i.e. at all) so we're duplicating the
    // accelerators in the dialog - nasty but works for the
    // moment.
    //
/*
    m_playTransport->plug(m_transport->PlayButton);
    m_stopTransport->plug(m_transport->StopButton);
    m_rewindTransport->plug(m_transport->RewindButton);
    m_ffwdTransport->plug(m_transport->FfwdButton);
*/

    QAccel *a = new QAccel(m_transport);

    connect((QObject *) m_transport->PlayButton,
             SIGNAL(released()),
             SLOT(slotPlay()));

    a->connectItem(a->insertItem(Key_Enter),
                   this,
                   SLOT(slotPlay()));

    connect((QObject *) m_transport->StopButton,
             SIGNAL(released()),
             SLOT(slotStop()));
             
    a->connectItem(a->insertItem(Key_Insert),
                   this,
                   SLOT(slotStop()));

    connect((QObject *) m_transport->FfwdButton,
             SIGNAL(released()),
             SLOT(slotFastforward()));
            
    a->connectItem(a->insertItem(Key_PageDown),
                   this,
                   SLOT(slotFastforward()));

    connect((QObject *) m_transport->RewindButton,
             SIGNAL(released()),
             SLOT(slotRewind()));

    a->connectItem(a->insertItem(Key_End),
                   this,
                   SLOT(slotRewind()));

    connect((QObject *) m_transport->RecordButton,
             SIGNAL(released()),
             SLOT(slotRecord()));

    a->connectItem(a->insertItem(Key_Space),
                   this,
                   SLOT(slotRecord()));

    connect((QObject *) m_transport->RewindEndButton,
             SIGNAL(released()),
             SLOT(slotRewindToBeginning()));


    connect((QObject *) m_transport->FfwdEndButton,
             SIGNAL(released()),
             SLOT(slotFastForwardToEnd()));

    connect((QObject *)m_transport->MetronomeButton,
            SIGNAL(released()),
            SLOT(slotToggleMetronome()));
            
    connect((QObject *) m_transport->TimeDisplayButton,
             SIGNAL(released()),
             SLOT(slotRefreshTimeDisplay()));

    connect((QObject *) m_transport->ToEndButton,
             SIGNAL(released()),
             SLOT(slotRefreshTimeDisplay()));

    createGUI("rosegardenui.rc");

    // Ensure that the checkbox is unchecked if the dialog
    // is closed
    connect((QObject *)m_transport, SIGNAL(closed()),
                                    SLOT(slotCloseTransport()));

    // Handle loop setting and unsetting from the transport loop button
    //
    connect(m_transport, SIGNAL(setLoop()), SLOT(slotSetLoop()));
    connect(m_transport, SIGNAL(unsetLoop()), SLOT(slotUnsetLoop()));
}


void RosegardenGUIApp::initStatusBar()
{
    statusBar()->insertItem(KTmpStatusMsg::getDefaultMsg(),
			    KTmpStatusMsg::getDefaultId(), 1);
    statusBar()->setItemAlignment(KTmpStatusMsg::getDefaultId(), 
				  AlignLeft | AlignVCenter);
}

void RosegardenGUIApp::initDocument()
{
    m_doc = new RosegardenGUIDoc(this);
    m_doc->newDocument();

    m_doc->getCommandHistory()->attachView(actionCollection());
}

void RosegardenGUIApp::initView()
{ 
    ////////////////////////////////////////////////////////////////////
    // create the main widget here that is managed by KTMainWindow's view-region and
    // connect the widget to your document to display document contents.

    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::initView()" << endl;

    Rosegarden::Composition &comp = m_doc->getComposition();

    // Ensure that the start and end markers for the piece are set
    // to something reasonable
    //
    if (comp.getStartMarker() == 0 &&
        comp.getEndMarker() == 0)
    {
        int endMarker = comp.getBarRange(10 + comp.getNbBars()).second;
        comp.setEndMarker(endMarker);
    }
    
    m_view = new RosegardenGUIView(m_viewTrackLabels->isChecked(), this);

    // Connect up this signal so that we can force tool mode
    // changes from the view
    connect(m_view, SIGNAL(activateTool(SegmentCanvas::ToolType)),
            this,   SLOT(slotActivateTool(SegmentCanvas::ToolType)));

    // So that the GUIView can send immediate controllers
    //
    connect(m_view, SIGNAL(sendMappedEvent(Rosegarden::MappedEvent*)),
            this, SLOT(slotSendMappedEvent(Rosegarden::MappedEvent*)));

    m_doc->addView(m_view);
    setCentralWidget(m_view);
    setCaption(m_doc->getTitle());

    // set the pointer position
    //
    slotSetPointerPosition
	(comp.getElapsedRealTime(m_doc->getComposition().getPosition()));

    // set the tempo in the transport
    //
    m_transport->setTempo(comp.getTempo());

    // bring the transport to the front 
    //
    m_transport->raise();

    // set the play metronome button
    m_transport->MetronomeButton->setOn(comp.usePlayMetronome());


    // We only check for the SequenceManager to make sure
    // we're not on the first pass though - we don't want
    // to send these toggles twice on initialisation.
    //
    // Clunky but we just about get away with it for the
    // moment.
    //
    if(m_seqManager != 0)
    {
      slotToggleSegmentParameters();
      slotToggleInstrumentParameters();
      slotToggleRulers();
    }

}

void RosegardenGUIApp::openDocumentFile(const char* _cmdl)
{
    KTmpStatusMsg msg(i18n("Opening file..."), statusBar());
    
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::openDocumentFile("
                         << _cmdl << ")" << endl;

    m_doc->saveIfModified();
    m_doc->closeDocument();

    if (m_doc->openDocument(_cmdl)) {

        initView();

        // See comment in RosegardenGUIApp::openURL()
        // for an explanation on why we have to do this
        actionCollection()->action("draw")->activate();
        actionCollection()->action("move")->activate();
    }

}

void RosegardenGUIApp::openFile(const QString& url)
{

    setCaption(url);
    KURL *u = new KURL( url );

    if (u->isMalformed()) {
        KMessageBox::sorry(this, i18n("This is not a valid filename.\n"));
        return;
    }

    if (!u->isLocalFile()) {
        KMessageBox::sorry(this, i18n("This is not a local file.\n"));
        return;
    }

    QFileInfo info(u->path());

    if (!info.exists()) {
        KMessageBox::sorry(this, i18n("The specified file does not exist"));
        return;
    }

    if (info.isDir()) {
        KMessageBox::sorry(this, i18n("You have specified a directory"));
        return;
    }

    QFile file(u->path());

    if (!file.open(IO_ReadOnly)) {
        KMessageBox::sorry(this, i18n("You do not have read permission to this file."));
        return;
    }

    // Stop if playing
    //
    if (m_seqManager->getTransportStatus() == PLAYING)
      slotStop();

    m_doc->closeDocument();
    m_doc->openDocument(u->path());

    initView();

    // Ensure the sequencer knows about any audio files
    // we've loaded as part of the new Composition
    //
    m_doc->prepareAudio();

    Rosegarden::Composition &comp = m_doc->getComposition();

    // Set any loaded loop at the Composition and
    // on the marker on SegmentCanvas and clients
    //
    m_doc->setLoop(comp.getLoopStart(), comp.getLoopEnd());
}


RosegardenGUIDoc *RosegardenGUIApp::getDocument() const
{
    return m_doc;
}

void RosegardenGUIApp::saveOptions()
{	
    m_config->setGroup("General Options");
    m_config->writeEntry("Geometry", size());
    m_config->writeEntry("Show Toolbar",                 m_viewToolBar->isChecked());
    m_config->writeEntry("Show Tracks Toolbar",          m_viewTracksToolBar->isChecked());
    m_config->writeEntry("Show Transport",               m_viewTransport->isChecked());
    m_config->writeEntry("Expanded Transport",           m_transport->isExpanded());
    m_config->writeEntry("Show Track labels",            m_viewTrackLabels->isChecked());
    m_config->writeEntry("Show Statusbar",               m_viewStatusBar->isChecked());
    m_config->writeEntry("Show Segment Parameters",      m_viewSegmentParameters->isChecked());
    m_config->writeEntry("Show Instrument Parameters",   m_viewInstrumentParameters->isChecked());
    m_config->writeEntry("Show Rulers",                  m_viewRulers->isChecked());


    m_config->writeEntry("ToolBarPos", (int) toolBar("mainToolBar")->barPos());
    m_config->writeEntry("TracksToolBarPos", (int) toolBar("tracksToolBar")->barPos());

    m_fileRecent->saveEntries(m_config, "Recent Files");
}


void RosegardenGUIApp::readOptions()
{
    bool opt;

    m_config->setGroup("General Options");

    // status bar settings
    opt = m_config->readBoolEntry("Show Statusbar", true);
    m_viewStatusBar->setChecked(opt);
    slotToggleStatusBar();

    opt = m_config->readBoolEntry("Show Toolbar", true);
    m_viewToolBar->setChecked(opt);
    slotToggleToolBar();

    opt = m_config->readBoolEntry("Show Tracks Toolbar", true);
    m_viewTracksToolBar->setChecked(opt);
    slotToggleTracksToolBar();

    opt = m_config->readBoolEntry("Show Transport", false);
    m_viewTransport->setChecked(opt);
    slotToggleTransport();

    opt = m_config->readBoolEntry("Expanded Transport", false);
    if(opt)
        m_transport->slotPanelOpenButtonReleased();
    else
        m_transport->slotPanelCloseButtonReleased();

    opt = m_config->readBoolEntry("Show Track labels", true);
    m_viewTrackLabels->setChecked(opt);
    slotToggleTrackLabels();

    opt = m_config->readBoolEntry("Show Segment Parameters", false);
    m_viewSegmentParameters->setChecked(opt);
    slotToggleSegmentParameters();

    opt = m_config->readBoolEntry("Show Instrument Parameters", false);
    m_viewInstrumentParameters->setChecked(opt);
    slotToggleInstrumentParameters();

    opt = m_config->readBoolEntry("Show Rulers", false);
    m_viewRulers->setChecked(opt);
    slotToggleRulers();

    // bar position settings
    KToolBar::BarPosition toolBarPos;
    toolBarPos=(KToolBar::BarPosition) m_config->readNumEntry("ToolBarPos", KToolBar::Top);
    toolBar("mainToolBar")->setBarPos(toolBarPos);

    toolBarPos=(KToolBar::BarPosition) m_config->readNumEntry("TracksToolBarPos", KToolBar::Top);
    toolBar("tracksToolBar")->setBarPos(toolBarPos);
	
    // initialise the recent file list
    //
    m_fileRecent->loadEntries(m_config);

    QSize size(m_config->readSizeEntry("Geometry"));

    if(!size.isEmpty()) {
        resize(size);
    }
}

void RosegardenGUIApp::saveProperties(KConfig *_cfg)
{
    if (m_doc->getTitle()!=i18n("Untitled") && !m_doc->isModified()) {
        // saving to tempfile not necessary
    } else {
        QString filename=m_doc->getAbsFilePath();	
        _cfg->writeEntry("filename", filename);
        _cfg->writeEntry("modified", m_doc->isModified());
		
        QString tempname = kapp->tempSaveName(filename);
        m_doc->saveDocument(tempname);
    }
}


void RosegardenGUIApp::readProperties(KConfig* _cfg)
{
    QString filename = _cfg->readEntry("filename", "");
    bool modified = _cfg->readBoolEntry("modified", false);

    if (modified) {
            bool canRecover;
            QString tempname = kapp->checkRecoverFile(filename, canRecover);
  	
            if (canRecover) {
                m_doc->openDocument(tempname);
                m_doc->setModified();
                QFileInfo info(filename);
                m_doc->setAbsFilePath(info.absFilePath());
                m_doc->setTitle(info.fileName());
                QFile::remove(tempname);
            }
        } else {
            if (!filename.isEmpty()) {
                m_doc->openDocument(filename);
            }
        }

    QString caption=kapp->caption();
    setCaption(caption+": "+m_doc->getTitle());
}		

void RosegardenGUIApp::dragEnterEvent(QDragEnterEvent *event)
{
    // accept uri drops only
    event->accept(QUriDrag::canDecode(event));
}

void RosegardenGUIApp::dropEvent(QDropEvent *event)
{
    // this is a very simplistic implementation of a drop event.  we
    // will only accept a dropped URL.  the Qt dnd code can do *much*
    // much more, so please read the docs there
    QStrList uri;

    // see if we can decode a URI.. if not, just ignore it
    if (QUriDrag::decode(event, uri))
    {
        // okay, we have a URI.. process it
        QString url, target;
        url = uri.first();

        // load in the file
        openURL(KURL(url));
    }
}


bool RosegardenGUIApp::queryClose()
{
    return m_doc->saveIfModified();
}

bool RosegardenGUIApp::queryExit()
{
    saveOptions();
    return true;
}

/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////

// Not connected to anything at the moment
//
void RosegardenGUIApp::slotFileNewWindow()
{
    KTmpStatusMsg msg(i18n("Opening a new application window..."), statusBar());
	
    RosegardenGUIApp *new_window= new RosegardenGUIApp();
    new_window->show();
}

void RosegardenGUIApp::slotFileNew()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::slotFileNew()\n";

    KTmpStatusMsg msg(i18n("Creating new document..."), statusBar());

    bool makeNew = false;
    
    if (!m_doc->isModified()) {
        makeNew = true;
        m_doc->closeDocument();
    } else if (m_doc->saveIfModified()) {
        makeNew = true;
    }

    if (makeNew) {

        m_doc->newDocument();

        QString caption=kapp->caption();	
        setCaption(caption+": "+m_doc->getTitle());
        initView();

        // See comment in RosegardenGUIApp::openURL()
        // for an explanation on why we have to do this
        actionCollection()->action("move")->activate();
        actionCollection()->action("draw")->activate();
    }
}

void RosegardenGUIApp::openURL(const KURL& url)
{
    QString netFile = url.url();
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::openURL: " << netFile << endl;

    if (url.isMalformed()) {
        QString string;
        string = i18n( "Malformed URL\n%1").arg(netFile);

        KMessageBox::sorry(this, string);
        return;
    }

    QString target;

    if (KIO::NetAccess::download(url, target) == false) {
        KMessageBox::error(this, i18n("Cannot download file!"));
        return;
    }

    static QRegExp midiFile("\\.mid$"), rg21File("\\.rose$");

    if (midiFile.match(url.path()) != -1) {
        importMIDIFile(target);
    } else if (rg21File.match(url.path()) != -1) {
        importRG21File(target);
    } else {
        openFile(target);
    }

    setCaption(url.path());
    m_fileRecent->addURL(url);

    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::openURL() : enabling 'move'\n";

    // We have to do this to make sure that the 2nd call ("move")
    // actually has any effect. Activating the same radio action
    // doesn't work the 2nd time (like pressing down the same radio
    // button twice - it doesn't have any effect), so if you load two
    // files in a row, on the 2nd file a new SegmentCanvas will be
    // created but its tool won't be set, so clicking on the canvas
    // will crash.
    //
    actionCollection()->action("draw")->activate();
    actionCollection()->action("move")->activate();
}

void RosegardenGUIApp::slotFileOpen()
{
    slotStatusHelpMsg(i18n("Opening file..."));

    KURL url = KFileDialog::getOpenURL(QString::null, "*.rg", this,
                                       i18n("Open File"));
    if ( url.isEmpty() ) { return; }

    openURL(url);
}

void RosegardenGUIApp::slotFileOpenRecent(const KURL &url)
{
    KTmpStatusMsg msg(i18n("Opening file..."), statusBar());
	
//     if (!doc->saveIfModified()) {
//         // here saving wasn't successful
//     } else {
//         doc->closeDocument();
//         doc->openDocument(recentFiles.at(id_));
//         QString caption=kapp->caption();	
//         setCaption(caption+": "+doc->getTitle());
//     }

//     initView();

    openURL(url);
}

void RosegardenGUIApp::slotFileSave()
{
    if (!m_doc || !m_doc->isModified()) return;

    KTmpStatusMsg msg(i18n("Saving file..."), statusBar());

    if (!m_doc->getAbsFilePath())
       slotFileSaveAs();
    else
        m_doc->saveDocument(m_doc->getAbsFilePath());
}

void RosegardenGUIApp::slotFileSaveAs()
{
    if (!m_doc) return;

    KTmpStatusMsg msg(i18n("Saving file with a new filename..."), statusBar());

    QString newName=KFileDialog::getSaveFileName(QDir::currentDirPath(),
                                                 i18n("*.rg"), this, i18n("Save as..."));
    if (!newName.isEmpty()) {
        QFileInfo saveAsInfo(newName);
        m_doc->setTitle(saveAsInfo.fileName());
        m_doc->setAbsFilePath(saveAsInfo.absFilePath());
        m_doc->saveDocument(newName);
        m_fileRecent->addURL(newName);

        QString caption=kapp->caption();	
        setCaption(caption + ": " + m_doc->getTitle());
    }
}

void RosegardenGUIApp::slotFileClose()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::slotFileClose()" << endl;
    
    if (!m_doc) return;

    KTmpStatusMsg msg(i18n("Closing file..."), statusBar());
	
    m_doc->saveIfModified();

    m_doc->closeDocument();

    m_doc->newDocument();

    initView();

    // See comment in RosegardenGUIApp::openURL()
    // for an explanation on why we have to do this
    actionCollection()->action("move")->activate();
    actionCollection()->action("draw")->activate();

    close();
}

void RosegardenGUIApp::slotFilePrint()
{
    if (m_doc->getComposition().getNbSegments() == 0) {
        KMessageBox::sorry(0, "Please create some tracks first (until we implement menu state management)");
        return;
    }

    KTmpStatusMsg msg(i18n("Printing..."), statusBar());

    KPrinter printer;

    printer.setFullPage(true);

    if (printer.setup(this)) {
        m_view->print(&printer, &m_doc->getComposition());
    }
}

void RosegardenGUIApp::slotQuit()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::slotQuit()" << endl;
    
    slotStatusMsg(i18n("Exiting..."));
    saveOptions();
    // close the first window, the list makes the next one the first again.
    // This ensures thatslotQueryClose() is called on each window to ask for closing
    KMainWindow* w;
    if (memberList) {

        for(w=memberList->first(); w!=0; w=memberList->first()) {
            // only close the window if the closeEvent is accepted. If
            // the user presses Cancel on the saveIfModified() dialog,
            // the window and the application stay open.
            if (!w->close())
                break;
        }
    }	
}

void RosegardenGUIApp::slotEditCut()
{
    KTmpStatusMsg msg(i18n("Cutting selection..."), statusBar());
}

void RosegardenGUIApp::slotEditCopy()
{
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), statusBar());
}

void RosegardenGUIApp::slotEditPaste()
{
    KTmpStatusMsg msg(i18n("Inserting clipboard contents..."), statusBar());
}

void RosegardenGUIApp::slotToggleToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the toolbar..."), statusBar());

    if (m_viewToolBar->isChecked())
        toolBar("mainToolBar")->show();
    else
        toolBar("mainToolBar")->hide();
}

void RosegardenGUIApp::slotToggleTracksToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the tracks toolbar..."), statusBar());

    if (m_viewTracksToolBar->isChecked())
        toolBar("tracksToolBar")->show();
    else
        toolBar("tracksToolBar")->hide();
}

void RosegardenGUIApp::slotToggleTransport()
{
    KTmpStatusMsg msg(i18n("Toggle the Transport"), statusBar());

    if (m_viewTransport->isChecked())
    {
        m_transport->show();
        m_transport->raise();
    }
    else
        m_transport->hide();
}

void RosegardenGUIApp::slotToggleTrackLabels()
{
    if (m_viewTrackLabels->isChecked())
    {
        m_view->getTrackEditor()->getTrackButtons()->
            changeTrackInstrumentLabels(TrackButtons::ShowTrack);
    }
    else
    {
        m_view->getTrackEditor()->getTrackButtons()->
            changeTrackInstrumentLabels(TrackButtons::ShowInstrument);
    }
}

void RosegardenGUIApp::slotToggleSegmentParameters()
{
    m_view->slotShowSegmentParameters(m_viewSegmentParameters->isChecked());
}

void RosegardenGUIApp::slotToggleInstrumentParameters()
{
    m_view->slotShowInstrumentParameters(m_viewInstrumentParameters->isChecked());
}

void RosegardenGUIApp::slotToggleRulers()
{
    m_view->slotShowRulers(m_viewRulers->isChecked());
}


void RosegardenGUIApp::slotToggleStatusBar()
{
    KTmpStatusMsg msg(i18n("Toggle the statusbar..."), statusBar());

    if(!m_viewStatusBar->isChecked())
        statusBar()->hide();
    else
        statusBar()->show();
}


void RosegardenGUIApp::slotStatusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->changeItem(text, ID_STATUS_MSG);
}


void RosegardenGUIApp::slotStatusHelpMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}

void RosegardenGUIApp::slotPointerSelected()
{
    m_view->selectTool(SegmentCanvas::Selector);
}

void RosegardenGUIApp::slotEraseSelected()
{
    m_view->selectTool(SegmentCanvas::Eraser);
}

void RosegardenGUIApp::slotDrawSelected()
{
    m_view->selectTool(SegmentCanvas::Pencil);
}

void RosegardenGUIApp::slotMoveSelected()
{
    m_view->selectTool(SegmentCanvas::Mover);
}

void RosegardenGUIApp::slotResizeSelected()
{
    m_view->selectTool(SegmentCanvas::Resizer);
}

void RosegardenGUIApp::slotJoinSelected()
{
    m_view->selectTool(SegmentCanvas::Joiner);
}

void RosegardenGUIApp::slotSplitSelected()
{
    m_view->selectTool(SegmentCanvas::Splitter);
}



#include <qlayout.h>
#include <qspinbox.h>
#include <kdialog.h>

class GetTimeResDialog : public KDialog
{
public:
    GetTimeResDialog(QWidget *parent = 0,
                     const char *name = 0,
                     bool modal = false, WFlags f = 0);

    void setInitialTimeRes(unsigned int);
    
    unsigned int getTimeRes() const;
    
protected:
    QSpinBox *m_spinbox;
};

GetTimeResDialog::GetTimeResDialog(QWidget *parent,
                                   const char *name,
                                   bool modal, WFlags f)
    : KDialog(parent, name, modal, f),
      m_spinbox(0)
{
    QVBoxLayout *box = new QVBoxLayout(this);
    box->setAutoAdd(true);
    
    new QLabel("Enter new time resolution", this);
    m_spinbox = new QSpinBox(this);
}


unsigned int GetTimeResDialog::getTimeRes() const
{
    return m_spinbox->value();
}

void GetTimeResDialog::setInitialTimeRes(unsigned int v)
{
    m_spinbox->setValue(v);
}



void RosegardenGUIApp::slotChangeTimeResolution()
{
    GetTimeResDialog *dialog = new GetTimeResDialog(this);
    
    dialog->setInitialTimeRes(0);
    dialog->show();

    if (dialog->result()) {
        
        unsigned int timeResolution = dialog->getTimeRes();
    }
    
}


void RosegardenGUIApp::slotImportMIDI()
{
    KURL url = KFileDialog::getOpenURL(QString::null, "*.mid", this,
                                     i18n("Open MIDI File"));
    if (url.isEmpty()) { return; }

    QString tmpfile;
    KIO::NetAccess::download(url, tmpfile);
    importMIDIFile(tmpfile);

    // See comment in RosegardenGUIApp::openURL()
    // for an explanation on why we have to do this
    actionCollection()->action("draw")->activate();
    actionCollection()->action("move")->activate();
  
    KIO::NetAccess::removeTempFile( tmpfile );
}

void RosegardenGUIApp::importMIDIFile(const QString &file)
{
    Rosegarden::MidiFile *midiFile;

    midiFile = new Rosegarden::MidiFile(file.data());

    if (!midiFile->open())
    {
        KMessageBox::error(this,
          i18n("Couldn't understand MIDI file.\nIt might be corrupted."));
        return;
    }

    // Stop if playing
    //
    if (m_seqManager->getTransportStatus() == PLAYING)
        slotStop();

    m_doc->closeDocument();

    m_doc->newDocument();

    Rosegarden::Composition *tmpComp = midiFile->convertToRosegarden();

    m_doc->getComposition().swap(*tmpComp);

    delete tmpComp;

    // Set modification flag
    //
    m_doc->setModified();

    initView();
}

void RosegardenGUIApp::slotImportRG21()
{
    KURL url = KFileDialog::getOpenURL(QString::null, "*.rose", this,
                                       i18n("Open Rosegarden 2.1 File"));
    if (url.isEmpty()) { return; }

    QString tmpfile;
    KIO::NetAccess::download(url, tmpfile);

    // Stop if playing
    //
    if (m_seqManager->getTransportStatus() == PLAYING)
        slotStop();
    
    importRG21File(tmpfile);

    KIO::NetAccess::removeTempFile(tmpfile);
}

void RosegardenGUIApp::importRG21File(const QString &file)
{
    RG21Loader rg21Loader(file);

    m_doc->closeDocument();
    m_doc->newDocument();
    Rosegarden::Composition *tmpComp = rg21Loader.getComposition();

    m_doc->getComposition().swap(*tmpComp);

    delete tmpComp;

    // Set modification flag
    //
    m_doc->setModified();

    initView();
}

void RosegardenGUIApp::setPointerPosition(const long &posSec,
                                          const long &posUsec)
{
    Rosegarden::RealTime rT(posSec, posUsec);
    Rosegarden::Composition &comp = m_doc->getComposition();
    timeT elapsedTime = comp.getElapsedTimeForRealTime(rT);

    // Indicate to slotSetPointerPosition that we shouldn't propagate
    // the jump back to the sequencer, because it originated from the
    // sequencer in the first place.  This is not exactly elegant
    // and does rather rely being single-threaded.
    m_originatingJump = true;
    m_doc->setPointerPosition(elapsedTime);
    m_originatingJump = false;

    return;
}

void
RosegardenGUIApp::slotSetPointerPosition(Rosegarden::RealTime time)
{
    setPointerPosition(time.sec, time.usec);
}


void RosegardenGUIApp::slotSetPointerPosition(timeT t)
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    if ( m_seqManager->getTransportStatus() == PLAYING ||
         m_seqManager->getTransportStatus() == RECORDING_MIDI ||
         m_seqManager->getTransportStatus() == RECORDING_AUDIO )
    {
        if (t > comp.getEndMarker())
        {
            slotStop();
            t = comp.getEndMarker();
	    m_doc->setPointerPosition(t); //causes this method to be re-invoked
	    return;
        }

        try
        {
            if (!m_originatingJump) {
		m_seqManager->sendSequencerJump(comp.getElapsedRealTime(t));
	    }
        }
        catch(QString s)
        {
            KMessageBox::error(this, s);
        }
    }

    if (t != comp.getStartMarker() && t > comp.getEndMarker()) {
        m_doc->setPointerPosition(comp.getStartMarker());
	return;
    }

    // and the tempo
    m_transport->setTempo(comp.getTempoAt(t));

    // and the time sig
    m_transport->setTimeSignature(comp.getTimeSignatureAt(t));

    // and the time
    //
    if (m_transport->isShowingBarTime()) {

	slotDisplayBarTime(t);

    } else {

	Rosegarden::RealTime rT(comp.getElapsedRealTime(t));

	if (m_transport->isShowingTimeToEnd()) {
	    rT = rT - comp.getElapsedRealTime(comp.getDuration());
	}

	m_transport->displayTime(rT);
    }
}

void RosegardenGUIApp::slotDisplayBarTime(timeT t)
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    int barNo = comp.getBarNumber(t);
    timeT barStart = comp.getBarStart(barNo);

    Rosegarden::TimeSignature timeSig = comp.getTimeSignatureAt(t);
    timeT beatDuration = timeSig.getBeatDuration();

    int beatNo = (t - barStart) / beatDuration;
    int unitNo = (t - barStart) - (beatNo * beatDuration);
    
    if (m_transport->isShowingTimeToEnd()) {
	barNo = barNo + 1 - comp.getNbBars();
	beatNo = timeSig.getBeatsPerBar() - 1 - beatNo;
	unitNo = timeSig.getBeatDuration() - 1 - unitNo;
    }
    
    m_transport->displayBarTime(barNo, beatNo, unitNo);
}


void RosegardenGUIApp::slotRefreshTimeDisplay()
{
    slotSetPointerPosition(m_doc->getComposition().getPosition());
}



void RosegardenGUIApp::slotEditAllTracks()
{
    m_view->editAllTracks(&m_doc->getComposition());
}


// Sequencer auxiliary process management


bool RosegardenGUIApp::launchSequencer()
{
    if (m_sequencerProcess) return true;

    // If we've already registered a sequencer then don't start another
    // one
    //
    if (kapp->dcopClient()->
        isApplicationRegistered(QCString(ROSEGARDEN_SEQUENCER_APP_NAME)))
      return true;

    KTmpStatusMsg msg(i18n("Starting the sequencer..."), statusBar());
    
    m_sequencerProcess = new KProcess;

    (*m_sequencerProcess) << "rosegardensequencer";

    connect(m_sequencerProcess, SIGNAL(processExited(KProcess*)),
            this, SLOT(slotSequencerExited(KProcess*)));

    bool res = m_sequencerProcess->start();
    
    if (!res) {
        KMessageBox::error(0, i18n("Couldn't start the sequencer"));
        kdDebug(KDEBUG_AREA) << "Couldn't start the sequencer\n";
        m_sequencerProcess = 0;
    }

    return res;
}

void RosegardenGUIApp::slotSequencerExited(KProcess*)
{
    kdDebug(KDEBUG_AREA) << "Sequencer exited\n";

    KMessageBox::error(0, i18n("Sequencer exited"));

    m_sequencerProcess = 0;
}


void RosegardenGUIApp::slotExportMIDI()
{
    KTmpStatusMsg msg(i18n("Exporting to MIDI file..."), statusBar());

    QString fileName=KFileDialog::getSaveFileName(QDir::currentDirPath(),
                                                  i18n("*.mid"), this, i18n("Export as..."));

    if (fileName.isEmpty())
      return;

    // Add a .mid extension if we don't already have one
    if (fileName.find(".mid", -4) == -1)
      fileName += ".mid";

    KURL *u = new KURL(fileName);

    if (u->isMalformed())
    {
        KMessageBox::sorry(this, i18n("This is not a valid filename.\n"));
        return;
    }

    if (!u->isLocalFile())
    {
        KMessageBox::sorry(this, i18n("This is not a local file.\n"));
        return;
    }

    QFileInfo info(fileName);

    if (info.isDir())
    {
        KMessageBox::sorry(this, i18n("You have specified a directory"));
        return;
    }

    if (info.exists())
    {
        int overwrite = KMessageBox::questionYesNo(this,
                               i18n("The specified file exists.  Overwrite?"));

        if (overwrite != KMessageBox::Yes)
         return;
    }

    // Go ahead and export the file
    //
    exportMIDIFile(fileName);

}

void RosegardenGUIApp::exportMIDIFile(const QString &file)
{
    Rosegarden::MidiFile midiFile(file.data());

    midiFile.convertToMidi(m_doc->getComposition());

    if (!midiFile.write())
    {
        KMessageBox::sorry(this, i18n("The MIDI File has not been exported."));
        return;
    }
}

// Uncheck the transport window check box
void
RosegardenGUIApp::slotCloseTransport()
{
    m_viewTransport->setChecked(false);
}



// We use this slot to active a tool mode on the GUI
// from a layer below the top level.  For example when
// we select a Track on the trackeditor and want this
// action to go on to select all the Segments on that
// Track we must change the edit mode to "Selector"
//
//
void
RosegardenGUIApp::slotActivateTool(SegmentCanvas::ToolType tt)
{
    switch(tt)
    {
         case SegmentCanvas::Selector:
             actionCollection()->action("select")->activate();
             break;

         default:
             break;
    }
}


void
RosegardenGUIApp::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Key_Shift:  //  select add for segments
            m_view->setShift(true);
            break;

        case Key_Control:   // select copy of segments
            m_view->setControl(true);
            break;

        default:
            event->ignore();
            break;
    }

}


void
RosegardenGUIApp::keyReleaseEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Key_Shift:  //  select add for segments
            m_view->setShift(false);
            break;

        case Key_Control:   // select copy of segments
            m_view->setControl(false);
            break;

        default:
            event->ignore();
            break;
    }

}


// Process the outgoing sounds into some form of visual
// feedback at the GUI
//
void
RosegardenGUIApp::showVisuals(Rosegarden::MappedComposition *mC)
{
    Rosegarden::MappedComposition::iterator it;

    for (it = mC->begin(); it != mC->end(); ++it )
    {
        if ((*it)->getType() == Rosegarden::MappedEvent::MidiNote)
        {
            m_transport->setMidiOutLabel(*it);
            m_view->showVisuals(*it);
        }
    }

}



// Sets the play or record Metronome status according to
// the current transport status
//
void
RosegardenGUIApp::slotToggleMetronome()
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    if (m_seqManager->getTransportStatus() == STARTING_TO_RECORD_MIDI ||
        m_seqManager->getTransportStatus() == STARTING_TO_RECORD_AUDIO ||
        m_seqManager->getTransportStatus() == RECORDING_MIDI ||
        m_seqManager->getTransportStatus() == RECORDING_AUDIO)
    {
        if (comp.useRecordMetronome())
            comp.setRecordMetronome(false);
        else
            comp.setRecordMetronome(true);

        m_transport->MetronomeButton->setOn(comp.useRecordMetronome());
    }
    else
    {
        if (comp.usePlayMetronome())
            comp.setPlayMetronome(false);
        else
            comp.setPlayMetronome(true);

        m_transport->MetronomeButton->setOn(comp.usePlayMetronome());
    }
}

// ------------------------ SequenceManager ----------------------------
//
//
//
//
//

// This method is called from the Sequencer when it's playing.
// It's a request to get the next slice of events for the
// Sequencer to play.
//
const Rosegarden::MappedComposition&
RosegardenGUIApp::getSequencerSlice(long sliceStartSec, long sliceStartUsec,
                                    long sliceEndSec, long sliceEndUsec)
{

    Rosegarden::RealTime startTime(sliceStartSec, sliceStartUsec);
    Rosegarden::RealTime endTime(sliceEndSec, sliceEndUsec);

    Rosegarden::MappedComposition *mC =
        m_seqManager->getSequencerSlice(startTime, endTime);

    showVisuals(mC);

    return *mC;
}


void
RosegardenGUIApp::slotRewindToBeginning()
{
    m_seqManager->rewindToBeginning();
}


void
RosegardenGUIApp::slotFastForwardToEnd()
{
    m_seqManager->fastForwardToEnd();
}

// Called from the LoopRuler (usually a double click)
// to set position and start playing
//
void
RosegardenGUIApp::slotSetPlayPosition(Rosegarden::timeT time)
{
    m_seqManager->setPlayStartTime(time);
}


// Process unexpected MIDI events at the GUI - send them to the Transport
// or to a MIDI mixer for display purposes only.  Useful feature to enable
// the musician to prove to herself quickly that the MIDI input is still
// working.
//
void
RosegardenGUIApp::processAsynchronousMidi(const Rosegarden::MappedComposition &mC)
{
    m_seqManager->processAsynchronousMidi(mC);
}


void
RosegardenGUIApp::processRecordedMidi(const Rosegarden::MappedComposition &mC)
{
    m_seqManager->processRecordedMidi(mC);
}

// This method is a callback from the Sequencer to update the GUI
// with state change information.  The GUI requests the Sequencer
// to start playing or to start recording and enters a pending
// state (see rosegardendcop.h for TransportStatus values).
// The Sequencer replies when ready with it's status.  If anything
// fails then we default (or try to default) to STOPPED at both
// the GUI and the Sequencer.
//
void RosegardenGUIApp::notifySequencerStatus(const int& status)
{
    m_seqManager->setTransportStatus((TransportStatus) status);
}


// Called when we want to start recording from the GUI.
// This method tells the sequencer to start recording and
// from then on the sequencer returns MappedCompositions
// to the GUI via the "processRecordedMidi() method -
// also called via DCOP
//
void
RosegardenGUIApp::slotRecord()
{
    if (!m_sequencerProcess && !launchSequencer())
                return;

    try
    {
        m_seqManager->record();
    }
    catch(QString s)
    {
        KMessageBox::error(this, s);
    }
}

void
RosegardenGUIApp::slotSetLoop(Rosegarden::timeT lhs, Rosegarden::timeT rhs)
{
    try
    {
        m_seqManager->setLoop(lhs, rhs);

        // toggle the loop button
        if (lhs != rhs)
            m_transport->LoopButton->setOn(true);
        else
            m_transport->LoopButton->setOn(false);

    }
    catch(QString s)
    {
        KMessageBox::error(this, s);
    }
}

void RosegardenGUIApp::slotPlay()
{
    if (!m_sequencerProcess && !launchSequencer())
                return;

    try
    {
        m_seqManager->play();
    }
    catch(QString s)
    {
        KMessageBox::error(this, s);
    }

}

// Send stop request to Sequencer if playing, else
// return to start of segment
void RosegardenGUIApp::slotStop()
{
    try
    {
        m_seqManager->stop();
    }
    catch(QString s)
    {
        KMessageBox::error(this, s);
    }
}

// Jump to previous bar
//
void RosegardenGUIApp::slotRewind()
{
    m_seqManager->rewind();
}


// Jump to next bar
//
void RosegardenGUIApp::slotFastforward()
{
    m_seqManager->fastforward();
}

// Insert metronome clicks into the global MappedComposition that
// will be returned as part of the slice fetch from the Sequencer.
//
void
RosegardenGUIApp::insertMetronomeClicks(timeT sliceStart, timeT sliceEnd)
{
    m_seqManager->insertMetronomeClicks(sliceStart, sliceEnd);
}

// Set the loop to what we have stored (if the stored loop is (0,0) then
// nothing happens).
//
void
RosegardenGUIApp::slotSetLoop()
{
    // restore loop
    m_doc->setLoop(m_storedLoopStart, m_storedLoopEnd);
}

// Store the current loop locally and unset the loop on the Sequencer
//
void
RosegardenGUIApp::slotUnsetLoop()
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    // store the loop
    m_storedLoopStart = comp.getLoopStart();
    m_storedLoopEnd = comp.getLoopEnd();

    // clear the loop at the composition and propagate to the rest
    // of the display items
    m_doc->setLoop(0, 0);
}

void
RosegardenGUIApp::slotSendMappedEvent(Rosegarden::MappedEvent *mE)
{
    m_seqManager->sendMappedEvent(mE);
}


void
RosegardenGUIApp::performAutoload()
{
}




