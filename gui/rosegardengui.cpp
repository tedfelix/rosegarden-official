// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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
#include <qinputdialog.h>
#include <qobjectlist.h>
#include <qlayout.h>

// include files for KDE
#include <kprocess.h>
#include <kstdaccel.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kmenubar.h>
#include <klocale.h>
#include <klineeditdlg.h>
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
#include <ktip.h>
#include <kpopupmenu.h>
#include <kfilemetainfo.h>

// application specific includes

#include "Clipboard.h"
#include "Configuration.h"
#include "SegmentNotationHelper.h"
#include "Profiler.h"
#include "MidiFile.h"

#include "constants.h"
#include "kstartuplogo.h"
#include "rosestrings.h"
#include "rosegardengui.h"
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "rosegardenconfiguredialog.h"
#include "rosegardentransportdialog.h"
#include "rg21io.h"
#include "hydrogenio.h"
#include "mupio.h"
#include "csoundio.h"
#include "lilypondio.h"
#include "musicxmlio.h"
#include "rosegardendcop.h"
#include "ktmpstatusmsg.h"
#include "SegmentPerformanceHelper.h"
#include "NotationTypes.h"
#include "Selection.h"
#include "sequencemanager.h"
#include "trackbuttons.h"
#include "tracklabel.h"
#include "trackeditor.h"
#include "dialogs.h"
#include "editcommands.h"
#include "notationcommands.h"
#include "multiviewcommandhistory.h"
#include "segmentcommands.h"
#include "zoomslider.h"
#include "audiomanagerdialog.h"
#include "widgets.h"
#include "temporuler.h"
#include "chordnameruler.h"
#include "SoundDriver.h"
#include "segmenttool.h"
#include "matrixtool.h"
#include "notationtool.h"
#include "audiopluginmanager.h"
#include "studiocontrol.h"
#include "bankeditor.h"
#include "deviceeditor.h"
#include "devicemanager.h"
#include "mixer.h"
#include "midifilter.h"
#include "controleditor.h"
#include "markereditor.h"
#include "triggermanager.h"
#include "studiocommands.h"
#include "rgapplication.h"
#include "playlist.h"
#include "sequencermapper.h"
#include "segmentparameterbox.h"
#include "instrumentparameterbox.h"
#include "audioplugindialog.h"

#ifdef HAVE_LIBJACK
#include <jack/jack.h>
#endif


//!!! ditch these when harmonize() moves out
#include "CompositionTimeSliceAdapter.h"
#include "AnalysisTypes.h"

#define ID_STATUS_MSG 1

//#define SETTING_LOG_DEBUG
#ifdef SETTING_LOG_DEBUG
static void _settingLog(QString msg)
{
    RG_DEBUG << msg << endl;

    QFile log("/tmp/setting_log.txt");
    log.open(IO_WriteOnly | IO_Append);
    QTextStream out(&log);
    out << QDateTime::currentDateTime().toString() << " : " << msg << endl;
}
#endif


RosegardenGUIApp *RosegardenGUIApp::m_myself = 0;

using Rosegarden::timeT;
using Rosegarden::RosegardenTransportDialog;

RosegardenGUIApp::RosegardenGUIApp(bool useSequencer,
				   bool useExistingSequencer,
                                   QObject *startupStatusMessageReceiver)
    : DCOPObject("RosegardenIface"), RosegardenIface(this), KDockMainWindow(0), 
      m_actionsSetup(false),
      m_fileRecent(0),
      m_view(0),
      m_swapView(0),
      m_mainDockWidget(0),
      m_dockLeft(0),
      m_doc(0),
      m_sequencerProcess(0),
      m_zoomSlider(0),
      m_seqManager(0),
      m_transport(0),
      m_audioManagerDialog(0),
      m_originatingJump(false),
      m_storedLoopStart(0),
      m_storedLoopEnd(0),
      m_useSequencer(useSequencer),
      m_dockVisible(true),
      m_autoSaveTimer(new QTimer(this)),
      m_clipboard(new Rosegarden::Clipboard),
      m_playList(0),
      m_deviceManager(0),
      m_audioMixer(0),
      m_midiMixer(0),
      m_bankEditor(0),
      m_markerEditor(0),
      m_triggerSegmentManager(0),
      m_playTimer(new QTimer(this)),
      m_stopTimer(new QTimer(this))
{
    m_myself = this;


    if (startupStatusMessageReceiver) {
        QObject::connect(this, SIGNAL(startupStatusMessage(QString)),
                         startupStatusMessageReceiver,
                         SLOT(slotShowStatusMessage(QString)));
    }

    // Try to start the sequencer
    //
    if (m_useSequencer) {

#ifdef HAVE_LIBJACK
        // First we check if jackd is running allready

        std::string jackClientName = "rosegarden";

        // attempt connection to JACK server
        //
        jack_client_t* testJackClient;
        testJackClient =jack_client_new(jackClientName.c_str());
        if (testJackClient == 0 ) {

            // If no connection to JACK
            // try to launch JACK - if the configuration wants us to.
            if (!launchJack()) {
                KStartupLogo::hideIfStillThere();
                KMessageBox::error(this, i18n("Attempted to launch JACK audio daemon failed.  Audio will be disabled.\nPlease check configuration (Settings->Configure Rosegarden->Sequencer->JACK control)\n and restart."));
            }
        } else {
            //this client was just for testing
            jack_client_close(testJackClient);
        }
#endif // HAVE_LIBJACK

        emit startupStatusMessage(i18n("Starting sequencer..."));
        launchSequencer(useExistingSequencer);

    } else RG_DEBUG << "RosegardenGUIApp : don't use sequencer\n";

    // Plugin manager
    //
    emit startupStatusMessage(i18n("Initialising plugin manager..."));
    m_pluginManager = new Rosegarden::AudioPluginManager();

    // call inits to invoke all other construction parts
    //
    emit startupStatusMessage(i18n("Initialising view..."));
    initStatusBar();
    setupActions();
    iFaceDelayedInit(this);
    initZoomToolbar();

    QPixmap dummyPixmap; // any icon will do
    m_mainDockWidget = createDockWidget("Rosegarden MainDockWidget", dummyPixmap, 0L, "main_dock_widget");
    // allow others to dock to the left and right sides only
    m_mainDockWidget->setDockSite(KDockWidget::DockLeft | KDockWidget::DockRight);
    // forbit docking abilities of m_mainDockWidget itself
    m_mainDockWidget->setEnableDocking(KDockWidget::DockNone);
    setView(m_mainDockWidget); // central widget in a KDE mainwindow
    setMainDockWidget(m_mainDockWidget); // master dockwidget

    m_dockLeft = createDockWidget("params dock", dummyPixmap, 0L,
                                  i18n("Segment & Instrument Parameters"));
    m_dockLeft->manualDock(m_mainDockWidget,            // dock target
                           KDockWidget::DockLeft, // dock site
                           20);                   // relation target/this (in percent)

    connect(m_dockLeft, SIGNAL(iMBeingClosed()),
            this, SLOT(slotParametersClosed()));
    connect(m_dockLeft, SIGNAL(hasUndocked()),
            this, SLOT(slotParametersClosed()));
    // Apparently, hasUndocked() is emitted when the dock widget's
    // 'close' button on the dock handle is clicked.
    connect(m_mainDockWidget, SIGNAL(docking(KDockWidget*, KDockWidget::DockPosition)),
            this, SLOT(slotParametersDockedBack(KDockWidget*, KDockWidget::DockPosition)));

    stateChanged("parametersbox_closed", KXMLGUIClient::StateReverse);

    RosegardenGUIDoc* doc = new RosegardenGUIDoc(this, m_pluginManager);

    QFrame* vbox = new QFrame(m_dockLeft);
    QVBoxLayout* vboxLayout = new QVBoxLayout(vbox, 5);
    m_dockLeft->setWidget(vbox);
    m_segmentParameterBox = new SegmentParameterBox(doc, vbox);
    vboxLayout->addWidget(m_segmentParameterBox);
    m_instrumentParameterBox = new InstrumentParameterBox(doc, vbox);
    vboxLayout->addWidget(m_instrumentParameterBox);
    vboxLayout->addStretch();

    connect(m_instrumentParameterBox,
	    SIGNAL(selectPlugin(QWidget *, Rosegarden::InstrumentId, int)),
	    this,
	    SLOT(slotShowPluginDialog(QWidget *, Rosegarden::InstrumentId, int)));

    // Load the initial document (this includes doc's own autoload)
    //
    setDocument(doc);

    emit startupStatusMessage(i18n("Starting sequence manager..."));

    // transport is created by setupActions()
    m_seqManager = new Rosegarden::SequenceManager(m_doc, m_transport);

    // Make sure we get the sequencer status now
    //
    emit startupStatusMessage(i18n("Getting sound driver status..."));
    (void)m_seqManager->getSoundDriverStatus();

    if (m_seqManager->getSoundDriverStatus() & Rosegarden::AUDIO_OK) {
	slotStateChanged("got_audio", true);
    } else {
	slotStateChanged("got_audio", false);
    }

    // If we're restarting the gui then make sure any transient
    // studio objects are cleared away.
    emit startupStatusMessage(i18n("Clearing studio data..."));
    m_seqManager->reinitialiseSequencerStudio();

    // Send the transport control statuses for MMC and JACK
    //
    m_seqManager->sendTransportControlStatuses();

    // Get the plugins available at the sequencer
    //
    emit startupStatusMessage(i18n("Enumerating plugins..."));
    m_seqManager->getSequencerPlugins(m_pluginManager);

    // Now autoload
    //
    stateChanged("new_file");
    stateChanged("have_segments",    KXMLGUIClient::StateReverse);
    stateChanged("have_selection",   KXMLGUIClient::StateReverse);
    slotTestClipboard();

    // Check for lack of MIDI devices and disable Studio options accordingly
    //
    if (!m_doc->getStudio().haveMidiDevices())
        stateChanged("got_midi_devices", KXMLGUIClient::StateReverse);

    emit startupStatusMessage(i18n("Starting..."));

    readOptions();

    // All toolbars should be created before this is called
    setAutoSaveSettings(RosegardenGUIApp::MainWindowConfigGroup, true);
}

RosegardenGUIApp::~RosegardenGUIApp()
{
    RG_DEBUG << "~RosegardenGUIApp()\n";

    if (isSequencerRunning() && !isSequencerExternal()) {
        m_sequencerProcess->blockSignals(true);
        delete m_sequencerProcess;
    }

    if (m_transport)
        delete m_transport;

    delete m_seqManager;
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
    KStdAction::revert(this, SLOT(slotRevertToSaved()),     actionCollection());
    KStdAction::close (this, SLOT(slotFileClose()),         actionCollection());
    KStdAction::print (this, SLOT(slotFilePrint()),         actionCollection());
    KStdAction::printPreview (this, SLOT(slotFilePrintPreview()),         actionCollection());

    new KAction(i18n("Import &MIDI file..."), 0, 0, this,
                SLOT(slotImportMIDI()), actionCollection(),
                "file_import_midi");

    new KAction(i18n("Import &Rosegarden 2.1 file..."), 0, 0, this,
                SLOT(slotImportRG21()), actionCollection(),
                "file_import_rg21");

    new KAction(i18n("Import &Hydrogen file..."), 0, 0, this,
                SLOT(slotImportHydrogen()), actionCollection(),
                "file_import_hydrogen");

    new KAction(i18n("Merge &File..."), 0, 0, this,
                SLOT(slotMerge()), actionCollection(),
                "file_merge");

    new KAction(i18n("Merge &MIDI file..."), 0, 0, this,
                SLOT(slotMergeMIDI()), actionCollection(),
                "file_merge_midi");

    new KAction(i18n("Merge &Rosegarden 2.1 file..."), 0, 0, this,
                SLOT(slotMergeRG21()), actionCollection(),
                "file_merge_rg21");

    new KAction(i18n("Merge &Hydrogen file..."), 0, 0, this,
                SLOT(slotMergeHydrogen()), actionCollection(),
                "file_merge_hydrogen");

    new KAction(i18n("Export &MIDI file..."), 0, 0, this,
                SLOT(slotExportMIDI()), actionCollection(),
                "file_export_midi");

    new KAction(i18n("Export &Lilypond file..."), 0, 0, this,
                SLOT(slotExportLilypond()), actionCollection(),
                "file_export_lilypond");

    new KAction(i18n("Export Music&XML file..."), 0, 0, this,
                SLOT(slotExportMusicXml()), actionCollection(),
                "file_export_musicxml");

    new KAction(i18n("Export &Csound score file..."), 0, 0, this,
                SLOT(slotExportCsound()), actionCollection(),
                "file_export_csound");

    new KAction(i18n("Export M&up file..."), 0, 0, this,
                SLOT(slotExportMup()), actionCollection(),
                "file_export_mup");

    new KAction(i18n("Play&list"), 0, 0, this,
                SLOT(slotPlayList()), actionCollection(),
                "file_show_playlist");

    KStdAction::quit  (this, SLOT(slotQuit()),              actionCollection());

    // setup edit menu
    KStdAction::cut      (this, SLOT(slotEditCut()),        actionCollection());
    KStdAction::copy     (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste    (this, SLOT(slotEditPaste()),      actionCollection());

    //
    // undo/redo actions are special in that they are connected to
    // slots later on, when the current document is set up - see
    // MultiViewCommandHistory::attachView
    //
    new KToolBarPopupAction(i18n("Und&o"),
                            "undo",
                            KStdAccel::key(KStdAccel::Undo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Undo));

    new KToolBarPopupAction(i18n("Re&do"),
                            "redo",
                            KStdAccel::key(KStdAccel::Redo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Redo));
    /////
    


    // setup Settings menu
    //
    m_viewToolBar = KStdAction::showToolbar  (this, SLOT(slotToggleToolBar()), actionCollection(),
	                                    "show_stock_toolbar");

    m_viewToolsToolBar = new KToggleAction(i18n("Show T&ools Toolbar"), 0, this,
                                            SLOT(slotToggleToolsToolBar()), actionCollection(),
                                            "show_tools_toolbar");
    
    m_viewTracksToolBar = new KToggleAction(i18n("Show Trac&ks Toolbar"), 0, this,
                                            SLOT(slotToggleTracksToolBar()), actionCollection(),
                                            "show_tracks_toolbar");

    m_viewEditorsToolBar = new KToggleAction(i18n("Show &Editors Toolbar"), 0, this,
                                            SLOT(slotToggleEditorsToolBar()), actionCollection(),
                                            "show_editors_toolbar");

    m_viewTransportToolBar = new KToggleAction(i18n("Show Trans&port Toolbar"), 0, this,
                                            SLOT(slotToggleTransportToolBar()), actionCollection(),
                                            "show_transport_toolbar");

    m_viewZoomToolBar = new KToggleAction(i18n("Show &Zoom Toolbar"), 0, this,
                                            SLOT(slotToggleZoomToolBar()), actionCollection(),
                                            "show_zoom_toolbar");

    m_viewStatusBar = KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()),
	                                    actionCollection(), "show_status_bar");

    m_viewTransport = new KToggleAction(i18n("Show Tra&nsport"), Key_T, this,
                                        SLOT(slotToggleTransport()),
                                        actionCollection(),
                                        "show_transport");

    m_viewTrackLabels = new KToggleAction(i18n("Show Track &Labels"), 0, this,
                                          SLOT(slotToggleTrackLabels()),
                                          actionCollection(),
                                          "show_tracklabels");

    m_viewRulers = new KToggleAction(i18n("Show Playback Position R&uler"), 0, this,
                                     SLOT(slotToggleRulers()),
                                     actionCollection(),
                                     "show_rulers");

    m_viewTempoRuler = new KToggleAction(i18n("Show Te&mpo Ruler"), 0, this,
                                     SLOT(slotToggleTempoRuler()),
                                     actionCollection(),
                                     "show_tempo_ruler");

    m_viewChordNameRuler = new KToggleAction(i18n("Show Cho&rd Name Ruler"), 0, this,
                                             SLOT(slotToggleChordNameRuler()),
                                             actionCollection(),
                                             "show_chord_name_ruler");

    m_viewPreviews = new KToggleAction(i18n("Show Segment Pre&views"), 0, this,
                                       SLOT(slotTogglePreviews()),
                                       actionCollection(),
                                       "show_previews");

    new KAction(i18n("Show Se&gment and Instrument Parameters"), Key_P, this,
                SLOT(slotDockParametersBack()),
                actionCollection(),
                "show_inst_segment_parameters");

/*  dmm - This was a retarded hack I did to deal with settings amnesia of the day...  we
 *  should get rid of it.   
    new KAction(i18n("Toggle &All of the Above"), 0, this,
                SLOT(slotToggleAll()),
                actionCollection(),
                "toggle_all");  */

#if KDE_VERSION >= 196614 // KDE 3.1
    KStdAction::tipOfDay( this, SLOT( slotShowTip() ), actionCollection() );
#else
    new KAction(i18n("Show Tip of the &Day"), 0, this,
                SLOT(slotShowTip()),
                actionCollection(),
                "help_show_tip");
