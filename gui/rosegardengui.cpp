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
#include <qpushbutton.h>
#include <qregexp.h>
#include <qlabel.h>

#include <qmetaobject.h> // remove this

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
#include <ktoolbar.h>
#include <dcopclient.h>
#include <qiconset.h>
#include <kstddirs.h>
#include <kstatusbar.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>
#include <kaction.h>
#include <kstdaction.h>

// application specific includes
#include "kstartuplogo.h"
#include "rosestrings.h"
#include "rosegardengui.h"
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "rosegardenconfiguredialog.h"
#include "rosegardentransportdialog.h"
#include "MidiFile.h"
#include "rg21io.h"
#include "csoundio.h"
#include "lilypondio.h"
#include "rosegardendcop.h"
#include "ktmpstatusmsg.h"
#include "SegmentPerformanceHelper.h"
#include "NotationTypes.h"
#include "Selection.h"
#include "sequencemanager.h"
#include "trackbuttons.h"
#include "trackeditor.h"
#include "dialogs.h"
#include "editcommands.h"
#include "multiviewcommandhistory.h"
#include "segmentcommands.h"
#include "zoomslider.h"
#include "audiomanagerdialog.h"
#include "widgets.h"

//!!! ditch these when harmonize() moves out
#include "CompositionTimeSliceAdapter.h"
#include "AnalysisTypes.h"

#define ID_STATUS_MSG 1

using Rosegarden::timeT;