#endif

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
    action = new KRadioAction(i18n("&Select"), icon, Key_F1,
                              this, SLOT(slotPointerSelected()),
                              actionCollection(), "select");

    action->setExclusiveGroup("segmenttools");
                             
    action = new KRadioAction(i18n("&Erase"), "eraser", Key_F3,
                              this, SLOT(slotEraseSelected()),
                              actionCollection(), "erase");
    action->setExclusiveGroup("segmenttools");

    action = new KRadioAction(i18n("&Draw"), "pencil", Key_F2,
                              this, SLOT(slotDrawSelected()),
                              actionCollection(), "draw");
    action->setExclusiveGroup("segmenttools");

    action = new KRadioAction(i18n("&Move"), "move", Key_F4,
                              this, SLOT(slotMoveSelected()),
                              actionCollection(), "move");
    action->setExclusiveGroup("segmenttools");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/resize.xpm"));
    action = new KRadioAction(i18n("&Resize"), icon, Key_F5,
                              this, SLOT(slotResizeSelected()),
                              actionCollection(), "resize");
    action->setExclusiveGroup("segmenttools");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/split.xpm"));
    action = new KRadioAction(i18n("&Split"), icon, Key_F6,
                              this, SLOT(slotSplitSelected()),
                              actionCollection(), "split");
    action->setExclusiveGroup("segmenttools");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/join.xpm"));
    action = new KRadioAction(i18n("&Join"), icon, 0/* Key_F7 */,
                              this, SLOT(slotJoinSelected()),
                              actionCollection(), "join");
    action->setExclusiveGroup("segmenttools");


    new KAction(i18n("&Harmonize"), 0, this,
                SLOT(slotHarmonizeSelection()), actionCollection(),
                "harmonize_selection");

    new KAction(AddTimeSignatureCommand::getGlobalName(),
                0,
                this, SLOT(slotEditTimeSignature()),
                actionCollection(), "add_time_signature");

    new KAction(i18n("Open Tempo and Time Signature Editor"), 0, this,
		SLOT(slotEditTempos()), actionCollection(), "edit_tempos");
    
    //
    // Edit menu
    //
    new KAction(i18n("De&lete"), Key_Delete, this,
                SLOT(slotDeleteSelectedSegments()), actionCollection(),
                "delete");

    new KAction(i18n("Select &All Segments"), Key_A + CTRL, this,
                SLOT(slotSelectAll()), actionCollection(),
                "select_all");

    new KAction(AddTempoChangeCommand::getGlobalName(),
                0,
                this, SLOT(slotEditTempo()),
                actionCollection(), "add_tempo");
    
    new KAction(ChangeCompositionLengthCommand::getGlobalName(),
                0,
                this, SLOT(slotChangeCompositionLength()),
                actionCollection(), "change_composition_length");

    new KAction(i18n("Edit Mar&kers..."), Key_K + CTRL, this,
                SLOT(slotEditMarkers()),
                actionCollection(), "edit_markers");

    new KAction(i18n("Edit Document P&roperties..."), 0, this,
                SLOT(slotEditDocumentProperties()),
                actionCollection(), "edit_doc_properties");


    //
    // Segments menu
    //
    new KAction(i18n("Open in &Default Editor"), Key_Return, this,
                SLOT(slotEdit()), actionCollection(),
                "edit_default");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/matrix.xpm"));
    new KAction(i18n("Open in Matri&x Editor"), icon, Key_M, this,
                SLOT(slotEditInMatrix()), actionCollection(),
                "edit_matrix");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/notation.xpm"));
    new KAction(i18n("Open in &Notation Editor"), icon, Key_N, this,
                SLOT(slotEditAsNotation()), actionCollection(),
                "edit_notation");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/eventlist.xpm"));
    new KAction(i18n("Open in &Event List Editor"), icon, Key_E, this,
                SLOT(slotEditInEventList()), actionCollection(),
                "edit_event_list");

    new KAction(SegmentLabelCommand::getGlobalName(),
                0,
                this, SLOT(slotRelabelSegments()),
                actionCollection(), "relabel_segment");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/quantize.xpm"));
    new KAction(i18n("&Quantize..."), icon, Key_Equal, this,
                SLOT(slotQuantizeSelection()), actionCollection(),
                "quantize_selection");

    new KAction(i18n("Repeat Last Quantize"), Key_Plus, this,
                SLOT(slotRepeatQuantizeSelection()), actionCollection(),
                "repeat_quantize");

    new KAction(SegmentRescaleCommand::getGlobalName(), 0, this,
                SLOT(slotRescaleSelection()), actionCollection(),
                "rescale");

    new KAction(SegmentAutoSplitCommand::getGlobalName(), 0, this,
                SLOT(slotAutoSplitSelection()), actionCollection(),
                "auto_split");

    new KAction(SegmentSplitByPitchCommand::getGlobalName(), 0, this,
                SLOT(slotSplitSelectionByPitch()), actionCollection(),
                "split_by_pitch");

    new KAction(i18n("Jog &Left"), Key_Left + ALT, this,
                SLOT(slotJogLeft()), actionCollection(),
                "jog_left");

    new KAction(i18n("Jog &Right"), Key_Right + ALT, this,
                SLOT(slotJogRight()), actionCollection(),
                "jog_right");

    new KAction(i18n("Set Start Time..."), 0, this,
		SLOT(slotSetSegmentStartTimes()), actionCollection(),
		"set_segment_start");

    new KAction(i18n("Set Duration..."), 0, this,
		SLOT(slotSetSegmentDurations()), actionCollection(),
		"set_segment_duration");

    new KAction(SegmentMergeCommand::getGlobalName(),
                Key_J + CTRL,
                this, SLOT(slotJoinSegments()),
                actionCollection(), "join_segments");

    new KAction(i18n("Turn Re&peats into Copies"),
                0,
                this, SLOT(slotRepeatingSegments()),
                actionCollection(), "repeats_to_real_copies");

    new KAction(i18n("Manage Tri&ggered Segments"), 0, 
                this, SLOT(slotManageTriggerSegments()),
                actionCollection(), "manage_trigger_segments");

    new KAction(i18n("Set Tempos from &Beat Segment"), 0, this,
                SLOT(slotGrooveQuantize()), actionCollection(),
                "groove_quantize");

    new KAction(i18n("Set &Tempo to Audio Segment Duration"), 0, this,
                SLOT(slotTempoToSegmentLength()), actionCollection(),
                "set_tempo_to_segment_length");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/manage-audio-segments.xpm"));
    new KAction(i18n("Manage A&udio Segments"), icon,
                Key_U + CTRL, 
                this, SLOT(slotAudioManager()),
                actionCollection(), "audio_manager");


    //
    // Tracks menu
    //
    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/add_tracks.xpm"));
    new KAction(i18n("Add &Track"), icon, CTRL + Key_T,
                this, SLOT(slotAddTrack()),
                actionCollection(), "add_track");

    new KAction(i18n("&Add Tracks..."), 0,
                this, SLOT(slotAddTracks()),
                actionCollection(), "add_tracks");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/delete_track.xpm"));
    new KAction(i18n("D&elete Track"), icon, CTRL + Key_D,
                this, SLOT(slotDeleteTrack()),
                actionCollection(), "delete_track");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/move_track_down.xpm"));
    new KAction(i18n("Move Track &Down"), icon, SHIFT + Key_Down,
                this, SLOT(slotMoveTrackDown()),
                actionCollection(), "move_track_down");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/move_track_up.xpm"));
    new KAction(i18n("Move Track &Up"), icon, SHIFT + Key_Up,
                this, SLOT(slotMoveTrackUp()),
                actionCollection(), "move_track_up");

    new KAction(i18n("Select &Next Track"),
                Key_Down, 
                this, SLOT(slotTrackDown()),
                actionCollection(), "select_next_track");

    new KAction(i18n("Select &Previous Track"),
                Key_Up, 
                this, SLOT(slotTrackUp()),
                actionCollection(), "select_previous_track");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/mute-all.xpm"));
    new KAction(i18n("&Mute all Tracks"), icon, 0, 
                this, SLOT(slotMuteAllTracks()),
                actionCollection(), "mute_all_tracks");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/un-mute-all.xpm"));
    new KAction(i18n("&Unmute all Tracks"), icon, 0, 
                this, SLOT(slotUnmuteAllTracks()),
                actionCollection(), "unmute_all_tracks");

    new KAction(i18n("&Remap Instruments..."), 0, this,
                SLOT(slotRemapInstruments()),
                actionCollection(), "remap_instruments");

    //
    // Studio menu
    //
    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/mixer.xpm"));
    new KAction(i18n("&Audio Mixer"), icon, 0, this,
		SLOT(slotOpenAudioMixer()),
		actionCollection(), "audio_mixer");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/midimixer.xpm"));
    new KAction(i18n("Midi Mi&xer"), icon, 0, this,
		SLOT(slotOpenMidiMixer()),
		actionCollection(), "midi_mixer");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "toolbar/manage-midi-devices.xpm"));
    new KAction(i18n("Manage MIDI &Devices"), icon, 0, this,
                SLOT(slotManageMIDIDevices()),
                actionCollection(), "manage_devices");

    new KAction(i18n("Modify MIDI &Filters"), "filter", 0, this,
                SLOT(slotModifyMIDIFilters()),
                actionCollection(), "modify_midi_filters");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "toolbar/time-musical.xpm"));
    new KAction(i18n("Manage &Metronome"), 0, this,
                SLOT(slotManageMetronome()),
                actionCollection(), "manage_metronome");

    new KAction(i18n("&Save Current Document as Default Studio"), 0, this,
                SLOT(slotSaveDefaultStudio()),
                actionCollection(), "save_default_studio");

    new KAction(i18n("&Import Default Studio"), 0, this,
	        SLOT(slotImportDefaultStudio()),
		actionCollection(), "load_default_studio");

    new KAction(i18n("Im&port Studio from File..."), 0, this,
	        SLOT(slotImportStudio()),
		actionCollection(), "load_studio");

    new KAction(i18n("&Reset MIDI Network"), 0, this,
	        SLOT(slotResetMidiNetwork()),
		actionCollection(), "reset_midi_network");

    //
    // Transport menu
    //

    // Transport controls [rwb]
    //
    // We set some default key bindings - with numlock off
    // use 1 (End) and 3 (Page Down) for Rwd and Ffwd and
    // 0 (insert) and keypad Enter for Play and Stop 
    //
    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/transport-play.xpm"));
    m_playTransport = new KAction(i18n("&Play"), icon, Key_Enter, this,
                                  SLOT(slotPlay()), actionCollection(),
                                  "play");
    m_playTransport->setGroup(RosegardenTransportDialog::ConfigGroup);

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/transport-stop.xpm"));
    m_stopTransport = new KAction(i18n("&Stop"), icon, Key_Insert, this,
                                  SLOT(slotStop()), actionCollection(),
                                  "stop");
    m_stopTransport->setGroup(RosegardenTransportDialog::ConfigGroup);

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/transport-ffwd.xpm"));
    m_ffwdTransport = new KAction(i18n("&Fast Forward"), icon, Key_PageDown,
                                  this,
                                  SLOT(slotFastforward()), actionCollection(),
                                  "fast_forward");
    m_ffwdTransport->setGroup(RosegardenTransportDialog::ConfigGroup);

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/transport-rewind.xpm"));
    m_rewindTransport = new KAction(i18n("Re&wind"), icon, Key_End, this,
                                    SLOT(slotRewind()), actionCollection(),
                                    "rewind");
    m_rewindTransport->setGroup(RosegardenTransportDialog::ConfigGroup);

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/transport-record.xpm"));
    m_recordTransport = new KAction(i18n("P&unch in Record"), icon, Key_Space, this,
                                    SLOT(slotToggleRecord()), actionCollection(),
                                    "recordtoggle");
    m_recordTransport->setGroup(RosegardenTransportDialog::ConfigGroup);

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/transport-record.xpm"));
    m_recordTransport = new KAction(i18n("&Record"), icon, 0, this,
                                    SLOT(slotRecord()), actionCollection(),
                                    "record");
    m_recordTransport->setGroup(RosegardenTransportDialog::ConfigGroup);

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/transport-rewind-end.xpm"));
    m_rewindEndTransport = new KAction(i18n("Rewind to &Beginning"), icon, 0, this,
                                       SLOT(slotRewindToBeginning()), actionCollection(),
                                       "rewindtobeginning");
    m_rewindEndTransport->setGroup(RosegardenTransportDialog::ConfigGroup);

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/transport-ffwd-end.xpm"));
    m_ffwdEndTransport = new KAction(i18n("Fast Forward to &End"), icon, 0, this,
                                     SLOT(slotFastForwardToEnd()), actionCollection(),
                                     "fastforwardtoend");
    m_ffwdEndTransport->setGroup(RosegardenTransportDialog::ConfigGroup);

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/transport-tracking.xpm"));
    (new KToggleAction(i18n("Scro&ll to Follow Playback"), icon, Key_Pause, this,
		       SLOT(slotToggleTracking()), actionCollection(),
		       "toggle_tracking"))->setChecked(true);

    // create the Transport GUI and add the callbacks to the
    // buttons and keyboard accelerators
    //
    m_transport =
        new Rosegarden::RosegardenTransportDialog(this);
    plugAccelerators(m_transport, m_transport->getAccelerators());

    // create main gui
    //
    createGUI("rosegardenui.rc", false);

    // Ensure that the checkbox is unchecked if the dialog
    // is closed
    connect(m_transport, SIGNAL(closed()),
            SLOT(slotCloseTransport()));

    // Handle loop setting and unsetting from the transport loop button
    //
    
    connect(m_transport, SIGNAL(setLoop()), SLOT(slotSetLoop()));
    connect(m_transport, SIGNAL(unsetLoop()), SLOT(slotUnsetLoop()));
    connect(m_transport, SIGNAL(panic()), SLOT(slotPanic()));

    connect(m_transport, SIGNAL(editTempo(QWidget*)),
            SLOT(slotEditTempo(QWidget*)));

    connect(m_transport, SIGNAL(editTimeSignature(QWidget*)),
            SLOT(slotEditTimeSignature(QWidget*)));
    
    // transport toolbar is hidden by default - TODO : this should be in options
    //
    //toolBar("Transport Toolbar")->hide();

    QPopupMenu* setTrackInstrumentMenu = dynamic_cast<QPopupMenu*>(factory()->container("set_track_instrument", this));

    if (setTrackInstrumentMenu) {
        connect(setTrackInstrumentMenu, SIGNAL(aboutToShow()),
                this, SLOT(slotPopulateTrackInstrumentPopup()));
    } else {
        RG_DEBUG << "RosegardenGUIApp::setupActions() : couldn't find set_track_instrument menu - check rosegardenui.rcn\n";
    }

    setRewFFwdToAutoRepeat();
}

void RosegardenGUIApp::setRewFFwdToAutoRepeat()
{
    QWidget* transportToolbar = factory()->container("Transport Toolbar", this);

    if (transportToolbar) {
        QObjectList *l = transportToolbar->queryList();
        QObjectListIt it(*l); // iterate over the buttons
        QObject *obj;

        while ( (obj = it.current()) != 0 ) {
            // for each found object...
            ++it;
            // RG_DEBUG << "obj name : " << obj->name() << endl;
            QString objName = obj->name();
            
            if (objName.endsWith("rewind") || objName.endsWith("fast_forward")) {
                QButton* btn = dynamic_cast<QButton*>(obj);
                if (!btn) {
                    RG_DEBUG << "Very strange - found widgets in transport_toolbar which aren't buttons\n";
                    
                    continue;
                }
                btn->setAutoRepeat(true);
            }
            
            
        }
        delete l;

    } else {
        RG_DEBUG << "transportToolbar == 0\n";
    }
 
}

void RosegardenGUIApp::initZoomToolbar()
{
    KToolBar *zoomToolbar = toolBar("Zoom Toolbar");
    if (!zoomToolbar) {
        RG_DEBUG << "RosegardenGUIApp::initZoomToolbar() : "
                             << "zoom toolbar not found" << endl;
        return;
    }

    new QLabel(i18n("  Zoom:  "), zoomToolbar, "kde toolbar widget");

    std::vector<double> zoomSizes; // in units-per-pixel
    double defaultBarWidth44 = 100.0;
    double duration44 = Rosegarden::TimeSignature(4,4).getBarDuration();
    static double factors[] = { 0.025, 0.05, 0.1, 0.2, 0.5,
                                1.0, 1.5, 2.5, 5.0, 10.0 , 20.0 };

    for (unsigned int i = 0; i < sizeof(factors)/sizeof(factors[0]); ++i) {
        zoomSizes.push_back(duration44 / (defaultBarWidth44 * factors[i]));
    }

    // zoom labels
    QString minZoom = QString("%1%").arg(factors[0] * 100.0);
    QString maxZoom = QString("%1%").arg(factors[(sizeof(factors)/sizeof(factors[0])) - 1] * 100.0);

    m_zoomSlider = new ZoomSlider<double>
        (zoomSizes, -1, QSlider::Horizontal, zoomToolbar, "kde toolbar widget");
    m_zoomSlider->setTracking(true);
    m_zoomSlider->setFocusPolicy(QWidget::NoFocus);
    m_zoomLabel = new QLabel(minZoom, zoomToolbar, "kde toolbar widget");
    m_zoomLabel->setIndent(10);

    connect(m_zoomSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeZoom(int)));

    // set initial zoom - we might want to make this a config option
    //    m_zoomSlider->setToDefault();

}


void RosegardenGUIApp::initStatusBar()
{
    KTmpStatusMsg::setDefaultMsg(i18n("  Ready."));
    statusBar()->insertItem(KTmpStatusMsg::getDefaultMsg(),
                            KTmpStatusMsg::getDefaultId(), 1);
    statusBar()->setItemAlignment(KTmpStatusMsg::getDefaultId(), 
                                  AlignLeft | AlignVCenter);

    m_progressBar = new RosegardenProgressBar(100, true, statusBar());
    m_progressBar->setMinimumWidth(100);
    statusBar()->addWidget(m_progressBar);
}

void RosegardenGUIApp::initView()
{ 
    ////////////////////////////////////////////////////////////////////
    // create the main widget here that is managed by KTMainWindow's view-region and
    // connect the widget to your document to display document contents.

    RG_DEBUG << "RosegardenGUIApp::initView()" << endl;

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
    
    m_swapView = new RosegardenGUIView(m_viewTrackLabels->isChecked(),
                                       m_segmentParameterBox,
                                       m_instrumentParameterBox, this);

    // Connect up this signal so that we can force tool mode
    // changes from the view
    connect(m_swapView, SIGNAL(activateTool(QString)),
            this,   SLOT(slotActivateTool(QString)));

    connect(m_swapView,
            SIGNAL(segmentsSelected(const Rosegarden::SegmentSelection &)),
            SIGNAL(segmentsSelected(const Rosegarden::SegmentSelection &)));

    connect(m_swapView,
            SIGNAL(addAudioFile(Rosegarden::AudioFileId)),
            SLOT(slotAddAudioFile(Rosegarden::AudioFileId)));

    connect(m_swapView, SIGNAL(toggleSolo(bool)), SLOT(slotToggleSolo(bool)));

    m_doc->attachView(m_swapView);

    m_mainDockWidget->setWidget(m_swapView);

//     setCentralWidget(m_swapView);
    setCaption(m_doc->getTitle());

    // set the pointer position
    //
    slotSetPointerPosition(m_doc->getComposition().getPosition());


    // Transport setup
    //

    slotEnableTransport(true);

    // and the time signature
    //
    m_transport->setTimeSignature(comp.getTimeSignatureAt(comp.getPosition()));

    // set the tempo in the transport
    //
    m_transport->setTempo(comp.getTempo());

    // bring the transport to the front 
    //
    m_transport->raise();

    // set the play metronome button
    m_transport->MetronomeButton()->setOn(comp.usePlayMetronome());

    // Set the solo button
    m_transport->SoloButton()->setOn(comp.isSolo());

    // make sure we show
    //
    RosegardenGUIView *oldView = m_view;
    m_view = m_swapView;

    connect(m_view, SIGNAL(stateChange(QString, bool)),
            this,   SLOT  (slotStateChanged(QString, bool)));

    // We only check for the SequenceManager to make sure
    // we're not on the first pass though - we don't want
    // to send these toggles twice on initialisation.
    //
    // Clunky but we just about get away with it for the
    // moment.
    //
    if (m_seqManager != 0)
    {
        slotToggleChordNameRuler();
        slotToggleRulers();
        slotToggleTempoRuler();
        slotTogglePreviews();

        // Reset any loop on the sequencer
        //
        try
        {
            if (isUsingSequencer()) m_seqManager->setLoop(0, 0);
        }
        catch(QString s)
        {
	    KStartupLogo::hideIfStillThere();
	    CurrentProgressDialog::freeze();
            KMessageBox::error(this, s);
	    CurrentProgressDialog::thaw();
        }

    }

    delete m_playList;
    m_playList = 0;

    delete m_deviceManager;
    m_deviceManager = 0;

    delete m_audioMixer;
    m_audioMixer = 0;

    delete m_bankEditor;
    m_bankEditor = 0;

    delete m_markerEditor;
    m_markerEditor = 0;

    delete m_triggerSegmentManager;
    m_triggerSegmentManager = 0;

    delete oldView;

    // set the highlighted track
    m_view->slotSelectTrackSegments(comp.getSelectedTrack());

    // play tracking on in the editor by default: turn off if need be
    KToggleAction *trackingAction = dynamic_cast<KToggleAction *>
        (actionCollection()->action("toggle_tracking"));
    if (trackingAction && !trackingAction->isChecked()) {
	m_view->getTrackEditor()->slotToggleTracking();
    }

    m_view->show();

    // We have to do this to make sure that the 2nd call ("select")
    // actually has any effect. Activating the same radio action
    // doesn't work the 2nd time (like pressing down the same radio
    // button twice - it doesn't have any effect), so if you load two
    // files in a row, on the 2nd file a new SegmentCanvas will be
    // created but its tool won't be set, even though it will appear
    // to be selected.
    //
    actionCollection()->action("move")->activate();
    if (m_doc->getComposition().getNbSegments() > 0)
        actionCollection()->action("select")->activate();
    else
        actionCollection()->action("draw")->activate();

    int zoomLevel = m_doc->getConfiguration().
        get<Rosegarden::Int>
            (Rosegarden::DocumentConfiguration::ZoomLevel);

    m_zoomSlider->setSize(double(zoomLevel)/1000.0);
    slotChangeZoom(zoomLevel);

    //slotChangeZoom(int(m_zoomSlider->getCurrentSize()));

    stateChanged("new_file");

    RosegardenProgressDialog::processEvents();
    
    if (m_viewChordNameRuler->isChecked()) {
	SetWaitCursor swc;
	m_view->initChordNameRuler();
    } else {
	m_view->initChordNameRuler();
    }
}

void RosegardenGUIApp::setDocument(RosegardenGUIDoc* newDocument)
{
    if (m_doc == newDocument) return;

    emit documentAboutToChange();
    kapp->processEvents(); // to make sure all opened dialogs (mixer, midi devices...) are closed

    // Take care of all subparts which depend on the document

    // Caption
    //
    QString caption= kapp->caption();   
    setCaption(caption + ": " + newDocument->getTitle());

//     // reset AudioManagerDialog
//     //
//     delete m_audioManagerDialog; // TODO : replace this with a connection to documentAboutToChange() sig.
//     m_audioManagerDialog = 0;

    RosegardenGUIDoc* oldDoc = m_doc;

    m_doc = newDocument;

    if (m_seqManager) // when we're called at startup, the seq. man. isn't created yet
        m_seqManager->setDocument(m_doc);

    if (m_markerEditor) m_markerEditor->setDocument(m_doc);
    if (m_triggerSegmentManager) m_triggerSegmentManager->setDocument(m_doc);

    m_segmentParameterBox->setDocument(m_doc);
    m_instrumentParameterBox->setDocument(m_doc);

    // this will delete all edit views
    //
    if (oldDoc != 0) delete oldDoc;
    
    // connect needed signals
    //
    connect(m_segmentParameterBox, SIGNAL(documentModified()),
            m_doc, SLOT(slotDocumentModified()));

    connect(m_doc, SIGNAL(pointerPositionChanged(Rosegarden::timeT)),
            this,   SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    connect(m_doc, SIGNAL(documentModified(bool)),
            this,   SLOT(slotDocumentModified(bool)));

    connect(m_doc, SIGNAL(loopChanged(Rosegarden::timeT, Rosegarden::timeT)),
            this,  SLOT(slotSetLoop(Rosegarden::timeT, Rosegarden::timeT)));

    m_doc->getCommandHistory()->attachView(actionCollection());

    connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
            SLOT(update()));
    connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
            SLOT(slotTestClipboard()));

    // connect and start the autosave timer
    connect(m_autoSaveTimer, SIGNAL(timeout()), this, SLOT(slotAutoSave()));
    m_autoSaveTimer->start(m_doc->getAutoSavePeriod() * 1000);

    // Connect the playback timer
    //
    connect(m_playTimer, SIGNAL(timeout()), this, SLOT(slotUpdatePlaybackPosition()));
    connect(m_stopTimer, SIGNAL(timeout()), this, SLOT(slotUpdateMonitoring()));

    // finally recreate the main view
    //
    initView();

    m_doc->syncDevices();
    m_doc->clearModifiedStatus();

    if (newDocument->getStudio().haveMidiDevices()) {
	stateChanged("got_midi_devices");
    } else {
	stateChanged("got_midi_devices", KXMLGUIClient::StateReverse);
    }

    // Ensure the sequencer knows about any audio files
    // we've loaded as part of the new Composition
    //
    m_doc->prepareAudio();

    // Do not reset instrument prog. changes after all.