RosegardenGUIApp::RosegardenGUIApp(bool useSequencer)
    : KMainWindow(0), RosegardenIface(this), DCOPObject("RosegardenIface"),
      m_config(kapp->config()),
      m_actionsSetup(false),
      m_fileRecent(0),
      m_view(0),
      m_doc(0),
      m_sequencerProcess(0),
      m_zoomSlider(0),
      m_seqManager(0),
      m_transport(0),
      m_audioManagerDialog(0),
      m_originatingJump(false),
      m_storedLoopStart(0),
      m_storedLoopEnd(0),
      m_useSequencer(useSequencer)
{
    // accept dnd
    setAcceptDrops(true);

    // Try to start the sequencer
    //
    if (m_useSequencer) launchSequencer();
    else kdDebug(KDEBUG_AREA) << "RosegardenGUIApp : don't use sequencer\n";

    ///////////////////////////////////////////////////////////////////
    // call inits to invoke all other construction parts
    //
    
    initStatusBar();
    initDocument();
    setupActions();
    iFaceDelayedInit(this);
    initZoomToolbar();

    if (!performAutoload())
        initView();

    // Create a sequence manager
    m_seqManager = new Rosegarden::SequenceManager(m_doc, m_transport);

    connect(m_doc, SIGNAL(pointerPositionChanged(Rosegarden::timeT)),
            this,   SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    connect(m_doc, SIGNAL(documentModified()),
            this,   SLOT(slotDocumentModified()));

    connect(m_doc, SIGNAL(loopChanged(Rosegarden::timeT, Rosegarden::timeT)),
            this,  SLOT(slotSetLoop(Rosegarden::timeT, Rosegarden::timeT)));

    readOptions();

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
#ifdef RGKDE3
    stateChanged("new_file");
    stateChanged("have_segments",    KXMLGUIClient::StateReverse);
    stateChanged("segment_selected", KXMLGUIClient::StateReverse);
    slotTestClipboard();
#endif
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

    new KAction(i18n("Export &MIDI file..."), 0, 0, this,
                SLOT(slotExportMIDI()), actionCollection(),
                "file_export_midi");

    new KAction(i18n("Export &Lilypond file..."), 0, 0, this,
                SLOT(slotExportLilypond()), actionCollection(),
                "file_export_lilypond");

    new KAction(i18n("Export &Csound score file..."), 0, 0, this,
                SLOT(slotExportCsound()), actionCollection(),
                "file_export_csound");

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

    m_viewPreviews = new KToggleAction(i18n("Show Segment Pre&views"), 0, this,
				       SLOT(slotTogglePreviews()),
				       actionCollection(),
				       "show_previews");

    // Standard Actions 
    //
    KStdAction::saveOptions(this,
                            SLOT(slotSaveOptions()),
                            actionCollection());

    KStdAction::preferences(this,
                            SLOT(slotConfigure()),
                            actionCollection());

    KStdAction::keyBindings(this,
                            SLOT(slotEditKeys()),
                            actionCollection());

    KStdAction::configureToolbars(this,
                                  SLOT(slotEditToolbars()),
                                  actionCollection());

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

    new KAction(i18n("&Audio File Manager"),
                0, 
                this, SLOT(slotAudioManager()),
                actionCollection(), "audio_manager");

    new KAction(i18n("&Add Tracks..."), 
                0,
                this, SLOT(slotAddTracks()),
                actionCollection(), "add_tracks");

    new KAction(i18n("Select &Next Track"),
                Key_Down, 
                this, SLOT(slotTrackDown()),
                actionCollection(), "select_next_track");

    new KAction(i18n("Select &Previous Track"),
                Key_Up, 
                this, SLOT(slotTrackUp()),
                actionCollection(), "select_previous_track");

    new KAction(i18n("Select &All Segments"), 0, this,
		SLOT(slotSelectAll()), actionCollection(),
		"select_all");

    new KAction(i18n("De&lete"), Key_Delete, this,
		SLOT(slotDeleteSelectedSegments()), actionCollection(),
		"delete");

    new KAction(i18n("&Quantize..."), 0, this,
		SLOT(slotQuantizeSelection()), actionCollection(),
		"quantize_selection");

    new KAction(i18n("&Harmonize"), 0, this,
		SLOT(slotHarmonizeSelection()), actionCollection(),
		"harmonize_selection");

    new KAction(i18n(SegmentRescaleCommand::getGlobalName()), 0, this,
		SLOT(slotRescaleSelection()), actionCollection(),
		"rescale");

    new KAction(i18n(SegmentAutoSplitCommand::getGlobalName()), 0, this,
		SLOT(slotAutoSplitSelection()), actionCollection(),
		"auto_split");

    new KAction(i18n("Open in Matri&x Editor"), 0, this,
		SLOT(slotEditInMatrix()), actionCollection(),
		"edit_matrix");

    new KAction(i18n("Open in &Notation Editor"), 0, this,
		SLOT(slotEditAsNotation()), actionCollection(),
		"edit_notation");

    new KAction(i18n(AddTempoChangeCommand::getGlobalName()),
                0,
                this, SLOT(slotEditTempo()),
                actionCollection(), "add_tempo");

    new KAction(i18n(AddTimeSignatureCommand::getGlobalName()),
		0,
		this, SLOT(slotEditTimeSignature()),
		actionCollection(), "add_time_signature");

    new KAction(i18n("Edit Document P&roperties..."), 0, this,
                SLOT(slotEditDocumentProperties()),
                actionCollection(), "edit_doc_properties");

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

    m_recordTransport = new KAction(i18n("&Record"), 0, Key_Space, this,
                                    SLOT(slotRecord()), actionCollection(),
                                    "record");

    m_recordTransport->setGroup("transportcontrols");

    m_rewindEndTransport = new KAction(i18n("Rewind to &Beginning"), 0, 0, this,
                                       SLOT(slotRewindToBeginning()), actionCollection(),
                                       "rewindtobeginning");

    m_rewindEndTransport->setGroup("transportcontrols");

    m_ffwdEndTransport = new KAction(i18n("Fast Forward to &End"), 0, 0, this,
                                     SLOT(slotFastForwardToEnd()), actionCollection(),
                                     "fastforwardtoend");

    m_ffwdEndTransport->setGroup("transportcontrols");

    // create the Transport GUI and add the callbacks to the
    // buttons and keyboard accelerators
    //
    m_transport = new Rosegarden::RosegardenTransportDialog(this, "");
    plugAccelerators(m_transport, m_transport->getAccelerators());

    // create main gui
    //
    createGUI("rosegardenui.rc");

    // Ensure that the checkbox is unchecked if the dialog
    // is closed
    connect(m_transport, SIGNAL(closed()),
            SLOT(slotCloseTransport()));

    // Handle loop setting and unsetting from the transport loop button
    //
    
    connect(m_transport, SIGNAL(setLoop()), SLOT(slotSetLoop()));
    connect(m_transport, SIGNAL(unsetLoop()), SLOT(slotUnsetLoop()));

    connect(m_transport, SIGNAL(editTempo(QWidget*)),
            SLOT(slotEditTempo(QWidget*)));

    connect(m_transport, SIGNAL(editTimeSignature(QWidget*)),
            SLOT(slotEditTimeSignature(QWidget*)));
    
    m_actionsSetup = true;
}


void RosegardenGUIApp::initZoomToolbar()
{
    KToolBar *zoomToolbar = toolBar("zoomToolBar");
    if (!zoomToolbar) {
	kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::initZoomToolbar() : "
			     << "zoom toolbar not found" << endl;
	return;
    }

    zoomToolbar->setBarPos(KToolBar::Right);

    new QLabel(i18n("  Zoom:  "), zoomToolbar);

    std::vector<double> zoomSizes; // in units-per-pixel
    double defaultBarWidth44 = 100.0;
    double duration44 = Rosegarden::TimeSignature(4,4).getBarDuration();
    static double factors[] = { 0.025, 0.05, 0.1, 0.2, 0.5, 1.0, 1.5, 2.5, 5.0 };
    for (unsigned int i = 0; i < sizeof(factors)/sizeof(factors[0]); ++i) {
	zoomSizes.push_back(duration44 / (defaultBarWidth44 * factors[i]));
    }

    m_zoomSlider = new ZoomSlider<double>
	(zoomSizes, -1, QSlider::Horizontal, zoomToolbar);
    m_zoomSlider->setTracking(true);
    m_zoomSlider->setFocusPolicy(QWidget::NoFocus);

    connect(m_zoomSlider, SIGNAL(valueChanged(int)),
	    this, SLOT(slotChangeZoom(int)));
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
    m_doc = new RosegardenGUIDoc(this, m_useSequencer);
    m_doc->newDocument();
    m_doc->getCommandHistory()->attachView(actionCollection());
    connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
	    SLOT(update()));
    connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
	    SLOT(slotTestClipboard()));
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
        int endMarker = comp.getBarRange(100 + comp.getNbBars()).second;
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


    // We have to do this to make sure that the 2nd call ("select")
    // actually has any effect. Activating the same radio action
    // doesn't work the 2nd time (like pressing down the same radio
    // button twice - it doesn't have any effect), so if you load two
    // files in a row, on the 2nd file a new SegmentCanvas will be
    // created but its tool won't be set, even though it will appear
    // to be selected.
    //
    actionCollection()->action("move")->activate();
    actionCollection()->action("select")->activate();

    //
    // Transport setup
    //

    // set the tempo in the transport
    //
    m_transport->setTempo(comp.getTempo());

    // and the time signature
    //
    m_transport->setTimeSignature(comp.getTimeSignatureAt(comp.getPosition()));

    // bring the transport to the front 
    //
    m_transport->raise();

    // set the play metronome button
    m_transport->MetronomeButton->setOn(comp.usePlayMetronome());

    // Set the solo button
    m_transport->SoloButton->setOn(comp.isSolo());

    // set the highlighted track
    m_view->selectTrack(comp.getSelectedTrack());

    // set zoom slider to default value
    m_zoomSlider->setToDefault();

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
        slotTogglePreviews();

        // Reset any loop on the sequencer
        //
        try
        {
            m_seqManager->setLoop(0, 0);
        }
        catch(QString s)
        {
            KMessageBox::error(this, s);
        }

    }

#ifdef RGKDE3
    connect(m_view, SIGNAL(stateChange(const QString&, bool)),
            this,   SLOT  (slotStateChanged(const QString&, bool)));
#endif

    // make sure we show
    //
    m_view->show();
    slotChangeZoom(-1);

#ifdef RGKDE3
    stateChanged("new_file");
#endif

    // Refresh the audioManagerDialog if it's hanging around
    //
    if (m_audioManagerDialog)
        m_audioManagerDialog->slotPopulateFileList();
}

bool RosegardenGUIApp::openDocumentFile(const char* _cmdl)
{
    KTmpStatusMsg msg(i18n("Opening file..."), statusBar());
    
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::openDocumentFile("
                         << _cmdl << ")" << endl;

    m_doc->saveIfModified();
    m_doc->closeDocument();

    if (m_doc->openDocument(_cmdl)) {

        initView();
        return true;
    }

    return false;
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

    m_doc->setModified(false);
}


RosegardenGUIDoc *RosegardenGUIApp::getDocument() const
{
    return m_doc;
}

void RosegardenGUIApp::slotSaveOptions()
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
    m_config->writeEntry("Show Previews",                m_viewPreviews->isChecked());


    m_config->writeEntry("ToolBarPos", (int) toolBar("mainToolBar")->barPos());
    m_config->writeEntry("TracksToolBarPos", (int) toolBar("tracksToolBar")->barPos());
    m_config->writeEntry("ZoomToolBarPos", (int) toolBar("zoomToolBar")->barPos());

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

    opt = m_config->readBoolEntry("Show Previews", false);
    m_viewPreviews->setChecked(opt);
    slotTogglePreviews();

    // bar position settings
    KToolBar::BarPosition toolBarPos;
    toolBarPos=(KToolBar::BarPosition) m_config->readNumEntry("ToolBarPos", KToolBar::Top);
    toolBar("mainToolBar")->setBarPos(toolBarPos);

    toolBarPos=(KToolBar::BarPosition) m_config->readNumEntry("TracksToolBarPos", KToolBar::Top);
    toolBar("tracksToolBar")->setBarPos(toolBarPos);
	
    toolBarPos=(KToolBar::BarPosition) m_config->readNumEntry("ZoomToolBarPos", KToolBar::Top);
    toolBar("zoomToolBar")->setBarPos(toolBarPos);
	
    // initialise the recent file list
    //
    m_fileRecent->loadEntries(m_config);

    QSize size(m_config->readSizeEntry("Geometry"));

    if(!size.isEmpty()) {
        resize(size);
    }
}