//     if (m_seqManager)
//         m_seqManager->preparePlayback(true);

    Rosegarden::Composition &comp = m_doc->getComposition();

    // Set any loaded loop at the Composition and
    // on the marker on SegmentCanvas and clients
    //
    if (m_seqManager) m_doc->setLoop(comp.getLoopStart(), comp.getLoopEnd());

    emit documentChanged(m_doc);

    m_doc->clearModifiedStatus(); // because it's set as modified by the various
    // init operations
    // TODO: this sucks, have to sort it out somehow.

    // Readjust canvas size
    //
    m_view->getTrackEditor()->slotReadjustCanvasSize();

    m_stopTimer->start(100);
}


void
RosegardenGUIApp::openFile(QString filePath, ImportType type)
{
    RG_DEBUG << "RosegardenGUIApp::openFile " << filePath << endl;

    RosegardenGUIDoc *doc = createDocument(filePath, type);
    if (doc) {
        setDocument(doc);
        QFileInfo fInfo(filePath);
        m_fileRecent->addURL(fInfo.absFilePath());
    }
}

RosegardenGUIDoc*
RosegardenGUIApp::createDocument(QString filePath, ImportType importType)
{
    QFileInfo info(filePath);
    RosegardenGUIDoc *doc = 0;

    if (!info.exists()) {
        // can happen with command-line arg, so...
        KStartupLogo::hideIfStillThere();
        KMessageBox::sorry(this, i18n("File \"%1\" does not exist").arg(filePath));
        return 0;
    }

    if (info.isDir()) {
        KStartupLogo::hideIfStillThere();
        KMessageBox::sorry(this, i18n("File \"%1\" is actually a directory"));
        return 0;
    }

    QFile file(filePath);

    if (!file.open(IO_ReadOnly)) {
        KStartupLogo::hideIfStillThere();
        QString errStr =
            QString(i18n("You do not have read permission for \"%1\"")).
            arg(filePath);

        KMessageBox::sorry(this, errStr);
        return 0;
    }

    // Stop if playing
    //
    if (m_seqManager && m_seqManager->getTransportStatus() == PLAYING)
        slotStop();

    slotEnableTransport(false);

    if (importType == ImportCheckType) {
        KMimeType::Ptr fileMimeType = KMimeType::findByPath(filePath);
        if (fileMimeType->name() == "audio/x-midi")
            importType = ImportMIDI;
        else if (fileMimeType->name() == "audio/x-rosegarden")
            importType = ImportRG4;
        else if (filePath.endsWith(".rose"))
            importType = ImportRG21;
        else if (filePath.endsWith(".h2song"))
            importType = ImportHydrogen;
    }
    

    switch (importType) {
    case ImportMIDI:
        doc = createDocumentFromMIDIFile(filePath);
        break;
    case ImportRG21:
        doc = createDocumentFromRG21File(filePath);
        break;
    case ImportHydrogen:
        doc = createDocumentFromHydrogenFile(filePath);
        break;
    default:
        doc = createDocumentFromRGFile(filePath);
    }
    
    slotEnableTransport(true);

    return doc;
}

RosegardenGUIDoc* 
RosegardenGUIApp::createDocumentFromRGFile(QString filePath)
{
    // Check for an autosaved file to recover
    QString effectiveFilePath = filePath;
    bool canRecover = false;
    QString autoSaveFileName = kapp->checkRecoverFile(filePath, canRecover);
        
    if (canRecover) {
        // First check if the auto-save file is more recent than the doc
        QFileInfo docFileInfo(filePath), autoSaveFileInfo(autoSaveFileName);

        if (docFileInfo.lastModified() < autoSaveFileInfo.lastModified()) {

            RG_DEBUG << "RosegardenGUIApp::openFile : "
                     << "found a more recent autosave file\n";

            // At this point the splash screen may still be there, hide it if
            // it's the case
            KStartupLogo::hideIfStillThere();

            // It is, so ask the user if he wants to use the autosave file
            int reply = KMessageBox::questionYesNo(this,
                                                   i18n("An auto-save file for this document has been found\nDo you want to open it instead ?"));

            if (reply == KMessageBox::Yes)
                // open the autosave file instead
                effectiveFilePath = autoSaveFileName;
            else {
                // user doesn't want the autosave, so delete it
                // so it won't bother us again if we reload
                canRecover = false;
                QFile::remove(autoSaveFileName);
            }
            
        } else
            canRecover = false;
    }

    // Create a new blank document
    //
    RosegardenGUIDoc *newDoc = new RosegardenGUIDoc(this, m_pluginManager,
						    true); // skipAutoload

    // ignore return thingy
    //
    if(newDoc->openDocument(effectiveFilePath))
    {
        if (canRecover)
        {
            // Mark the document as modified,
            // set the "regular" filepath and name (not those of
            // the autosaved doc)
            //
            newDoc->slotDocumentModified();
            QFileInfo info(filePath);
            newDoc->setAbsFilePath(info.absFilePath());
            newDoc->setTitle(info.fileName());
        }
        else
        {
            newDoc->clearModifiedStatus();
        }
    }
    else
    {
        delete newDoc;
        return 0;
    }
    
    return newDoc;
}


RosegardenGUIDoc *RosegardenGUIApp::getDocument() const
{
    return m_doc;
}

void RosegardenGUIApp::slotSaveOptions()
{
    RG_DEBUG << "RosegardenGUIApp::slotSaveOptions()\n";

#ifdef SETTING_LOG_DEBUG
    _settingLog(QString("SETTING 2 : transport flap extended = %1").arg(m_transport->isExpanded()));
    _settingLog(QString("SETTING 2 : show track labels = %1").arg(m_viewTrackLabels->isChecked()));
#endif
    kapp->config()->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    kapp->config()->writeEntry("Show Transport",               m_viewTransport->isChecked());
    kapp->config()->writeEntry("Expanded Transport",           m_transport ? m_transport->isExpanded() : false);
    kapp->config()->writeEntry("Show Track labels",            m_viewTrackLabels->isChecked());
    kapp->config()->writeEntry("Show Rulers",                  m_viewRulers->isChecked());
    kapp->config()->writeEntry("Show Tempo Ruler",             m_viewTempoRuler->isChecked());
    kapp->config()->writeEntry("Show Chord Name Ruler",        m_viewChordNameRuler->isChecked());
    kapp->config()->writeEntry("Show Previews",                m_viewPreviews->isChecked());
    kapp->config()->writeEntry("Show Parameters",              m_dockVisible);

#ifdef SETTING_LOG_DEBUG
    RG_DEBUG << "SHOW PARAMETERS = " << m_dockVisible << endl;
#endif

    m_fileRecent->saveEntries(kapp->config());

//     saveMainWindowSettings(kapp->config(), RosegardenGUIApp::MainWindowConfigGroup);
    kapp->config()->sync();
}


void RosegardenGUIApp::readOptions()
{
    applyMainWindowSettings(kapp->config(), 
            RosegardenGUIApp::MainWindowConfigGroup);

    kapp->config()->reparseConfiguration();
    
    // Statusbar and toolbars toggling action status
    //
    m_viewStatusBar       ->setChecked(!statusBar()                ->isHidden());
    m_viewToolBar         ->setChecked(!toolBar()                  ->isHidden());
    m_viewToolsToolBar    ->setChecked(!toolBar("Tools Toolbar")   ->isHidden());
    m_viewTracksToolBar   ->setChecked(!toolBar("Tracks Toolbar")  ->isHidden());
    m_viewEditorsToolBar  ->setChecked(!toolBar("Editors Toolbar")  ->isHidden());
    m_viewTransportToolBar->setChecked(!toolBar("Transport Toolbar")->isHidden());
    m_viewZoomToolBar     ->setChecked(!toolBar("Zoom Toolbar")     ->isHidden());

    bool opt;

    kapp->config()->setGroup(Rosegarden::GeneralOptionsConfigGroup);

    opt = kapp->config()->readBoolEntry("Show Transport", true);
    m_viewTransport->setChecked(opt);
    slotToggleTransport();

    opt = kapp->config()->readBoolEntry("Expanded Transport", false);

#ifdef SETTING_LOG_DEBUG
    _settingLog(QString("SETTING 3 : transport flap extended = %1").arg(opt));
#endif

    if(opt)
        m_transport->slotPanelOpenButtonClicked();
    else
        m_transport->slotPanelCloseButtonClicked();

    opt = kapp->config()->readBoolEntry("Show Track labels", true);

#ifdef SETTING_LOG_DEBUG
    _settingLog(QString("SETTING 3 : show track labels = %1").arg(opt));
#endif

    m_viewTrackLabels->setChecked(opt);
    slotToggleTrackLabels();

    opt = kapp->config()->readBoolEntry("Show Rulers", true);
    m_viewRulers->setChecked(opt);
    slotToggleRulers();

    opt = kapp->config()->readBoolEntry("Show Tempo Ruler", true);
    m_viewTempoRuler->setChecked(opt);
    slotToggleTempoRuler();

    opt = kapp->config()->readBoolEntry("Show Chord Name Ruler", false);
    m_viewChordNameRuler->setChecked(opt);
    slotToggleChordNameRuler();

    opt = kapp->config()->readBoolEntry("Show Previews", true);
    m_viewPreviews->setChecked(opt);
    slotTogglePreviews();

    opt = kapp->config()->readBoolEntry("Show Parameters", true);
    if (!opt)
    {
        m_dockLeft->undock();
        m_dockLeft->hide();
        stateChanged("parametersbox_closed", KXMLGUIClient::StateNoReverse);
        m_dockVisible = false;
    }

    // initialise the recent file list
    //
    m_fileRecent->loadEntries(kapp->config());

    m_actionsSetup = true;

}

void RosegardenGUIApp::saveGlobalProperties(KConfig *cfg)
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


void RosegardenGUIApp::readGlobalProperties(KConfig* _cfg)
{
    QString filename = _cfg->readEntry("filename", "");
    bool modified = _cfg->readBoolEntry("modified", false);

    if (modified) {
            bool canRecover;
            QString tempname = kapp->checkRecoverFile(filename, canRecover);
        
            if (canRecover) {
                slotEnableTransport(false);
                m_doc->openDocument(tempname);
                slotEnableTransport(true);
                m_doc->slotDocumentModified();
                QFileInfo info(filename);
                m_doc->setAbsFilePath(info.absFilePath());
                m_doc->setTitle(info.fileName());
            }
        } else {
            if (!filename.isEmpty()) {
                slotEnableTransport(false);
                m_doc->openDocument(filename);
                slotEnableTransport(true);
            }
        }

    QString caption=kapp->caption();
    setCaption(caption+": "+m_doc->getTitle());
}               

void RosegardenGUIApp::showEvent(QShowEvent* e)
{
    RG_DEBUG << "RosegardenGUIApp::showEvent()\n";

    m_transport->raise();
    KMainWindow::showEvent(e);
}

bool RosegardenGUIApp::queryClose()
{
    RG_DEBUG << "RosegardenGUIApp::queryClose" << endl;
#ifdef SETTING_LOG_DEBUG
    _settingLog(QString("SETTING 1 : transport flap extended = %1").arg(m_transport->isExpanded()));
    _settingLog(QString("SETTING 1 : show track labels = %1").arg(m_viewTrackLabels->isChecked()));
#endif

    bool canClose = m_doc->saveIfModified();

    /*
    if (canClose && m_transport) {

        // or else the closing of the transport will toggle off the 
        // 'view transport' action, and its state will be saved as 
        // 'off'
        //

        disconnect(m_transport, SIGNAL(closed()),
                   this, SLOT(slotCloseTransport()));
    }
    */

    return canClose;

}

bool RosegardenGUIApp::queryExit()
{
    RG_DEBUG << "RosegardenGUIApp::queryExit" << endl;
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
    KTmpStatusMsg msg(i18n("Opening a new application window..."), this);
        
    RosegardenGUIApp *new_window= new RosegardenGUIApp();
    new_window->show();
}

void RosegardenGUIApp::slotFileNew()
{
    RG_DEBUG << "RosegardenGUIApp::slotFileNew()\n";

    KTmpStatusMsg msg(i18n("Creating new document..."), this);

    bool makeNew = false;
    
    if (!m_doc->isModified()) {
        makeNew = true;
        // m_doc->closeDocument();
    } else if (m_doc->saveIfModified()) {
        makeNew = true;
    }

    if (makeNew) {

        setDocument(new RosegardenGUIDoc(this, m_pluginManager));
    }
}

void RosegardenGUIApp::slotOpenDroppedURL(QString url)
{
    RosegardenProgressDialog::processEvents(); // or else we get a crash because the
    // track editor is erased too soon - it is the originator of the signal
    // this slot is connected to.

    if (!m_doc->saveIfModified()) return;

    openURL(KURL(url));
}

void RosegardenGUIApp::openURL(QString url)
{
    RG_DEBUG << "RosegardenGUIApp::openURL: QString " << url << endl;
    openURL(KURL(url));
}

void RosegardenGUIApp::openURL(const KURL& url)
{
    SetWaitCursor waitCursor;

    QString netFile = url.prettyURL();
    RG_DEBUG << "RosegardenGUIApp::openURL: KURL " << netFile << endl;

    if (url.isMalformed()) {
        QString string;
        string = i18n( "Malformed URL\n%1").arg(netFile);

        KMessageBox::sorry(this, string);
        return;
    }

    QString target;

    if (KIO::NetAccess::download(url, target) == false) {
        KMessageBox::error(this, QString(i18n("Cannot download file %1"))
                           .arg(url.prettyURL()));
        return;
    }

    RG_DEBUG << "RosegardenGUIApp::openURL: target : " << target << endl;

    if (!m_doc->saveIfModified()) return;

    openFile(target);

    setCaption(url.path());
}

void RosegardenGUIApp::slotFileOpen()
{
    slotStatusHelpMsg(i18n("Opening file..."));

    KURL url = KFileDialog::getOpenURL
	(
#if KDE_VERSION >= 196614 
         ":ROSEGARDEN",
#else
         QString::null,
#endif
	 "audio/x-rosegarden audio/x-midi audio/x-rosegarden21", this,
	 i18n("Open File"));
    if ( url.isEmpty() ) { return; }

    if (m_doc) {
        
        if (!m_doc->saveIfModified()) {
            return;
        
        }
    }

    openURL(url);
}

void RosegardenGUIApp::slotMerge()
{
    KURL url = KFileDialog::getOpenURL
	(
#if KDE_VERSION >= 196614
         ":ROSEGARDEN",
#else
         QString::null,
#endif
	 "audio/x-rosegarden audio/x-midi audio/x-rosegarden21", this,
	 i18n("Open File"));
    if ( url.isEmpty() ) { return; }

    
    QString target;

    if (KIO::NetAccess::download(url, target) == false) {
        KMessageBox::error(this, QString(i18n("Cannot download file %1"))
                           .arg(url.prettyURL()));
        return;
    }

    mergeFile(target);

    KIO::NetAccess::removeTempFile( target );
}

void RosegardenGUIApp::slotFileOpenRecent(const KURL &url)
{
    KTmpStatusMsg msg(i18n("Opening file..."), this);

    if (m_doc) {
        
        if (!m_doc->saveIfModified()) {
            return;
        
        }
    }
    
    openURL(url);
}

void RosegardenGUIApp::slotFileSave()
{
    if (!m_doc || !m_doc->isModified()) return;

    KTmpStatusMsg msg(i18n("Saving file..."), this);

    // if it's a new file (no file path), or an imported file
    // (file path doesn't end with .rg), call saveAs
    //
    if (!m_doc->isRegularDotRGFile()) {

        slotFileSaveAs();

    } else {

        SetWaitCursor waitCursor;
        m_doc->saveDocument(m_doc->getAbsFilePath());

    }
}

QString
RosegardenGUIApp::getValidWriteFile(QString descriptiveExtension,
                                    QString label)
{
    // extract first extension listed in descriptiveExtension, for instance,
    // ".rg" from "*.rg|Rosegarden files", or ".mid" from "*.mid *.midi|MIDI Files"
    //
    QString extension = descriptiveExtension.left(descriptiveExtension.find('|')).mid(1).section(' ', 0, 0);

    RG_DEBUG << "RosegardenGUIApp::getValidWriteFile() : extension = " << extension << endl;

    // It's too bad there isn't this functionality within 
    // KFileDialog::getSaveFileName
    KFileDialog saveFileDialog(":ROSEGARDEN", descriptiveExtension, this, label, true);
    if (m_doc) {
        QString saveFileName=m_doc->getAbsFilePath();
        // Show filename without the old extension
        int dotLoc=saveFileName.findRev('.');
        if (dotLoc > int(saveFileName.length() - 4)) {
            saveFileName=saveFileName.left(dotLoc);
        }
        saveFileDialog.setSelection(saveFileName);
    }
    saveFileDialog.exec();
    QString name=saveFileDialog.selectedFile();

//     RG_DEBUG << "RosegardenGUIApp::getValidWriteFile() : KFileDialog::getSaveFileName returned "
//              << name << endl;
    

    if (name.isEmpty()) return name;

    // Append extension if we don't have one
    //
    if (!extension.isEmpty()) {
        static QRegExp rgFile("\\..{1,4}$");
        if (rgFile.match(name) == -1) {
            name += extension;
        }
    }

    KURL *u = new KURL(name);

    if (u->isMalformed()) {
        KMessageBox::sorry(this, i18n("This is not a valid filename.\n"));
        return "";
    }

    if (!u->isLocalFile()) {
        KMessageBox::sorry(this, i18n("This is not a local file.\n"));
        return "";
    }

    QFileInfo info(name);

    if (info.isDir()) {
        KMessageBox::sorry(this, i18n("You have specified a directory"));
        return "";
    }

    if (info.exists()) {
        int overwrite = KMessageBox::questionYesNo
            (this, i18n("The specified file exists.  Overwrite?"));
        
        if (overwrite != KMessageBox::Yes) return "";
    }

    return name;
}
    

void RosegardenGUIApp::slotFileSaveAs()
{
    if (!m_doc) return;

    KTmpStatusMsg msg(i18n("Saving file with a new filename..."), this);

    QString newName = getValidWriteFile("*.rg|" + i18n("Rosegarden-4 files") +
                                        "\n*|" + i18n("All files"),
                                        i18n("Save as..."));
    if (newName.isEmpty()) return;

    SetWaitCursor waitCursor;
    QFileInfo saveAsInfo(newName);
    m_doc->setTitle(saveAsInfo.fileName());
    m_doc->setAbsFilePath(saveAsInfo.absFilePath());
    m_doc->saveDocument(newName);
    m_fileRecent->addURL(newName);
    
    QString caption = kapp->caption();  
    setCaption(caption + ": " + m_doc->getTitle());
}

void RosegardenGUIApp::slotFileClose()
{
    RG_DEBUG << "RosegardenGUIApp::slotFileClose()" << endl;
    
    if (!m_doc) return;

    KTmpStatusMsg msg(i18n("Closing file..."), this);

    if (m_doc->saveIfModified()) {
        setDocument(new RosegardenGUIDoc(this, m_pluginManager));
    }
    
    // Don't close the whole view (i.e. Quit), just close the doc.
    //    close();
}

void RosegardenGUIApp::slotFilePrint()
{
    if (m_doc->getComposition().getNbSegments() == 0) {
        KMessageBox::sorry(0, "Please create some tracks first (until we implement menu state management)");
        return;
    }

    KTmpStatusMsg msg(i18n("Printing..."), this);

    m_view->print(&m_doc->getComposition());
}

void RosegardenGUIApp::slotFilePrintPreview()
{
    if (m_doc->getComposition().getNbSegments() == 0) {
        KMessageBox::sorry(0, "Please create some tracks first (until we implement menu state management)");
        return;
    }

    KTmpStatusMsg msg(i18n("Previewing..."), this);

    m_view->print(&m_doc->getComposition(), true);
}

void RosegardenGUIApp::slotQuit()
{
    slotStatusMsg(i18n("Exiting..."));

    Rosegarden::Profiles::getInstance()->dump();

    // close the first window, the list makes the next one the first again.
    // This ensures that queryClose() is called on each window to ask for closing
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
    KTmpStatusMsg msg(i18n("Cutting selection..."), this);

    Rosegarden::SegmentSelection selection(m_view->getSelection());
    m_doc->getCommandHistory()->addCommand
        (new CutCommand(selection, m_clipboard));
}

void RosegardenGUIApp::slotEditCopy()
{
    if (!m_view->haveSelection()) return;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), this);

    Rosegarden::SegmentSelection selection(m_view->getSelection());
    m_doc->getCommandHistory()->addCommand
        (new CopyCommand(selection, m_clipboard));
}