void RosegardenGUIApp::saveProperties(KConfig *cfg)
{
    if (m_doc->getTitle()!=i18n("Untitled") && !m_doc->isModified()) {
        // saving to tempfile not necessary
    } else {
        QString filename=m_doc->getAbsFilePath();	
        cfg->writeEntry("filename", filename);
        cfg->writeEntry("modified", m_doc->isModified());
		
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

void RosegardenGUIApp::paintEvent(QPaintEvent* e)
{
    slotRefreshTimeDisplay();
    KMainWindow::paintEvent(e);
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
    if (m_actionsSetup) slotSaveOptions();
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
    }
}

void RosegardenGUIApp::openURL(const KURL& url)
{
    SetWaitCursor waitCursor;

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
    else {
	SetWaitCursor waitCursor;
        m_doc->saveDocument(m_doc->getAbsFilePath());
    }
}

void RosegardenGUIApp::slotFileSaveAs()
{
    if (!m_doc) return;

    KTmpStatusMsg msg(i18n("Saving file with a new filename..."), statusBar());

    QString newName=KFileDialog::getSaveFileName(QDir::currentDirPath(),
                                                 i18n("*.rg"), this, i18n("Save as..."));
    if (!newName.isEmpty()) {

        // Append .rg extension if we don't have one
        //
        QRegExp rgFile("\\.rg$");
        if (rgFile.match(newName) == -1) {
            newName += ".rg";
        }

	SetWaitCursor waitCursor;
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
    slotSaveOptions();
    // close the first window, the list makes the next one the first again.
    // This ensures thatslotQueryClose() is called on each window to ask for closing
    KMainWindow* w;
    if (memberList) {

        for(w=memberList->first(); w!=0; w=memberList->next()) {
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
    if (!m_view->haveSelection()) return;
    KTmpStatusMsg msg(i18n("Cutting selection..."), statusBar());

    Rosegarden::SegmentSelection selection(m_view->getSelection());
    m_doc->getCommandHistory()->addCommand
	(new CutCommand(selection, m_doc->getClipboard()));
}

void RosegardenGUIApp::slotEditCopy()
{
    if (!m_view->haveSelection()) return;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), statusBar());

    Rosegarden::SegmentSelection selection(m_view->getSelection());
    m_doc->getCommandHistory()->addCommand
	(new CopyCommand(selection, m_doc->getClipboard()));
}

void RosegardenGUIApp::slotEditPaste()
{
    Rosegarden::Clipboard *clipboard = m_doc->getClipboard();
    if (clipboard->isEmpty()) {
	KTmpStatusMsg msg(i18n("Clipboard is empty"), statusBar());
	return;
    }
    KTmpStatusMsg msg(i18n("Inserting clipboard contents..."), statusBar());

    // for now, but we could paste at the time of the first copied
    // segment and then do ghosting drag or something
    timeT insertionTime = m_doc->getComposition().getPosition();
    m_doc->getCommandHistory()->addCommand
	(new PasteSegmentsCommand(&m_doc->getComposition(),
				  clipboard, insertionTime));
}

void RosegardenGUIApp::slotSelectAll()
{
    m_view->slotSelectAllSegments();
}

void RosegardenGUIApp::slotDeleteSelectedSegments()
{
    m_view->getTrackEditor()->slotDeleteSelectedSegments();
}

void RosegardenGUIApp::slotQuantizeSelection()
{
    if (!m_view->haveSelection()) return;

    //!!! this should all be in rosegardenguiview

    QuantizeDialog *dialog = new QuantizeDialog
	(m_view,
	 Rosegarden::Quantizer::RawEventData,
	 Rosegarden::Quantizer::RawEventData);
    if (dialog->exec() != QDialog::Accepted) return;

    Rosegarden::SegmentSelection selection = m_view->getSelection();
    
    KMacroCommand *command = new KMacroCommand
	(EventQuantizeCommand::getGlobalName());

    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
	 i != selection.end(); ++i) {
	command->addCommand(new EventQuantizeCommand
			    (**i, (*i)->getStartTime(), (*i)->getEndTime(),
			     dialog->getQuantizer()));
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenGUIApp::slotRescaleSelection()
{
    if (!m_view->haveSelection()) return;

    //!!! this should all be in rosegardenguiview
    //!!! should it?

    RescaleDialog *dialog = new RescaleDialog(m_view);
    if (dialog->exec() != QDialog::Accepted) return;

    Rosegarden::SegmentSelection selection = m_view->getSelection();

    KMacroCommand *command = new KMacroCommand
	(SegmentRescaleCommand::getGlobalName());

    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
	 i != selection.end(); ++i) {
	command->addCommand(new SegmentRescaleCommand(*i,
						      dialog->getMultiplier(),
						      dialog->getDivisor()));
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenGUIApp::slotAutoSplitSelection()
{
    if (!m_view->haveSelection()) return;

    //!!! this should all be in rosegardenguiview
    //!!! or should it?

    Rosegarden::SegmentSelection selection = m_view->getSelection();
    
    KMacroCommand *command = new KMacroCommand
	(SegmentAutoSplitCommand::getGlobalName());

    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
	 i != selection.end(); ++i) {
	command->addCommand(new SegmentAutoSplitCommand(*i));
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenGUIApp::slotHarmonizeSelection()
{
    if (!m_view->haveSelection()) return;

    Rosegarden::SegmentSelection selection = m_view->getSelection();
    //!!! This should be somewhere else too

    Rosegarden::CompositionTimeSliceAdapter adapter(&m_doc->getComposition(),
						    &selection);

    Rosegarden::AnalysisHelper helper;
    Rosegarden::Segment *segment = new Segment;
    helper.guessHarmonies(adapter, *segment);

    //!!! do nothing with the results yet
    delete segment;
}



void RosegardenGUIApp::slotEditAsNotation()
{
    m_view->slotEditSegmentNotation(0);
}

void RosegardenGUIApp::slotEditInMatrix()
{
    Rosegarden::SegmentSelection selection = m_view->getSelection();
    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
	 i != selection.end(); ++i) {
	m_view->slotEditSegmentMatrix(*i);
    }
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

void RosegardenGUIApp::slotTogglePreviews()
{
    m_view->slotShowPreviews(m_viewPreviews->isChecked());
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
#include <kdialogbase.h>

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


void RosegardenGUIApp::slotAddTracks()
{
    if (m_view)
        m_view->slotAddTracks(5);
}


void RosegardenGUIApp::slotImportMIDI()
{
    KURL url = KFileDialog::getOpenURL(QString::null, "*.mid", this,
                                     i18n("Open MIDI File"));
    if (url.isEmpty()) { return; }

    QString tmpfile;
    KIO::NetAccess::download(url, tmpfile);
    importMIDIFile(tmpfile);

    KIO::NetAccess::removeTempFile( tmpfile );
}

void RosegardenGUIApp::importMIDIFile(const QString &file)
{
    Rosegarden::MidiFile *midiFile;

    RosegardenProgressDialog *progressDlg =
            new RosegardenProgressDialog(i18n("Importing MIDI file..."),
                                         i18n("Cancel"),
                                         100,
                                         this);

    midiFile = new Rosegarden::MidiFile(qstrtostr(file),
                                       &m_doc->getStudio(),
                                        progressDlg);

    if (!midiFile->open())
    {
        KMessageBox::error(this,
          i18n("Couldn't understand MIDI file.\nIt might be corrupted."));
        delete progressDlg;
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

    // Set the caption
    //
    m_doc->setTitle(file);

    // Reinitialise
    //
    initView();

    // drop progress dialog
    //
    delete progressDlg;

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
    SetWaitCursor waitCursor;

    RG21Loader rg21Loader(file, &m_doc->getStudio());

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

void RosegardenGUIApp::setPointerPosition(long posSec,
                                          long posUsec,
                                          bool clearToSend)
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

    // stop if we've reached the end marker
    //
    if (elapsedTime >= comp.getEndMarker())
        slotStop();

    // Check for a pending stop if we're clear to send
    //
    if (clearToSend) checkForStop();

    return;
}

void
RosegardenGUIApp::slotSetPointerPosition(Rosegarden::RealTime time)
{
    setPointerPosition(time.sec, time.usec, false);
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
    if (m_transport->getCurrentMode() ==
	Rosegarden::RosegardenTransportDialog::BarMode) {

	slotDisplayBarTime(t);

    } else {
	Rosegarden::RealTime rT(comp.getElapsedRealTime(t));

	if (m_transport->isShowingTimeToEnd()) {
	    rT = rT - comp.getElapsedRealTime(comp.getDuration());
	}

	if (m_transport->getCurrentMode() ==
	    Rosegarden::RosegardenTransportDialog::RealMode) {

	    m_transport->displayRealTime(rT);

	} else {
	    m_transport->displaySMPTETime(rT);
	}
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
    } else {
	// convert to 1-based display bar numbers
	barNo += 1;
	beatNo += 1;
    }
    
    m_transport->displayBarTime(barNo, beatNo, unitNo);
}


void RosegardenGUIApp::slotRefreshTimeDisplay()
{
    slotSetPointerPosition(m_doc->getComposition().getPosition());
}



// Sequencer auxiliary process management


bool RosegardenGUIApp::launchSequencer()
{
    if (m_useSequencer == false)
        return false;

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
	m_useSequencer = false; // otherwise we hang waiting for it
    }

    return res;
}

void RosegardenGUIApp::slotSequencerExited(KProcess*)
{
    kdDebug(KDEBUG_AREA) << "Sequencer exited\n";

    KStartupLogo* logo = KStartupLogo::getInstance();
    if (logo) logo->close();

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
    RosegardenProgressDialog *progressDlg =
            new RosegardenProgressDialog(i18n("Importing MIDI file..."),
                                         i18n("Cancel"),
                                         100,
                                         this);

    Rosegarden::MidiFile midiFile(qstrtostr(file),
                                 &m_doc->getStudio(),
                                  progressDlg);

    midiFile.convertToMidi(m_doc->getComposition());

    if (!midiFile.write())
    {
        KMessageBox::sorry(this, i18n("The MIDI File has not been exported."));
        delete progressDlg;
        return;
    }

    delete progressDlg;
}

void RosegardenGUIApp::slotExportCsound()
{
    KTmpStatusMsg msg(i18n("Exporting to Csound scorefile..."), statusBar());

    QString fileName=KFileDialog::getSaveFileName(QDir::currentDirPath(),
                                                  i18n("*"), this, i18n("Export as..."));

    if (fileName.isEmpty())
      return;

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
    exportCsoundFile(fileName);
}

void RosegardenGUIApp::exportCsoundFile(const QString &file)
{
    SetWaitCursor waitCursor;

    CsoundExporter e(&m_doc->getComposition(), qstrtostr(file));
    if (!e.write()) {
	KMessageBox::sorry(this, i18n("The Csound file has not been exported."));
    }
}

void RosegardenGUIApp::slotExportLilypond()
{
    KTmpStatusMsg msg(i18n("Exporting to Lilypond file..."), statusBar());

    QString fileName=KFileDialog::getSaveFileName(QDir::currentDirPath(),
                                                  i18n("*.ly"), this, i18n("Export as..."));

    if (fileName.isEmpty())
      return;

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
    exportLilypondFile(fileName);
}

void RosegardenGUIApp::exportLilypondFile(const QString &file)
{
    SetWaitCursor waitCursor;

    LilypondExporter e(&m_doc->getComposition(), qstrtostr(file));
    if (!e.write()) {
	KMessageBox::sorry(this, i18n("The Lilypond file has not been exported."));
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
        if ((*it)->getType() == Rosegarden::MappedEvent::MidiNote ||
            (*it)->getType() == Rosegarden::MappedEvent::MidiNoteOneShot)
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
                                    long sliceEndSec, long sliceEndUsec,
                                    unsigned char firstFetch)
{
    // have to convert from char
    bool firstFetchBool = (bool)firstFetch;

    Rosegarden::RealTime startTime(sliceStartSec, sliceStartUsec);
    Rosegarden::RealTime endTime(sliceEndSec, sliceEndUsec);

    Rosegarden::MappedComposition *mC =
        m_seqManager->getSequencerSlice(startTime, endTime, firstFetchBool);

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

void
RosegardenGUIApp:: processRecordedAudio(long recordTimeSec,
                                        long recordTimeUsec,
                                        float audioLevel)
{
    m_seqManager->processRecordedAudio(Rosegarden::RealTime(recordTimeSec,
                                                            recordTimeUsec),
                                       audioLevel);
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
        m_doc->setModified();

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

// Send stop request to Sequencer.  This'll set the flag 
// to attempt a stop next time a good window appears.
//
void RosegardenGUIApp::slotStop()
{
    m_seqManager->stopping();
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


// Find and load the autoload file to set up a default Studio
//
//
bool RosegardenGUIApp::performAutoload()
{
    QString autoloadFile =
        KGlobal::dirs()->findResource("appdata", "autoload.rg");

    QFileInfo autoloadFileInfo(autoloadFile);

    if (!autoloadFileInfo.isReadable())
    {
        kdDebug(KDEBUG_AREA)
            << "RosegardenGUIApp::performAutoload() - "
            << "Can't find autoload file - no default Studio loaded\n";

        return false;
    }

    // Else we try to open it
    //
    kdDebug(KDEBUG_AREA)
        << "RosegardenGUIApp::performAutoload() - autoloading\n";

    bool res = openDocumentFile(autoloadFile.utf8().data());

    // So we don't get the "autoload" title
    //
    m_doc->setTitle(i18n("Untitled"));
    setCaption(m_doc->getTitle());

    return res;
}



// Just set the solo value in the Composition equal to the state
// of the button
//
void RosegardenGUIApp::slotToggleSolo()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::slotToggleSolo\n";

    m_doc->getComposition().setSolo(m_transport->SoloButton->isOn());
    m_doc->setModified();
}

void RosegardenGUIApp::slotTrackUp()
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    Rosegarden::TrackId tid = comp.getSelectedTrack();
    Rosegarden::TrackId pos = comp.getTrackByIndex(tid)->getPosition();

    // If at top already
    if (pos == 0)
        return;

    Rosegarden::Track *track = comp.getTrackByPosition(pos - 1);

    // If the track exists
    if (track)
    {
       comp.setSelectedTrack(pos - 1);
       m_view->selectTrack(comp.getSelectedTrack());
    }

}

void RosegardenGUIApp::slotTrackDown()
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    Rosegarden::TrackId tid = comp.getSelectedTrack();
    Rosegarden::TrackId pos = comp.getTrackByIndex(tid)->getPosition();

    Rosegarden::Track *track = comp.getTrackByPosition(pos + 1);

    // If the track exists
    if (track)
    {
       comp.setSelectedTrack(pos + 1);
       m_view->selectTrack(comp.getSelectedTrack());
    }

}

void RosegardenGUIApp::slotConfigure()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::slotConfigure\n";

    Rosegarden::ConfigureDialog *configDlg = 
        new Rosegarden::ConfigureDialog(m_config, this);

    configDlg->show();
}

void RosegardenGUIApp::slotEditDocumentProperties()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::slotEditDocumentProperties\n";

    Rosegarden::DocumentConfigureDialog *configDlg = 
        new Rosegarden::DocumentConfigureDialog(m_doc, this);

    configDlg->show();
}

void RosegardenGUIApp::slotEditKeys()
{
    KKeyDialog::configureKeys(actionCollection(), xmlFile(), true, this);
}


void RosegardenGUIApp::slotEditToolbars()
{
    KEditToolbar dlg(actionCollection(), "rosegardenui.rc");

    connect( &dlg, SIGNAL(newToolbarConfig()),
             SLOT(slotUpdateToolbars()) );

    dlg.exec();
}

void RosegardenGUIApp::slotUpdateToolbars()
{
  createGUI("rosegardenui.rc");
  m_viewToolBar->setChecked(!toolBar()->isHidden());
}

void RosegardenGUIApp::slotEditTempo()
{
    slotEditTempo(this);
}

void RosegardenGUIApp::slotEditTimeSignature()
{
    slotEditTimeSignature(this);
}

void RosegardenGUIApp::slotEditTempo(QWidget *parent)
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::slotEditTempo\n";

    TempoDialog *tempoDlg = new TempoDialog(parent, m_doc);

    connect(tempoDlg,
            SIGNAL(changeTempo(Rosegarden::timeT,
                               double, TempoDialog::TempoDialogAction)),
            SLOT(slotChangeTempo(Rosegarden::timeT,
                               double, TempoDialog::TempoDialogAction)));

    tempoDlg->setTempoPosition(m_doc->getComposition().getPosition());
    tempoDlg->show();


}

void RosegardenGUIApp::slotEditTimeSignature(QWidget *parent)
{
    Rosegarden::Composition &composition(m_doc->getComposition());

    Rosegarden::timeT time = composition.getPosition();
    int barNo = composition.getBarNumber(time);
    bool atStartOfBar = (time == composition.getBarStart(barNo));
    Rosegarden::TimeSignature sig = composition.getTimeSignatureAt(time);

    TimeSignatureDialog *dialog = new TimeSignatureDialog
	(parent, sig, barNo, atStartOfBar);

    if (dialog->exec() == QDialog::Accepted) {

	TimeSignatureDialog::Location location = dialog->getLocation();
	if (location == TimeSignatureDialog::StartOfBar) {
	    time = composition.getBarStartForTime(time);
	}

	if (dialog->shouldNormalizeRests()) {
	    m_doc->getCommandHistory()->addCommand
		(new AddTimeSignatureAndNormalizeCommand
		 (&composition, time, dialog->getTimeSignature()));
	} else { 
	    m_doc->getCommandHistory()->addCommand
		(new AddTimeSignatureCommand
		 (&composition, time, dialog->getTimeSignature()));
	}
    }

    delete dialog;
}

void RosegardenGUIApp::slotChangeZoom(int)
{
    m_view->setZoomSize(m_zoomSlider->getCurrentSize());
}


// The sequencer calls this method when it's alive to get us
// to poll it for Device/Instrument information.  We also tell
// it to shut up at the same time.
//
void RosegardenGUIApp::alive()
{
    m_doc->alive();
}


void
RosegardenGUIApp::slotChangeTempo(Rosegarden::timeT time,
                                  double value,
                                  TempoDialog::TempoDialogAction action)
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    // We define a macro command here and build up the command
    // label as we add commands on.
    //
    if (action == TempoDialog::AddTempo)
    {
        m_doc->getCommandHistory()->addCommand(new
                AddTempoChangeCommand(&comp, time, value));
    }
    else if (action == TempoDialog::ReplaceTempo)
    {
	//!!! I'm sure this should be unnecessary, as tempo changes
	// should replace if you just insert at the same time anyway

        int index = comp.getTempoChangeNumberAt(time);

        // if there's no previous tempo change then just set globally
        //
        if (index == -1)
        {
            m_doc->getCommandHistory()->addCommand(new
                    AddTempoChangeCommand(&comp, 0, value));
            return;
        }

        // get time of previous tempo change
        Rosegarden::timeT prevTime = comp.getRawTempoChange(index).first;

        KMacroCommand *macro =
            new KMacroCommand(i18n("Replace Tempo Change at %1").arg(time));

        macro->addCommand(new RemoveTempoChangeCommand(&comp, index));
        macro->addCommand(new AddTempoChangeCommand(&comp, prevTime, value));

        m_doc->getCommandHistory()->addCommand(macro);

    }
    else if (action == TempoDialog::AddTempoAtBarStart)
    {
        m_doc->getCommandHistory()->addCommand(new
                AddTempoChangeCommand(&comp, comp.getBarStartForTime(time),
				      value));
    }
    else if (action == TempoDialog::GlobalTempo ||
             action == TempoDialog::GlobalTempoWithDefault)
    {
        KMacroCommand *macro = new KMacroCommand(i18n("Set Global Tempo"));

        // Remove all tempo changes in reverse order so as the index numbers
        // don't becoming meaningless as the command gets unwound.
        //
        for (int i = 0; i < comp.getTempoChangeCount(); i++)
            macro->addCommand(new RemoveTempoChangeCommand(&comp,
                                  (comp.getTempoChangeCount() - 1 - i)));

        // add tempo change at time zero
        //
        macro->addCommand(new AddTempoChangeCommand(&comp, 0, value));

        // are we setting default too?
        //
        if (action == TempoDialog::GlobalTempoWithDefault)
        {
            macro->setName(i18n("Set Global and Default Tempo"));
            macro->addCommand(new ModifyDefaultTempoCommand(&comp, value));
        }

        m_doc->getCommandHistory()->addCommand(macro);

    } 
    else
    {
        kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::slotChangeTempo() - "
                             << "unrecognised tempo command" << endl;
    }

}

void
RosegardenGUIApp::slotDocumentModified()
{
#ifdef RGKDE3
    stateChanged("file_modified");
#endif
}

void
RosegardenGUIApp::slotStateChanged(const QString& s,
                                   bool reverse)
{
#ifdef RGKDE3
    stateChanged(s, reverse ? KXMLGUIClient::StateReverse : KXMLGUIClient::StateNoReverse);
#endif
}

void
RosegardenGUIApp::slotTestClipboard()
{
#ifdef RGKDE3
    if (m_doc->getClipboard()->isEmpty()) {
	stateChanged("have_clipboard", KXMLGUIClient::StateReverse);
	stateChanged("have_clipboard_single_segment",
		     KXMLGUIClient::StateReverse);
    } else {
	stateChanged("have_clipboard", KXMLGUIClient::StateNoReverse);
	stateChanged("have_clipboard_single_segment",
		     (m_doc->getClipboard()->isSingleSegment() ?
		      KXMLGUIClient::StateNoReverse :
		      KXMLGUIClient::StateReverse));
    }
#endif
}

void
RosegardenGUIApp::plugAccelerators(QWidget *widget, QAccel *acc)
{

    acc->connectItem(acc->insertItem(Key_Enter),
                     this,
                     SLOT(slotPlay()));

    acc->connectItem(acc->insertItem(Key_Insert),
                     this,
                     SLOT(slotStop()));

    acc->connectItem(acc->insertItem(Key_PageDown),
                     this,
                     SLOT(slotFastforward()));

    acc->connectItem(acc->insertItem(Key_End),
                     this,
                     SLOT(slotRewind()));

    acc->connectItem(acc->insertItem(Key_Space),
                     this,
                     SLOT(slotRecord()));

    Rosegarden::RosegardenTransportDialog *transport =
        dynamic_cast<Rosegarden::RosegardenTransportDialog*>(widget);

    if (transport)
    {
        connect((QObject *) transport->PlayButton,
                SIGNAL(released()),
                this,
                SLOT(slotPlay()));

        connect((QObject *) transport->StopButton,
                SIGNAL(released()),
                this,
                SLOT(slotStop()));
             
        connect((QObject *) transport->FfwdButton,
                SIGNAL(released()),
                SLOT(slotFastforward()));
            
        connect((QObject *) transport->RewindButton,
                SIGNAL(released()),
                this,
                SLOT(slotRewind()));

        connect((QObject *) transport->RecordButton,
                SIGNAL(released()),
                this,
                SLOT(slotRecord()));

        connect((QObject *) transport->RewindEndButton,
                SIGNAL(released()),
                this,
                SLOT(slotRewindToBeginning()));

        connect((QObject *) transport->FfwdEndButton,
                SIGNAL(released()),
                this,
                SLOT(slotFastForwardToEnd()));

        connect((QObject *) transport->MetronomeButton,
                SIGNAL(released()),
                this,
                SLOT(slotToggleMetronome()));
            
        connect((QObject *) transport->SoloButton,
                SIGNAL(released()),
                this,
                SLOT(slotToggleSolo()));
            
        connect((QObject *) transport->TimeDisplayButton,
                SIGNAL(released()),
                this,
                SLOT(slotRefreshTimeDisplay()));

        connect((QObject *) transport->ToEndButton,
                SIGNAL(released()),
                SLOT(slotRefreshTimeDisplay()));
    }
}

// Return the path to a new audio file as a QString (DCOP friendly)
//
QString
RosegardenGUIApp::createNewAudioFile()
{
    return QString(m_doc->createNewAudioFile().c_str());
}


void
RosegardenGUIApp::slotAudioManager()
{
    if (m_audioManagerDialog == 0)
    {
        m_audioManagerDialog =
          new Rosegarden::AudioManagerDialog(this, m_doc);

        connect(m_audioManagerDialog, SIGNAL(closeClicked()),
                SLOT(slotAudioManagerClosed()));

        connect(m_audioManagerDialog,
                SIGNAL(playAudioFile(unsigned int,
                                     const Rosegarden::RealTime &,
                                     const Rosegarden::RealTime&)),
                SLOT(slotPlayAudioFile(unsigned int,
                                       const Rosegarden::RealTime &,
                                       const Rosegarden::RealTime &)));

        connect(m_audioManagerDialog, SIGNAL(addAudioFile(unsigned int)),
                SLOT(slotAddAudioFile(unsigned int)));

        connect(m_audioManagerDialog, SIGNAL(deleteAudioFile(unsigned int)),
                SLOT(slotDeleteAudioFile(unsigned int)));

        connect(m_audioManagerDialog,
                SIGNAL(segmentSelected(Rosegarden::Segment*)),
                SLOT(slotSelectSegment(Rosegarden::Segment*)));

        connect(m_audioManagerDialog,
                SIGNAL(deleteSegment(Rosegarden::Segment*)),
                SLOT(slotDeleteSegment(Rosegarden::Segment*)));

        m_audioManagerDialog->show();
    }
}

void
RosegardenGUIApp::slotAudioManagerClosed()
{
    if (m_audioManagerDialog)
        m_audioManagerDialog = 0;
}


void
RosegardenGUIApp::slotPlayAudioFile(unsigned int id,
                                    const Rosegarden::RealTime &startTime,
                                    const Rosegarden::RealTime &duration)
{
    Rosegarden::AudioFile *aF = m_doc->getAudioFileManager().getAudioFile(id);

    if (aF == 0)
        return;

    Rosegarden::MappedEvent *mE =
        new Rosegarden::MappedEvent(m_doc->getStudio().
                                        getAudioPreviewInstrument(),
                                    id,
                                    Rosegarden::RealTime(0, 0),  // event time
                                    duration,                    // duration
                                    startTime);                  // start index

    m_seqManager->sendMappedEvent(mE);
}

// Add an audio file to the sequencer - the AudioManagerDialog has
// already added it to the AudioFileManager.
//
void
RosegardenGUIApp::slotAddAudioFile(unsigned int id)
{
    Rosegarden::AudioFile *aF = m_doc->getAudioFileManager().getAudioFile(id);

    if (aF == 0)
        return;

    QCString replyType;
    QByteArray replyData;
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    // We have to pass the filename as a QString
    //
    streamOut << QString(strtoqstr(aF->getFilename()));
    streamOut << aF->getId();

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "addAudioFile(QString, int)", data, replyType, replyData))
    {
        std::cerr << "RosegardenGUIApp::slotAddAudioFile - "
                  << "couldn't add audio file"
                  << std::endl;
        return;
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
        if (!result)
        {
            std::cerr << "RosegardenGUIApp::slotAddAudioFile - "
                      << "failed to add file \""
                      << aF->getFilename() << "\"" << endl;
        }
    }
}

// File has already been removed locally - remove it from the
// sequencer too.
//
void
RosegardenGUIApp::slotDeleteAudioFile(unsigned int id)
{
    QCString replyType;
    QByteArray replyData;
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    // file id
    //
    streamOut << id;

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "removeAudioFile(int)", data,
                                  replyType, replyData))
    {
        std::cerr << "RosegardenGUIApp::slotDeleteAudioFile - "
                  << "couldn't add audio file"
                  << std::endl;
        return;
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
        if (!result)
        {
            std::cerr << "RosegardenGUIApp::slotDeleteAudioFile - "
                      << "failed to remove file id "
                      << id << endl;
        }
    }

}

void
RosegardenGUIApp::checkForStop()
{
    if (m_seqManager == 0) return;

    try
    {
        m_seqManager->stop();
    }
    catch(QString s)
    {
        KMessageBox::error(this, s);
    }
}

void
RosegardenGUIApp::slotSelectSegment(Rosegarden::Segment *segment)
{
    Rosegarden::SegmentSelection selection;
    selection.insert(segment);
    m_view->slotSetSelectedSegments(selection);
}

void
RosegardenGUIApp::slotDeleteSegment(Rosegarden::Segment *segment)
{
    slotSelectSegment(segment);
    slotDeleteSelectedSegments();
}