void RosegardenGUIApp::slotEditPaste()
{
    if (m_clipboard->isEmpty()) {
        KTmpStatusMsg msg(i18n("Clipboard is empty"), this);
        return;
    }
    KTmpStatusMsg msg(i18n("Inserting clipboard contents..."), this);

    // for now, but we could paste at the time of the first copied
    // segment and then do ghosting drag or something
    timeT insertionTime = m_doc->getComposition().getPosition();
    m_doc->getCommandHistory()->addCommand
        (new PasteSegmentsCommand(&m_doc->getComposition(),
                                  m_clipboard, insertionTime));

    // User preference? Update song pointer position on paste
    m_doc->setPointerPosition(m_doc->getComposition().getPosition());
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

    QuantizeDialog dialog(m_view);
    if (dialog.exec() != QDialog::Accepted) return;

    Rosegarden::SegmentSelection selection = m_view->getSelection();
    
    KMacroCommand *command = new KMacroCommand
        (EventQuantizeCommand::getGlobalName());

    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
         i != selection.end(); ++i) {
        command->addCommand(new EventQuantizeCommand
                            (**i, (*i)->getStartTime(), (*i)->getEndTime(),
                             dialog.getQuantizer()));
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenGUIApp::slotRepeatQuantizeSelection()
{
    if (!m_view->haveSelection()) return;

    //!!! this should all be in rosegardenguiview

    Rosegarden::SegmentSelection selection = m_view->getSelection();
    
    KMacroCommand *command = new KMacroCommand
        (EventQuantizeCommand::getGlobalName());

    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
         i != selection.end(); ++i) {
        command->addCommand(new EventQuantizeCommand
                            (**i, (*i)->getStartTime(), (*i)->getEndTime(),
			     "Quantize Dialog Grid", false)); // no i18n (config group name)
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenGUIApp::slotGrooveQuantize()
{
    if (!m_view->haveSelection()) return;

    Rosegarden::SegmentSelection selection = m_view->getSelection();

    if (selection.size() != 1) {
	KMessageBox::sorry(this, i18n("This function needs no more than one segment to be selected."));
	return;
    }
    
    Rosegarden::Segment *s = *selection.begin();
    m_view->slotAddCommandToHistory(new CreateTempoMapFromSegmentCommand(s));
}

void RosegardenGUIApp::slotJoinSegments()
{
    if (!m_view->haveSelection()) return;

    //!!! this should all be in rosegardenguiview
    //!!! should it?

    Rosegarden::SegmentSelection selection = m_view->getSelection();
    if (selection.size() == 0) return;

    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
         i != selection.end(); ++i) {
        if ((*i)->getType() != Rosegarden::Segment::Internal) {
            KMessageBox::sorry(this, i18n("Can't join Audio segments"));
            return;
        }
    }

    m_view->slotAddCommandToHistory(new SegmentMergeCommand(selection));
}

void RosegardenGUIApp::slotRescaleSelection()
{
    if (!m_view->haveSelection()) return;

    //!!! this should all be in rosegardenguiview
    //!!! should it?

    Rosegarden::SegmentSelection selection = m_view->getSelection();

    Rosegarden::timeT startTime = 0, endTime = 0;
    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
         i != selection.end(); ++i) {
	if ((i == selection.begin()) || ((*i)->getStartTime() < startTime)) {
	    startTime = (*i)->getStartTime();
	}
	if ((i == selection.begin()) || ((*i)->getEndMarkerTime() > endTime)) {
	    endTime = (*i)->getEndMarkerTime();
	}
    }

    RescaleDialog dialog(m_view, &m_doc->getComposition(),
			 startTime, endTime - startTime,
			 false);
    if (dialog.exec() != QDialog::Accepted) return;

    KMacroCommand *command = new KMacroCommand
        (SegmentRescaleCommand::getGlobalName());

    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
         i != selection.end(); ++i) {
        command->addCommand(new SegmentRescaleCommand(*i,
						      dialog.getNewDuration(),
                                                      endTime - startTime));
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

        if ((*i)->getType() == Rosegarden::Segment::Audio)
        {
            AudioSplitDialog aSD(this, (*i), m_doc);

            if (aSD.exec() == QDialog::Accepted)
            {
                // split to threshold
                //
                command->addCommand(
                        new AudioSegmentAutoSplitCommand(m_doc,
                                                         *i,
                                                         aSD.getThreshold()));
            }
        } else {
            command->addCommand(new SegmentAutoSplitCommand(*i));
        }
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenGUIApp::slotJogLeft()
{
    RG_DEBUG << "RosegardenGUIApp::slotJogLeft" << endl;
    jogSelection(-Rosegarden::Note(Rosegarden::Note::Demisemiquaver).getDuration());
}

void RosegardenGUIApp::slotJogRight()
{
    RG_DEBUG << "RosegardenGUIApp::slotJogRight" << endl;
    jogSelection(Rosegarden::Note(Rosegarden::Note::Demisemiquaver).getDuration());
}

void RosegardenGUIApp::jogSelection(Rosegarden::timeT amount)
{
    if (!m_view->haveSelection()) return;

    Rosegarden::SegmentSelection selection = m_view->getSelection();

    SegmentReconfigureCommand *command = new SegmentReconfigureCommand(i18n("Jog Selection"));

    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
         i != selection.end(); ++i) {

        command->addSegment((*i),
                            (*i)->getStartTime() + amount,
                            (*i)->getEndTime(),
                            (*i)->getTrack());
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenGUIApp::slotSplitSelectionByPitch()
{
    if (!m_view->haveSelection()) return;

    SplitByPitchDialog dialog(m_view);
    if (dialog.exec() != QDialog::Accepted) return;

    Rosegarden::SegmentSelection selection = m_view->getSelection();

    KMacroCommand *command = new KMacroCommand
        (SegmentSplitByPitchCommand::getGlobalName());

    bool haveSomething = false;

    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
         i != selection.end(); ++i) {

        if ((*i)->getType() == Rosegarden::Segment::Audio) {
            // nothing
        } else {
            command->addCommand
                (new SegmentSplitByPitchCommand
                 (*i,
                  dialog.getPitch(),
                  dialog.getShouldRange(),
                  dialog.getShouldDuplicateNonNoteEvents(),
                  (SegmentSplitByPitchCommand::ClefHandling)
                  dialog.getClefHandling()));
            haveSomething = true;
        }
    }

    if (haveSomething) m_view->slotAddCommandToHistory(command);
    //!!! else complain
}


void
RosegardenGUIApp::slotSetSegmentStartTimes()
{
    if (!m_view->haveSelection()) return;

    Rosegarden::SegmentSelection selection = m_view->getSelection();
    if (selection.empty()) return;

    Rosegarden::timeT someTime = (*selection.begin())->getStartTime();

    TimeDialog dialog(m_view, i18n("Segment Start Time"),
		      &m_doc->getComposition(),
		      someTime);

    if (dialog.exec() == QDialog::Accepted) {

	bool plural = (selection.size() > 1);
	
	SegmentReconfigureCommand *command =
	    new SegmentReconfigureCommand(plural ?
					  i18n("Set Segment Start Times") :
					  i18n("Set Segment Start Time"));
	
	for (Rosegarden::SegmentSelection::iterator i = selection.begin();
	     i != selection.end(); ++i) {

	    command->addSegment
		(*i, dialog.getTime(),
		 (*i)->getEndMarkerTime() - (*i)->getStartTime() + dialog.getTime(),
		 (*i)->getTrack());
	}

        m_view->slotAddCommandToHistory(command);
    }
}


void
RosegardenGUIApp::slotSetSegmentDurations()
{
    if (!m_view->haveSelection()) return;

    Rosegarden::SegmentSelection selection = m_view->getSelection();
    if (selection.empty()) return;

    Rosegarden::timeT someTime =
	(*selection.begin())->getStartTime();

    Rosegarden::timeT someDuration =
	(*selection.begin())->getEndMarkerTime() -
	(*selection.begin())->getStartTime();

    TimeDialog dialog(m_view, i18n("Segment Duration"),
		      &m_doc->getComposition(),
		      someTime,
		      someDuration);

    if (dialog.exec() == QDialog::Accepted) {

	bool plural = (selection.size() > 1);
	
	SegmentReconfigureCommand *command =
	    new SegmentReconfigureCommand(plural ?
					  i18n("Set Segment Durations") :
					  i18n("Set Segment Duration"));
	
	for (Rosegarden::SegmentSelection::iterator i = selection.begin();
	     i != selection.end(); ++i) {

	    command->addSegment
		(*i, (*i)->getStartTime(),
		 (*i)->getStartTime() + dialog.getTime(),
		 (*i)->getTrack());
	}

        m_view->slotAddCommandToHistory(command);
    }
}


void RosegardenGUIApp::slotHarmonizeSelection()
{
    if (!m_view->haveSelection()) return;

    Rosegarden::SegmentSelection selection = m_view->getSelection();
    //!!! This should be somewhere else too

    Rosegarden::CompositionTimeSliceAdapter adapter(&m_doc->getComposition(),
                                                    &selection);

    Rosegarden::AnalysisHelper helper;
    Rosegarden::Segment *segment = new Rosegarden::Segment;
    helper.guessHarmonies(adapter, *segment);

    //!!! do nothing with the results yet
    delete segment;
}

void RosegardenGUIApp::slotTempoToSegmentLength()
{
    std::cout << "RosegardenGUIApp::slotTempoToSegmentLength" << std::endl;

    if (!m_view->haveSelection()) return;

    Rosegarden::SegmentSelection selection = m_view->getSelection();

    // Only set for a single selection
    //
    if (selection.size() == 1 &&
            (*selection.begin())->getType() == Rosegarden::Segment::Audio)
    {
        Rosegarden::Composition &comp = m_doc->getComposition();
        Rosegarden::Segment *seg = *selection.begin();

        Rosegarden::TimeSignature timeSig =
            comp.getTimeSignatureAt( seg->getStartTime());
        int beats = timeSig.getBeatsPerBar();

        timeT endTime = seg->getEndTime();

        if (seg->getRawEndMarkerTime())
            endTime = seg->getEndMarkerTime();

        Rosegarden::RealTime segDuration =
            seg->getAudioEndTime() - seg->getAudioStartTime();

        double beatLengthUsec =
            double(segDuration.sec * 1000000 + segDuration.usec()) /
            double(beats);

        // New tempo is a minute divided by time of beat
        //
        double newTempo = 60.0 * 1000000.0 / beatLengthUsec;

        KMacroCommand *macro = new KMacroCommand(i18n("Set Global Tempo"));

        // Remove all tempo changes in reverse order so as the index numbers
        // don't becoming meaningless as the command gets unwound.
        //
        for (int i = 0; i < comp.getTempoChangeCount(); i++)
            macro->addCommand(new RemoveTempoChangeCommand(&comp,
                                  (comp.getTempoChangeCount() - 1 - i)));

        // add tempo change at time zero
        //
        macro->addCommand(new AddTempoChangeCommand(&comp, 0, newTempo));

        // execute
        m_doc->getCommandHistory()->addCommand(macro);
    }

}



void RosegardenGUIApp::slotEdit()
{
    m_view->slotEditSegment(0);
}

void RosegardenGUIApp::slotEditAsNotation()
{
    m_view->slotEditSegmentNotation(0);
}

void RosegardenGUIApp::slotEditInMatrix()
{
    m_view->slotEditSegmentMatrix(0);
}

void RosegardenGUIApp::slotEditInEventList()
{
    m_view->slotEditSegmentEventList(0);
}

void RosegardenGUIApp::slotEditTempos()
{
    m_view->slotEditTempos(m_doc->getComposition().getPosition());
}

void RosegardenGUIApp::slotEditTempos(Rosegarden::timeT openAtTime)
{
    m_view->slotEditTempos(openAtTime);
}

void RosegardenGUIApp::slotToggleToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the toolbar..."), this);

    if (m_viewToolBar->isChecked())
        toolBar("mainToolBar")->show();
    else
        toolBar("mainToolBar")->hide();
}

void RosegardenGUIApp::slotToggleToolsToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the tools toolbar..."), this);

    if (m_viewToolsToolBar->isChecked())
        toolBar("Tools Toolbar")->show();
    else
        toolBar("Tools Toolbar")->hide();
}

void RosegardenGUIApp::slotToggleTracksToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the tracks toolbar..."), this);

    if (m_viewTracksToolBar->isChecked())
        toolBar("Tracks Toolbar")->show();
    else
        toolBar("Tracks Toolbar")->hide();
}


void RosegardenGUIApp::slotToggleEditorsToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the editor toolbar..."), this);

    if (m_viewEditorsToolBar->isChecked())
        toolBar("Editors Toolbar")->show();
    else
        toolBar("Editors Toolbar")->hide();
}

void RosegardenGUIApp::slotToggleTransportToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the transport toolbar..."), this);

    if (m_viewTransportToolBar->isChecked())
        toolBar("Transport Toolbar")->show();
    else
        toolBar("Transport Toolbar")->hide();
}

void RosegardenGUIApp::slotToggleZoomToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the zoom toolbar..."), this);

    if (m_viewZoomToolBar->isChecked())
        toolBar("Zoom Toolbar")->show();
    else
        toolBar("Zoom Toolbar")->hide();
}

void RosegardenGUIApp::slotToggleTransport()
{
    KTmpStatusMsg msg(i18n("Toggle the Transport"), this);

    if (m_viewTransport->isChecked())
    {
        m_transport->show();
        m_transport->raise();
        m_transport->blockSignals(false);
    }
    else
    {
        m_transport->hide();
        m_transport->blockSignals(true);
    }
}

void RosegardenGUIApp::slotToggleTrackLabels()
{
    if (m_viewTrackLabels->isChecked())
    {
#ifdef SETTING_LOG_DEBUG
        _settingLog("toggle track labels on");
#endif
        m_view->getTrackEditor()->getTrackButtons()->
            changeTrackInstrumentLabels(TrackLabel::ShowTrack);
    }
    else
    {
#ifdef SETTING_LOG_DEBUG
        _settingLog("toggle track labels off");
#endif
        m_view->getTrackEditor()->getTrackButtons()->
            changeTrackInstrumentLabels(TrackLabel::ShowInstrument);
    }
}

void RosegardenGUIApp::slotToggleRulers()
{
    m_view->slotShowRulers(m_viewRulers->isChecked());
}

void RosegardenGUIApp::slotToggleTempoRuler()
{
    m_view->slotShowTempoRuler(m_viewTempoRuler->isChecked());
}

void RosegardenGUIApp::slotToggleChordNameRuler()
{
    m_view->slotShowChordNameRuler(m_viewChordNameRuler->isChecked());
}

void RosegardenGUIApp::slotTogglePreviews()
{
    m_view->slotShowPreviews(m_viewPreviews->isChecked());
}

void RosegardenGUIApp::slotDockParametersBack()
{
    m_dockLeft->dockBack();
}

void RosegardenGUIApp::slotParametersClosed()
{
    stateChanged("parametersbox_closed");
    m_dockVisible = false;
}

void RosegardenGUIApp::slotParametersDockedBack(KDockWidget* dw, KDockWidget::DockPosition)
{
    if (dw == m_dockLeft) {
        stateChanged("parametersbox_closed", KXMLGUIClient::StateReverse);
        m_dockVisible = true;
    }
}

/* dmm - commenting this out for impending removal...
void RosegardenGUIApp::slotToggleAll()
{
    int c = 0;

    c += m_viewTransport->isChecked();
    c += m_viewRulers->isChecked();
    c += m_viewChordNameRuler->isChecked();
    c += m_viewTempoRuler->isChecked();
    c += m_viewPreviews->isChecked();
    c += m_viewTrackLabels->isChecked();

    bool state = false;
    
    if (c < 4) state = true;

    m_viewTransport->setChecked(state);
    slotToggleTransport();
    
    m_viewRulers->setChecked(state);
    slotToggleRulers();
   
    m_viewChordNameRuler->setChecked(state);
    slotToggleChordNameRuler();
    
    m_viewTempoRuler->setChecked(state);
    slotToggleTempoRuler();
    
    m_viewPreviews->setChecked(state);
    slotTogglePreviews();
    
    m_viewTrackLabels->setChecked(state);
    slotToggleTrackLabels();
} */


void RosegardenGUIApp::slotToggleStatusBar()
{
    KTmpStatusMsg msg(i18n("Toggle the statusbar..."), this);

    if(!m_viewStatusBar->isChecked())
        statusBar()->hide();
    else
        statusBar()->show();
}


void RosegardenGUIApp::slotStatusMsg(QString text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->changeItem(text, ID_STATUS_MSG);
}


void RosegardenGUIApp::slotStatusHelpMsg(QString text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}

void RosegardenGUIApp::slotEnableTransport(bool enable)
{
    if (m_transport)
        m_transport->setEnabled(enable);
}

void RosegardenGUIApp::slotPointerSelected()
{
    m_view->selectTool(SegmentSelector::ToolName);
}

void RosegardenGUIApp::slotEraseSelected()
{
    m_view->selectTool(SegmentEraser::ToolName);
}

void RosegardenGUIApp::slotDrawSelected()
{
    m_view->selectTool(SegmentPencil::ToolName);
}

void RosegardenGUIApp::slotMoveSelected()
{
    m_view->selectTool(SegmentMover::ToolName);
}

void RosegardenGUIApp::slotResizeSelected()
{
    m_view->selectTool(SegmentResizer::ToolName);
}

void RosegardenGUIApp::slotJoinSelected()
{
    KMessageBox::information(this,
        i18n("The join tool isn't implemented yet.  Instead please highlight "
             "the segments you want to join and then use the menu option:\n\n"
             "        Segments->Collapse Segments.\n"),
        i18n("Join tool not yet implemented"));
                 
    m_view->selectTool(SegmentJoiner::ToolName);
}

void RosegardenGUIApp::slotSplitSelected()
{
    m_view->selectTool(SegmentSplitter::ToolName);
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


void RosegardenGUIApp::slotAddTrack()
{
    if (!m_view) return;

    // default to the base number - might not actually exist though
    //
    Rosegarden::InstrumentId id = Rosegarden::MidiInstrumentBase;

    // Get the first Internal/MIDI instrument
    //
    Rosegarden::DeviceList *devices = m_doc->getStudio().getDevices();
    bool have = false;

    for (Rosegarden::DeviceList::iterator it = devices->begin();
	 it != devices->end() && !have; it++) {

        if ((*it)->getType() != Rosegarden::Device::Midi) continue;

	Rosegarden::InstrumentList instruments = (*it)->getAllInstruments();
	for (Rosegarden::InstrumentList::iterator iit = instruments.begin();
	     iit != instruments.end(); iit++) {

	    if ((*iit)->getId() >= Rosegarden::MidiInstrumentBase) {
		id = (*iit)->getId();
		have = true;
		break;
	    }
        }
    }

    m_view->slotAddTracks(1, id);
}

void RosegardenGUIApp::slotAddTracks()
{
    if (!m_view) return;

    // default to the base number - might not actually exist though
    //
    Rosegarden::InstrumentId id = Rosegarden::MidiInstrumentBase;

    // Get the first Internal/MIDI instrument
    //
    Rosegarden::DeviceList *devices = m_doc->getStudio().getDevices();
    bool have = false;

    for (Rosegarden::DeviceList::iterator it = devices->begin();
	 it != devices->end() && !have; it++) {

        if ((*it)->getType() != Rosegarden::Device::Midi) continue;

	Rosegarden::InstrumentList instruments = (*it)->getAllInstruments();
	for (Rosegarden::InstrumentList::iterator iit = instruments.begin();
	     iit != instruments.end(); iit++) {

	    if ((*iit)->getId() >= Rosegarden::MidiInstrumentBase) {
		id = (*iit)->getId();
		have = true;
		break;
	    }
        }
    }

    bool ok = false;

    int tracks = QInputDialog::getInteger(
                i18n("How many tracks do you want to add? (max. 24)"),
                i18n("Tracks to add"),
                1,
                1,
                24,
                1,
                &ok,
                this);

    // create tracks if ok
    //
    if (ok) m_view->slotAddTracks(tracks, id);
}

void RosegardenGUIApp::slotDeleteTrack()
{
    if (!m_view) return;

    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::TrackId trackId = comp.getSelectedTrack();
    Rosegarden::Track *track = comp.getTrackById(trackId);

    if (track == 0) return;

    // Always have at least one track in a composition
    //
    if (comp.getNbTracks() == 1) return;

    int position = track->getPosition();

    // Delete the track
    //
    std::vector<Rosegarden::TrackId> tracks;
    tracks.push_back(trackId);

    m_view->slotDeleteTracks(tracks);

    // Select a new valid track
    //
    if (comp.getTrackByPosition(position))
        trackId = comp.getTrackByPosition(position)->getId();
    else if (comp.getTrackByPosition(position - 1))
        trackId = comp.getTrackByPosition(position - 1)->getId();
    else 
    {
        RG_DEBUG << "RosegardenGUIApp::slotDeleteTrack - "
                 << "can't select a highlighted track after delete"
                 << endl;
    }

    comp.setSelectedTrack(trackId);

    Rosegarden::Instrument *inst = m_doc->getStudio().
        getInstrumentById(comp.getTrackById(trackId)->getInstrument());

    if (inst && inst->getType() == Rosegarden::Instrument::Midi)
        comp.setRecordTrack(trackId);

    m_view->slotSelectTrackSegments(trackId);
}

void RosegardenGUIApp::slotMoveTrackDown()
{
    RG_DEBUG << "RosegardenGUIApp::slotMoveTrackDown" << endl;

    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *srcTrack = comp.getTrackById(comp.getSelectedTrack());

    // Check for track object
    //
    if (srcTrack == 0) return;

    // Check destination track exists
    //
    Rosegarden::Track *destTrack =
        comp.getTrackByPosition(srcTrack->getPosition() + 1);

    if (destTrack == 0) return;

    MoveTracksCommand *command =
        new MoveTracksCommand(&comp, srcTrack->getId(), destTrack->getId());

    m_doc->getCommandHistory()->addCommand(command);

    // make sure we're showing the right selection
    m_view->slotSelectTrackSegments(comp.getSelectedTrack());

}

void RosegardenGUIApp::slotMoveTrackUp()
{
    RG_DEBUG << "RosegardenGUIApp::slotMoveTrackUp" << endl;

    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *srcTrack = comp.getTrackById(comp.getSelectedTrack());

    // Check for track object
    //
    if (srcTrack == 0) return;

    // Check we're not at the top already
    //
    if (srcTrack->getPosition() == 0) return;

    // Check destination track exists
    //
    Rosegarden::Track *destTrack =
        comp.getTrackByPosition(srcTrack->getPosition() - 1);

    if (destTrack == 0) return;

    MoveTracksCommand *command =
        new MoveTracksCommand(&comp, srcTrack->getId(), destTrack->getId());

    m_doc->getCommandHistory()->addCommand(command);

    // make sure we're showing the right selection
    m_view->slotSelectTrackSegments(comp.getSelectedTrack());
}

void RosegardenGUIApp::slotRevertToSaved()
{
    RG_DEBUG << "RosegardenGUIApp::slotRevertToSaved" << endl;

    if(m_doc->isModified())
    {
        int revert =
            KMessageBox::questionYesNo(this,
                   i18n("Revert modified document to previous saved version?"));

        if (revert == KMessageBox::No) return;

        openFile(m_doc->getAbsFilePath());
    }
}

void RosegardenGUIApp::slotImportMIDI()
{
    KURL url = KFileDialog::getOpenURL
        (
#if KDE_VERSION >= 196614
         ":MIDI",
#else
         QString::null,
#endif
         "audio/x-midi", this,
         i18n("Open MIDI File"));
    if (url.isEmpty()) { return; }

    QString tmpfile;
    KIO::NetAccess::download(url, tmpfile);
    openFile(tmpfile, ImportMIDI); // does everything including setting the document

    KIO::NetAccess::removeTempFile( tmpfile );
}

void RosegardenGUIApp::slotMergeMIDI()
{
    KURL url = KFileDialog::getOpenURL
        (
#if KDE_VERSION >= 196614
         ":MIDI",
#else
         QString::null,
#endif
         "audio/x-midi", this,
         i18n("Merge MIDI File"));
    if (url.isEmpty()) { return; }

    QString tmpfile;
    KIO::NetAccess::download(url, tmpfile);
    mergeFile(tmpfile, ImportMIDI);

    KIO::NetAccess::removeTempFile( tmpfile );
}

RosegardenGUIDoc*
RosegardenGUIApp::createDocumentFromMIDIFile(QString file)
{
    //if (!merge && !m_doc->saveIfModified()) return;

    // Create new document (autoload is inherent)
    //
    RosegardenGUIDoc *newDoc = new RosegardenGUIDoc(this, m_pluginManager);

    std::string fname(QFile::encodeName(file));

    Rosegarden::MidiFile midiFile(fname,
                                  &newDoc->getStudio());

    KStartupLogo::hideIfStillThere();
    RosegardenProgressDialog progressDlg(i18n("Importing MIDI file..."),
                                         200,
                                         this);

    CurrentProgressDialog::set(&progressDlg);

    connect(&midiFile, SIGNAL(setProgress(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));

    connect(&midiFile, SIGNAL(incrementProgress(int)),
            progressDlg.progressBar(), SLOT(advance(int)));

    if (!midiFile.open())
    {
        CurrentProgressDialog::freeze();
        KMessageBox::error(this, strtoqstr(midiFile.getError())); //!!! i18n
        delete newDoc;
        return 0;
    }

    midiFile.convertToRosegarden(newDoc->getComposition(),
                                 Rosegarden::MidiFile::CONVERT_REPLACE);

    // Set modification flag
    //
    newDoc->slotDocumentModified();

    // Set the caption
    //
    newDoc->setTitle(QFileInfo(file).fileName());
    newDoc->setAbsFilePath(QFileInfo(file).absFilePath());

    // Clean up for notation purposes (after reinitialise, because that
    // sets the composition's end marker time which is needed here)

    progressDlg.slotSetOperationName(i18n("Calculating notation..."));
    RosegardenProgressDialog::processEvents();

    Rosegarden::Composition *comp = &newDoc->getComposition();

    for (Rosegarden::Composition::iterator i = comp->begin();
         i != comp->end(); ++i) {

        Rosegarden::Segment &segment = **i;
        Rosegarden::SegmentNotationHelper helper(segment);
        segment.insert(helper.guessClef(segment.begin(),
                                        segment.getEndMarker()).getAsEvent
                       (segment.getStartTime()));
    }

    progressDlg.progressBar()->setProgress(100);

    for (Rosegarden::Composition::iterator i = comp->begin();
         i != comp->end(); ++i) {

        // find first key event in each segment (we'd have done the
        // same for clefs, except there is no MIDI clef event)

        Rosegarden::Segment &segment = **i;
        Rosegarden::timeT firstKeyTime = segment.getEndMarkerTime();

        for (Rosegarden::Segment::iterator si = segment.begin();
             segment.isBeforeEndMarker(si); ++si) {
            if ((*si)->isa(Rosegarden::Key::EventType)) {
                firstKeyTime = (*si)->getAbsoluteTime();
                break;
            }
        }

        if (firstKeyTime > segment.getStartTime()) {
            Rosegarden::CompositionTimeSliceAdapter adapter
                (comp, Rosegarden::timeT(0), firstKeyTime);
            Rosegarden::AnalysisHelper helper;
            segment.insert(helper.guessKey(adapter).getAsEvent
                           (segment.getStartTime()));
        }
    }

    int progressPer = 100;
    if (comp->getNbSegments() > 0)
        progressPer = (int)(100.0 / double(comp->getNbSegments()));

    KMacroCommand *command = new KMacroCommand(i18n("Calculate Notation"));

    for (Rosegarden::Composition::iterator i = comp->begin();
         i != comp->end(); ++i) {

        Rosegarden::Segment &segment = **i;
        Rosegarden::timeT startTime(segment.getStartTime());
        Rosegarden::timeT endTime(segment.getEndMarkerTime());

        RG_DEBUG << "segment: start time " << segment.getStartTime() << ", end time " << segment.getEndTime() << ", end marker time " << segment.getEndMarkerTime() << ", events " << segment.size() << endl;

	EventQuantizeCommand *subCommand = new EventQuantizeCommand
	    (segment, startTime, endTime, "Notation Options", true);

	subCommand->setProgressTotal(progressPer + 1);
	QObject::connect(subCommand, SIGNAL(incrementProgress(int)),
			 progressDlg.progressBar(), SLOT(advance(int)));

	command->addCommand(subCommand);
    }

    newDoc->getCommandHistory()->addCommand(command);

    if (comp->getTimeSignatureCount() == 0) {
        Rosegarden::CompositionTimeSliceAdapter adapter(comp);
        Rosegarden::AnalysisHelper analysisHelper;
        Rosegarden::TimeSignature timeSig =
            analysisHelper.guessTimeSignature(adapter);
        comp->addTimeSignature(0, timeSig);
    }

    return newDoc;
}

void RosegardenGUIApp::slotImportRG21()
{
    if (!m_doc->saveIfModified()) return;

    KURL url = KFileDialog::getOpenURL
        (
#if KDE_VERSION >= 196614
         ":ROSEGARDEN21",
#else
         QString::null,
#endif
         i18n("*.rose|Rosegarden-2 files\n*|All files"), this,
         i18n("Open Rosegarden 2.1 File"));
    if (url.isEmpty()) { return; }

    QString tmpfile;
    KIO::NetAccess::download(url, tmpfile);
    openFile(tmpfile, ImportRG21);

    KIO::NetAccess::removeTempFile(tmpfile);
}

void RosegardenGUIApp::slotMergeRG21()
{
    KURL url = KFileDialog::getOpenURL
        (
#if KDE_VERSION >= 196614
         ":ROSEGARDEN21",
#else
         QString::null,
#endif
         i18n("*.rose|Rosegarden-2 files\n*|All files"), this,
         i18n("Open Rosegarden 2.1 File"));
    if (url.isEmpty()) { return; }

    QString tmpfile;
    KIO::NetAccess::download(url, tmpfile);
    mergeFile(tmpfile, ImportRG21);

    KIO::NetAccess::removeTempFile( tmpfile );
}

RosegardenGUIDoc*
RosegardenGUIApp::createDocumentFromRG21File(QString file)
{
    KStartupLogo::hideIfStillThere();
    RosegardenProgressDialog progressDlg(
            i18n("Importing Rosegarden 2.1 file..."), 100, this);

    CurrentProgressDialog::set(&progressDlg);

    // Inherent autoload
    //
    RosegardenGUIDoc *newDoc = new RosegardenGUIDoc(this, m_pluginManager);

    RG21Loader rg21Loader(&newDoc->getStudio());

    // TODO: make RG21Loader to actually emit these signals
    //
    connect(&rg21Loader, SIGNAL(setProgress(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));

    connect(&rg21Loader, SIGNAL(incrementProgress(int)),
            progressDlg.progressBar(), SLOT(advance(int)));

    // "your starter for 40%" - helps the "freeze" work
    //
    progressDlg.progressBar()->advance(40);

    if (!rg21Loader.load(file, newDoc->getComposition()))
    {
	CurrentProgressDialog::freeze();
	KMessageBox::error(this,
			   i18n("Can't load Rosegarden 2.1 file.  It appears to be corrupted."));
	delete newDoc;
	return 0;
    }

    // Set modification flag
    //
    newDoc->slotDocumentModified();

    // Set the caption and add recent
    //
    newDoc->setTitle(QFileInfo(file).fileName());
    newDoc->setAbsFilePath(QFileInfo(file).absFilePath());

    return newDoc;

}

void
RosegardenGUIApp::slotImportHydrogen()
{
    if (!m_doc->saveIfModified()) return;

    KURL url = KFileDialog::getOpenURL
        (
#if KDE_VERSION >= 196614
         ":HYDROGEN",
#else
         QString::null,
#endif
         i18n("*.h2song|Hydrogen files\n*|All files"), this,
         i18n("Open Hydrogen File"));
    if (url.isEmpty()) { return; }

    QString tmpfile;
    KIO::NetAccess::download(url, tmpfile);
    openFile(tmpfile, ImportHydrogen);

    KIO::NetAccess::removeTempFile(tmpfile);
}

void RosegardenGUIApp::slotMergeHydrogen()
{
    KURL url = KFileDialog::getOpenURL
        (
#if KDE_VERSION >= 196614
         ":HYDROGEN",
#else
         QString::null,
#endif
         i18n("*.h2song|Hydrogen files\n*|All files"), this,
         i18n("Open Hydrogen File"));
    if (url.isEmpty()) { return; }

    QString tmpfile;
    KIO::NetAccess::download(url, tmpfile);
    mergeFile(tmpfile, ImportHydrogen);

    KIO::NetAccess::removeTempFile( tmpfile );
}

RosegardenGUIDoc*
RosegardenGUIApp::createDocumentFromHydrogenFile(QString file)
{
    KStartupLogo::hideIfStillThere();
    RosegardenProgressDialog progressDlg(
            i18n("Importing Hydrogen file..."), 100, this);

    CurrentProgressDialog::set(&progressDlg);

    // Inherent autoload
    //
    RosegardenGUIDoc *newDoc = new RosegardenGUIDoc(this, m_pluginManager);

    HydrogenLoader hydrogenLoader(&newDoc->getStudio());

    // TODO: make RG21Loader to actually emit these signals
    //
    connect(&hydrogenLoader, SIGNAL(setProgress(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));

    connect(&hydrogenLoader, SIGNAL(incrementProgress(int)),
            progressDlg.progressBar(), SLOT(advance(int)));

    // "your starter for 40%" - helps the "freeze" work
    //
    progressDlg.progressBar()->advance(40);

    if (!hydrogenLoader.load(file, newDoc->getComposition()))
    {
	CurrentProgressDialog::freeze();
	KMessageBox::error(this,
			   i18n("Can't load Hydrogen file.  It appears to be corrupted."));
	delete newDoc;
	return 0;
    }

    // Set modification flag
    //
    newDoc->slotDocumentModified();

    // Set the caption and add recent
    //
    newDoc->setTitle(QFileInfo(file).fileName());
    newDoc->setAbsFilePath(QFileInfo(file).absFilePath());

    return newDoc;

}

void
RosegardenGUIApp::mergeFile(QString filePath, ImportType type)
{
    RosegardenGUIDoc *doc = createDocument(filePath, type);

    if (doc) {
	if (m_doc) {

	    bool timingsDiffer = false;
	    Rosegarden::Composition &c1 = m_doc->getComposition();
	    Rosegarden::Composition &c2 =   doc->getComposition();

	    // compare tempos and time sigs in the two -- rather laborious

	    if (c1.getTimeSignatureCount() != c2.getTimeSignatureCount()) {
		timingsDiffer = true;
	    } else {
		for (int i = 0; i < c1.getTimeSignatureCount(); ++i) {
		    std::pair<timeT, Rosegarden::TimeSignature> t1 =
			c1.getTimeSignatureChange(i);
		    std::pair<timeT, Rosegarden::TimeSignature> t2 =
			c2.getTimeSignatureChange(i);
		    if (t1.first != t2.first || t1.second != t2.second) {
			timingsDiffer = true;
			break;
		    }
		}
	    }

	    if (c1.getTempoChangeCount() != c2.getTempoChangeCount()) {
		timingsDiffer = true;
	    } else {
		for (int i = 0; i < c1.getTempoChangeCount(); ++i) {
		    std::pair<timeT, long> t1 = c1.getRawTempoChange(i);
		    std::pair<timeT, long> t2 = c2.getRawTempoChange(i);
		    if (t1.first != t2.first || t1.second != t2.second) {
			timingsDiffer = true;
			break;
		    }
		}
	    }

	    FileMergeDialog dialog(this, filePath, timingsDiffer);
	    if (dialog.exec() == QDialog::Accepted) {
		m_doc->mergeDocument(doc, dialog.getMergeOptions());
	    }

	    delete doc;

	} else {
	    setDocument(doc);
	}
    }
}

void
RosegardenGUIApp::slotUpdatePlaybackPosition()
{
    // Either sequencer mappper or the sequence manager could be missing at
    // this point.
    //
    if (!m_seqManager || !m_seqManager->getSequencerMapper()) return;

    SequencerMapper *mapper = m_seqManager->getSequencerMapper();

    Rosegarden::MappedEvent ev;
    bool haveEvent = mapper->getVisual(ev);
    if (haveEvent) m_transport->setMidiOutLabel(&ev);

    Rosegarden::RealTime position = mapper->getPositionPointer();

    Rosegarden::Composition &comp = m_doc->getComposition();
    timeT elapsedTime = comp.getElapsedTimeForRealTime(position);

    m_originatingJump = true;
    m_doc->setPointerPosition(elapsedTime);
    m_originatingJump = false;

    if (m_seqManager->getTransportStatus() == RECORDING_MIDI) {

	Rosegarden::MappedComposition mC;
	if (mapper->getRecordedEvents(mC) > 0) {
	    m_seqManager->processAsynchronousMidi(mC, 0);
	    m_doc->insertRecordedMidi(mC, RECORDING_MIDI);
	}
	m_doc->updateRecordingSegment();

    } else if (m_seqManager->getTransportStatus() == RECORDING_AUDIO) {

	m_doc->insertRecordedAudio(position, RECORDING_AUDIO);
    }

    if (m_audioMixer && m_audioMixer->isVisible())
        m_audioMixer->updateMeters(mapper);

    if (m_midiMixer && m_midiMixer->isVisible())
        m_midiMixer->updateMeters(mapper);

    m_view->updateMeters(mapper);

    if (elapsedTime >= comp.getEndMarker())
        slotStop();
}

void
RosegardenGUIApp::slotUpdateMonitoring()
{
    // Either sequencer mappper or the sequence manager could be missing at
    // this point.
    //
    if (!m_seqManager || !m_seqManager->getSequencerMapper()) return;

    SequencerMapper *mapper = m_seqManager->getSequencerMapper();

    if (m_audioMixer && m_audioMixer->isVisible())
        m_audioMixer->updateMonitorMeter(mapper);

    if (m_midiMixer && m_midiMixer->isVisible())
        m_midiMixer->updateMonitorMeter(mapper);
}

void RosegardenGUIApp::slotSetPointerPosition(timeT t)
{
    if (!m_seqManager) return;

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

    // and the time sig
    m_transport->setTimeSignature(comp.getTimeSignatureAt(t));

    // and the tempo
    m_transport->setTempo(comp.getTempoAt(t));

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

    // Update position on the marker editor if it's available
    //
    if (m_markerEditor) m_markerEditor->updatePosition();
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

    // show units in hemidemis (or whatever), not in raw time ticks
    unitNo /= Rosegarden::Note(Rosegarden::Note::Shortest).getDuration();
    
    m_transport->displayBarTime(barNo, beatNo, unitNo);
}


void RosegardenGUIApp::slotRefreshTimeDisplay()
{
    if ( m_seqManager->getTransportStatus() == PLAYING ||
         m_seqManager->getTransportStatus() == RECORDING_MIDI ||
         m_seqManager->getTransportStatus() == RECORDING_AUDIO ) {
	return; // it'll be refreshed in a moment anyway
    }
    slotSetPointerPosition(m_doc->getComposition().getPosition());
}


void RosegardenGUIApp::slotToggleTracking()
{
    m_view->getTrackEditor()->slotToggleTracking();
}
    

// Sequencer auxiliary process management
//
//
bool RosegardenGUIApp::launchSequencer(bool useExisting)
{
    if (!isUsingSequencer()) {
        RG_DEBUG << "RosegardenGUIApp::launchSequencer() - not using seq. - returning\n";
        return false; // no need to launch anything
    }
    
    if (isSequencerRunning())
    {
        RG_DEBUG << "RosegardenGUIApp::launchSequencer() - sequencer already running - returning\n";
        if (m_seqManager) m_seqManager->checkSoundDriverStatus();
        return true;
    }

    // Check to see if we're clearing down sequencer processes - 
    // if we're not we check DCOP for an existing sequencer and
    // try to talk to use that (that's the "developer" mode).
    //
    // User mode should clear down sequencer processes.
    //
    if (kapp->dcopClient()->isApplicationRegistered(
                QCString(ROSEGARDEN_SEQUENCER_APP_NAME)))
    {
        RG_DEBUG << "RosegardenGUIApp::launchSequencer() - "
                 << "existing DCOP registered sequencer found\n";

	if (useExisting) {
	    if (m_seqManager) m_seqManager->checkSoundDriverStatus();
	    m_sequencerProcess = (KProcess*)SequencerExternal;
	    return true;
	}

        KProcess *proc = new KProcess;
        *proc << "/usr/bin/killall";
        *proc << "rosegardensequencer";
        *proc << "lt-rosegardensequencer";

        proc->start(KProcess::Block, KProcess::All);

        if (proc->exitStatus()) {
            RG_DEBUG << "couldn't kill any sequencer processes" << endl;
	}
	delete proc;

	sleep(1);
	
	if (kapp->dcopClient()->isApplicationRegistered(
                QCString(ROSEGARDEN_SEQUENCER_APP_NAME)))
	{
	    RG_DEBUG << "RosegardenGUIApp::launchSequencer() - "
		     << "failed to kill existing sequencer\n";

	    KProcess *proc = new KProcess;
	    *proc << "/usr/bin/killall";
	    *proc << "-9";
	    *proc << "rosegardensequencer";
	    *proc << "lt-rosegardensequencer";

	    proc->start(KProcess::Block, KProcess::All);

	    if (proc->exitStatus()) {
		RG_DEBUG << "couldn't kill any sequencer processes" << endl;
	    }
	    delete proc;

	    sleep(1);
	}
    }

    //
    // No sequencer is running, so start one
    //
    KTmpStatusMsg msg(i18n("Starting the sequencer..."), this);

    if (!m_sequencerProcess) {
        m_sequencerProcess = new KProcess;

        (*m_sequencerProcess) << "rosegardensequencer";

        // Command line arguments
        //
        KConfig *config = kapp->config();
        config->setGroup(Rosegarden::SequencerOptionsConfigGroup);
        QString options = config->readEntry("commandlineoptions");
        if (!options.isEmpty()) {
            (*m_sequencerProcess) << options;
            RG_DEBUG << "sequencer options \"" << options << "\"" << endl;
        }

    } else {
        RG_DEBUG << "RosegardenGUIApp::launchSequencer() - sequencer KProcess already created\n";
        m_sequencerProcess->disconnect(); // disconnect processExit signal
        // it will be reconnected later on
    }
    
    bool res = m_sequencerProcess->start();
    
    if (!res) {
        KMessageBox::error(0, i18n("Couldn't start the sequencer"));
        RG_DEBUG << "Couldn't start the sequencer\n";
        m_sequencerProcess = 0;
        // If starting it didn't even work, fall back to no sequencer mode
        m_useSequencer = false;
    }
    else {
        // connect processExited only after start, otherwise
        // a failed startup will call slotSequencerExited()
        // right away and we don't get to check the result
        // of m_sequencerProcess->start() and thus make the distinction
        // between the case where the sequencer was successfully launched
        // but crashed right away, or the case where the process couldn't
        // be launched at all (missing executable, etc...)
        //
        // We also re-check that the process is still running at this
        // point in case it crashed between the moment we check res above
        // and now.
        //
        //usleep(1000 * 1000); // even wait half a sec. to make sure it's actually running
        if (m_sequencerProcess->isRunning()) {

            try {
//                 if (m_seqManager) {
//                     RG_DEBUG << "RosegardenGUIApp::launchSequencer : checking sound driver status\n";
//                     m_seqManager->checkSoundDriverStatus();
//                 }
                
                stateChanged("sequencer_running");
                slotEnableTransport(true);

                connect(m_sequencerProcess, SIGNAL(processExited(KProcess*)),
                        this, SLOT(slotSequencerExited(KProcess*)));

            } catch (Rosegarden::Exception e) {
                m_sequencerProcess = 0;
                m_useSequencer = false;
                stateChanged("sequencer_running", KXMLGUIClient::StateReverse);
                slotEnableTransport(false);
            }

        } else { // if it crashed so fast, it's probably pointless
            // to try restarting it later, so also fall back to no
            // sequencer mode
            m_sequencerProcess = 0;
            m_useSequencer = false;
            stateChanged("sequencer_running", KXMLGUIClient::StateReverse);
            slotEnableTransport(false);
        }
            
    }

    // Sync current devices with the sequencer
    //
    if (m_doc) m_doc->syncDevices();

    if (m_doc && m_doc->getStudio().haveMidiDevices()) {
	stateChanged("got_midi_devices");
    } else {
	stateChanged("got_midi_devices", KXMLGUIClient::StateReverse);
    }
    
    return res;
}

#ifdef HAVE_LIBJACK
bool RosegardenGUIApp::launchJack()
{
    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);

    bool startJack = config->readBoolEntry("jackstart", false);
    if (!startJack) return true; // we don't do anything

    QString jackPath = config->readEntry("jackcommand", "");

    emit startupStatusMessage(i18n("Clearing down jackd..."));

    KProcess *proc = new KProcess; // TODO: do it in a less clumsy way
    *proc << "/usr/bin/killall";
    *proc << "-9";
    *proc << "jackd";

    proc->start(KProcess::Block, KProcess::All);

    if (proc->exitStatus())
        RG_DEBUG << "couldn't kill any jackd processes" << endl;
    else
        RG_DEBUG << "killed old jackd processes" << endl;

    emit startupStatusMessage(i18n("Starting jackd..."));

    if (jackPath != "") {

        RG_DEBUG << "starting jack \"" << jackPath << "\"" << endl;

        QStringList splitCommand;
        splitCommand = QStringList::split(" ", jackPath);

        RG_DEBUG << "RosegardenGUIApp::launchJack() : splitCommand length : "
                 << splitCommand.size() << endl;
        
        // start jack process
        m_jackProcess = new KProcess;
        
        *m_jackProcess << splitCommand;

        m_jackProcess->start();
    }


    return m_jackProcess != 0 ? m_jackProcess->isRunning() : true;
}
#endif //HAVE_LIBJACK

void RosegardenGUIApp::slotSequencerExited(KProcess*)
{
    RG_DEBUG << "RosegardenGUIApp::slotSequencerExited Sequencer exited\n";

    KStartupLogo::hideIfStillThere();

    KMessageBox::error(0, i18n("The Rosegarden sequencer could not be started, so sound and recording will be unavailable for this session.\nFor assistance with correct audio and MIDI configuration, go to http://rosegardenmusic.com."));

    m_sequencerProcess = 0; // isSequencerRunning() will return false
    // but isUsingSequencer() will keep returning true
    // so pressing the play button may attempt to restart the sequencer
}


void RosegardenGUIApp::slotExportMIDI()
{
    KTmpStatusMsg msg(i18n("Exporting MIDI file..."), this);

    QString fileName = getValidWriteFile
        ("*.mid *.midi|" + i18n("Standard MIDI files\n") +
         "\n*|" + i18n("All files"),
         i18n("Export as..."));

    if (fileName.isEmpty()) return;

    exportMIDIFile(fileName);
}

void RosegardenGUIApp::exportMIDIFile(QString file)
{
    RosegardenProgressDialog progressDlg(i18n("Exporting MIDI file..."),
                                         100,
                                         this);

    std::string fname(QFile::encodeName(file));

    Rosegarden::MidiFile midiFile(fname,
                                  &m_doc->getStudio());

    connect(&midiFile, SIGNAL(setProgress(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));

    connect(&midiFile, SIGNAL(incrementProgress(int)),
            progressDlg.progressBar(), SLOT(advance(int)));

    midiFile.convertToMidi(m_doc->getComposition());

    if (!midiFile.write())
    {
	CurrentProgressDialog::freeze();
	KMessageBox::sorry(this, i18n("Export failed.  The file could not be opened for writing."));
    }
}

void RosegardenGUIApp::slotExportCsound()
{
    KTmpStatusMsg msg(i18n("Exporting Csound score file..."), this);

    QString fileName = getValidWriteFile(QString("*|") + i18n("All files"),
                                         i18n("Export as..."));
    if (fileName.isEmpty()) return;

    exportCsoundFile(fileName);
}

void RosegardenGUIApp::exportCsoundFile(QString file)
{
    RosegardenProgressDialog progressDlg(i18n("Exporting Csound score file..."),
                                         100,
                                         this);

    CsoundExporter e(this, &m_doc->getComposition(), std::string(QFile::encodeName(file)));

    connect(&e, SIGNAL(setProgress(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));

    connect(&e, SIGNAL(incrementProgress(int)),
            progressDlg.progressBar(), SLOT(advance(int)));

    if (!e.write()) {
	CurrentProgressDialog::freeze();
        KMessageBox::sorry(this, i18n("Export failed.  The file could not be opened for writing."));
    }
}

void RosegardenGUIApp::slotExportMup()
{
    KTmpStatusMsg msg(i18n("Exporting Mup file..."), this);

    QString fileName = getValidWriteFile
        ("*.mup|" + i18n("Mup files\n") + "\n*|" + i18n("All files"),
         i18n("Export as..."));
    if (fileName.isEmpty()) return;

    exportMupFile(fileName);
}

void RosegardenGUIApp::exportMupFile(QString file)
{
    RosegardenProgressDialog progressDlg(i18n("Exporting Mup file..."),
                                         100,
                                         this);

    MupExporter e(this, &m_doc->getComposition(), std::string(QFile::encodeName(file)));

    connect(&e, SIGNAL(setProgress(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));

    connect(&e, SIGNAL(incrementProgress(int)),
            progressDlg.progressBar(), SLOT(advance(int)));

    if (!e.write()) {
	CurrentProgressDialog::freeze();
        KMessageBox::sorry(this, i18n("Export failed.  The file could not be opened for writing."));
    }
}

void RosegardenGUIApp::slotExportLilypond()
{
    KTmpStatusMsg msg(i18n("Exporting Lilypond file..."), this);

    QString fileName = getValidWriteFile
        (QString("*.ly|") + i18n("Lilypond files") +
         "\n*|" + i18n("All files"),
         i18n("Export as..."));

    if (fileName.isEmpty()) return;

    exportLilypondFile(fileName);
}

void RosegardenGUIApp::exportLilypondFile(QString file)
{
    LilypondOptionsDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;

    RosegardenProgressDialog progressDlg(i18n("Exporting Lilypond file..."),
                                         100,
                                         this);
    
    LilypondExporter e(this, &m_doc->getComposition(), std::string(QFile::encodeName(file)));

    connect(&e, SIGNAL(setProgress(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));

    connect(&e, SIGNAL(incrementProgress(int)),
            progressDlg.progressBar(), SLOT(advance(int)));

    if (!e.write()) {
	CurrentProgressDialog::freeze();
        KMessageBox::sorry(this, i18n("Export failed.  The file could not be opened for writing."));
    }
}

void RosegardenGUIApp::slotExportMusicXml()
{
    KTmpStatusMsg msg(i18n("Exporting MusicXML file..."), this);

    QString fileName = getValidWriteFile
        (QString("*.xml|") + i18n("XML files") +
         "\n*|" + i18n("All files"), i18n("Export as..."));

    if (fileName.isEmpty()) return;

    exportMusicXmlFile(fileName);
}

void RosegardenGUIApp::exportMusicXmlFile(QString file)
{
    RosegardenProgressDialog progressDlg(i18n("Exporting MusicXML file..."),
                                         100,
                                         this);

    MusicXmlExporter e(this, m_doc, std::string(QFile::encodeName(file)));

    connect(&e, SIGNAL(setProgress(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));

    connect(&e, SIGNAL(incrementProgress(int)),
            progressDlg.progressBar(), SLOT(advance(int)));

    if (!e.write()) {
	CurrentProgressDialog::freeze();
        KMessageBox::sorry(this, i18n("Export failed.  The file could not be opened for writing."));
    }
}

// Uncheck the transport window check box
void
RosegardenGUIApp::slotCloseTransport()
{
    m_viewTransport->setChecked(false);
    slotToggleTransport(); // hides the transport
}

// We use this slot to active a tool mode on the GUI
// from a layer below the top level.  For example when
// we select a Track on the trackeditor and want this
// action to go on to select all the Segments on that
// Track we must change the edit mode to "Selector"
//
//
void
RosegardenGUIApp::slotActivateTool(QString toolName)
{
    if (toolName == SegmentSelector::ToolName) {
        actionCollection()->action("select")->activate();
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
        m_seqManager->getTransportStatus() == RECORDING_AUDIO ||
        m_seqManager->getTransportStatus() == RECORDING_ARMED)
    {
        if (comp.useRecordMetronome())
            comp.setRecordMetronome(false);
        else
            comp.setRecordMetronome(true);

        m_transport->MetronomeButton()->setOn(comp.useRecordMetronome());
    }
    else
    {
        if (comp.usePlayMetronome())
            comp.setPlayMetronome(false);
        else
            comp.setPlayMetronome(true);

        m_transport->MetronomeButton()->setOn(comp.usePlayMetronome());
    }
}

// ------------------------ SequenceManager ----------------------------
//
//
//
//
//


void
RosegardenGUIApp::slotRewindToBeginning()
{
    // ignore requests if recording
    //
    if (m_seqManager->getTransportStatus() == RECORDING_MIDI ||
        m_seqManager->getTransportStatus() == RECORDING_AUDIO)
        return;

    m_seqManager->rewindToBeginning();
}


void
RosegardenGUIApp::slotFastForwardToEnd()
{
    // ignore requests if recording
    //
    if (m_seqManager->getTransportStatus() == RECORDING_MIDI ||
        m_seqManager->getTransportStatus() == RECORDING_AUDIO)
        return;

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

// This method is a callback from the Sequencer to update the GUI
// with state change information.  The GUI requests the Sequencer
// to start playing or to start recording and enters a pending
// state (see rosegardendcop.h for TransportStatus values).
// The Sequencer replies when ready with it's status.  If anything
// fails then we default (or try to default) to STOPPED at both
// the GUI and the Sequencer.
//
void RosegardenGUIApp::notifySequencerStatus(int status)
{
    stateChanged("not_playing",
                 (status == PLAYING ||
                  status == RECORDING_MIDI ||
                  status == RECORDING_AUDIO) ?
                 KXMLGUIClient::StateReverse : KXMLGUIClient::StateNoReverse);

    if (m_seqManager)
        m_seqManager->setTransportStatus((TransportStatus) status);
}

// Called from sequencer whenever an event arrives outside of the
// recording context.
//
void RosegardenGUIApp::processAsynchronousMidi(const Rosegarden::MappedComposition &mC)
{
    if (!m_seqManager) return; // probably getting this from a not-yet-killed runaway sequencer

    m_seqManager->processAsynchronousMidi(mC, 0);
    SequencerMapper *mapper = m_seqManager->getSequencerMapper();
    if (mapper) m_view->updateMeters(mapper);
}


// Called when we want to start recording from the GUI.
// This method tells the sequencer to start recording and
// from then on the sequencer returns MappedCompositions
// to the GUI via the sequencer mmapper
//
void
RosegardenGUIApp::slotRecord()
{
    if (!isUsingSequencer()) return;

    if (!isSequencerRunning()) {

        // Try to launch sequencer and return if we fail
        //
        if (!launchSequencer(false))
            return;
    }

    if (m_seqManager->getTransportStatus() == RECORDING_MIDI ||
        m_seqManager->getTransportStatus() == RECORDING_AUDIO)
    {
        slotStop();
    }

    // Attempt to start recording
    //
    try
    {
        m_seqManager->record(false);
    }
    catch(QString s)
    {
        // We should already be stopped by this point so just unset
        // the buttons after clicking the dialog.
        //
        KMessageBox::error(this, s);

        m_transport->MetronomeButton()->setOn(false);
        m_transport->RecordButton()->setOn(false);
        m_transport->PlayButton()->setOn(false);
    }
    catch(Rosegarden::Exception e)
    {
        KMessageBox::error(this, strtoqstr(e.getMessage()));

        m_transport->MetronomeButton()->setOn(false);
        m_transport->RecordButton()->setOn(false);
        m_transport->PlayButton()->setOn(false);
    }

    // plugin the keyboard accelerators for focus on this dialog
    plugAccelerators(m_seqManager->getCountdownDialog(),
                     m_seqManager->getCountdownDialog()->getAccelerators());

    // Start the playback timer - this fetches the current sequencer position &c
    //
    m_stopTimer->stop();
    m_playTimer->start(23); // avoid multiples of 10 just so as
			    // to avoid always having the same digit
			    // in one place on the transport.  How
			    // shallow.)
}

// Toggling record whilst stopped prepares us for record next time we
// hit play.  If we're already playing we perform punch-in recording.
//
// All of the logic for this is handled in the sequencemanager.
//
void
RosegardenGUIApp::slotToggleRecord()
{
    if (!isUsingSequencer() ||
        (!isSequencerRunning() && !launchSequencer(false)))
        return;

    try
    {
        m_seqManager->record(true);
    }
    catch(QString s)
    {
        KMessageBox::error(this, s);
    }
    catch(Rosegarden::Exception e)
    {
        KMessageBox::error(this, strtoqstr(e.getMessage()));
    }

}

void
RosegardenGUIApp::slotSetLoop(Rosegarden::timeT lhs, Rosegarden::timeT rhs)
{
    try
    {
        m_doc->slotDocumentModified();

        m_seqManager->setLoop(lhs, rhs);

        // toggle the loop button
        if (lhs != rhs)
            m_transport->LoopButton()->setOn(true);
        else
            m_transport->LoopButton()->setOn(false);

    }
    catch(QString s)
    {
        KMessageBox::error(this, s);
    }
}

// Called from sequencer to sync up devices
//
void RosegardenGUIApp::alive()
{
    if (m_doc) m_doc->syncDevices();

    if (m_doc && m_doc->getStudio().haveMidiDevices()) {
	stateChanged("got_midi_devices");
    } else {
	stateChanged("got_midi_devices", KXMLGUIClient::StateReverse);
    }
}

void RosegardenGUIApp::slotPlay()
{
    if (!isUsingSequencer()) return;

    if (!isSequencerRunning()) {

        // Try to launch sequencer and return if it fails
        //
        if (!launchSequencer(false))
            return;
    }

    if (!m_seqManager) return;

    // If we're armed and ready to record then do this instead (calling
    // slotRecord ensures we don't toggle the recording state in
    // SequenceManager)
    //
    if (m_seqManager->getTransportStatus() == RECORDING_ARMED)
    {
        slotRecord();
        return;
    }

    // Send the controllers at start of playback if required
    //
    KConfig *config = kapp->config();
    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);
    bool sendControllers = config->readBoolEntry("alwayssendcontrollers", false);

    if (sendControllers)
        m_doc->initialiseControllers();

    bool pausedPlayback = false;
    
    try {
        pausedPlayback = m_seqManager->play(); // this will stop playback (pause) if it's already running
        // Check the new state of the transport and start or stop timer
        // accordingly
        //
        if (!pausedPlayback) {
        
            // Start the playback timer - this fetches the current sequencer position &c
            //
	    m_stopTimer->stop();
            m_playTimer->start(23);
        } else {
            m_playTimer->stop();
	    m_stopTimer->start(100);
        }
    }
    catch(QString s) {
        KMessageBox::error(this, s);
        m_playTimer->stop();
	m_stopTimer->start(100);
    }
    
}

void RosegardenGUIApp::slotJumpToTime(int sec, int usec)
{
    Rosegarden::Composition *comp = &m_doc->getComposition();
    Rosegarden::timeT t = comp->getElapsedTimeForRealTime
	(Rosegarden::RealTime(sec, usec * 1000));
    m_doc->setPointerPosition(t);
}

void RosegardenGUIApp::slotStartAtTime(int sec, int usec)
{
    slotJumpToTime(sec, usec);
    slotPlay();
}


// Send stop request to Sequencer.  This'll set the flag 
// to attempt a stop next time a good window appears.
//
void RosegardenGUIApp::slotStop()
{
    try
    {
        if (m_seqManager)
            m_seqManager->stopping();
    }
    catch(Rosegarden::Exception e)
    {
        KMessageBox::error(this, strtoqstr(e.getMessage()));
    }

    // stop the playback timer
    m_playTimer->stop();
    m_stopTimer->start(100);
}

// Jump to previous bar
//
void RosegardenGUIApp::slotRewind()
{
    // ignore requests if recording
    //
    if (m_seqManager->getTransportStatus() == RECORDING_MIDI ||
        m_seqManager->getTransportStatus() == RECORDING_AUDIO)
        return;
    if (m_seqManager)
        m_seqManager->rewind();
}


// Jump to next bar
//
void RosegardenGUIApp::slotFastforward()
{
    // ignore requests if recording
    //
    if (m_seqManager->getTransportStatus() == RECORDING_MIDI ||
        m_seqManager->getTransportStatus() == RECORDING_AUDIO)
        return;

    if (m_seqManager)
        m_seqManager->fastforward();
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


// Just set the solo value in the Composition equal to the state
// of the button
//
void RosegardenGUIApp::slotToggleSolo(bool value)
{
    RG_DEBUG << "RosegardenGUIApp::slotToggleSolo value = " << value << endl;

    //m_doc->getComposition().setSolo(m_transport->SoloButton()->isOn());
    m_doc->getComposition().setSolo(value);
    m_transport->SoloButton()->setOn(value);

    m_doc->slotDocumentModified();
}

void RosegardenGUIApp::slotTrackUp()
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    Rosegarden::TrackId tid = comp.getSelectedTrack();
    Rosegarden::TrackId pos = comp.getTrackById(tid)->getPosition();

    // If at top already
    if (pos == 0)
        return;

    Rosegarden::Track *track = comp.getTrackByPosition(pos - 1);

    // If the track exists
    if (track)
    {
       comp.setSelectedTrack(track->getId());
       m_view->slotSelectTrackSegments(comp.getSelectedTrack());
    }

}

void RosegardenGUIApp::slotTrackDown()
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    Rosegarden::TrackId tid = comp.getSelectedTrack();
    Rosegarden::TrackId pos = comp.getTrackById(tid)->getPosition();

    Rosegarden::Track *track = comp.getTrackByPosition(pos + 1);

    // If the track exists
    if (track)
    {
       comp.setSelectedTrack(track->getId());
       m_view->slotSelectTrackSegments(comp.getSelectedTrack());
    }

}

void RosegardenGUIApp::slotMuteAllTracks()
{
    RG_DEBUG << "RosegardenGUIApp::slotMuteAllTracks" << endl;

    Rosegarden::Composition &comp = m_doc->getComposition();

    Rosegarden::Composition::trackcontainer tracks = comp.getTracks();
    Rosegarden::Composition::trackiterator tit;
    for (tit = tracks.begin(); tit != tracks.end(); ++tit)
        m_view->slotSetMute((*tit).second->getInstrument(), true);
}

void RosegardenGUIApp::slotUnmuteAllTracks()
{
    RG_DEBUG << "RosegardenGUIApp::slotUnmuteAllTracks" << endl;

    Rosegarden::Composition &comp = m_doc->getComposition();

    Rosegarden::Composition::trackcontainer tracks = comp.getTracks();
    Rosegarden::Composition::trackiterator tit;
    for (tit = tracks.begin(); tit != tracks.end(); ++tit)
        m_view->slotSetMute((*tit).second->getInstrument(), false);
}



void RosegardenGUIApp::slotConfigure()
{
    RG_DEBUG << "RosegardenGUIApp::slotConfigure\n";

    Rosegarden::ConfigureDialog *configDlg = 
        new Rosegarden::ConfigureDialog(m_doc, kapp->config(), this);

    connect(configDlg, SIGNAL(updateAutoSaveInterval(unsigned int)),
            this, SLOT(slotUpdateAutoSaveInterval(unsigned int)));

    configDlg->show();
}

void RosegardenGUIApp::slotEditDocumentProperties()
{
    RG_DEBUG << "RosegardenGUIApp::slotEditDocumentProperties\n";

    Rosegarden::DocumentConfigureDialog *configDlg = 
        new Rosegarden::DocumentConfigureDialog(m_doc, this);

    configDlg->show();
}

void RosegardenGUIApp::slotEditKeys()
{
    KKeyDialog::configure(actionCollection());
}

void RosegardenGUIApp::slotEditToolbars()
{
    KEditToolbar dlg(actionCollection(), "rosegardenui.rc");

    connect(&dlg, SIGNAL(newToolbarConfig()),
            SLOT(slotUpdateToolbars()));

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
    RG_DEBUG << "RosegardenGUIApp::slotEditTempo\n";

    TempoDialog tempoDialog(parent, m_doc);

    connect(&tempoDialog,
            SIGNAL(changeTempo(Rosegarden::timeT,
                               double, TempoDialog::TempoDialogAction)),
            SLOT(slotChangeTempo(Rosegarden::timeT,
                                 double, TempoDialog::TempoDialogAction)));

    tempoDialog.setTempoPosition(m_doc->getComposition().getPosition());
    tempoDialog.exec();
}

void RosegardenGUIApp::slotEditTimeSignature(QWidget *parent)
{
    Rosegarden::Composition &composition(m_doc->getComposition());

    Rosegarden::timeT time = composition.getPosition();
    Rosegarden::TimeSignature sig = composition.getTimeSignatureAt(time);

    TimeSignatureDialog dialog(parent, &composition, time, sig);

    if (dialog.exec() == QDialog::Accepted) {

	time = dialog.getTime();

        if (dialog.shouldNormalizeRests()) {
            m_doc->getCommandHistory()->addCommand
                (new AddTimeSignatureAndNormalizeCommand
                 (&composition, time, dialog.getTimeSignature()));
        } else { 
            m_doc->getCommandHistory()->addCommand
                (new AddTimeSignatureCommand
                 (&composition, time, dialog.getTimeSignature()));
        }
    }
}

void RosegardenGUIApp::slotChangeZoom(int)
{
    double duration44 = Rosegarden::TimeSignature(4,4).getBarDuration();
    double value = double(m_zoomSlider->getCurrentSize());
    m_zoomLabel->setText(i18n("%1%").arg(duration44/value));

    RG_DEBUG << "RosegardenGUIApp::slotChangeZoom : zoom size = "
             << m_zoomSlider->getCurrentSize() << endl;

    // initZoomToolbar sets the zoom value. With some old versions of
    // Qt3.0, this can cause slotChangeZoom() to be called while the
    // view hasn't been initialized yet, so we need to check it's not
    // null
    //
    if (m_view)
        m_view->setZoomSize(m_zoomSlider->getCurrentSize());

    long newZoom = int(m_zoomSlider->getCurrentSize() * 1000.0);
    
    if (m_doc->getConfiguration().get<Rosegarden::Int>
	(Rosegarden::DocumentConfiguration::ZoomLevel) != newZoom) {

	m_doc->getConfiguration().set<Rosegarden::Int>
	    (Rosegarden::DocumentConfiguration::ZoomLevel, newZoom);
	
	m_doc->slotDocumentModified();
    }
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
        RG_DEBUG << "RosegardenGUIApp::slotChangeTempo() - "
                             << "unrecognised tempo command" << endl;
    }
}

void
RosegardenGUIApp::slotDocumentModified(bool m)
{
    RG_DEBUG << "RosegardenGUIApp::slotDocumentModified(" << m << ") - doc path = "
             << m_doc->getAbsFilePath() << endl;

    if (!m_doc->getAbsFilePath().isEmpty()) {
        slotStateChanged("saved_file_modified", m);
    } else {
        slotStateChanged("new_file_modified", m);
    }
    
}

void
RosegardenGUIApp::slotStateChanged(QString s,
                                   bool noReverse)
{
    RG_DEBUG << "RosegardenGUIApp::slotStateChanged " << s << "," << noReverse << endl;

    stateChanged(s, noReverse ? KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse);
}

void
RosegardenGUIApp::slotTestClipboard()
{
    if (m_clipboard->isEmpty()) {
        stateChanged("have_clipboard", KXMLGUIClient::StateReverse);
        stateChanged("have_clipboard_single_segment",
                     KXMLGUIClient::StateReverse);
    } else {
        stateChanged("have_clipboard", KXMLGUIClient::StateNoReverse);
        stateChanged("have_clipboard_single_segment",
                     (m_clipboard->isSingleSegment() ?
                      KXMLGUIClient::StateNoReverse :
                      KXMLGUIClient::StateReverse));
    }
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
                     SLOT(slotToggleRecord()));

    Rosegarden::RosegardenTransportDialog *transport =
        dynamic_cast<Rosegarden::RosegardenTransportDialog*>(widget);

    if (transport)
    {
        connect(transport->PlayButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotPlay()));

        connect(transport->StopButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotStop()));
             
        connect(transport->FfwdButton(),
                SIGNAL(clicked()),
                SLOT(slotFastforward()));
            
        connect(transport->RewindButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotRewind()));

        connect(transport->RecordButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotRecord()));

        connect(transport->RewindEndButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotRewindToBeginning()));

        connect(transport->FfwdEndButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotFastForwardToEnd()));

        connect(transport->MetronomeButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotToggleMetronome()));
            
        connect(transport->SoloButton(),
                SIGNAL(toggled(bool)),
                this,
                SLOT(slotToggleSolo(bool)));
            
        connect(transport->TimeDisplayButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotRefreshTimeDisplay()));

        connect(transport->ToEndButton(),
                SIGNAL(clicked()),
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
    if (m_audioManagerDialog)
    {
        m_audioManagerDialog->raise();
        return;
    }
        
    m_audioManagerDialog =
        new Rosegarden::AudioManagerDialog(this, m_doc);

    connect(m_audioManagerDialog,
            SIGNAL(playAudioFile(Rosegarden::AudioFileId,
                                 const Rosegarden::RealTime &,
                                 const Rosegarden::RealTime&)),
            SLOT(slotPlayAudioFile(Rosegarden::AudioFileId,
                                   const Rosegarden::RealTime &,
                                   const Rosegarden::RealTime &)));

    connect(m_audioManagerDialog,
            SIGNAL(addAudioFile(Rosegarden::AudioFileId)),
            SLOT(slotAddAudioFile(Rosegarden::AudioFileId)));

    connect(m_audioManagerDialog,
            SIGNAL(deleteAudioFile(Rosegarden::AudioFileId)),
            SLOT(slotDeleteAudioFile(Rosegarden::AudioFileId)));

    //
    // Sync segment selection between audio man. dialog and main window
    //

    // from dialog to us...
    connect(m_audioManagerDialog,
            SIGNAL(segmentsSelected(const Rosegarden::SegmentSelection&)),
            m_view,
            SLOT(slotSetSelectedSegments(const Rosegarden::SegmentSelection&)));

    // and from us to dialog
    connect(this, SIGNAL(segmentsSelected(const Rosegarden::SegmentSelection&)),
            m_audioManagerDialog,
            SLOT(slotSegmentSelection(const Rosegarden::SegmentSelection&)));


    connect(m_audioManagerDialog,
            SIGNAL(deleteSegments(const Rosegarden::SegmentSelection&)),
            SLOT(slotDeleteSegments(const Rosegarden::SegmentSelection&)));

    connect(m_audioManagerDialog,
            SIGNAL(insertAudioSegment(Rosegarden::AudioFileId,
                                      const Rosegarden::RealTime&,
                                      const Rosegarden::RealTime&)),
            m_view,
            SLOT(slotAddAudioSegmentDefaultPosition(Rosegarden::AudioFileId,
                                                    const Rosegarden::RealTime&,
                                                    const Rosegarden::RealTime&)));
    connect(m_audioManagerDialog,
            SIGNAL(cancelPlayingAudioFile(Rosegarden::AudioFileId)),
            SLOT(slotCancelAudioPlayingFile(Rosegarden::AudioFileId)));

    connect(m_audioManagerDialog,
            SIGNAL(deleteAllAudioFiles()),
            SLOT(slotDeleteAllAudioFiles()));

    // Make sure we know when the audio man. dialog is closing
    //
    connect(m_audioManagerDialog,
            SIGNAL(closing()),
            SLOT(slotAudioManagerClosed()));

    // And that it goes away when the current document is changing
    //
    connect(this, SIGNAL(documentAboutToChange()),
            m_audioManagerDialog, SLOT(close()));

    m_audioManagerDialog->setAudioSubsystemStatus(
            m_seqManager->getSoundDriverStatus() & Rosegarden::AUDIO_OK);

    plugAccelerators(m_audioManagerDialog,
                     m_audioManagerDialog->getAccelerators());
    
    m_audioManagerDialog->show();
}

void
RosegardenGUIApp::slotPlayAudioFile(unsigned int id,
                                    const Rosegarden::RealTime &startTime,
                                    const Rosegarden::RealTime &duration)
{
    Rosegarden::AudioFile *aF = m_doc->getAudioFileManager().getAudioFile(id);

    if (aF == 0)
        return;

    Rosegarden::MappedEvent mE(m_doc->getStudio().
                               getAudioPreviewInstrument(),
                               id,
                               Rosegarden::RealTime(-120, 0),
                               duration,                  // duration
                               startTime);                // start index

    Rosegarden::StudioControl::sendMappedEvent(mE);

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

    if (rgapp->sequencerCall("addAudioFile(QString, int)", replyType, replyData, data)) {
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
        if (!result) {
            KMessageBox::error(this, QString(i18n("Sequencer failed to add audio file %1"))
                               .arg(aF->getFilename().c_str()));
        }
    }
}

// File has already been removed locally - remove it from the
// sequencer too.
//
void
RosegardenGUIApp::slotDeleteAudioFile(unsigned int id)
{
    if (m_doc->getAudioFileManager().removeFile(id) == false)
        return;

    QCString replyType;
    QByteArray replyData;
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    // file id
    //
    streamOut << id;

    if (rgapp->sequencerCall("removeAudioFile(int)", replyType, replyData, data)) {
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
        if (!result) {
            KMessageBox::error(this, QString(i18n("Sequencer failed to remove audio file id %1"))
                               .arg(id));
        }
    }
}

void
RosegardenGUIApp::slotDeleteSegments(const Rosegarden::SegmentSelection &selection)
{
    m_view->slotSetSelectedSegments(selection);
    slotDeleteSelectedSegments();
}

void
RosegardenGUIApp::slotCancelAudioPlayingFile(Rosegarden::AudioFileId id)
{
    Rosegarden::AudioFile *aF = m_doc->getAudioFileManager().getAudioFile(id);

    if (aF == 0)
        return;

    Rosegarden::MappedEvent mE(m_doc->getStudio().
                               getAudioPreviewInstrument(),
                               Rosegarden::MappedEvent::AudioCancel,
                               id);

    Rosegarden::StudioControl::sendMappedEvent(mE);
}

void
RosegardenGUIApp::slotDeleteAllAudioFiles()
{
    m_doc->getAudioFileManager().clear();

    // Clear at the sequencer
    //
    QCString replyType;
    QByteArray replyData;
    QByteArray data;

    rgapp->sequencerCall("clearAllAudioFiles()", replyType, replyData, data);
}

void
RosegardenGUIApp::skippedSlices(unsigned int /*slices*/)
{
    //std::cout << "SEQUENCER HAS SKIPPED " << slices << " SLICES" << std::endl;
}

void
RosegardenGUIApp::slotRepeatingSegments()
{
    m_view->getTrackEditor()->slotTurnRepeatingSegmentToRealCopies();
}

void
RosegardenGUIApp::slotRelabelSegments()
{
    if (!m_view->haveSelection()) return;

    Rosegarden::SegmentSelection selection(m_view->getSelection());
    QString editLabel;

    if (selection.size() == 0) return;
    else if (selection.size() == 1) editLabel = i18n("Modify Segment label");
    else editLabel = i18n("Modify Segments label");

    KTmpStatusMsg msg(i18n("Relabelling selection..."), this);

    // Generate label
    QString label = strtoqstr((*selection.begin())->getLabel());

    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
         i != selection.end(); ++i)
    {
        if (strtoqstr((*i)->getLabel()) != label) label = "";
    }

    bool ok = false;

    QString newLabel = KLineEditDlg::getText(editLabel,
                                             i18n("Enter new label"),
                                             label,
                                             &ok,
                                             this);

    if (ok)
    {
        m_doc->getCommandHistory()->addCommand
            (new SegmentLabelCommand(selection, newLabel));
    }
}

void
RosegardenGUIApp::slotChangeCompositionLength()
{
    CompositionLengthDialog dialog(this, &m_doc->getComposition());

    if (dialog.exec() == QDialog::Accepted)
    {
        ChangeCompositionLengthCommand *command
            = new ChangeCompositionLengthCommand(
                    &m_doc->getComposition(),
                    dialog.getStartMarker(),
                    dialog.getEndMarker());

        m_doc->getCommandHistory()->addCommand(command);
    }
}

 
void
RosegardenGUIApp::slotManageMIDIDevices()
{
    if (m_deviceManager) {
	m_deviceManager->raise();
	return;
    }

    m_deviceManager = new DeviceManagerDialog(this, m_doc);
    
    connect(m_deviceManager, SIGNAL(closing()),
            this, SLOT(slotDeviceManagerClosed()));

    connect(this, SIGNAL(documentAboutToChange()),
            m_deviceManager, SLOT(close()));

    // Cheating way of updating the track/instrument list
    //
    connect(m_deviceManager, SIGNAL(deviceNamesChanged()),
            m_view, SLOT(slotSynchroniseWithComposition()));

    connect(m_deviceManager, SIGNAL(editBanks(Rosegarden::DeviceId)),
	    this, SLOT(slotEditBanks(Rosegarden::DeviceId)));

    connect(m_deviceManager, SIGNAL(editControllers(Rosegarden::DeviceId)),
	    this, SLOT(slotEditControlParameters(Rosegarden::DeviceId)));

    if (m_midiMixer)
    {
        connect(m_deviceManager, SIGNAL(deviceNamesChanged()),
                m_midiMixer, SLOT(slotSynchronise()));

    }

 
    m_deviceManager->show();
}

void
RosegardenGUIApp::slotOpenAudioMixer()
{
    if (m_audioMixer) {
	m_audioMixer->raise();
	return;
    }

    m_audioMixer = new AudioMixerWindow(this, m_doc);
    
    connect(m_audioMixer, SIGNAL(closing()),
            this, SLOT(slotAudioMixerClosed()));

    connect(m_audioMixer, SIGNAL(selectPlugin(QWidget *, Rosegarden::InstrumentId, int)),
	    this, SLOT(slotShowPluginDialog(QWidget *, Rosegarden::InstrumentId, int)));

    connect(this, SIGNAL(documentAboutToChange()),
            m_audioMixer, SLOT(close()));

    connect(m_view, SIGNAL(checkTrackAssignments()),
	    m_audioMixer, SLOT(slotTrackAssignmentsChanged()));

    connect(m_audioMixer, SIGNAL(play()),
	    this, SLOT(slotPlay()));
    connect(m_audioMixer, SIGNAL(stop()),
	    this, SLOT(slotStop()));
    connect(m_audioMixer, SIGNAL(fastForwardPlayback()),
	    this, SLOT(slotFastforward()));
    connect(m_audioMixer, SIGNAL(rewindPlayback()),
	    this, SLOT(slotRewind()));
    connect(m_audioMixer, SIGNAL(fastForwardPlaybackToEnd()),
	    this, SLOT(slotFastForwardToEnd()));
    connect(m_audioMixer, SIGNAL(rewindPlaybackToBeginning()),
	    this, SLOT(slotRewindToBeginning()));
    connect(m_audioMixer, SIGNAL(record()),
	    this, SLOT(slotRecord()));

    connect(m_audioMixer,
	    SIGNAL(instrumentParametersChanged(Rosegarden::InstrumentId)),
	    m_instrumentParameterBox,
	    SLOT(slotInstrumentParametersChanged(Rosegarden::InstrumentId)));

    connect(m_instrumentParameterBox,
	    SIGNAL(instrumentParametersChanged(Rosegarden::InstrumentId)),
	    m_audioMixer,
	    SLOT(slotUpdateInstrument(Rosegarden::InstrumentId)));

    plugAccelerators(m_audioMixer, m_audioMixer->getAccelerators());

    m_audioMixer->show();
}


void
RosegardenGUIApp::slotOpenMidiMixer()
{
    if (m_midiMixer) {
	m_midiMixer->raise();
	return;
    }

    m_midiMixer = new MidiMixerWindow(this, m_doc);
    
    connect(m_midiMixer, SIGNAL(closing()),
            this, SLOT(slotMidiMixerClosed()));

    connect(this, SIGNAL(documentAboutToChange()),
            m_midiMixer, SLOT(close()));

    /*
    connect(m_midiMixer, SIGNAL(selectPlugin(QWidget *, Rosegarden::InstrumentId, int)),
	    this, SLOT(slotShowPluginDialog(QWidget *, Rosegarden::InstrumentId, int)));

    connect(m_view, SIGNAL(checkTrackAssignments()),
	    m_midiMixer, SLOT(slotTrackAssignmentsChanged()));
            */

    connect(m_midiMixer, SIGNAL(play()),
	    this, SLOT(slotPlay()));
    connect(m_midiMixer, SIGNAL(stop()),
	    this, SLOT(slotStop()));
    connect(m_midiMixer, SIGNAL(fastForwardPlayback()),
	    this, SLOT(slotFastforward()));
    connect(m_midiMixer, SIGNAL(rewindPlayback()),
	    this, SLOT(slotRewind()));
    connect(m_midiMixer, SIGNAL(fastForwardPlaybackToEnd()),
	    this, SLOT(slotFastForwardToEnd()));
    connect(m_midiMixer, SIGNAL(rewindPlaybackToBeginning()),
	    this, SLOT(slotRewindToBeginning()));
    connect(m_midiMixer, SIGNAL(record()),
	    this, SLOT(slotRecord()));

    connect(m_midiMixer,
	    SIGNAL(instrumentParametersChanged(Rosegarden::InstrumentId)),
	    m_instrumentParameterBox,
	    SLOT(slotInstrumentParametersChanged(Rosegarden::InstrumentId)));

    connect(m_instrumentParameterBox,
	    SIGNAL(instrumentParametersChanged(Rosegarden::InstrumentId)),
	    m_midiMixer,
	    SLOT(slotUpdateInstrument(Rosegarden::InstrumentId)));

    plugAccelerators(m_midiMixer, m_midiMixer->getAccelerators());

    m_midiMixer->show();
}

void
RosegardenGUIApp::slotEditControlParameters(Rosegarden::DeviceId device)
{
    for (std::set<ControlEditorDialog *>::iterator i = m_controlEditors.begin();
	 i != m_controlEditors.end(); ++i) {
	if ((*i)->getDevice() == device) {
	    (*i)->raise();
	    return;
	}
    }

    ControlEditorDialog *controlEditor = new ControlEditorDialog(this, m_doc,
								 device);
    m_controlEditors.insert(controlEditor);

    RG_DEBUG << "inserting control editor dialog, have " << m_controlEditors.size() << " now" << endl;

    connect(controlEditor, SIGNAL(closing()),
            SLOT(slotControlEditorClosed()));

    connect(this, SIGNAL(documentAboutToChange()),
            controlEditor, SLOT(close()));

    connect(m_doc, SIGNAL(devicesResyncd()),
	    controlEditor, SLOT(slotUpdate()));

    controlEditor->show();
}

void
RosegardenGUIApp::slotEditBanks()
{
    slotEditBanks(Rosegarden::Device::NO_DEVICE);
}

void
RosegardenGUIApp::slotEditBanks(Rosegarden::DeviceId device)
{
    if (m_bankEditor) {
	if (device != Rosegarden::Device::NO_DEVICE)
	    m_bankEditor->setCurrentDevice(device);
	m_bankEditor->raise();
	return;
    }

    m_bankEditor = new BankEditorDialog(this, m_doc, device);
    
    connect(m_bankEditor, SIGNAL(closing()),
            this, SLOT(slotBankEditorClosed()));

    connect(this, SIGNAL(documentAboutToChange()),
            m_bankEditor, SLOT(slotFileClose()));

    // Cheating way of updating the track/instrument list
    //
    connect(m_bankEditor, SIGNAL(deviceNamesChanged()),
            m_view, SLOT(slotSynchroniseWithComposition()));
 
    m_bankEditor->show();
}

void
RosegardenGUIApp::slotManageTriggerSegments()
{
    if (m_triggerSegmentManager) {
	m_triggerSegmentManager->raise();
	return;
    }

    m_triggerSegmentManager = new TriggerSegmentManager(this, m_doc);

    connect(m_triggerSegmentManager, SIGNAL(closing()),
            SLOT(slotTriggerManagerClosed()));

    connect(m_triggerSegmentManager, SIGNAL(editTriggerSegment(int)),
	    m_view, SLOT(slotEditTriggerSegment(int)));
    
    m_triggerSegmentManager->show();
}


void
RosegardenGUIApp::slotTriggerManagerClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotTriggerManagerClosed" << endl;

    m_triggerSegmentManager = 0;
}

void
RosegardenGUIApp::slotEditMarkers()
{
    if (m_markerEditor) {
	m_markerEditor->raise();
	return;
    }

    m_markerEditor = new MarkerEditorDialog(this, m_doc);

    connect(m_markerEditor, SIGNAL(closing()),
            SLOT(slotMarkerEditorClosed()));

    connect(m_markerEditor, SIGNAL(jumpToMarker(Rosegarden::timeT)),
            this, SLOT(slotSetPointer(Rosegarden::timeT)));

    plugAccelerators(m_markerEditor, m_markerEditor->getAccelerators());

    m_markerEditor->show();
}

void
RosegardenGUIApp::slotMarkerEditorClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotMarkerEditorClosed" << endl;

    m_markerEditor = 0;
}

void
RosegardenGUIApp::slotControlEditorClosed()
{
    const QObject *s = sender();

    RG_DEBUG << "RosegardenGUIApp::slotControlEditorClosed" << endl;

    for (std::set<ControlEditorDialog *>::iterator i = m_controlEditors.begin();
	 i != m_controlEditors.end(); ++i) {
	if (*i == s) {
	    m_controlEditors.erase(i);
	    RG_DEBUG << "removed control editor dialog, have " << m_controlEditors.size() << " left" << endl;
	    return;
	}
    }

    std::cerr << "WARNING: control editor " << s << " closed, but couldn't find it in our control editor list (we have " << m_controlEditors.size() << " editors)" << std::endl;
}

void
RosegardenGUIApp::slotShowPluginDialog(QWidget *parent,
				       Rosegarden::InstrumentId instrumentId,
				       int index)
{
    if (!parent) parent = this;

    int key = (index << 24) + instrumentId;

    if (m_pluginDialogs[key]) {
	m_pluginDialogs[key]->raise();
	return;
    }

    Rosegarden::Instrument *instrument = m_doc->getStudio().
        getInstrumentById(instrumentId);
    
    // only create a dialog if we've got a plugin instance
    Rosegarden::AudioPluginInstance *inst = 
	instrument->getPlugin(index);

    if (!inst) {
	RG_DEBUG << "RosegardenGUIApp::slotShowPluginDialog - "
		 << "no AudioPluginInstance found for index "
		 << index << endl;
	return;
    }

    // Create the plugin dialog
    //
    Rosegarden::AudioPluginDialog *dialog =
	new Rosegarden::AudioPluginDialog(parent,
					  m_doc->getPluginManager(),
					  instrument,
					  index);

    // Plug the new dialog into the standard keyboard accelerators so
    // that we can use them still while the plugin has focus.
    //
    plugAccelerators(dialog, dialog->getAccelerators());

    // connect up to the instrument parameter box so it can update its label
    connect(dialog,
	    SIGNAL(pluginSelected(Rosegarden::InstrumentId, int, int)),
	    m_instrumentParameterBox,
	    SLOT(slotPluginSelected(Rosegarden::InstrumentId, int, int)));

    connect(dialog,
	    SIGNAL(bypassed(Rosegarden::InstrumentId, int, bool)),
	    m_instrumentParameterBox,
	    SLOT(slotPluginBypassed(Rosegarden::InstrumentId, int, bool)));

    // and to the mixer, if we have it
    if (m_audioMixer) {
	connect(dialog,
		SIGNAL(pluginSelected(Rosegarden::InstrumentId, int, int)),
		m_audioMixer,
		SLOT(slotPluginSelected(Rosegarden::InstrumentId, int, int)));

	connect(dialog,
		SIGNAL(bypassed(Rosegarden::InstrumentId, int, bool)),
		m_audioMixer,
		SLOT(slotPluginBypassed(Rosegarden::InstrumentId, int, bool)));
    }
    
    // and to us so we can keep the sequencer informed
    connect(dialog,
	    SIGNAL(pluginSelected(Rosegarden::InstrumentId, int, int)),
	    this,
	    SLOT(slotPluginSelected(Rosegarden::InstrumentId, int, int)));
    
    connect(dialog,
	    SIGNAL(pluginPortChanged(Rosegarden::InstrumentId, int, int, float)),
	    this,
	    SLOT(slotPluginPortChanged(Rosegarden::InstrumentId, int, int, float)));

    connect(dialog,
	    SIGNAL(bypassed(Rosegarden::InstrumentId, int, bool)),
	    this,
	    SLOT(slotPluginBypassed(Rosegarden::InstrumentId, int, bool)));

    connect(dialog,
	    SIGNAL(destroyed(Rosegarden::InstrumentId, int)),
	    this,
	    SLOT(slotPluginDialogDestroyed(Rosegarden::InstrumentId, int)));

    m_pluginDialogs[key] = dialog;
    m_pluginDialogs[key]->show();

    // Set modified
    m_doc->slotDocumentModified();
}  

void
RosegardenGUIApp::slotPluginSelected(Rosegarden::InstrumentId instrumentId,
				     int index, int plugin)
{
    Rosegarden::Instrument *instrument = m_doc->getStudio().
        getInstrumentById(instrumentId);
    
    Rosegarden::AudioPluginInstance *inst = 
        instrument->getPlugin(index);

    if (!inst) {
        RG_DEBUG << "RosegardenGUIApp::slotPluginSelected - "
                 << "got index of unknown plugin!" << endl;
	return;
    }

    if (plugin == -1)
    {
	// Destroy plugin instance
	if (Rosegarden::StudioControl::
	    destroyStudioObject(inst->getMappedId()))
	{
	    RG_DEBUG << "RosegardenGUIApp::slotPluginSelected - "
		     << "cannot destroy Studio object "
		     << inst->getMappedId() << endl;
	}
	
	inst->setAssigned(false);
    }
    else
    {
	Rosegarden::AudioPlugin *plgn = 
	    m_doc->getPluginManager()->getPlugin(plugin);
	
	// If unassigned then create a sequencer instance of this
	// AudioPluginInstance.
	//
	if (inst->isAssigned())
	{
	    // unassign, destory and recreate
	    std::cout << "MAPPED ID = " << inst->getMappedId() 
		      << " for Instrument " << inst->getId() << std::endl;
	    
	    RG_DEBUG << "RosegardenGUIApp::slotPluginSelected - "
		     << "MappedObjectId = "
		     << inst->getMappedId()
		     << " - UniqueId = " << plgn->getUniqueId()
		     << endl;
	    
#ifdef HAVE_LADSPA
	    Rosegarden::StudioControl::setStudioObjectProperty
		(inst->getMappedId(),
		 Rosegarden::MappedLADSPAPlugin::UniqueId,
		 plgn->getUniqueId());
#endif
	}
	else
	{
	    // create a studio object at the sequencer
	    Rosegarden::MappedObjectId newId =
		Rosegarden::StudioControl::createStudioObject
		(Rosegarden::MappedObject::LADSPAPlugin);
	    
	    RG_DEBUG << "RosegardenGUIApp::slotPluginSelected - "
		     << " new MappedObjectId = " << newId << endl;
	    
	    // set the new Mapped ID and that this instance
	    // is assigned
	    inst->setMappedId(newId);
	    inst->setAssigned(true);
	    
#ifdef HAVE_LADSPA
	    // set the instrument id
	    Rosegarden::StudioControl::setStudioObjectProperty
		(newId,
		 Rosegarden::MappedObject::Instrument,
		 Rosegarden::MappedObjectValue(instrumentId));
	    
	    // set the position
	    Rosegarden::StudioControl::setStudioObjectProperty
		(newId,
		 Rosegarden::MappedObject::Position,
		 Rosegarden::MappedObjectValue(index));
	    
	    // set the plugin id
	    Rosegarden::StudioControl::setStudioObjectProperty
		(newId,
		 Rosegarden::MappedLADSPAPlugin::UniqueId,
		 Rosegarden::MappedObjectValue(
		     plgn->getUniqueId()));
#endif
	}
    }
    
    // Set modified
    m_doc->slotDocumentModified();
}

void
RosegardenGUIApp::slotPluginPortChanged(Rosegarden::InstrumentId instrumentId,
					int pluginIndex,
					int portIndex,
					float value)
{
    Rosegarden::Instrument *instrument = m_doc->getStudio().
        getInstrumentById(instrumentId);
    
    Rosegarden::AudioPluginInstance *inst = instrument->getPlugin(pluginIndex);

    if (inst)
    {

#ifdef HAVE_LADSPA

        Rosegarden::StudioControl::
            setStudioPluginPort(inst->getMappedId(),
                                portIndex,
                                value);
                                
        RG_DEBUG << "RosegardenGUIApp::slotPluginPortChanged - "
                 << "setting plugin port (" << portIndex << ") to "
                 << value << endl;

        // Set modified
        m_doc->slotDocumentModified();

#endif // HAVE_LADSPA

	(void)portIndex; // avoid compiler warnings
	(void)value;
    }

}

void
RosegardenGUIApp::slotPluginDialogDestroyed(Rosegarden::InstrumentId instrumentId,
					    int index)
{
    int key = (index << 24) + instrumentId;
    m_pluginDialogs[key] = 0;
}

void
RosegardenGUIApp::slotPluginBypassed(Rosegarden::InstrumentId instrumentId,
				     int pluginIndex, bool bp)
{
    Rosegarden::Instrument *instrument = m_doc->getStudio().
        getInstrumentById(instrumentId);
    
    Rosegarden::AudioPluginInstance *inst = instrument->getPlugin(pluginIndex);

    if (inst)
    {
#ifdef HAVE_LADSPA
        Rosegarden::StudioControl::setStudioObjectProperty
            (inst->getMappedId(),
             Rosegarden::MappedLADSPAPlugin::Bypassed,
             Rosegarden::MappedObjectValue(bp));
#endif // HAVE_LADSPA

        // Set the bypass on the instance
        //
        inst->setBypass(bp);

        // Set modified
        m_doc->slotDocumentModified();
    }
}

void
RosegardenGUIApp::slotPlayList()
{
    if (!m_playList) {
        m_playList = new PlayListDialog(i18n("Play List"), this);
        connect(m_playList, SIGNAL(closing()),
                SLOT(slotPlayListClosed()));
        connect(m_playList->getPlayList(), SIGNAL(play(QString)),
                SLOT(slotPlayListPlay(QString)));
    }

    m_playList->show();
}

void
RosegardenGUIApp::slotPlayListPlay(QString url)
{
    slotStop();
    openURL(url);
    slotPlay();
}

void
RosegardenGUIApp::slotPlayListClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotPlayListClosed()\n";
    m_playList = 0;
}

void
RosegardenGUIApp::slotBankEditorClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotBankEditorClosed()\n";

    if (m_doc->isModified()) {
        if (m_view)
            m_view->slotSelectTrackSegments(m_doc->getComposition().getSelectedTrack());
    }
    
    m_bankEditor = 0;
}

void
RosegardenGUIApp::slotDeviceManagerClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotDeviceManagerClosed()\n";

    if (m_doc->isModified()) {
        if (m_view)
            m_view->slotSelectTrackSegments(m_doc->getComposition().getSelectedTrack());
    }
    
    m_deviceManager = 0;
}

void
RosegardenGUIApp::slotAudioMixerClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotAudioMixerClosed()\n";

    m_audioMixer = 0;
}

void
RosegardenGUIApp::slotMidiMixerClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotMidiMixerClosed()\n";

    m_midiMixer = 0;
}

void
RosegardenGUIApp::slotAudioManagerClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotAudioManagerClosed()\n";

    if (m_doc->isModified()) {
        if (m_view)
            m_view->slotSelectTrackSegments(m_doc->getComposition().getSelectedTrack());
    }
    
    m_audioManagerDialog = 0;
}

void
RosegardenGUIApp::slotPanic()
{
    if (m_seqManager)
    {
        RosegardenProgressDialog progressDlg(i18n("Queueing MIDI panic events for tranmission..."),
                                             100,
                                             this);
        CurrentProgressDialog::set(&progressDlg);
        RosegardenProgressDialog::processEvents();

        connect(m_seqManager, SIGNAL(setProgress(int)),
                progressDlg.progressBar(), SLOT(setValue(int)));
        connect(m_seqManager, SIGNAL(incrementProgress(int)),
                progressDlg.progressBar(), SLOT(advance(int)));
        
        m_seqManager->panic();

    }
}

void
RosegardenGUIApp::slotSetPointer(Rosegarden::timeT t)
{
    m_doc->setPointerPosition(t);
}


void
RosegardenGUIApp::slotPopulateTrackInstrumentPopup()
{
    RG_DEBUG << "RosegardenGUIApp::slotSetTrackInstrument\n";
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *track = comp.getTrackById(comp.getSelectedTrack());

    if (!track) {
	RG_DEBUG << "Weird: no track available for instrument popup!" << endl;
	return;
    }

    Rosegarden::Instrument* instrument = m_doc->getStudio().getInstrumentById(track->getInstrument());

    QPopupMenu* popup = dynamic_cast<QPopupMenu*>(factory()->container("set_track_instrument", this));

    m_view->getTrackEditor()->getTrackButtons()->populateInstrumentPopup(instrument, popup);
}

void
RosegardenGUIApp::slotRemapInstruments()
{
    RG_DEBUG << "RosegardenGUIApp::slotRemapInstruments\n";
    RemapInstrumentDialog dialog(this, m_doc);

    connect(&dialog, SIGNAL(applyClicked()),
            m_view->getTrackEditor()->getTrackButtons(),
            SLOT(slotSynchroniseWithComposition()));

    if (dialog.exec() == QDialog::Accepted)
    {
        RG_DEBUG << "slotRemapInstruments - accepted\n";
    }

}

void
RosegardenGUIApp::slotSaveDefaultStudio()
{
    RG_DEBUG << "RosegardenGUIApp::slotSaveDefaultStudio\n";

    int reply = KMessageBox::warningYesNo
        (this, i18n("Are you sure you want to save this as your default studio?"));
    
    if (reply != KMessageBox::Yes) return;

    KTmpStatusMsg msg(i18n("Saving current document as default studio..."), this);

    QString autoloadFile = ::locateLocal("appdata", "autoload.rg");

    RG_DEBUG << "RosegardenGUIApp::slotSaveDefaultStudio : saving studio in "
             << autoloadFile << endl;
    
    SetWaitCursor waitCursor;
    m_doc->saveDocument(autoloadFile);
}

void
RosegardenGUIApp::slotImportDefaultStudio()
{
    int reply = KMessageBox::warningYesNo
        (this, i18n("Are you sure you want to import your default studio and lose the current one?"));
    
    if (reply != KMessageBox::Yes) return;

    QString autoloadFile =
        KGlobal::dirs()->findResource("appdata", "autoload.rg"); 

    QFileInfo autoloadFileInfo(autoloadFile);

    if (!autoloadFileInfo.isReadable())
    {
        RG_DEBUG << "RosegardenGUIDoc::slotImportDefaultStudio - "
                 << "can't find autoload file - defaulting" << endl;
        return;
    }

    slotImportStudioFromFile(autoloadFile);
}

void
RosegardenGUIApp::slotImportStudio()
{
    RG_DEBUG << "RosegardenGUIApp::slotImportStudio()\n";

    QString studioDir = KGlobal::dirs()->findResource("appdata", "library/");
    QDir dir(studioDir);
    if (!dir.exists()) {
        studioDir = ":ROSEGARDENDEVICE";
    } else {
        studioDir = "file://" + studioDir;
    }

    KURL url = KFileDialog::getOpenURL
        (
#if KDE_VERSION >= 196614
         studioDir,
#else
         QString::null,
#endif
         "audio/x-rosegarden-device audio/x-rosegarden",
         this, i18n("Import Studio from File"));

    if (url.isEmpty()) return;

    QString target;
    if (KIO::NetAccess::download(url, target) == false) {
        KMessageBox::error(this, i18n("Cannot download file %1")
                           .arg(url.prettyURL()));
        return;
    }

    slotImportStudioFromFile(target);
}

void
RosegardenGUIApp::slotImportStudioFromFile(const QString &file)
{
    RosegardenGUIDoc *doc = new RosegardenGUIDoc(this, 0, true); // skipAutoload

    Rosegarden::Studio &oldStudio = m_doc->getStudio();
    Rosegarden::Studio &newStudio =   doc->getStudio();

    // Add some dummy devices for when we open the document.  We guess
    // that the file won't have more than 32 devices.
    //
//    for (unsigned int i = 0; i < 32; i++) {
//        newStudio.addDevice("", i, Rosegarden::Device::Midi);
//    }

    if (doc->openDocument(file, true)) { // true because we actually
					 // do want to create devices
	                                 // on the sequencer here

	KMacroCommand *command = new KMacroCommand(i18n("Import Studio"));
	doc->syncDevices();

	// We actually only copy across MIDI play devices... for now
	std::vector<Rosegarden::DeviceId> midiPlayDevices;

	for (Rosegarden::DeviceList::const_iterator i =
		 oldStudio.begin(); i != oldStudio.end(); ++i) {

	    Rosegarden::MidiDevice *md =
		dynamic_cast<Rosegarden::MidiDevice *>(*i);

	    if (md && (md->getDirection() == Rosegarden::MidiDevice::Play)) {
		midiPlayDevices.push_back((*i)->getId());
	    }
	}

	std::vector<Rosegarden::DeviceId>::iterator di(midiPlayDevices.begin());

	for (Rosegarden::DeviceList::const_iterator i =
		 newStudio.begin(); i != newStudio.end(); ++i) {

	    Rosegarden::MidiDevice *md =
		dynamic_cast<Rosegarden::MidiDevice *>(*i);

	    if (md && (md->getDirection() == Rosegarden::MidiDevice::Play)) {
		if (di != midiPlayDevices.end()) {
		    Rosegarden::MidiDevice::VariationType variation
			(md->getVariationType());
		    Rosegarden::BankList bl(md->getBanks());
		    Rosegarden::ProgramList pl(md->getPrograms());
		    Rosegarden::ControlList cl(md->getControlParameters());

                    ModifyDeviceCommand* mdCommand = new ModifyDeviceCommand(&oldStudio,
                                                                             *di,
                                                                             md->getName(),
                                                                             md->getLibrarianName(),
                                                                             md->getLibrarianEmail());
                    mdCommand->setVariation(variation);
                    mdCommand->setBankList(bl);
                    mdCommand->setProgramList(pl);
                    mdCommand->setControlList(cl);
                    mdCommand->setOverwrite(true);
                    mdCommand->setRename(md->getName() != "");

		    command->addCommand(mdCommand);
		    ++di;
		}
	    }
	}
	
	while (di != midiPlayDevices.end()) {
	    command->addCommand(new CreateOrDeleteDeviceCommand
				(&oldStudio,
				 *di));
	}

	oldStudio.setMIDIThruFilter(newStudio.getMIDIThruFilter());
	oldStudio.setMIDIRecordFilter(newStudio.getMIDIRecordFilter());

	m_doc->getCommandHistory()->addCommand(command);
	m_doc->syncDevices();
    }

    delete doc;
}

void
RosegardenGUIApp::slotResetMidiNetwork()
{
    if (m_seqManager) {
        
        m_seqManager->preparePlayback(true);

        m_seqManager->resetMidiNetwork();
    }
    
}


void
RosegardenGUIApp::slotModifyMIDIFilters()
{
    RG_DEBUG << "RosegardenGUIApp::slotModifyMIDIFilters" << endl;

    MidiFilterDialog dialog(this, m_doc);

    if (dialog.exec() == QDialog::Accepted)
    {
        RG_DEBUG << "slotModifyMIDIFilters - accepted" << endl;
    }
}

void
RosegardenGUIApp::slotManageMetronome()
{
    RG_DEBUG << "RosegardenGUIApp::slotManageMetronome" << endl;

    ManageMetronomeDialog dialog(this, m_doc);

    if (dialog.exec() == QDialog::Accepted)
    {
        RG_DEBUG << "slotManageMetronome - accepted" << endl;
    }
}

void 
RosegardenGUIApp::slotAutoSave()
{
    if (!m_seqManager ||
        m_seqManager->getTransportStatus() == PLAYING ||
        m_seqManager->getTransportStatus() == RECORDING_MIDI ||
        m_seqManager->getTransportStatus() == RECORDING_AUDIO)
        return;

    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    if (!config->readBoolEntry("autosave", true)) return;

    m_doc->slotAutoSave();
}


void
RosegardenGUIApp::slotUpdateAutoSaveInterval(unsigned int interval)
{
    RG_DEBUG << "RosegardenGUIApp::slotUpdateAutoSaveInterval - "
             << "changed interval to " << interval << endl;
    m_autoSaveTimer->changeInterval(int(interval) * 1000);
}


void
RosegardenGUIApp::slotShowTip()
{
    RG_DEBUG << "RosegardenGUIApp::slotShowTip" << endl;
    KTipDialog::showTip(this, locate("data", "rosegarden/tips"), true);
}

const void* RosegardenGUIApp::SequencerExternal = (void*)-1;
const char* const RosegardenGUIApp::MainWindowConfigGroup = "MainView";
