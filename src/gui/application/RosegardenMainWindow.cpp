/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <Q3Canvas>

#include "RosegardenMainWindow.h"

#include "gui/editors/segment/TrackEditor.h"
#include "gui/editors/segment/TrackButtons.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "gui/application/TransportStatus.h"
#include "base/AnalysisTypes.h"
#include "base/AudioPluginInstance.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/CompositionTimeSliceAdapter.h"
#include "base/Configuration.h"
#include "base/Device.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/Selection.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "commands/edit/CopyCommand.h"
#include "commands/edit/CutCommand.h"
#include "commands/edit/EventQuantizeCommand.h"
#include "commands/edit/PasteSegmentsCommand.h"
#include "commands/edit/TransposeCommand.h"
#include "commands/edit/AddMarkerCommand.h"
#include "commands/edit/ModifyMarkerCommand.h"
#include "commands/edit/RemoveMarkerCommand.h"
#include "commands/notation/KeyInsertionCommand.h"
#include "commands/segment/AddTempoChangeCommand.h"
#include "commands/segment/AddTimeSignatureAndNormalizeCommand.h"
#include "commands/segment/AddTimeSignatureCommand.h"
#include "commands/segment/AudioSegmentAutoSplitCommand.h"
#include "commands/segment/AudioSegmentRescaleCommand.h"
#include "commands/segment/AudioSegmentSplitCommand.h"
#include "commands/segment/ChangeCompositionLengthCommand.h"
#include "commands/segment/CreateTempoMapFromSegmentCommand.h"
#include "commands/segment/CutRangeCommand.h"
#include "commands/segment/DeleteRangeCommand.h"
#include "commands/segment/InsertRangeCommand.h"
#include "commands/segment/ModifyDefaultTempoCommand.h"
#include "commands/segment/MoveTracksCommand.h"
#include "commands/segment/PasteRangeCommand.h"
#include "commands/segment/RemoveTempoChangeCommand.h"
#include "commands/segment/SegmentAutoSplitCommand.h"
#include "commands/segment/SegmentChangeTransposeCommand.h"
#include "commands/segment/SegmentJoinCommand.h"
#include "commands/segment/SegmentLabelCommand.h"
#include "commands/segment/SegmentReconfigureCommand.h"
#include "commands/segment/SegmentRescaleCommand.h"
#include "commands/segment/SegmentSplitByPitchCommand.h"
#include "commands/segment/SegmentSplitByRecordingSrcCommand.h"
#include "commands/segment/SegmentSplitCommand.h"
#include "commands/segment/SegmentTransposeCommand.h"
#include "commands/studio/CreateOrDeleteDeviceCommand.h"
#include "commands/studio/ModifyDeviceCommand.h"
#include "document/io/CsoundExporter.h"
#include "document/io/HydrogenLoader.h"
#include "document/io/LilyPondExporter.h"
#include "document/CommandHistory.h"
#include "document/io/RG21Loader.h"
#include "document/io/MupExporter.h"
#include "document/io/MusicXmlExporter.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/dialogs/AddTracksDialog.h"
#include "gui/dialogs/AudioManagerDialog.h"
#include "gui/dialogs/AudioPluginDialog.h"
#include "gui/dialogs/AudioSplitDialog.h"
#include "gui/dialogs/BeatsBarsDialog.h"
#include "gui/dialogs/CompositionLengthDialog.h"
#include "gui/dialogs/ConfigureDialog.h"
#include "gui/dialogs/CountdownDialog.h"
#include "gui/dialogs/DocumentConfigureDialog.h"
#include "gui/dialogs/FileMergeDialog.h"
#include "gui/dialogs/IdentifyTextCodecDialog.h"
#include "gui/dialogs/IntervalDialog.h"
#include "gui/dialogs/LilyPondOptionsDialog.h"
#include "gui/dialogs/ManageMetronomeDialog.h"
#include "gui/dialogs/QuantizeDialog.h"
#include "gui/dialogs/RescaleDialog.h"
#include "gui/dialogs/SplitByPitchDialog.h"
#include "gui/dialogs/SplitByRecordingSrcDialog.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/dialogs/TimeDialog.h"
#include "gui/dialogs/TimeSignatureDialog.h"
#include "gui/dialogs/TransportDialog.h"
#include "gui/editors/parameters/InstrumentParameterBox.h"
#include "gui/editors/parameters/RosegardenParameterArea.h"
#include "gui/editors/parameters/SegmentParameterBox.h"
#include "gui/editors/parameters/TrackParameterBox.h"
#include "gui/editors/segment/compositionview/CompositionView.h"
#include "gui/editors/segment/ControlEditorDialog.h"
#include "gui/editors/segment/MarkerEditor.h"
#include "gui/editors/segment/PlayListDialog.h"
#include "gui/editors/segment/PlayList.h"
#include "gui/editors/segment/compositionview/SegmentEraser.h"
#include "gui/editors/segment/compositionview/SegmentJoiner.h"
#include "gui/editors/segment/compositionview/SegmentMover.h"
#include "gui/editors/segment/compositionview/SegmentPencil.h"
#include "gui/editors/segment/compositionview/SegmentResizer.h"
#include "gui/editors/segment/compositionview/SegmentSelector.h"
#include "gui/editors/segment/compositionview/SegmentSplitter.h"
#include "gui/editors/segment/compositionview/SegmentToolBox.h"
#include "gui/editors/segment/TrackLabel.h"
#include "gui/editors/segment/TriggerSegmentManager.h"
#include "gui/editors/tempo/TempoView.h"
#include "gui/general/EditViewBase.h"
#include "gui/general/IconLoader.h"
#include "gui/general/FileSource.h"
#include "gui/general/ResourceFinder.h"
#include "gui/general/AutoSaveFinder.h"
#include "gui/general/LilyPondProcessor.h"
#include "gui/general/ProjectPackager.h"
#include "gui/widgets/StartupLogo.h"
#include "gui/widgets/TmpStatusMsg.h"
#include "gui/seqmanager/MidiFilterDialog.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/studio/AudioMixerWindow.h"
#include "gui/studio/AudioPlugin.h"
#include "gui/studio/AudioPluginManager.h"
#include "gui/studio/AudioPluginOSCGUIManager.h"
#include "gui/studio/BankEditorDialog.h"
#include "gui/studio/DeviceManagerDialog.h"
#include "gui/studio/MidiMixerWindow.h"
#include "gui/studio/RemapInstrumentDialog.h"
#include "gui/studio/StudioControl.h"
#include "gui/studio/SynthPluginManagerDialog.h"
#include "gui/widgets/CurrentProgressDialog.h"
#include "gui/widgets/ProgressBar.h"
#include "gui/widgets/ProgressDialog.h"
#include "LircClient.h"
#include "LircCommander.h"
#include "RosegardenMainViewWidget.h"
#include "SetWaitCursor.h"
#include "sequencer/RosegardenSequencer.h"
#include "sequencer/SequencerThread.h"
#include "sound/AudioFile.h"
#include "sound/AudioFileManager.h"
#include "sound/MappedCommon.h"
#include "sound/MappedComposition.h"
#include "sound/MappedEvent.h"
#include "sound/MappedStudio.h"
#include "sound/MidiFile.h"
#include "sound/PluginIdentifier.h"
#include "sound/SoundDriver.h"
#include "StartupTester.h"
#include "gui/widgets/TmpStatusMsg.h"
#include "gui/studio/DeviceManagerDialog.h"
#include "gui/widgets/InputDialog.h"

#include <QApplication>
#include <QSettings>
#include <QShortcut>
#include <QDockWidget>
#include <QMessageBox>
#include <QProcess>
#include <QTemporaryFile>
#include <QToolTip>
#include <QByteArray>
#include <QCursor>
#include <QDataStream>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QLabel>
#include <QObject>
#include <QObjectList>
#include <QPixmap>
#include <QToolTip>
#include <QPushButton>
#include <QRegExp>
#include <QSlider>
#include <QString>
#include <QStringList>
#include <QTextCodec>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <QPushButton>
#include <QToolButton>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QUrl>
#include <QDialog>
#include <QFileDialog>
#include <QPrintDialog>
#include <QColorDialog>
#include <QFontDialog>
#include <QPageSetupDialog>

#ifdef HAVE_LIBJACK
#include <jack/jack.h>
#endif


namespace Rosegarden
{

RosegardenMainWindow::RosegardenMainWindow(bool useSequencer,
                                   QObject *startupStatusMessageReceiver) :
    QMainWindow(0),
    m_actionsSetup(false),
    m_view(0),
    m_swapView(0),
    m_mainDockWidget(0),
    m_dockLeft(0),
    m_doc(0),
    m_sequencerThread(0),
    m_sequencerCheckedIn(false),
#ifdef HAVE_LIBJACK
    m_jackProcess(0),
#endif
    m_zoomSlider(0),
    m_seqManager(0),
    m_transport(0),
    m_audioManagerDialog(0),
    m_originatingJump(false),
    m_storedLoopStart(0),
    m_storedLoopEnd(0),
    m_useSequencer(useSequencer),
    m_dockVisible(true),
    m_autoSaveTimer(new QTimer(static_cast<QObject *>(this))),
    m_clipboard(new Clipboard),
    m_playList(0),
    m_synthManager(0),
    m_audioMixer(0),
    m_midiMixer(0),
    m_bankEditor(0),
    m_markerEditor(0),
    m_tempoView(0),
    m_triggerSegmentManager(0),
    m_pluginGUIManager(new AudioPluginOSCGUIManager(this)),
    m_playTimer(new QTimer(static_cast<QObject *>(this))),
    m_stopTimer(new QTimer(static_cast<QObject *>(this))),
    m_startupTester(0),
#ifdef HAVE_LIRC
    m_lircClient(0),
    m_lircCommander(0),
#endif
    m_firstRun(false),
    m_haveAudioImporter(false),
    m_parameterArea(0),
    m_devicesManagerNew(0)
{
    setObjectName("App");
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
#define OFFER_JACK_START_OPTION 1
#ifdef OFFER_JACK_START_OPTION
        // First we check if jackd is running already

        std::string jackClientName = "rosegarden";

        // attempt connection to JACK server
        //
        jack_client_t* testJackClient;
        testJackClient = jack_client_new(jackClientName.c_str());
        if (testJackClient == 0) {

            // If no connection to JACK
            // try to launch JACK - if the configuration wants us to.
            if (!launchJack()) {
                StartupLogo::hideIfStillThere();
                QMessageBox::critical(dynamic_cast<QWidget*>(this), "", tr("Attempted to launch JACK audio daemon failed.  Audio will be disabled.\nPlease check configuration (Settings -> Configure Rosegarden -> Audio -> Startup)\n and restart."));
            }
        } else {
            //this client was just for testing
            jack_client_close(testJackClient);
        }
#endif // OFFER_JACK_START_OPTION
#endif // HAVE_LIBJACK

        // This causes the QPainter::begin debug message
        emit startupStatusMessage(tr("Starting sequencer..."));
        launchSequencer();

    } else
        RG_DEBUG << "RosegardenMainWindow : don't use sequencer\n";

    // Plugin manager
    //
    emit startupStatusMessage(tr("Initializing plugin manager..."));
    m_pluginManager = new AudioPluginManager();

    // call inits to invoke all other construction parts
    //
    emit startupStatusMessage(tr("Initializing view...")); 
    initStatusBar();
    setupActions();
    initZoomToolbar();

    QPixmap dummyPixmap; // any icon will do
    
    
    
    // start of docking code 
    this->setDockOptions(// QMainWindow::AllowNestedDocks // not required with ::ForceTabbedDocks
                            QMainWindow::AnimatedDocks
//                            |QMainWindow::ForceTabbedDocks
                            //|QMainWindow::VerticalTabs
                       );
    


    
    
/*
    connect(m_dockLeft, SIGNAL(iMBeingClosed()),
            this, SLOT(slotParametersClosed()));
    connect(m_dockLeft, SIGNAL(hasUndocked()),
            this, SLOT(slotParametersClosed()));
*/
    
    // Apparently, hasUndocked() is emitted when the dock widget's
    // 'close' button on the dock handle is clicked.
    //
    //&&& disabled mainDockWidget connection, may not be required
//    connect(m_mainDockWidget, SIGNAL(docking(QDockWidget*, QDockWidget::DockPosition)),
//            this, SLOT(slotParametersDockedBack(QDockWidget*, QDockWidget::DockPosition)));
    

    RosegardenDocument* doc = new RosegardenDocument(this, m_pluginManager);

    m_dockLeft = new QDockWidget(tr("Special Parameters"), this);
    m_dockLeft->setMinimumSize(180, 200);    //### fix arbitrary value for min-size
    addDockWidget(Qt::LeftDockWidgetArea, m_dockLeft);

    m_dockLeft->setFeatures(QDockWidget::DockWidgetMovable
            | QDockWidget::DockWidgetFloatable
//            | QDockWidget::DockWidgetVerticalTitleBar
            | QDockWidget::DockWidgetClosable
                       );
    

    m_parameterArea = new RosegardenParameterArea(m_dockLeft, "RosegardenParameterArea");
    m_dockLeft->setWidget(m_parameterArea);

    // Populate the parameter-box area with the respective
    // parameter box widgets.
/*@@@
    QDockWidget *dw = new QDockWidget(tr("Segment Parameters"), this);
    m_segmentParameterBox = new SegmentParameterBox(doc, dw);
    dw->setWidget(m_segmentParameterBox);
    addDockWidget(Qt::LeftDockWidgetArea, dw);

    dw = new QDockWidget(tr("Track Parameters"), this);
    m_trackParameterBox = new TrackParameterBox(doc, dw);
    dw->setWidget(m_trackParameterBox);
    addDockWidget(Qt::LeftDockWidgetArea, dw);


    dw = new QDockWidget(tr("Instrument Parameters"), this);
    m_instrumentParameterBox = new InstrumentParameterBox(doc, dw);
    dw->setWidget(m_instrumentParameterBox);
    addDockWidget(Qt::LeftDockWidgetArea, dw);
*/

    m_segmentParameterBox = new SegmentParameterBox(doc, m_parameterArea);
    m_parameterArea->addRosegardenParameterBox(m_segmentParameterBox);
    m_trackParameterBox = new TrackParameterBox(doc, m_parameterArea);
    m_parameterArea->addRosegardenParameterBox(m_trackParameterBox);
    m_instrumentParameterBox = new InstrumentParameterBox(doc, m_parameterArea);
    m_parameterArea->addRosegardenParameterBox(m_instrumentParameterBox);
//    m_instrumentParameterBox = new InstrumentParameterBox(doc, 0);
//    m_parameterArea->addRosegardenParameterBox(m_instrumentParameterBox);

    // Now that we've added the parameter boxes, we set this as the QScrollArea's widget
    m_parameterArea->setScrollAreaWidget();
    m_dockLeft->setMaximumWidth(m_dockLeft->sizeHint().width());

    // Lookup the configuration parameter that specifies the default
    // arrangement, and instantiate it.
    
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);

    m_parameterArea->setArrangement((RosegardenParameterArea::Arrangement)
                                    settings.value("sidebarstyle",
                                    RosegardenParameterArea::CLASSIC_STYLE).toUInt());

//    m_dockLeft->update();

    connect(m_instrumentParameterBox,
            SIGNAL(selectPlugin(QWidget *, InstrumentId, int)),
            this,
            SLOT(slotShowPluginDialog(QWidget *, InstrumentId, int)));

    connect(m_instrumentParameterBox,
            SIGNAL(showPluginGUI(InstrumentId, int)),
            this,
            SLOT(slotShowPluginGUI(InstrumentId, int)));

    // relay this through our own signal so that others can use it too
    connect(m_instrumentParameterBox,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            this,
            SIGNAL(instrumentParametersChanged(InstrumentId)));

    connect(this,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            m_instrumentParameterBox,
            SLOT(slotInstrumentParametersChanged(InstrumentId)));

    connect(this,
            SIGNAL(pluginSelected(InstrumentId, int, int)),
            m_instrumentParameterBox,
            SLOT(slotPluginSelected(InstrumentId, int, int)));

    connect(this,
            SIGNAL(pluginBypassed(InstrumentId, int, bool)),
            m_instrumentParameterBox,
            SLOT(slotPluginBypassed(InstrumentId, int, bool)));

    // Load the initial document (this includes doc's own autoload)
    //
    setDocument(doc);

    emit startupStatusMessage(tr("Starting sequence manager..."));

    // transport is created by setupActions()
    m_seqManager = new SequenceManager(m_doc, getTransport());

    if (m_useSequencer) {
        // Check the sound driver status and warn the user of any
        // problems.  This warning has to happen early, in case it
        // affects the ability to load plugins etc from a file on the
        // command line.
        m_seqManager->checkSoundDriverStatus(true);
    }

    if (m_view) {
        connect(m_seqManager, SIGNAL(controllerDeviceEventReceived(MappedEvent *)),
                m_view, SLOT(slotControllerDeviceEventReceived(MappedEvent *)));
                
                
        
        MIDIInstrumentParameterPanel *mipp;
        mipp = m_instrumentParameterBox->getMIDIInstrumentParameterPanel();
        if(! mipp){
            RG_DEBUG << "Error: m_instrumentParameterBox->getMIDIInstrumentParameterPanel() is NULL in RosegardenMainWindow.cpp 445 \n";
        }
        connect(m_seqManager, SIGNAL(signalSelectProgramNoSend(int,int,int)), (QObject*)mipp, SLOT(slotSelectProgramNoSend(int,int,int)));
                
    }

    if (m_seqManager->getSoundDriverStatus() & AUDIO_OK) {
        slotStateChanged("got_audio", true);
    } else {
        slotStateChanged("got_audio", false);
    }

    // If we're restarting the gui then make sure any transient
    // studio objects are cleared away.
    emit startupStatusMessage(tr("Clearing studio data..."));
    m_seqManager->reinitialiseSequencerStudio();

    // Send the transport control statuses for MMC and JACK
    //
    m_seqManager->sendTransportControlStatuses();

    // Now autoload
    //
    enterActionState("new_file"); //@@@ JAS orig. 0
    leaveActionState("have_segments"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    leaveActionState("have_selection"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    slotTestClipboard();

    // Check for lack of MIDI devices and disable Studio options accordingly
    //
    if (!m_doc->getStudio().haveMidiDevices())
        leaveActionState("got_midi_devices"); //@@@ JAS orig. KXMLGUIClient::StateReverse

    emit startupStatusMessage(tr("Starting..."));

    setupFileDialogSpeedbar();
    readOptions();

    // All toolbars should be created before this is called
    //### implement or find alternative : rgTempQtIV->setAutoSaveSettings(MainWindowConfigGroup, true);

#ifdef HAVE_LIRC

    try {
        m_lircClient = new LircClient();
    } catch (Exception e) {
        RG_DEBUG << e.getMessage().c_str() << endl;
        // continue without
        m_lircClient = 0;
    }
    if (m_lircClient) {
        m_lircCommander = new LircCommander(m_lircClient, this);
    }
#endif

    leaveActionState("have_project_packager"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    leaveActionState("have_lilypondview"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    QTimer::singleShot(1000, this, SLOT(slotTestStartupTester()));

    settings.endGroup();
}

RosegardenMainWindow::~RosegardenMainWindow()
{
    RG_DEBUG << "~RosegardenMainWindow()\n";

    if (getView() &&
            getView()->getTrackEditor() &&
            getView()->getTrackEditor()->getCompositionView()) {
        getView()->getTrackEditor()->getCompositionView()->endAudioPreviewGeneration();
    }

    delete m_pluginGUIManager;

    if (isSequencerRunning()) {
        RosegardenSequencer::getInstance()->quit();
        usleep(300000);
        delete m_sequencerThread;
    }

    delete m_transport;

    delete m_seqManager;

#ifdef HAVE_LIRC

    delete m_lircCommander;
    delete m_lircClient;
#endif

    delete m_doc;
    Profiles::getInstance()->dump();
}

void RosegardenMainWindow::setupActions()
{
    createAction("file_new", SLOT(slotFileNew()));
    createAction("file_open", SLOT(slotFileOpen()));
    createAction("file_save", SLOT(slotFileSave()));
    createAction("file_save_as", SLOT(slotFileSaveAs()));
    createAction("file_save_as_template", SLOT(slotFileSaveAsTemplate()));
    createAction("file_revert", SLOT(slotRevertToSaved()));
    createAction("file_close", SLOT(slotFileClose()));
    createAction("file_print", SLOT(slotFilePrint()));
    createAction("file_print_preview", SLOT(slotFilePrintPreview()));
    createAction("file_quit", SLOT(slotQuit()));

    createAction("edit_cut", SLOT(slotEditCut()));
    createAction("edit_copy", SLOT(slotEditCopy()));
    createAction("edit_paste", SLOT(slotEditPaste()));

    //!!! I don't like Save Settings -- I think we should lose it, and
    // just always save them
    createAction("options_save_options", SLOT(slotSaveOptions()));

    createAction("options_configure", SLOT(slotConfigure()));

    createAction("file_import_project", SLOT(slotImportProject()));
    createAction("file_import_midi", SLOT(slotImportMIDI()));
    createAction("file_import_rg21", SLOT(slotImportRG21()));
    createAction("file_import_hydrogen", SLOT(slotImportHydrogen()));
    createAction("file_merge", SLOT(slotMerge()));
    createAction("file_merge_midi", SLOT(slotMergeMIDI()));
    createAction("file_merge_rg21", SLOT(slotMergeRG21()));
    createAction("file_merge_hydrogen", SLOT(slotMergeHydrogen()));
    createAction("file_export_project", SLOT(slotExportProject()));
    createAction("file_export_midi", SLOT(slotExportMIDI()));
    createAction("file_export_lilypond", SLOT(slotExportLilyPond()));
    createAction("file_export_musicxml", SLOT(slotExportMusicXml()));
    createAction("file_export_csound", SLOT(slotExportCsound()));
    createAction("file_export_mup", SLOT(slotExportMup()));
    createAction("file_print_lilypond", SLOT(slotPrintLilyPond()));
    createAction("file_preview_lilypond", SLOT(slotPreviewLilyPond()));
    createAction("file_show_playlist", SLOT(slotPlayList()));
    createAction("tutorial", SLOT(slotTutorial()));
    createAction("guidelines", SLOT(slotBugGuidelines()));

    //&&& Connect CommandHistory to the edit menu and toolbar
    // (looking up the menu & toolbar by name)
    //!!! NO -- ActionFileParser should do this for us if the actions
    // are declared in the .rc file.  It isn't working properly atm,
    // but it needs fixing there, not here
//    new KToolBarPopupAction(tr("Und&o"), "undo", KStdAccel::shortcut(KStdAccel::Undo), actionCollection(), KStdAction::stdName(KStdAction::Undo));
//    new KToolBarPopupAction(tr("Re&do"), "redo", KStdAccel::shortcut(KStdAccel::Redo), actionCollection(), KStdAction::stdName(KStdAction::Redo));

    createAction("show_stock_toolbar", SLOT(slotToggleToolBar()));
    createAction("show_tools_toolbar", SLOT(slotToggleToolsToolBar()));
    createAction("show_tracks_toolbar", SLOT(slotToggleTracksToolBar()));
    createAction("show_editors_toolbar", SLOT(slotToggleEditorsToolBar()));
    createAction("show_transport_toolbar", SLOT(slotToggleTransportToolBar()));
    createAction("show_zoom_toolbar", SLOT(slotToggleZoomToolBar()));
    createAction("show_status_bar", SLOT(slotToggleStatusBar()));
    createAction("show_transport", SLOT(slotToggleTransport()));
    createAction("show_tracklabels", SLOT(slotToggleTrackLabels()));
    createAction("show_rulers", SLOT(slotToggleRulers()));
    createAction("show_tempo_ruler", SLOT(slotToggleTempoRuler()));
    createAction("show_chord_name_ruler", SLOT(slotToggleChordNameRuler()));
    createAction("show_previews", SLOT(slotTogglePreviews()));
    createAction("show_inst_segment_parameters", SLOT(slotDockParametersBack()));
    createAction("select", SLOT(slotPointerSelected()));
    createAction("draw", SLOT(slotDrawSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("move", SLOT(slotMoveSelected()));
    createAction("resize", SLOT(slotResizeSelected()));
    createAction("split", SLOT(slotSplitSelected()));
    createAction("join", SLOT(slotJoinSelected()));
    createAction("harmonize_selection", SLOT(slotHarmonizeSelection()));
    createAction("add_time_signature", SLOT(slotEditTimeSignature()));
    createAction("edit_tempos", SLOT(slotEditTempos()));
    createAction("cut_range", SLOT(slotCutRange()));
    createAction("copy_range", SLOT(slotCopyRange()));
    createAction("paste_range", SLOT(slotPasteRange()));
    createAction("delete_range", SLOT(slotDeleteRange()));
    createAction("insert_range", SLOT(slotInsertRange()));
    createAction("delete", SLOT(slotDeleteSelectedSegments()));
    createAction("select_all", SLOT(slotSelectAll()));
    createAction("add_tempo", SLOT(slotEditTempo()));
    createAction("change_composition_length", SLOT(slotChangeCompositionLength()));
    createAction("edit_markers", SLOT(slotEditMarkers()));
    createAction("edit_doc_properties", SLOT(slotEditDocumentProperties()));
    createAction("edit_default", SLOT(slotEdit()));
    createAction("edit_matrix", SLOT(slotEditInMatrix()));
    createAction("edit_percussion_matrix", SLOT(slotEditInPercussionMatrix()));
    createAction("edit_notation", SLOT(slotEditAsNotation()));
    createAction("edit_event_list", SLOT(slotEditInEventList()));
    createAction("quantize_selection", SLOT(slotQuantizeSelection()));
    createAction("relabel_segment", SLOT(slotRelabelSegments()));
    createAction("transpose", SLOT(slotTransposeSegments()));
    createAction("repeat_quantize", SLOT(slotRepeatQuantizeSelection()));
    createAction("rescale", SLOT(slotRescaleSelection()));
    createAction("auto_split", SLOT(slotAutoSplitSelection()));
    createAction("split_by_pitch", SLOT(slotSplitSelectionByPitch()));
    createAction("split_by_recording", SLOT(slotSplitSelectionByRecordedSrc()));
    createAction("split_at_time", SLOT(slotSplitSelectionAtTime()));
    createAction("jog_left", SLOT(slotJogLeft()));
    createAction("jog_right", SLOT(slotJogRight()));
    createAction("set_segment_start", SLOT(slotSetSegmentStartTimes()));
    createAction("set_segment_duration", SLOT(slotSetSegmentDurations()));
    createAction("join_segments", SLOT(slotJoinSegments()));
    createAction("repeats_to_real_copies", SLOT(slotRepeatingSegments()));
    createAction("manage_trigger_segments", SLOT(slotManageTriggerSegments()));
    createAction("groove_quantize", SLOT(slotGrooveQuantize()));
    createAction("set_tempo_to_segment_length", SLOT(slotTempoToSegmentLength()));
    createAction("audio_manager", SLOT(slotAudioManager()));
    createAction("show_segment_labels", SLOT(slotToggleSegmentLabels()));
    createAction("add_track", SLOT(slotAddTrack()));
    createAction("add_tracks", SLOT(slotAddTracks()));
    createAction("delete_track", SLOT(slotDeleteTrack()));
    createAction("move_track_down", SLOT(slotMoveTrackDown()));
    createAction("move_track_up", SLOT(slotMoveTrackUp()));
    createAction("select_next_track", SLOT(slotTrackDown()));
    createAction("select_previous_track", SLOT(slotTrackUp()));
    createAction("toggle_mute_track", SLOT(slotToggleMutedCurrentTrack()));
    createAction("toggle_arm_track", SLOT(slotToggleRecordCurrentTrack()));
    createAction("mute_all_tracks", SLOT(slotMuteAllTracks()));
    createAction("unmute_all_tracks", SLOT(slotUnmuteAllTracks()));
    createAction("remap_instruments", SLOT(slotRemapInstruments()));
    createAction("audio_mixer", SLOT(slotOpenAudioMixer()));
    createAction("midi_mixer", SLOT(slotOpenMidiMixer()));
    createAction("manage_midi_devices", SLOT(slotManageMIDIDevices()));
    createAction("manage_synths", SLOT(slotManageSynths()));
    createAction("modify_midi_filters", SLOT(slotModifyMIDIFilters()));
    createAction("enable_midi_routing", SLOT(slotEnableMIDIThruRouting()));
    createAction("manage_metronome", SLOT(slotManageMetronome()));
    createAction("save_default_studio", SLOT(slotSaveDefaultStudio()));
    createAction("load_default_studio", SLOT(slotImportDefaultStudio()));
    createAction("load_studio", SLOT(slotImportStudio()));
    createAction("reset_midi_network", SLOT(slotResetMidiNetwork()));
    createAction("set_quick_marker", SLOT(slotSetQuickMarker()));
    createAction("jump_to_quick_marker", SLOT(slotJumpToQuickMarker()));

// These were commented out in the old KDE3 code as well
//    createAction("insert_marker_here", SLOT(slotInsertMarkerHere()));
//    createAction("insert_marker_at_pointer", SLOT(slotInsertMarkerAtPointer()));
//    createAction("delete_marker", SLOT(slotDeleteMarker()));

    createAction("play", SLOT(slotPlay()));
    createAction("stop", SLOT(slotStop()));
    createAction("fast_forward", SLOT(slotFastforward()));
    createAction("rewind", SLOT(slotRewind()));
    createAction("recordtoggle", SLOT(slotToggleRecord()));
    createAction("record", SLOT(slotRecord()));
    createAction("rewindtobeginning", SLOT(slotRewindToBeginning()));
    createAction("fastforwardtoend", SLOT(slotFastForwardToEnd()));
    createAction("toggle_tracking", SLOT(slotToggleTracking()));
    createAction("panic", SLOT(slotPanic()));
    createAction("debug_dump_segments", SLOT(slotDebugDump()));

    createGUI("rosegardenui.rc");

    setupRecentFilesMenu();
    createAndSetupTransport();

    // transport toolbar is hidden by default - TODO : this should be in options
    //
    //toolBar("Transport Toolbar")->hide();

    // was QPopupMenu
    QMenu* setTrackInstrumentMenu = this->findChild<QMenu*>("set_track_instrument");

    if (setTrackInstrumentMenu) {
        connect(setTrackInstrumentMenu, SIGNAL(aboutToShow()),
                this, SLOT(slotPopulateTrackInstrumentPopup()));
    } else {
        RG_DEBUG << "RosegardenMainWindow::setupActions() : couldn't find set_track_instrument menu - check rosegardenui.rcn\n";
    }

    setRewFFwdToAutoRepeat();
}


void
RosegardenMainWindow::setupRecentFilesMenu()
{
    QMenu *menu = findMenu("file_open_recent");
    if (!menu) {
        std::cerr << "ERROR: RosegardenMainWindow::setupRecentFilesMenu: No recent files menu!" << std::endl;
        return;
    }
    menu->clear();
    std::vector<QString> files = m_recentFiles.getRecent();
    for (size_t i = 0; i < files.size(); ++i) {
    QAction *action = new QAction(files[i], this);
    connect(action, SIGNAL(triggered()), this, SLOT(slotFileOpenRecent()));
    menu->addAction(action);
    }
}


void RosegardenMainWindow::setRewFFwdToAutoRepeat()
{
    //QWidget* transportToolbar = factory()->container("Transport Toolbar", this);
    QWidget* transportToolbar = this->findToolbar("Transport Toolbar");

    if (transportToolbar) {
        //QList<QPushButton *> allPButtons = parentWidget.findChildren<QPushButton *>();
        QToolButton *rew = transportToolbar->findChild<QToolButton*>("rewind");
        QToolButton *ffw = transportToolbar->findChild<QToolButton*>("fast_forward");
        if (rew) rew->setAutoRepeat(true);
        if (ffw) ffw->setAutoRepeat(true);
        
        /*
        //&&& //### reimplement all this:
        //
        //
        //### changed var *l from <QObjectList*> to <QListWidget*>
        QListWidget *l = transportToolbar->queryList();        //###################### FIX next
        QListWidgetItem it(*l); // iterate over the buttons
        QObject *obj;
        
        while ((obj = it.current()) != 0) {
            // for each found object...
            ++it;
            // RG_DEBUG << "obj name : " << obj->objectName() << endl;
            QString objName = obj->objectName();

            if (objName.endsWith("rewind") || objName.endsWith("fast_forward")) {
                QPushButton* btn = dynamic_cast<QPushButton*>(obj);
                if (!btn) {
                    RG_DEBUG << "Very strange - found widgets in transport_toolbar which aren't buttons\n";

                    continue;
                }
                btn->setAutoRepeat(true);
            }


        }
        delete l;
        */
    } else {
        RG_DEBUG << "transportToolbar == 0\n";
    }

}

void RosegardenMainWindow::initZoomToolbar()
{
    //### Zoom toolbar has already been created. Find it instead.
    // QToolBar *zoomToolbar = this->addToolBar("Zoom Toolbar");
    //
    QToolBar *zoomToolbar = findToolbar("Zoom Toolbar");
    if (!zoomToolbar) {
        RG_DEBUG << "RosegardenMainWindow::initZoomToolbar() : "
        << "zoom toolbar not found" << endl;
        return ;
    }

    QLabel *label = new QLabel(tr("  Zoom:  "));
    label->setObjectName("Humbug");
    zoomToolbar->addWidget(label);

    std::vector<double> zoomSizes; // in units-per-pixel
    double defaultBarWidth44 = 100.0;
    double duration44 = TimeSignature(4, 4).getBarDuration();
    static double factors[] = { 0.025, 0.05, 0.1, 0.2, 0.5,
                                1.0, 1.5, 2.5, 5.0, 10.0 , 20.0 };

    for (unsigned int i = 0; i < sizeof(factors) / sizeof(factors[0]); ++i) {
        zoomSizes.push_back(duration44 / (defaultBarWidth44 * factors[i]));
    }

    // zoom labels
    QString minZoom = QString("%1%").arg(factors[0] * 100.0);
    QString maxZoom = QString("%1%").arg(factors[(sizeof(factors) / sizeof(factors[0])) - 1] * 100.0);

    m_zoomSlider = new ZoomSlider<double>
                   (zoomSizes, -1, Qt::Horizontal, zoomToolbar);
    m_zoomSlider->setTracking(true);
    m_zoomSlider->setFocusPolicy(Qt::NoFocus);
    m_zoomLabel = new QLabel(minZoom, zoomToolbar);
    m_zoomLabel->setIndent(10);
    m_zoomLabel->setObjectName("Humbug");

    connect(m_zoomSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeZoom(int)));

    zoomToolbar->addWidget(m_zoomSlider);
    zoomToolbar->addWidget(m_zoomLabel);

    // set initial zoom - we might want to make this a settings option
    //    m_zoomSlider->setToDefault();

}

void RosegardenMainWindow::initStatusBar()
{
    TmpStatusMsg::setDefaultMsg("");
    // QStatusBar::addPermanentWidget (QWidget * widget, int stretch = 0)
//    statusBar()->addPermanentWidget(TmpStatusMsg::getDefaultMsg(),
//              TmpStatusMsg::getDefaultId(), 1);
//    statusBar()->setItemAlignment(TmpStatusMsg::getDefaultId(),
//              AlignLeft | AlignVCenter);
    m_statusBarLabel1 = new QLabel(tr("status"), this);
    
    statusBar()->addPermanentWidget(m_statusBarLabel1);
//    statusBar()->setItemAlignment(TmpStatusMsg::getDefaultId(),
//              AlignLeft | AlignVCenter);

    m_progressBar = new ProgressBar(100, true, statusBar());
    //    m_progressBar()->setMinimumWidth(100);
    m_progressBar->setFixedWidth(60);
    m_progressBar->setFixedHeight(18);
//    m_progressBar->setTextEnabled(false);  //&&&
    statusBar()->addPermanentWidget(m_progressBar);
    
    
    /* init toolbar */
    /****************/
    
    //### Main toolbar seems to appear twice. Commenting out the empty one.
    // addToolBar(Qt::TopToolBarArea, new QToolBar("Main Toolbar"));
}

void RosegardenMainWindow::initView()
{
    ////////////////////////////////////////////////////////////////////
    // create the main widget here that is managed by KTMainWindow's view-region and
    // connect the widget to your document to display document contents.

    RG_DEBUG << "RosegardenMainWindow::initView()" << endl;

    Composition &comp = m_doc->getComposition();

    // Ensure that the start and end markers for the piece are set
    // to something reasonable
    //
    if (comp.getStartMarker() == 0 && comp.getEndMarker() == 0) {
        int endMarker = comp.getBarRange(100 + comp.getNbBars()).second;
        comp.setEndMarker(endMarker);
    }

    m_swapView = new RosegardenMainViewWidget(findAction("show_tracklabels")->isChecked(),
                                       m_segmentParameterBox,
                                       m_instrumentParameterBox,
                                       m_trackParameterBox,
                                       this);

    // Connect up this signal so that we can force tool mode
    // changes from the view
    connect(m_swapView, SIGNAL(activateTool(QString)),
            this, SLOT(slotActivateTool(QString)));

    connect(m_swapView,
            SIGNAL(segmentsSelected(const SegmentSelection &)),
            SIGNAL(segmentsSelected(const SegmentSelection &)));

    connect(m_swapView,
            SIGNAL(addAudioFile(AudioFileId)),
            SLOT(slotAddAudioFile(AudioFileId)));

    connect(m_swapView, SIGNAL(toggleSolo(bool)), SLOT(slotToggleSolo(bool)));

    m_doc->attachView(m_swapView);
    
    // m_mainDockWidget->setWidget(m_swapView);    //&&& later: check dockWidget code

    //@@@ This was commented out... why?
    setCentralWidget(m_swapView);

    setWindowTitle(tr("%1 - %2").arg(m_doc->getTitle()).arg(qApp->applicationName()));
    
    /*
    // if we wanted to apply a qt4-qss stylesheet in code:
    //
    QFile qss("data/rosegarden.qss");
    qss.open(QFile::ReadOnly);
    qApp->setStyleSheet(qss.readAll());
    qss.close();
    // */
    
    // Transport setup
    //
    std::string transportMode = m_doc->getConfiguration().
                       get
                        <String>
                        (DocumentConfiguration::TransportMode);
 

    slotEnableTransport(true);

    // and the time signature
    //
    getTransport()->setTimeSignature(comp.getTimeSignatureAt(comp.getPosition()));

    // set the tempo in the transport
    //
    getTransport()->setTempo(comp.getCurrentTempo());

    // bring the transport to the front
    //
    getTransport()->raise();

    // set the play metronome button
    getTransport()->MetronomeButton()->setOn(comp.usePlayMetronome());

    // Set the solo button
    getTransport()->SoloButton()->setOn(comp.isSolo());

    // set the transport mode found in the configuration
    getTransport()->setNewMode(transportMode);

    // set the pointer position
    //
    slotSetPointerPosition(m_doc->getComposition().getPosition());

    // make sure we show
    //
    RosegardenMainViewWidget *oldView = m_view;
    m_view = m_swapView;

    connect(m_view, SIGNAL(stateChange(QString, bool)),
            this, SLOT (slotStateChanged(QString, bool)));

    connect(m_view, SIGNAL(instrumentParametersChanged(InstrumentId)),
            this, SIGNAL(instrumentParametersChanged(InstrumentId)));

    // We only check for the SequenceManager to make sure
    // we're not on the first pass though - we don't want
    // to send these toggles twice on initialisation.
    //
    // Clunky but we just about get away with it for the
    // moment.
    //
    if (m_seqManager != 0) {
        slotToggleChordNameRuler();
        slotToggleRulers();
        slotToggleTempoRuler();
        slotTogglePreviews();
        slotToggleSegmentLabels();

        // Reset any loop on the sequencer
        //
        try {
            if (isUsingSequencer())
                m_seqManager->setLoop(0, 0);
        leaveActionState("have_range"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        } catch (QString s) {
            StartupLogo::hideIfStillThere();
            CurrentProgressDialog::freeze();
            QMessageBox::critical(dynamic_cast<QWidget*>(this), "", s, QMessageBox::Ok, QMessageBox::Ok);
            CurrentProgressDialog::thaw();
        }

        connect(m_seqManager, SIGNAL(controllerDeviceEventReceived(MappedEvent *)),
                m_view, SLOT(slotControllerDeviceEventReceived(MappedEvent *)));
    }

    //    delete m_playList;
    //    m_playList = 0;

    delete m_synthManager;
    m_synthManager = 0;

    delete m_audioMixer;
    m_audioMixer = 0;

    delete m_bankEditor;
    m_bankEditor = 0;

    delete m_markerEditor;
    m_markerEditor = 0;

    delete m_tempoView;
    m_tempoView = 0;

    delete m_triggerSegmentManager;
    m_triggerSegmentManager = 0;

    delete oldView;

    // set the highlighted track
    m_view->slotSelectTrackSegments(comp.getSelectedTrack());

    // play tracking on in the editor by default: turn off if need be
    /* was toggle */ 
    // old. QAction *trackingAction = dynamic_cast<QAction*>
    //                                (actionCollection()->action("toggle_tracking"));
    QAction *trackingAction = findAction("toggle_tracking");
    if (trackingAction && !trackingAction->isChecked()) {
        m_view->getTrackEditor()->slotToggleTracking();
    }

    m_view->show();

    connect(m_view->getTrackEditor()->getCompositionView(),
            SIGNAL(showContextHelp(const QString &)),
            this,
            SLOT(slotShowToolHelp(const QString &)));

    // We have to do this to make sure that the 2nd call ("select")
    // actually has any effect. Activating the same radio action
    // doesn't work the 2nd time (like pressing down the same radio
    // button twice - it doesn't have any effect), so if you load two
    // files in a row, on the 2nd file a new CompositionView will be
    // created but its tool won't be set, even though it will appear
    // to be selected.
    //
    QAction *actionx = this->findAction(QString("move"));
    actionx->trigger();
    
    if (m_doc->getComposition().getNbSegments() > 0){
        QAction *actionx = this->findAction(QString("select"));
        actionx->trigger();
    } else {
        QAction *actionx = this->findAction(QString("draw"));
        actionx->trigger();
    }
    
    
/*    int zoomLevel = m_doc->getConfiguration().
                    get
                        <Int>
                        (DocumentConfiguration::ZoomLevel);
    */
    //QSettings settings;
    int zoomLevel = m_doc->getConfiguration().get<Int>(DocumentConfiguration::ZoomLevel);

    m_zoomSlider->setSize(double(zoomLevel) / 1000.0);
    slotChangeZoom(zoomLevel);

    //slotChangeZoom(int(m_zoomSlider->getCurrentSize()));

    enterActionState("new_file"); //@@@ JAS orig. 0

    ProgressDialog::processEvents();

    if (findAction("show_chord_name_ruler")->isChecked()) {
        SetWaitCursor swc;
        m_view->initChordNameRuler();
    } else {
        m_view->initChordNameRuler();
    }
}

void RosegardenMainWindow::setDocument(RosegardenDocument* newDocument)
{
    if (m_doc == newDocument)
        return ;

    emit documentAboutToChange();
    qApp->processEvents(); // to make sure all opened dialogs (mixer, midi devices...) are closed

    // Take care of all subparts which depend on the document

    // Caption
    //
    QString caption = qApp->applicationName();
    setWindowTitle(tr("%1 - %2").arg(newDocument->getTitle()).arg(caption));
    

    //     // reset AudioManagerDialog
    //     //
    //     delete m_audioManagerDialog; // TODO : replace this with a connection to documentAboutToChange() sig.
    //     m_audioManagerDialog = 0;

    RosegardenDocument* oldDoc = m_doc;

    m_doc = newDocument;

    if (m_seqManager) // when we're called at startup, the seq. man. isn't created yet
        m_seqManager->setDocument(m_doc);

    if (m_markerEditor)
        m_markerEditor->setDocument(m_doc);
    if (m_tempoView) {
        delete m_tempoView;
        m_tempoView = 0;
    }
    if (m_triggerSegmentManager)
        m_triggerSegmentManager->setDocument(m_doc);

    m_trackParameterBox->setDocument(m_doc);
    m_segmentParameterBox->setDocument(m_doc);
    m_instrumentParameterBox->setDocument(m_doc);

    if (m_pluginGUIManager) {
        m_pluginGUIManager->stopAllGUIs();
        m_pluginGUIManager->setStudio(&m_doc->getStudio());
    }

    if (getView() &&
        getView()->getTrackEditor() &&
        getView()->getTrackEditor()->getCompositionView()) {
        getView()->getTrackEditor()->getCompositionView()->endAudioPreviewGeneration();
    }

    // this will delete all edit views
    //
    delete oldDoc;

    // connect needed signals
    //
    connect(m_segmentParameterBox, SIGNAL(documentModified()),
            m_doc, SLOT(slotDocumentModified()));

    connect(m_doc, SIGNAL(pointerPositionChanged(timeT)),
            this, SLOT(slotSetPointerPosition(timeT)));

    connect(m_doc, SIGNAL(documentModified(bool)),
            this, SLOT(slotDocumentModified(bool)));

    connect(m_doc, SIGNAL(loopChanged(timeT, timeT)),
            this, SLOT(slotSetLoop(timeT, timeT)));

//    CommandHistory::getInstance()->attachView(actionCollection());        //&&& needed ? how to ?

    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            SLOT(update()));
    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            SLOT(slotTestClipboard()));

    // connect and start the autosave timer
    connect(m_autoSaveTimer, SIGNAL(timeout()), this, SLOT(slotAutoSave()));
    m_autoSaveTimer->start(m_doc->getAutoSavePeriod() * 1000);

    // Connect the playback timer
    //
    connect(m_playTimer, SIGNAL(timeout()), this, SLOT(slotUpdatePlaybackPosition()));
    connect(m_stopTimer, SIGNAL(timeout()), this, SLOT(slotUpdateMonitoring()));

    connect(m_playTimer, SIGNAL(timeout()), this, SLOT(slotCheckTransportStatus()));
    connect(m_stopTimer, SIGNAL(timeout()), this, SLOT(slotCheckTransportStatus()));

    // finally recreate the main view
    //
    initView();

    if (getView() && getView()->getTrackEditor()) {
        connect(m_doc, SIGNAL(makeTrackVisible(int)),
                getView()->getTrackEditor(), SLOT(slotScrollToTrack(int)));
    }

    connect(m_doc, SIGNAL(devicesResyncd()),
            this, SLOT(slotDocumentDevicesResyncd()));

    m_doc->syncDevices();
    m_doc->clearModifiedStatus();

    if (newDocument->getStudio().haveMidiDevices()) {
        enterActionState("got_midi_devices"); //@@@ JAS orig. 0
    } else {
        leaveActionState("got_midi_devices"); //@@@ JAS orig KXMLGUIClient::StateReverse
    }

    // Ensure the sequencer knows about any audio files
    // we've loaded as part of the new Composition
    //
    m_doc->prepareAudio();

    // Do not reset instrument prog. changes after all.
    //     if (m_seqManager)
    //         m_seqManager->preparePlayback(true);

    Composition &comp = m_doc->getComposition();

    // Set any loaded loop at the Composition and
    // on the marker on CompositionView and clients
    //
    if (m_seqManager)
        m_doc->setLoop(comp.getLoopStart(), comp.getLoopEnd());

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
RosegardenMainWindow::openFile(QString filePath, ImportType type)
{
    RG_DEBUG << "RosegardenMainWindow::openFile " << filePath << endl;

    if (type == ImportCheckType && filePath.endsWith(".rgp")) {
        importProject(filePath);
        return ;
    }

    RosegardenDocument *doc = createDocument(filePath, type);
    if (doc) {
        setDocument(doc);

        // fix # 1235755, "SPB combo not updating after document swap"
        RG_DEBUG << "RosegardenMainWindow::openFile(): calling slotDocColoursChanged() in doc" << endl;
        doc->slotDocColoursChanged();

        QSettings settings;
        settings.beginGroup(GeneralOptionsConfigGroup);

        if (qStrToBool(settings.value("alwaysusedefaultstudio", "false"))) {

            QString autoloadFile = ResourceFinder().getAutoloadPath();

            QFileInfo autoloadFileInfo(autoloadFile);
            if (autoloadFile != "" && autoloadFileInfo.isReadable()) {

                RG_DEBUG << "Importing default studio from " << autoloadFile << endl;

                slotImportStudioFromFile(autoloadFile);
            }
        }

        QFileInfo fInfo(filePath);
        QString tmp (fInfo.absFilePath());
        m_recentFiles.add(tmp);
        
        settings.endGroup();
    }
}

RosegardenDocument*
RosegardenMainWindow::createDocument(QString filePath, ImportType importType)
{
    QFileInfo info(filePath);
    RosegardenDocument *doc = 0;

    if (!info.exists()) {
        // can happen with command-line arg, so...
//        StartupLogo::hideIfStillThere();  //&&& TODO: use QSplashScreen instead
        QMessageBox::warning(dynamic_cast<QWidget*>(this), filePath, tr("File \"%1\" does not exist"), QMessageBox::Ok, QMessageBox::Ok);
        return 0;
    }
    
    if (info.isDir()) {
//        StartupLogo::hideIfStillThere();  //&&& TODO: use QSplashScreen instead
        QMessageBox::warning(dynamic_cast<QWidget*>(this), filePath, tr("File \"%1\" is actually a directory"), QMessageBox::Ok, QMessageBox::Ok);
        return 0;
    }

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
//        StartupLogo::hideIfStillThere();  //&&& TODO: use QSplashScreen instead
        QString errStr =
            tr("You do not have read permission for \"%1\"").arg(filePath);

        QMessageBox::warning(this, "", errStr, QMessageBox::Ok, QMessageBox::Ok);
        return 0;
    }

    // Stop if playing
    //
    if (m_seqManager && m_seqManager->getTransportStatus() == PLAYING)
        slotStop();

    slotEnableTransport(false);

    if (importType == ImportCheckType) {
        //KMimeType::Ptr fileMimeType = KMimeType::findByPath(filePath); //&&& disabled mime, used file-ext. instead
        /*
        if (fileMimeType->objectName() == "audio/x-midi")
            importType = ImportMIDI;
        else if (fileMimeType->objectName() == "audio/x-rosegarden")
            */
        
        if(filePath.endsWith(".mid") || filePath.endsWith(".midi"))
                importType = ImportMIDI;
        else if (filePath.endsWith(".rg"))
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

RosegardenDocument*
RosegardenMainWindow::createDocumentFromRGFile(QString filePath)
{
    // Check for an autosaved file to recover
    QString effectiveFilePath = filePath;
    bool canRecover = false;

    QString autoSaveFileName = AutoSaveFinder().checkAutoSaveFile(filePath);
    bool recovering = (autoSaveFileName != "");

    if (recovering) {

        // First check if the auto-save file is more recent than the doc
        QFileInfo docFileInfo(filePath), autoSaveFileInfo(autoSaveFileName);

        if (docFileInfo.lastModified() < autoSaveFileInfo.lastModified()) {

            RG_DEBUG << "RosegardenMainWindow::openFile : "
                     << "found a more recent autosave file\n";

            // At this point the splash screen may still be there, hide it if
            // it's the case
            StartupLogo::hideIfStillThere();

            // It is, so ask the user if he wants to use the autosave file
            int reply = QMessageBox::question(this, "",
                                              tr("An auto-save file for this document has been found\nDo you want to open it instead ?"), QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes)
                // open the autosave file instead
                effectiveFilePath = autoSaveFileName;
            else {
                // user doesn't want the autosave, so delete it
                // so it won't bother us again if we reload
                QFile::remove(autoSaveFileName);
                recovering = false;
            }
        } else {
            recovering = false;
        }
    }

    // Create a new blank document
    //
    RosegardenDocument *newDoc = new RosegardenDocument(this, m_pluginManager,
                               true); // skipAutoload

    // ignore return thingy
    //
    if (newDoc->openDocument(effectiveFilePath)) {
        if (recovering) {
            // Mark the document as modified,
            // set the "regular" filepath and name (not those of
            // the autosaved doc)
            //
            newDoc->slotDocumentModified();
            QFileInfo info(filePath);
            newDoc->setAbsFilePath(info.absFilePath());
            newDoc->setTitle(info.fileName());
        } else {
            newDoc->clearModifiedStatus();
        }
    } else {
        delete newDoc;
        return 0;
    }

    return newDoc;
}

void RosegardenMainWindow::slotSaveOptions()
{
    RG_DEBUG << "RosegardenMainWindow::slotSaveOptions()\n";

#ifdef SETTING_LOG_DEBUG

    _settingLog(QString("SETTING 2 : transport flap extended = %1").arg(getTransport()->isExpanded()));
    _settingLog(QString("SETTING 2 : show track labels = %1").arg(findAction("show_tracklabels")->isChecked()));
#endif

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);

    settings.setValue("Show Transport", findAction("show_transport")->isChecked());
    settings.setValue("Expanded Transport", m_transport ? getTransport()->isExpanded() : true);
    settings.setValue("Show Track labels", findAction("show_tracklabels")->isChecked());
    settings.setValue("Show Rulers", findAction("show_rulers")->isChecked());
    settings.setValue("Show Tempo Ruler", findAction("show_tempo_ruler")->isChecked());
    settings.setValue("Show Chord Name Ruler", findAction("show_chord_name_ruler")->isChecked());
    settings.setValue("Show Previews", findAction("show_previews")->isChecked());
    settings.setValue("Show Segment Labels", findAction("show_segment_labels")->isChecked());
    settings.setValue("Show Parameters", m_dockVisible);
    settings.setValue("MIDI Thru Routing", findAction("enable_midi_routing")->isChecked());

#ifdef SETTING_LOG_DEBUG

    RG_DEBUG << "SHOW PARAMETERS = " << m_dockVisible << endl;
#endif

    //     saveMainWindowSettings(RosegardenMainWindow::MainWindowConfigGroup); - no need to, done by KMainWindow
    settings.sync();

    settings.endGroup();
}

void RosegardenMainWindow::setupFileDialogSpeedbar()
{
    QSettings settings;
    settings.beginGroup("QFileDialog Speedbar");

    RG_DEBUG << "RosegardenMainWindow::setupFileDialogSpeedbar" << endl;

    bool hasSetExamplesItem = qStrToBool(settings.value("Examples Set", "false")) ;

    RG_DEBUG << "RosegardenMainWindow::setupFileDialogSpeedbar: examples set " << hasSetExamplesItem << endl;

    if (!hasSetExamplesItem) {

        unsigned int n = settings.value("Number of Entries", 0).toUInt() ;

        settings.setValue(QString("Description_%1").arg(n), tr("Example Files"));
        settings.setValue(QString("IconGroup_%1").arg(n), 4);
        settings.setValue(QString("Icon_%1").arg(n), "folder");
        settings.setValue(QString("URL_%1").arg(n),
                          ResourceFinder().getResourceDir("examples"));

        RG_DEBUG << "wrote url " << settings.value(QString("URL_%1").arg(n)).toString() << endl;

        settings.setValue("Examples Set", true);
        settings.setValue("Number of Entries", n + 1);
        settings.sync();
    }

    settings.endGroup();
}

void RosegardenMainWindow::readOptions()
{
    QSettings settings;
//    applyMainWindowSettings(MainWindowConfigGroup);    //&&& not required

//    settings.reparseConfiguration();  //&&& may not be required: reparseConfig..()

    // Statusbar and toolbars toggling action status
    //
    findAction("show_status_bar")->setChecked(!statusBar() ->isHidden());
    findAction("show_stock_toolbar")->setChecked(!findToolbar("Main Toolbar") ->isHidden());
    findAction("show_tools_toolbar")->setChecked(!findToolbar("Tools Toolbar") ->isHidden());
    findAction("show_tracks_toolbar")->setChecked(!findToolbar("Tracks Toolbar") ->isHidden());
    findAction("show_editors_toolbar")->setChecked(!findToolbar("Editors Toolbar") ->isHidden());
    findAction("show_transport_toolbar")->setChecked(!findToolbar("Transport Toolbar")->isHidden());
    findAction("show_zoom_toolbar")->setChecked(!findToolbar("Zoom Toolbar") ->isHidden());

    bool opt;

    settings.beginGroup(GeneralOptionsConfigGroup);

    opt = qStrToBool(settings.value("Show Transport", "true")) ;
    findAction("show_transport")->setChecked(opt);
    slotToggleTransport();

    opt = qStrToBool(settings.value("Expanded Transport", "true")) ;

#ifdef SETTING_LOG_DEBUG

    _settingLog(QString("SETTING 3 : transport flap extended = %1").arg(opt));
#endif

    if (opt)
        getTransport()->slotPanelOpenButtonClicked();
    else
        getTransport()->slotPanelCloseButtonClicked();

    opt = qStrToBool(settings.value("Show Track labels", "true")) ;

#ifdef SETTING_LOG_DEBUG

    _settingLog(QString("SETTING 3 : show track labels = %1").arg(opt));
#endif

    findAction("show_tracklabels")->setChecked(opt);
    slotToggleTrackLabels();

    opt = qStrToBool(settings.value("Show Rulers", "true")) ;
    findAction("show_rulers")->setChecked(opt);
    slotToggleRulers();

    opt = qStrToBool(settings.value("Show Tempo Ruler", "true")) ;
    findAction("show_tempo_ruler")->setChecked(opt);
    slotToggleTempoRuler();

    opt = qStrToBool(settings.value("Show Chord Name Ruler", "false")) ;
    findAction("show_chord_name_ruler")->setChecked(opt);
    slotToggleChordNameRuler();

    opt = qStrToBool(settings.value("Show Previews", "true")) ;
    findAction("show_previews")->setChecked(opt);
    slotTogglePreviews();

    opt = qStrToBool(settings.value("Show Segment Labels", "true")) ;
    findAction("show_segment_labels")->setChecked(opt);
    slotToggleSegmentLabels();
/*&&&
    opt = qStrToBool(settings.value("Show Parameters", "true")) ;
    if (!opt) {
        //m_dockLeft->undock();
    m_dockLeft->setFloating(true);
        //m_dockLeft->hide();
    m_dockLeft->setVisible(false);
    m_dockVisible = false;
    }
*/
    // MIDI Thru routing
    opt = qStrToBool(settings.value("MIDI Thru Routing", "true")) ;
    findAction("enable_midi_routing")->setChecked(opt);
    slotEnableMIDIThruRouting();

    settings.endGroup();

    m_actionsSetup = true;
}

void RosegardenMainWindow::saveGlobalProperties()
{
    QSettings settings;
    //@@@ JAS Do we need a settings.startGroup() here?

    if (m_doc->getTitle() != tr("Untitled") && !m_doc->isModified()) {
        // saving to tempfile not necessary
    } else {
        QString filename = m_doc->getAbsFilePath();
        settings.setValue("filename", filename);
        settings.setValue("modified", m_doc->isModified());

        QString tempname = AutoSaveFinder().getAutoSavePath(filename);
        if (tempname != "") {
            QString errMsg;
            bool res = m_doc->saveDocument(tempname, errMsg);
            if (!res) {
                if (!errMsg.isEmpty())
                    QMessageBox::critical(this, "", tr(qStrToCharPtrUtf8(QString("Could not save document at %1\nError was : %2").arg(tempname).arg(errMsg))));
                else
                    QMessageBox::critical(this, "", tr(qStrToCharPtrUtf8(QString("Could not save document at %1").arg(tempname))) );
            }
        }
    }
}

void RosegardenMainWindow::readGlobalProperties()
{
    QSettings settings;
    //@@@ JAS Do we need a settings.startGroup() here?

    QString filename = settings.value("filename", "").toString();
    bool modified = qStrToBool(settings.value("modified", "false")) ;

    if (modified) {
        bool canRecover;

        QString tempname = AutoSaveFinder().checkAutoSaveFile(filename);

        if (tempname != "") {
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

    QString caption = qApp->applicationName();
        setWindowTitle(tr("%1 - %2").arg(m_doc->getTitle()).arg(caption));
}

void RosegardenMainWindow::showEvent(QShowEvent* e)
{
    RG_DEBUG << "RosegardenMainWindow::showEvent()\n";

    getTransport()->raise();
    
    //KMainWindow::showEvent(e);  //&&& disabled. a debug function ?
    //QMainWindow::showEvent(e);
}

bool RosegardenMainWindow::queryClose()
{
    RG_DEBUG << "RosegardenMainWindow::queryClose" << endl;
#ifdef SETTING_LOG_DEBUG

    _settingLog(QString("SETTING 1 : transport flap extended = %1").arg(getTransport()->isExpanded()));
    _settingLog(QString("SETTING 1 : show track labels = %1").arg(findAction("show_tracklabels")->isChecked()));
#endif

    QString errMsg;

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

bool RosegardenMainWindow::queryExit()
{
    RG_DEBUG << "RosegardenMainWindow::queryExit" << endl;
    if (m_actionsSetup)
        slotSaveOptions();

    return true;
}

void RosegardenMainWindow::slotFileNewWindow()
{
    TmpStatusMsg msg(tr("Opening a new application window..."), this);

    RosegardenMainWindow *new_window = new RosegardenMainWindow();
    new_window->show();
}

void RosegardenMainWindow::slotFileNew()
{
    RG_DEBUG << "RosegardenMainWindow::slotFileNew()\n";

    TmpStatusMsg msg(tr("Creating new document..."), this);

    bool makeNew = false;

    if (!m_doc->isModified()) {
        makeNew = true;
        // m_doc->closeDocument();
    } else if (m_doc->saveIfModified()) {
        makeNew = true;
    }

    if (makeNew) {

        setDocument(new RosegardenDocument(this, m_pluginManager));
    }
}

void RosegardenMainWindow::slotOpenDroppedURL(QString url)
{
    ProgressDialog::processEvents(); // or else we get a crash because the
    // track editor is erased too soon - it is the originator of the signal
    // this slot is connected to.

    if (!m_doc->saveIfModified())
        return ;

    openURL(QUrl(url));
}

void RosegardenMainWindow::openURL(QString url)
{
    RG_DEBUG << "RosegardenMainWindow::openURL: QString " << url << endl;
    openURL(QUrl(url));
}

void RosegardenMainWindow::openURL(const QUrl& url)
{
    SetWaitCursor waitCursor;
    
    // related: http://doc.trolltech.com/4.3/qurl.html#FormattingOption-enum
    QString netFile = url.toString(QUrl::None);
    
    RG_DEBUG << "RosegardenMainWindow::openURL: QUrl " << netFile << endl;

    if (!url.isValid()) {
        QString string;
        string = tr("Malformed URL\n%1").arg(netFile);

        /* was sorry */ QMessageBox::warning(this, "", string);
        return ;
    }

    QString target;
    QString caption(url.path());

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", tr("Cannot download file %1").arg(url.toString()));
        return ;
    }

    target = source.getLocalFilename();
    
    RG_DEBUG << "RosegardenMainWindow::openURL: target : " << target << endl;

    if (!m_doc->saveIfModified())
        return ;

    source.waitForData();
    openFile(target);

    setWindowTitle(caption);
}

void RosegardenMainWindow::slotFileOpen()
{
    slotStatusHelpMsg(tr("Opening file..."));

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);

    QString lastOpenedVersion = settings.value("Last File Opened Version", "none").toString();
    settings.endGroup();

    if (lastOpenedVersion != VERSION) {

        // We haven't opened any files with this version of the
        // program before.  Default to the examples directory.
                          
        QString examplesDir = ResourceFinder().getResourceDir("examples");
        settings.beginGroup("Recent Dirs");

        QString recentString = settings.value("ROSEGARDEN", "").toString() ;
        settings.setValue
            ("ROSEGARDEN", QString("file:%1,%2").arg(examplesDir).arg(recentString));
    }

    QString fname = QFileDialog::getOpenFileName(this, "Open File", QDir::currentPath(),
                    tr("All supported files") + " (*.rg *.RG *.mid *.MID *.midi *.MIDI)" + ";;" +
                    tr("Rosegarden files") + " (*.rg *.RG *.rgp *.RGP *.rgt *.RGT)" + ";;" +
                    tr("MIDI files") + " (*.mid *.MID *.midi *.MIDI)" + ";;" +
                    tr("All files") + " (*)", 0, 0);
    
    QUrl url(fname);
    
    if (url.isEmpty()) {
        return ;
    }

    if (m_doc && !m_doc->saveIfModified())
        return ;

    settings.beginGroup(GeneralOptionsConfigGroup);
    settings.setValue("Last File Opened Version", VERSION);

    openURL(url);

    settings.endGroup();
}

void RosegardenMainWindow::slotMerge()
{
    QUrl url = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath(),
               tr("Rosegarden files") + " (*.rg *.RG)" + ";;" +
               tr("All files") + " (*)", 0, 0);

    if (url.isEmpty()) {
        return ;
    }


    QString target;

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", tr("Cannot download file %1").arg(url.toString()));
        return;
    }

    source.waitForData();
    target = source.getLocalFilename();

    mergeFile(target);
}

void RosegardenMainWindow::slotFileOpenRecent()
{
    QObject *obj = sender();
    QAction *action = dynamic_cast<QAction *>(obj);
    
    if (!action) {
    std::cerr << "WARNING: RosegardenMainWindow::slotFileOpenRecent: sender is not an action"
          << std::endl;
    return;
    }

    QString path = action->text();
    if (path == "") return;

    TmpStatusMsg msg(tr("Opening file..."), this);

    if (m_doc) {
        if (!m_doc->saveIfModified()) {
            return ;
        }
    }

    openURL(path);
}

void RosegardenMainWindow::slotFileSave()
{
    if (!m_doc /*|| !m_doc->isModified()*/)
        return ; // ALWAYS save, even if doc is not modified.

    TmpStatusMsg msg(tr("Saving file..."), this);

    // if it's a new file (no file path), or an imported file
    // (file path doesn't end with .rg), call saveAs
    //
    if (!m_doc->isRegularDotRGFile()) {

        slotFileSaveAs();

    } else {

        SetWaitCursor waitCursor;
        QString errMsg, docFilePath = m_doc->getAbsFilePath();

        bool res = m_doc->saveDocument(docFilePath, errMsg);
        if (!res) {
            if (! errMsg.isEmpty())
                QMessageBox::critical(this, "", tr(qStrToCharPtrUtf8(QString("Could not save document at %1\nError was : %2")
                                              .arg(docFilePath).arg(errMsg))));
            else
                QMessageBox::critical(this, "", tr(qStrToCharPtrUtf8(QString("Could not save document at %1")
                                              .arg(docFilePath))));
        }
    }
}

QString
RosegardenMainWindow::getValidWriteFileName(QString descriptiveExtension,
                                        QString label)
{
    // extract first extension listed in descriptiveExtension, for instance,
    // ".rg" from "Rosegarden files (*.rg)", or ".mid" from "MIDI Files (*.mid *.midi)"
    //
    int left = descriptiveExtension.indexOf("*.");
    int right = descriptiveExtension.indexOf(QRegExp("[ ]"),left);
    QString extension = descriptiveExtension.mid(left+1,right-left-1);

    RG_DEBUG << "RosegardenMainWindow::getValidWriteFileName() : extension = " << extension << endl;


    // Confirm the overwrite of the file later.
    //
    QString name = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::currentPath(), descriptiveExtension, 0, QFileDialog::DontConfirmOverwrite); 
    
    RG_DEBUG << "RosegardenMainWindow::getValidWriteFileName() : QFileDialog::getSaveFileName returned "
             << name << endl;


    if (name.isEmpty())
        return name;

    // Append extension if we don't have one
    //
    if (!extension.isEmpty()) {
        static QRegExp rgFile("\\..{1,4}$");
        if (rgFile.indexIn(name) == -1) {
            name += extension;
        }
    }

    QUrl *u = new QUrl(name);

    if (!u->isValid()) {
        /* was sorry */ QMessageBox::warning(this, "", tr("<qt>Sorry.<br>\"%1\" is not a valid filename.</qt>").arg(name));
        return QString("");
    }

    QFileInfo info(name);

    if (info.isDir()) {
        /* was sorry */ QMessageBox::warning(this, "", tr("You have specified a folder/directory."));
        return "";
    }

    if (info.exists()) {
        int overwrite = QMessageBox::question
                (this, "", tr("The specified file exists.  Overwrite?"), 
                 QMessageBox::Yes | QMessageBox::No,
                 QMessageBox::No);

        if (overwrite != QMessageBox::Yes)
            return "";
    }

    return name;
}

bool RosegardenMainWindow::slotFileSaveAs(bool asTemplate)
{
    if (!m_doc)
        return false;

    TmpStatusMsg msg(tr("Saving file%1with a new filename...").
                        arg(asTemplate ? " as a template " : " "), this);

    QString fileType(asTemplate ? tr("Rosegarden templates") : tr("Rosegarden files"));
    QString fileExtension(asTemplate ? " (*.rgt *.RGT)" : " (*.rg *.RG)");
    QString dialogMessage(asTemplate ? tr("Save as template...") : tr("Save as..."));

    QString newName = getValidWriteFileName
                      (fileType + fileExtension + ";;" +
                       tr("All files") + " (*)",
                       dialogMessage);
    if (newName.isEmpty())
        return false;

    SetWaitCursor waitCursor;
    QFileInfo saveAsInfo(newName);
    m_doc->setTitle(saveAsInfo.fileName());
    m_doc->setAbsFilePath(saveAsInfo.absFilePath());
    QString errMsg;
    bool res = m_doc->saveDocument(newName, errMsg);

    // I can't figure out any way to do this using the STL or Qt.  This will
    // only work on Linux, which isn't currently a problem.  Template files
    // should be made read only so that if you load one and change it, you'll
    // have to save it by some other name unless you go to some extraordinary
    // lengths.  (Doing the chmod might actually be pointless, and in fact seems
    // to be, since we save by writing to some new file, then delete the
    // original one after the new one is ready.  Hrm.  Interesting conundrum
    // actually.  In practice, it works OK, because unless you explicitly choose
    // Save As Template you will be prompted to save an .rg file, and you won't
    // overwrite the original file without some intent behind your action.  I
    // just feel safer going ahead and chmod -w'ing the file anyway though.
    // Plus I already wrote the code.
    // 
    if (asTemplate) {
        RG_DEBUG << "Running chmod a-w " << saveAsInfo.absFilePath() << endl;

        QProcess *proc = new QProcess;
        QStringList procArgs;
        procArgs << "a-w";
        procArgs << saveAsInfo.absFilePath();

        proc->execute("chmod", procArgs);
    }

    if (!res) {
        if (!errMsg.isEmpty())
            QMessageBox::critical(this, "", tr(qStrToCharPtrUtf8(QString("Could not save document at %1\nError was : %2")
                                          .arg(newName).arg(errMsg))));
        else
            QMessageBox::critical(this, "", tr(qStrToCharPtrUtf8(QString("Could not save document at %1")
                                          .arg(newName))));

    } else {

        m_recentFiles.add(newName);

        QString caption = qApp->applicationName();
        setWindowTitle(tr("%1 - %2").arg(m_doc->getTitle()).arg(caption));
        // update the edit view's captions too
        emit compositionStateUpdate();
    }

    return res;
}

void RosegardenMainWindow::slotFileClose()
{
    RG_DEBUG << "RosegardenMainWindow::slotFileClose()" << endl;

    if (!m_doc)
        return ;

    TmpStatusMsg msg(tr("Closing file..."), this);

    if (m_doc->saveIfModified()) {
        setDocument(new RosegardenDocument(this, m_pluginManager));
    }

    // Don't close the whole view (i.e. Quit), just close the doc.
    //    close();
}

void RosegardenMainWindow::slotFilePrint()
{
    if (m_doc->getComposition().getNbSegments() == 0) {
        /* was sorry */ QMessageBox::warning(this, "", "Please create some tracks first (until we implement menu state management)");
        return ;
    }

    TmpStatusMsg msg(tr("Printing..."), this);

    m_view->print(&m_doc->getComposition());
}

void RosegardenMainWindow::slotFilePrintPreview()
{
    if (m_doc->getComposition().getNbSegments() == 0) {
        /* was sorry */ QMessageBox::warning(this, "", "Please create some tracks first (until we implement menu state management)");
        return ;
    }

    TmpStatusMsg msg(tr("Previewing..."), this);

    m_view->print(&m_doc->getComposition(), true);
}

void RosegardenMainWindow::slotQuit()
{
    slotStatusMsg(tr("Exiting..."));

    Profiles::getInstance()->dump();

    if (queryClose()) close();
}

void RosegardenMainWindow::slotEditCut()
{
    if (!m_view->haveSelection())
        return ;
    TmpStatusMsg msg(tr("Cutting selection..."), this);

    SegmentSelection selection(m_view->getSelection());
    CommandHistory::getInstance()->addCommand
    (new CutCommand(selection, m_clipboard));
}

void RosegardenMainWindow::slotEditCopy()
{
    if (!m_view->haveSelection())
        return ;
    TmpStatusMsg msg(tr("Copying selection to clipboard..."), this);

    SegmentSelection selection(m_view->getSelection());
    CommandHistory::getInstance()->addCommand
    (new CopyCommand(selection, m_clipboard));
}

void RosegardenMainWindow::slotEditPaste()
{
    if (m_clipboard->isEmpty()) {
        TmpStatusMsg msg(tr("Clipboard is empty"), this);
        return ;
    }
    TmpStatusMsg msg(tr("Inserting clipboard contents..."), this);

    // for now, but we could paste at the time of the first copied
    // segment and then do ghosting drag or something
    timeT insertionTime = m_doc->getComposition().getPosition();
    CommandHistory::getInstance()->addCommand
    (new PasteSegmentsCommand(&m_doc->getComposition(),
                              m_clipboard, insertionTime,
                              m_doc->getComposition().getSelectedTrack(),
                              false));

    // User preference? Update song pointer position on paste
    m_doc->slotSetPointerPosition(m_doc->getComposition().getPosition());
}

void RosegardenMainWindow::slotCutRange()
{
    timeT t0 = m_doc->getComposition().getLoopStart();
    timeT t1 = m_doc->getComposition().getLoopEnd();

    if (t0 == t1)
        return ;

    CommandHistory::getInstance()->addCommand
    (new CutRangeCommand(&m_doc->getComposition(), t0, t1, m_clipboard));
}

void RosegardenMainWindow::slotCopyRange()
{
    timeT t0 = m_doc->getComposition().getLoopStart();
    timeT t1 = m_doc->getComposition().getLoopEnd();

    if (t0 == t1)
        return ;

    CommandHistory::getInstance()->addCommand
    (new CopyCommand(&m_doc->getComposition(), t0, t1, m_clipboard));
}

void RosegardenMainWindow::slotPasteRange()
{
    if (m_clipboard->isEmpty())
        return ;

    CommandHistory::getInstance()->addCommand
    (new PasteRangeCommand(&m_doc->getComposition(), m_clipboard,
                           m_doc->getComposition().getPosition()));

    m_doc->setLoop(0, 0);
}

void RosegardenMainWindow::slotDeleteRange()
{
    timeT t0 = m_doc->getComposition().getLoopStart();
    timeT t1 = m_doc->getComposition().getLoopEnd();

    if (t0 == t1)
        return ;

    CommandHistory::getInstance()->addCommand
    (new DeleteRangeCommand(&m_doc->getComposition(), t0, t1));

    m_doc->setLoop(0, 0);
}

void RosegardenMainWindow::slotInsertRange()
{
    timeT t0 = m_doc->getComposition().getPosition();
    std::pair<timeT, timeT> r = m_doc->getComposition().getBarRangeForTime(t0);
    TimeDialog dialog(m_view, tr("Duration of empty range to insert"),
                      &m_doc->getComposition(), t0, r.second - r.first, false);
    if (dialog.exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand
            (new InsertRangeCommand(&m_doc->getComposition(), t0, dialog.getTime()));
        m_doc->setLoop(0, 0);
    }
}

void RosegardenMainWindow::slotSelectAll()
{
    m_view->slotSelectAllSegments();
}

void RosegardenMainWindow::slotDeleteSelectedSegments()
{
    m_view->getTrackEditor()->slotDeleteSelectedSegments();
}

void RosegardenMainWindow::slotQuantizeSelection()
{
    if (!m_view->haveSelection())
        return ;

    //!!! this should all be in rosegardenguiview

    QuantizeDialog dialog(m_view);
    if (dialog.exec() != QDialog::Accepted)
        return ;

    SegmentSelection selection = m_view->getSelection();

    MacroCommand *command = new MacroCommand
                             (EventQuantizeCommand::getGlobalName());

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        command->addCommand(new EventQuantizeCommand
                            (**i, (*i)->getStartTime(), (*i)->getEndTime(),
                             dialog.getQuantizer()));
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenMainWindow::slotRepeatQuantizeSelection()
{
    if (!m_view->haveSelection())
        return ;

    //!!! this should all be in rosegardenguiview

    SegmentSelection selection = m_view->getSelection();

    MacroCommand *command = new MacroCommand
                             (EventQuantizeCommand::getGlobalName());

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        command->addCommand(new EventQuantizeCommand
                            (**i, (*i)->getStartTime(), (*i)->getEndTime(),
                             "Quantize Dialog Grid", false)); // no tr (config group name)
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenMainWindow::slotGrooveQuantize()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();

    if (selection.size() != 1) {
        /* was sorry */ QMessageBox::warning(this, "", tr("This function needs no more than one segment to be selected."));
        return ;
    }

    Segment *s = *selection.begin();
    m_view->slotAddCommandToHistory(new CreateTempoMapFromSegmentCommand(s));
}

void RosegardenMainWindow::slotJoinSegments()
{
    if (!m_view->haveSelection())
        return ;

    //!!! this should all be in rosegardenguiview
    //!!! should it?

    SegmentSelection selection = m_view->getSelection();
    if (selection.size() == 0)
        return ;

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        if ((*i)->getType() != Segment::Internal) {
            /* was sorry */ QMessageBox::warning(this, "", tr("Can't join Audio segments"));
            return ;
        }
    }

    m_view->slotAddCommandToHistory(new SegmentJoinCommand(selection));
    m_view->updateSelectionContents();
}

void RosegardenMainWindow::slotRescaleSelection()
{
    if (!m_view->haveSelection())
        return ;

    //!!! this should all be in rosegardenguiview
    //!!! should it?

    SegmentSelection selection = m_view->getSelection();

    timeT startTime = 0, endTime = 0;
    for (SegmentSelection::iterator i = selection.begin();
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
                         false, false);
    if (dialog.exec() != QDialog::Accepted)
        return ;

    std::vector<AudioSegmentRescaleCommand *> asrcs;

    int mult = dialog.getNewDuration();
    int div = endTime - startTime;
    float ratio = float(mult) / float(div);

    std::cerr << "slotRescaleSelection: mult = " << mult << ", div = " << div << ", ratio = " << ratio << std::endl;

    MacroCommand *command = new MacroCommand
                             (SegmentRescaleCommand::getGlobalName());

    bool pathTested = false;

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        if ((*i)->getType() == Segment::Audio) {
            if (!pathTested) {
            testAudioPath(tr("rescaling an audio file"));
                pathTested = true;
            }
            AudioSegmentRescaleCommand *asrc = new AudioSegmentRescaleCommand
                (m_doc, *i, ratio);
            command->addCommand(asrc);
            asrcs.push_back(asrc);
        } else {
            command->addCommand(new SegmentRescaleCommand(*i, mult, div));
        }
    }
    
    
    ProgressDialog *progressDlg = 0; //@@@ should this be QProgressDialog ??

    if (!asrcs.empty()) {
        progressDlg = new ProgressDialog
            (tr("Rescaling audio file..."), 100, this);
        progressDlg->setAutoClose(false);
        progressDlg->setAutoReset(false);
        progressDlg->show();
        for (size_t i = 0; i < asrcs.size(); ++i) {
            asrcs[i]->connectProgressDialog(progressDlg);
        }
    }

    m_view->slotAddCommandToHistory(command);

    if (!asrcs.empty()) {

        progressDlg->setLabelText(tr("Generating audio preview..."));

//        for (size_t i = 0; i < asrcs.size(); ++i) {
//            asrcs[i]->disconnectProgressDialog(progressDlg);    //&&& obsolete (?)
//        }

        connect(&m_doc->getAudioFileManager(), SIGNAL(setValue(int)),
                 m_progressBar, SLOT(setValue(int)));    // was progressDlg->progressBar()
        connect(progressDlg, SIGNAL(cancelClicked()),
                &m_doc->getAudioFileManager(), SLOT(slotStopPreview()));

        for (size_t i = 0; i < asrcs.size(); ++i) {
            int fid = asrcs[i]->getNewAudioFileId();
            if (fid >= 0) {
                slotAddAudioFile(fid);
                m_doc->getAudioFileManager().generatePreview(fid);
            }
        }
    }

    if (progressDlg) delete progressDlg;
}

bool
RosegardenMainWindow::testAudioPath(QString op)
{
    try {
        m_doc->getAudioFileManager().testAudioPath();
    } catch (AudioFileManager::BadAudioPathException) {
        if (QMessageBox::warning
                (this, tr("Warning"),
         tr("The audio file path does not exist or is not writable.\nYou must set the audio file path to a valid directory in Document Properties before %1.\nWould you like to set it now?", op),
                QMessageBox::Ok | QMessageBox::Cancel,
                 QMessageBox::Cancel 
               ) == QMessageBox::Ok 
          ){
            slotOpenAudioPathSettings();
        }
    return false;
    }
    return true;
}

void RosegardenMainWindow::slotAutoSplitSelection()
{
    if (!m_view->haveSelection())
        return ;

    //!!! this should all be in rosegardenguiview
    //!!! or should it?

    SegmentSelection selection = m_view->getSelection();

    MacroCommand *command = new MacroCommand
                             (SegmentAutoSplitCommand::getGlobalName());

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {

        if ((*i)->getType() == Segment::Audio) {
            AudioSplitDialog aSD(this, (*i), m_doc);

            if (aSD.exec() == QDialog::Accepted) {
                // split to threshold
                //
                command->addCommand(
                    new AudioSegmentAutoSplitCommand(m_doc,
                                                     *i,
                                                     aSD.getThreshold()));
                // dmm - verifying that widget->value() accessors *can* work without crashing
                //        std::cout << "SILVAN: getThreshold() = " << aSD.getThreshold() << std::endl;
            }
        } else {
            command->addCommand(new SegmentAutoSplitCommand(*i));
        }
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenMainWindow::slotJogLeft()
{
    RG_DEBUG << "RosegardenMainWindow::slotJogLeft" << endl;
    jogSelection(-Note(Note::Demisemiquaver).getDuration());
}

void RosegardenMainWindow::slotJogRight()
{
    RG_DEBUG << "RosegardenMainWindow::slotJogRight" << endl;
    jogSelection(Note(Note::Demisemiquaver).getDuration());
}

void RosegardenMainWindow::jogSelection(timeT amount)
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();

    SegmentReconfigureCommand *command = new SegmentReconfigureCommand(tr("Jog Selection"));

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {

        command->addSegment((*i),
                            (*i)->getStartTime() + amount,
                            (*i)->getEndMarkerTime() + amount,
                            (*i)->getTrack());
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenMainWindow::createAndSetupTransport()
{
    // create the Transport GUI and add the callbacks to the
    // buttons and keyboard shortcuts
    //
    m_transport = new TransportDialog(this);
    
    plugShortcuts(m_transport, m_transport->getShortcuts());


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

    connect(m_transport, SIGNAL(editTransportTime(QWidget*)),
            SLOT(slotEditTransportTime(QWidget*)));

    // Handle set loop start/stop time buttons.
    //
    connect(m_transport, SIGNAL(setLoopStartTime()), SLOT(slotSetLoopStart()));
    connect(m_transport, SIGNAL(setLoopStopTime()), SLOT(slotSetLoopStop()));

    if (m_seqManager != 0)
        m_seqManager->setTransport(m_transport);

}

void RosegardenMainWindow::slotSplitSelectionByPitch()
{
    if (!m_view->haveSelection())
        return ;

    SplitByPitchDialog dialog(m_view);
    if (dialog.exec() != QDialog::Accepted)
        return ;

    SegmentSelection selection = m_view->getSelection();

    MacroCommand *command = new MacroCommand
                             (SegmentSplitByPitchCommand::getGlobalName());

    bool haveSomething = false;

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {

        if ((*i)->getType() == Segment::Audio) {
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

    if (haveSomething)
        m_view->slotAddCommandToHistory(command);
    //!!! else complain
}

void
RosegardenMainWindow::slotSplitSelectionByRecordedSrc()
{
    if (!m_view->haveSelection())
        return ;

    SplitByRecordingSrcDialog dialog(m_view, m_doc);
    if (dialog.exec() != QDialog::Accepted)
        return ;

    SegmentSelection selection = m_view->getSelection();

    MacroCommand *command = new MacroCommand
                             (SegmentSplitByRecordingSrcCommand::getGlobalName());

    bool haveSomething = false;

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {

        if ((*i)->getType() == Segment::Audio) {
            // nothing
        } else {
            command->addCommand
            (new SegmentSplitByRecordingSrcCommand(*i,
                                                   dialog.getChannel(),
                                                   dialog.getDevice()));
            haveSomething = true;
        }
    }
    if (haveSomething)
        m_view->slotAddCommandToHistory(command);
}

void
RosegardenMainWindow::slotSplitSelectionAtTime()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();
    if (selection.empty())
        return ;

    timeT now = m_doc->getComposition().getPosition();

    QString title = tr("Split %n Segment(s) at Time", "",
                         selection.size());

    TimeDialog dialog(m_view, title,
                      &m_doc->getComposition(),
                      now, true);

    MacroCommand *command = new MacroCommand(title);

    if (dialog.exec() == QDialog::Accepted) {
        for (SegmentSelection::iterator i = selection.begin();
                i != selection.end(); ++i) {

            if ((*i)->getType() == Segment::Audio) {
                command->addCommand(new AudioSegmentSplitCommand(*i, dialog.getTime()));
            } else {
                command->addCommand(new SegmentSplitCommand(*i, dialog.getTime()));
            }
        }
        m_view->slotAddCommandToHistory(command);
    }
}

void
RosegardenMainWindow::slotSetSegmentStartTimes()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();
    if (selection.empty())
        return ;

    timeT someTime = (*selection.begin())->getStartTime();

    TimeDialog dialog(m_view, tr("Segment Start Time"),
                      &m_doc->getComposition(),
                      someTime, false);

    if (dialog.exec() == QDialog::Accepted) {

        bool plural = (selection.size() > 1);

        SegmentReconfigureCommand *command =
            new SegmentReconfigureCommand(plural ?
                                          tr("Set Segment Start Times") :
                                          tr("Set Segment Start Time"));

        for (SegmentSelection::iterator i = selection.begin();
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
RosegardenMainWindow::slotSetSegmentDurations()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();
    if (selection.empty())
        return ;

    timeT someTime =
        (*selection.begin())->getStartTime();

    timeT someDuration =
        (*selection.begin())->getEndMarkerTime() -
        (*selection.begin())->getStartTime();

    TimeDialog dialog(m_view, tr("Segment Duration"),
                      &m_doc->getComposition(),
                      someTime,
                      someDuration,
                      false);

    if (dialog.exec() == QDialog::Accepted) {

        bool plural = (selection.size() > 1);

        SegmentReconfigureCommand *command =
            new SegmentReconfigureCommand(plural ?
                                          tr("Set Segment Durations") :
                                          tr("Set Segment Duration"));

        for (SegmentSelection::iterator i = selection.begin();
                i != selection.end(); ++i) {

            command->addSegment
            (*i, (*i)->getStartTime(),
             (*i)->getStartTime() + dialog.getTime(),
             (*i)->getTrack());
        }

        m_view->slotAddCommandToHistory(command);
    }
}

void RosegardenMainWindow::slotHarmonizeSelection()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();
    //!!! This should be somewhere else too

    CompositionTimeSliceAdapter adapter(&m_doc->getComposition(),
                                        &selection);

    AnalysisHelper helper;
    Segment *segment = new Segment;
    helper.guessHarmonies(adapter, *segment);

    //!!! do nothing with the results yet
    delete segment;
}

void RosegardenMainWindow::slotTempoToSegmentLength()
{
    slotTempoToSegmentLength(this);
}

void RosegardenMainWindow::slotTempoToSegmentLength(QWidget* parent)
{
    RG_DEBUG << "RosegardenMainWindow::slotTempoToSegmentLength" << endl;

    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();

    // Only set for a single selection
    //
    if (selection.size() == 1 &&
            (*selection.begin())->getType() == Segment::Audio) {
        Composition &comp = m_doc->getComposition();
        Segment *seg = *selection.begin();

        TimeSignature timeSig =
            comp.getTimeSignatureAt(seg->getStartTime());

        timeT endTime = seg->getEndTime();

        if (seg->getRawEndMarkerTime())
            endTime = seg->getEndMarkerTime();

        RealTime segDuration =
            seg->getAudioEndTime() - seg->getAudioStartTime();

        int beats = 0;

        // Get user to tell us how many beats or bars the segment contains
        BeatsBarsDialog dialog(parent);
        if (dialog.exec() == QDialog::Accepted) {
            beats = dialog.getQuantity(); // beats (or bars)
            if (dialog.getMode() == 1)    // bars  (multiply by time sig)
                beats *= timeSig.getBeatsPerBar();
#ifdef DEBUG_TEMPO_FROM_AUDIO

            RG_DEBUG << "RosegardenMainWindow::slotTempoToSegmentLength - beats = " << beats
            << " mode = " << ((dialog.getMode() == 0) ? "bars" : "beats") << endl
            << " beats per bar = " << timeSig.getBeatsPerBar()
            << " user quantity = " << dialog.getQuantity()
            << " user mode = " << dialog.getMode() << endl;
#endif

        } else {
            RG_DEBUG << "RosegardenMainWindow::slotTempoToSegmentLength - BeatsBarsDialog aborted"
            << endl;
            return ;
        }

        double beatLengthUsec =
            double(segDuration.sec * 1000000 + segDuration.usec()) /
            double(beats);

        // New tempo is a minute divided by time of beat
        // converted up (#1414252) to a sane value via getTempoFoQpm()
        //
        tempoT newTempo =
            comp.getTempoForQpm(60.0 * 1000000.0 / beatLengthUsec);

#ifdef DEBUG_TEMPO_FROM_AUDIO

        RG_DEBUG << "RosegardenMainWindow::slotTempoToSegmentLength info: " << endl
        << " beatLengthUsec   = " << beatLengthUsec << endl
        << " segDuration.usec = " << segDuration.usec() << endl
        << " newTempo         = " << newTempo << endl;
#endif

        MacroCommand *macro = new MacroCommand(tr("Set Global Tempo"));

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
        CommandHistory::getInstance()->addCommand(macro);
    }
}

void RosegardenMainWindow::slotToggleSegmentLabels()
{
    QAction* act = this->findAction("show_segment_labels");
    
    if (act) {
        m_view->slotShowSegmentLabels(act->isChecked());
    }
}

void RosegardenMainWindow::slotEdit()
{
    m_view->slotEditSegment(0);
}

void RosegardenMainWindow::slotEditAsNotation()
{
    m_view->slotEditSegmentNotation(0);
}

void RosegardenMainWindow::slotEditInMatrix()
{
    m_view->slotEditSegmentMatrix(0);
}

void RosegardenMainWindow::slotEditInPercussionMatrix()
{
    m_view->slotEditSegmentPercussionMatrix(0);
}

void RosegardenMainWindow::slotEditInEventList()
{
    m_view->slotEditSegmentEventList(0);
}

void RosegardenMainWindow::slotEditTempos()
{
    slotEditTempos(m_doc->getComposition().getPosition());
}

void RosegardenMainWindow::slotToggleToolBar()
{
    TmpStatusMsg msg(tr("Toggle the toolbar..."), this);

    if (findAction("show_stock_toolbar")->isChecked())
        findToolbar("Main Toolbar")->show();
    else
        findToolbar("Main Toolbar")->hide();
}

void RosegardenMainWindow::slotToggleToolsToolBar()
{
    TmpStatusMsg msg(tr("Toggle the tools toolbar..."), this);

    if (findAction("show_tools_toolbar")->isChecked())
        findToolbar("Tools Toolbar")->show();
    else
        findToolbar("Tools Toolbar")->hide();
}

void RosegardenMainWindow::slotToggleTracksToolBar()
{
    TmpStatusMsg msg(tr("Toggle the tracks toolbar..."), this);

    if (findAction("show_tracks_toolbar")->isChecked())
        findToolbar("Tracks Toolbar")->show();
    else
        findToolbar("Tracks Toolbar")->hide();
}

void RosegardenMainWindow::slotToggleEditorsToolBar()
{
    TmpStatusMsg msg(tr("Toggle the editor toolbar..."), this);

    if (findAction("show_editors_toolbar")->isChecked())
        findToolbar("Editors Toolbar")->show();
    else
        findToolbar("Editors Toolbar")->hide();
}

void RosegardenMainWindow::slotToggleTransportToolBar()
{
    TmpStatusMsg msg(tr("Toggle the transport toolbar..."), this);

    if (findAction("show_transport_toolbar")->isChecked())
        findToolbar("Transport Toolbar")->show();
    else
        findToolbar("Transport Toolbar")->hide();
}

void RosegardenMainWindow::slotToggleZoomToolBar()
{
    TmpStatusMsg msg(tr("Toggle the zoom toolbar..."), this);

    if (findAction("show_zoom_toolbar")->isChecked())
        findToolbar("Zoom Toolbar")->show();
    else
        findToolbar("Zoom Toolbar")->hide();
}

void RosegardenMainWindow::slotToggleTransport()
{
    TmpStatusMsg msg(tr("Toggle the Transport"), this);

    if (findAction("show_transport")->isChecked()) {
        getTransport()->show();
        getTransport()->raise();
        getTransport()->blockSignals(false);
    } else {
        getTransport()->hide();
        getTransport()->blockSignals(true);
    }
}

void RosegardenMainWindow::slotToggleTransportVisibility()
{
    /**
     * We need this because selecting the menu items automatically toggles
     * the "show_transport" state, while pressing "T" key does not.
     */
    TmpStatusMsg msg(tr("Toggle the Transport"), this);

    QAction *a = findAction("show_transport");
    if (a->isChecked()) {
        a->setChecked(false);
    } else {
        a->setChecked(true);
    }
    slotToggleTransport();
}

/*###
 * Not used anymore -- is here something valuable to save for slotToggleTransport ? (hjj)
 *
void RosegardenMainWindow::slotHideTransport()
{
    QAction *a = findAction("show_transport");
    if (a && a->isChecked()) {
        a->blockSignals(true);
        a->setChecked(false);
        a->blockSignals(false);
    }
    getTransport()->hide();
    getTransport()->blockSignals(true);
}        
 */

void RosegardenMainWindow::slotToggleTrackLabels()
{
    if (findAction("show_tracklabels")->isChecked()) {
#ifdef SETTING_LOG_DEBUG
        _settingLog("toggle track labels on");
#endif

        m_view->getTrackEditor()->getTrackButtons()->
        changeTrackInstrumentLabels(TrackLabel::ShowTrack);
    } else {
#ifdef SETTING_LOG_DEBUG
        _settingLog("toggle track labels off");
#endif

        m_view->getTrackEditor()->getTrackButtons()->
        changeTrackInstrumentLabels(TrackLabel::ShowInstrument);
    }
}

void RosegardenMainWindow::slotToggleRulers()
{
    m_view->slotShowRulers(findAction("show_rulers")->isChecked());
}

void RosegardenMainWindow::slotToggleTempoRuler()
{
    m_view->slotShowTempoRuler(findAction("show_tempo_ruler")->isChecked());
}

void RosegardenMainWindow::slotToggleChordNameRuler()
{
    m_view->slotShowChordNameRuler(findAction("show_chord_name_ruler")->isChecked());
}

void RosegardenMainWindow::slotTogglePreviews()
{
    m_view->slotShowPreviews(findAction("show_previews")->isChecked());
}

void RosegardenMainWindow::slotDockParametersBack()
{
    if (findAction("show_inst_segment_parameters")->isChecked()) {
        m_dockLeft->setVisible(true);
    } else {
        m_dockLeft->setVisible(false);
    }

//    m_dockLeft->dockBack();
/*&&&
    m_dockLeft->setFloating(false);
    m_dockLeft->setVisible(true);
*/
}

void RosegardenMainWindow::slotParametersClosed()
{
    m_dockVisible = false;
}

void RosegardenMainWindow::slotParametersDockedBack(QDockWidget* dw, int) //qt4: Qt::DockWidgetAreas //qt3 was: QDockWidget::DockPosition)
{
    if (dw == m_dockLeft) {
        m_dockVisible = true;
    }
}

void RosegardenMainWindow::slotToggleStatusBar()
{
    TmpStatusMsg msg(tr("Toggle the statusbar..."), this);

    if (!findAction("show_status_bar")->isChecked())
        statusBar()->hide();
    else
        statusBar()->show();
}

void RosegardenMainWindow::slotStatusMsg(QString text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clearMessage();
//    statusBar()->changeItem(text, EditViewBase::ID_STATUS_MSG);
//    statusBar()->showMessage(text, EditViewBase::ID_STATUS_MSG);
    statusBar()->showMessage(text, 0);    // note: last param == timeout
}

void RosegardenMainWindow::slotStatusHelpMsg(QString text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}

void RosegardenMainWindow::slotEnableTransport(bool enable)
{
    if (m_transport)
        getTransport()->setEnabled(enable);
}

void RosegardenMainWindow::slotPointerSelected()
{
    m_view->selectTool(SegmentSelector::ToolName);
}

void RosegardenMainWindow::slotEraseSelected()
{
    m_view->selectTool(SegmentEraser::ToolName);
}

void RosegardenMainWindow::slotDrawSelected()
{
    m_view->selectTool(SegmentPencil::ToolName);
}

void RosegardenMainWindow::slotMoveSelected()
{
    m_view->selectTool(SegmentMover::ToolName);
}

void RosegardenMainWindow::slotResizeSelected()
{
    m_view->selectTool(SegmentResizer::ToolName);
}

void RosegardenMainWindow::slotJoinSelected()
{
    QMessageBox::information(this,
                             tr("The join tool isn't implemented yet.  Instead please highlight "
                                  "the segments you want to join and then use the menu option:\n\n"
                                  "        Segments->Collapse Segments.\n"),
                             tr("Join tool not yet implemented"));

    m_view->selectTool(SegmentJoiner::ToolName);
}

void RosegardenMainWindow::slotSplitSelected()
{
    m_view->selectTool(SegmentSplitter::ToolName);
}

void RosegardenMainWindow::slotAddTrack()
{
    if (!m_view)
        return ;

    // default to the base number - might not actually exist though
    //
    InstrumentId id = MidiInstrumentBase;

    // Get the first Internal/MIDI instrument
    //
    DeviceList *devices = m_doc->getStudio().getDevices();
    bool have = false;

    for (DeviceList::iterator it = devices->begin();
            it != devices->end() && !have; it++) {

        if ((*it)->getType() != Device::Midi)
            continue;

        InstrumentList instruments = (*it)->getAllInstruments();
        for (InstrumentList::iterator iit = instruments.begin();
                iit != instruments.end(); iit++) {

            if ((*iit)->getId() >= MidiInstrumentBase) {
                id = (*iit)->getId();
                have = true;
                break;
            }
        }
    }

    Composition &comp = m_doc->getComposition();
    TrackId trackId = comp.getSelectedTrack();
    Track *track = comp.getTrackById(trackId);

    int pos = -1;
    if (track) pos = track->getPosition() + 1;

    m_view->slotAddTracks(1, id, pos);
}

void RosegardenMainWindow::slotAddTracks()
{
    if (!m_view)
        return ;

    // default to the base number - might not actually exist though
    //
    InstrumentId id = MidiInstrumentBase;

    // Get the first Internal/MIDI instrument
    //
    DeviceList *devices = m_doc->getStudio().getDevices();
    bool have = false;

    for (DeviceList::iterator it = devices->begin();
            it != devices->end() && !have; it++) {

        if ((*it)->getType() != Device::Midi)
            continue;

        InstrumentList instruments = (*it)->getAllInstruments();
        for (InstrumentList::iterator iit = instruments.begin();
                iit != instruments.end(); iit++) {

            if ((*iit)->getId() >= MidiInstrumentBase) {
                id = (*iit)->getId();
                have = true;
                break;
            }
        }
    }

    Composition &comp = m_doc->getComposition();
    TrackId trackId = comp.getSelectedTrack();
    Track *track = comp.getTrackById(trackId);

    int pos = 0;
    if (track) pos = track->getPosition();

    bool ok = false;

    AddTracksDialog dialog(this, pos);

    if (dialog.exec() == QDialog::Accepted) {
        m_view->slotAddTracks(dialog.getTracks(), id, 
                              dialog.getInsertPosition());
    }
}

void RosegardenMainWindow::slotDeleteTrack()
{
    if (!m_view)
        return ;

    Composition &comp = m_doc->getComposition();
    TrackId trackId = comp.getSelectedTrack();
    Track *track = comp.getTrackById(trackId);

    RG_DEBUG << "RosegardenMainWindow::slotDeleteTrack() : about to delete track id "
    << trackId << endl;

    if (track == 0)
        return ;

    // Always have at least one track in a composition
    //
    if (comp.getNbTracks() == 1)
        return ;

    // VLADA
    if (m_view->haveSelection()) {

        SegmentSelection selection = m_view->getSelection();
        m_view->slotSelectTrackSegments(trackId);
        m_view->getTrackEditor()->slotDeleteSelectedSegments();
        m_view->slotPropagateSegmentSelection(selection);

    } else {

        m_view->slotSelectTrackSegments(trackId);
        m_view->getTrackEditor()->slotDeleteSelectedSegments();
    }
    //VLADA

    int position = track->getPosition();

    // Delete the track
    //
    std::vector<TrackId> tracks;
    tracks.push_back(trackId);

    m_view->slotDeleteTracks(tracks);

    // Select a new valid track
    //
    if (comp.getTrackByPosition(position))
        trackId = comp.getTrackByPosition(position)->getId();
    else if (comp.getTrackByPosition(position - 1))
        trackId = comp.getTrackByPosition(position - 1)->getId();
    else {
        RG_DEBUG << "RosegardenMainWindow::slotDeleteTrack - "
        << "can't select a highlighted track after delete"
        << endl;
    }

    comp.setSelectedTrack(trackId);

    Instrument *inst = m_doc->getStudio().
                       getInstrumentById(comp.getTrackById(trackId)->getInstrument());

    //VLADA
    //    m_view->slotSelectTrackSegments(trackId);
    //VLADA
}

void RosegardenMainWindow::slotMoveTrackDown()
{
    RG_DEBUG << "RosegardenMainWindow::slotMoveTrackDown" << endl;

    Composition &comp = m_doc->getComposition();
    Track *srcTrack = comp.getTrackById(comp.getSelectedTrack());

    // Check for track object
    //
    if (srcTrack == 0)
        return ;

    // Check destination track exists
    //
    Track *destTrack =
        comp.getTrackByPosition(srcTrack->getPosition() + 1);

    if (destTrack == 0)
        return ;

    MoveTracksCommand *command =
        new MoveTracksCommand(&comp, srcTrack->getId(), destTrack->getId());

    CommandHistory::getInstance()->addCommand(command);

    // make sure we're showing the right selection
    m_view->slotSelectTrackSegments(comp.getSelectedTrack());

}

void RosegardenMainWindow::slotMoveTrackUp()
{
    RG_DEBUG << "RosegardenMainWindow::slotMoveTrackUp" << endl;

    Composition &comp = m_doc->getComposition();
    Track *srcTrack = comp.getTrackById(comp.getSelectedTrack());

    // Check for track object
    //
    if (srcTrack == 0)
        return ;

    // Check we're not at the top already
    //
    if (srcTrack->getPosition() == 0)
        return ;

    // Check destination track exists
    //
    Track *destTrack =
        comp.getTrackByPosition(srcTrack->getPosition() - 1);

    if (destTrack == 0)
        return ;

    MoveTracksCommand *command =
        new MoveTracksCommand(&comp, srcTrack->getId(), destTrack->getId());

    CommandHistory::getInstance()->addCommand(command);

    // make sure we're showing the right selection
    m_view->slotSelectTrackSegments(comp.getSelectedTrack());
}

void RosegardenMainWindow::slotRevertToSaved()
{
    RG_DEBUG << "RosegardenMainWindow::slotRevertToSaved" << endl;

    if (m_doc->isModified()) {
        int revert =
            QMessageBox::question(this, "", 
                                       tr("Revert modified document to previous saved version?"));

        if (revert == QMessageBox::No)
            return ;

        openFile(m_doc->getAbsFilePath());
    }
}

void RosegardenMainWindow::slotImportProject()
{
    if (m_doc && !m_doc->saveIfModified())
        return ;

    QUrl url = QFileDialog::getOpenFileName(this, tr("Import Rosegarden Project File"), QDir::currentPath(),
               tr("Rosegarden Project files") + " (*.rgp *.RGP)" + ";;" +
               tr("All files") + " (*)", 0, 0);

    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", tr("Cannot download file %1").arg(url.toString()));
        return ;
    }
    source.waitForData();
    tmpfile = source.getLocalFilename();

    importProject(tmpfile);
}

void RosegardenMainWindow::importProject(QString filePath)
{
    //setup "rosegarden-project-package" process
    QProcess *proc = new QProcess;
    QStringList procArgs;
    procArgs << "--unpack";
    procArgs << filePath;

    StartupLogo::hideIfStillThere();
    proc->execute("rosegarden-project-package", procArgs);

    if ((proc->exitStatus() != QProcess::NormalExit) || proc->exitCode()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, filePath, tr("Failed to import project file \"%1\""));
        CurrentProgressDialog::thaw();
        delete proc;
        return ;
    }

    delete proc;

    QString rgFile = filePath;
    rgFile.replace(QRegExp(".rg.rgp$"), ".rg");
    rgFile.replace(QRegExp(".rgp$"), ".rg");
    openURL(rgFile);
}

void RosegardenMainWindow::slotImportMIDI()
{
    if (m_doc && !m_doc->saveIfModified())
        return ;

    QUrl url = QFileDialog::getOpenFileName(this, tr("Open MIDI File"), QDir::currentPath(),
               tr("MIDI files") + " (*.mid *.midi *.MID *.MIDI)" + ";;" +
               tr("All files") + " (*)", 0, 0);

    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", tr("Cannot download file %1").arg(url.toString()));
        return ;
    }

    source.waitForData();
    tmpfile = source.getLocalFilename();

    openFile(tmpfile, ImportMIDI); // does everything including setting the document
}

void RosegardenMainWindow::slotMergeMIDI()
{
    QUrl url = QFileDialog::getOpenFileName(this, tr("Merge MIDI File"), QDir::currentPath(),
               tr("MIDI files") + " (*.mid *.midi *.MID *.MIDI)" + ";;" +
               tr("All files") + " (*)", 0, 0);

    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", tr("Cannot download file %1").arg(url.toString()));
        return ;
    }

    tmpfile = source.getLocalFilename();
    source.waitForData();

    mergeFile(tmpfile, ImportMIDI);
}

QTextCodec *
RosegardenMainWindow::guessTextCodec(std::string text)
{
    QTextCodec *codec = 0;

    for (int c = 0; c < text.length(); ++c) {
        if (text[c] & 0x80) {

            CurrentProgressDialog::freeze();
            StartupLogo::hideIfStillThere();

            IdentifyTextCodecDialog dialog(0, text);
            dialog.exec();

            std::string codecName = dialog.getCodec();

            CurrentProgressDialog::thaw();

            if (codecName != "") {
                codec = QTextCodec::codecForName(codecName.c_str());
            }
            break;
        }
    }

    return codec;
}

void
RosegardenMainWindow::fixTextEncodings(Composition *c)

{
    QTextCodec *codec = 0;

    for (Composition::iterator i = c->begin();
            i != c->end(); ++i) {

        for (Segment::iterator j = (*i)->begin();
                j != (*i)->end(); ++j) {

            if ((*j)->isa(Text::EventType)) {

                std::string text;

                if ((*j)->get
                        <String>
                        (Text::TextPropertyName, text)) {

                    if (!codec)
                        codec = guessTextCodec(text);

                    if (codec) {
                        (*j)->set
                        <String>
                        (Text::TextPropertyName,
                         convertFromCodec(text, codec));
                    }
                }
            }
        }
    }

    if (!codec)
        codec = guessTextCodec(c->getCopyrightNote());
    if (codec)
        c->setCopyrightNote(convertFromCodec(c->getCopyrightNote(), codec));

    for (Composition::trackcontainer::iterator i =
                c->getTracks().begin(); i != c->getTracks().end(); ++i) {
        if (!codec)
            codec = guessTextCodec(i->second->getLabel());
        if (codec)
            i->second->setLabel(convertFromCodec(i->second->getLabel(), codec));
    }

    for (Composition::iterator i = c->begin(); i != c->end(); ++i) {
        if (!codec)
            codec = guessTextCodec((*i)->getLabel());
        if (codec)
            (*i)->setLabel(convertFromCodec((*i)->getLabel(), codec));
    }
}

RosegardenDocument*
RosegardenMainWindow::createDocumentFromMIDIFile(QString file)
{
    //if (!merge && !m_doc->saveIfModified()) return;

    // Create new document (autoload is inherent)
    //
    RosegardenDocument *newDoc = new RosegardenDocument(this, m_pluginManager);

    std::string fname(QFile::encodeName(file));

    MidiFile midiFile(fname,
                      &newDoc->getStudio());

    StartupLogo::hideIfStillThere();
    ProgressDialog progressDlg(tr("Importing MIDI file..."),
                               200,
                               this);

    CurrentProgressDialog::set
        (&progressDlg);

    connect(&midiFile, SIGNAL(setValue(int)),
            m_progressBar, SLOT(setValue(int)));

    connect(&midiFile, SIGNAL(incrementProgress(int)),
             m_progressBar, SLOT(advance(int)));

    if (!midiFile.open()) {
        CurrentProgressDialog::freeze();
        QMessageBox::critical(this, "", strtoqstr(midiFile.getError())); //!!! tr
        delete newDoc;
        return 0;
    }

    midiFile.convertToRosegarden(newDoc->getComposition(),
                                 MidiFile::CONVERT_REPLACE);

    fixTextEncodings(&newDoc->getComposition());

    // Set modification flag
    //
    newDoc->slotDocumentModified();

    // Set the caption
    //
    newDoc->setTitle(QFileInfo(file).fileName());
    newDoc->setAbsFilePath(QFileInfo(file).absFilePath());

    // Clean up for notation purposes (after reinitialise, because that
    // sets the composition's end marker time which is needed here)

    progressDlg.slotSetOperationName(tr("Calculating notation..."));
    ProgressDialog::processEvents();

    Composition *comp = &newDoc->getComposition();

    for (Composition::iterator i = comp->begin();
            i != comp->end(); ++i) {

        Segment &segment = **i;
        SegmentNotationHelper helper(segment);
        segment.insert(helper.guessClef(segment.begin(),
                                        segment.getEndMarker()).getAsEvent
                       (segment.getStartTime()));
    }

    //was: progressDlg.progressBar()
    m_progressBar->setValue(100);

    for (Composition::iterator i = comp->begin();
            i != comp->end(); ++i) {

        // find first key event in each segment (we'd have done the
        // same for clefs, except there is no MIDI clef event)

        Segment &segment = **i;
        timeT firstKeyTime = segment.getEndMarkerTime();

        for (Segment::iterator si = segment.begin();
                segment.isBeforeEndMarker(si); ++si) {
            if ((*si)->isa(Rosegarden::Key::EventType)) {
                firstKeyTime = (*si)->getAbsoluteTime();
                break;
            }
        }

        if (firstKeyTime > segment.getStartTime()) {
            CompositionTimeSliceAdapter adapter
            (comp, timeT(0), firstKeyTime);
            AnalysisHelper helper;
            segment.insert(helper.guessKey(adapter).getAsEvent
                           (segment.getStartTime()));
        }
    }

    int progressPer = 100;
    if (comp->getNbSegments() > 0)
        progressPer = (int)(100.0 / double(comp->getNbSegments()));

    MacroCommand *command = new MacroCommand(tr("Calculate Notation"));

    for (Composition::iterator i = comp->begin();
            i != comp->end(); ++i) {

        Segment &segment = **i;
        timeT startTime(segment.getStartTime());
        timeT endTime(segment.getEndMarkerTime());

//        std::cerr << "segment: start time " << segment.getStartTime() << ", end time " << segment.getEndTime() << ", end marker time " << segment.getEndMarkerTime() << ", events " << segment.size() << std::endl;

        EventQuantizeCommand *subCommand = new EventQuantizeCommand
                                           (segment, startTime, endTime, "Notation Options", true);

        subCommand->setProgressTotal(progressPer + 1);
        QObject::connect(subCommand, SIGNAL(incrementProgress(int)),
                         m_progressBar, SLOT(advance(int)));

        command->addCommand(subCommand);
    }

    CommandHistory::getInstance()->addCommand(command);

    if (comp->getTimeSignatureCount() == 0) {
        CompositionTimeSliceAdapter adapter(comp);
        AnalysisHelper analysisHelper;
        TimeSignature timeSig =
            analysisHelper.guessTimeSignature(adapter);
        comp->addTimeSignature(0, timeSig);
    }

    return newDoc;
}

void RosegardenMainWindow::slotImportRG21()
{
    if (m_doc && !m_doc->saveIfModified())
        return ;

    QUrl url = QFileDialog::getOpenFileName(this, tr("Open X11 Rosegarden File"), QDir::currentPath(),
               tr("X11 Rosegarden files") + " (*.rose)" + ";;" +
               tr("All files") + " (*)", 0, 0);

    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", tr("Cannot download file %1").arg(url.toString()));
        return ;
    }

    tmpfile = source.getLocalFilename();
    source.waitForData();

    openFile(tmpfile, ImportRG21);
}

void RosegardenMainWindow::slotMergeRG21()
{
    QUrl url = QFileDialog::getOpenFileName(this, tr("Open X11 Rosegarden File"), QDir::currentPath(),
               tr("X11 Rosegarden files") + " (*.rose)" + ";;" +
               tr("All files") + " (*)", 0, 0);

    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", tr("Cannot download file %1").arg(url.toString()));
        return ;
    }

    tmpfile = source.getLocalFilename();
    source.waitForData();

    mergeFile(tmpfile, ImportRG21);
}

RosegardenDocument*
RosegardenMainWindow::createDocumentFromRG21File(QString file)
{
    StartupLogo::hideIfStillThere();
    ProgressDialog progressDlg(
        tr("Importing X11 Rosegarden file..."), 100, this);

    CurrentProgressDialog::set
        (&progressDlg);

    // Inherent autoload
    //
    RosegardenDocument *newDoc = new RosegardenDocument(this, m_pluginManager);

    RG21Loader rg21Loader(&newDoc->getStudio());

    // TODO: make RG21Loader to actually emit these signals
    //
    connect(&rg21Loader, SIGNAL(setValue(int)),
            m_progressBar, SLOT(setValue(int)));

    connect(&rg21Loader, SIGNAL(incrementProgress(int)),
            m_progressBar, SLOT(advance(int)));

    // "your starter for 40%" - helps the "freeze" work
    //
    //progressDlg.progressBar()->advance(40);
    //m_progressBar->advance(40);
    m_progressBar->setValue(40);

    if (!rg21Loader.load(file, newDoc->getComposition())) {
        CurrentProgressDialog::freeze();
        QMessageBox::critical(this, "", 
                           tr("Can't load X11 Rosegarden file.  It appears to be corrupted."));
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
RosegardenMainWindow::slotImportHydrogen()
{
    if (m_doc && !m_doc->saveIfModified())
        return ;

    QUrl url = QFileDialog::getOpenFileName(this, tr("Open Hydrogen File"), QDir::currentPath(),
               tr("All files") + " (*)", 0, 0);

    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", tr("Cannot download file %1").arg(url.toString()));
        return ;
    }

    tmpfile = source.getLocalFilename();
    source.waitForData();

    openFile(tmpfile, ImportHydrogen);
}

void RosegardenMainWindow::slotMergeHydrogen()
{
    QUrl url = QFileDialog::getOpenFileName(this, tr("Open Hydrogen File"), QDir::currentPath(),
               tr("All files") + " (*)", 0, 0);

    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", tr("Cannot download file %1").arg(url.toString()));
        return ;
    }

    tmpfile = source.getLocalFilename();
    source.waitForData();

    mergeFile(tmpfile, ImportHydrogen);
}

RosegardenDocument*
RosegardenMainWindow::createDocumentFromHydrogenFile(QString file)
{
    StartupLogo::hideIfStillThere();
    ProgressDialog progressDlg(
        tr("Importing Hydrogen file..."), 100, this);

    CurrentProgressDialog::set
        (&progressDlg);

    // Inherent autoload
    //
    RosegardenDocument *newDoc = new RosegardenDocument(this, m_pluginManager);

    HydrogenLoader hydrogenLoader(&newDoc->getStudio());

    // TODO: make RG21Loader to actually emit these signals
    //
    connect(&hydrogenLoader, SIGNAL(setValue(int)),
             m_progressBar, SLOT(setValue(int)));

    connect(&hydrogenLoader, SIGNAL(incrementProgress(int)),
             m_progressBar, SLOT(advance(int)));

    // "your starter for 40%" - helps the "freeze" work
    //
    m_progressBar->setValue(40);

    if (!hydrogenLoader.load(file, newDoc->getComposition())) {
        CurrentProgressDialog::freeze();
        QMessageBox::critical(this, "",
                           tr("Can't load Hydrogen file.  It appears to be corrupted."));
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
RosegardenMainWindow::mergeFile(QString filePath, ImportType type)
{
    RosegardenDocument *doc = createDocument(filePath, type);

    if (doc) {
        if (m_doc) {

            bool timingsDiffer = false;
            Composition &c1 = m_doc->getComposition();
            Composition &c2 = doc->getComposition();

            // compare tempos and time sigs in the two -- rather laborious

            if (c1.getTimeSignatureCount() != c2.getTimeSignatureCount()) {
                timingsDiffer = true;
            } else {
                for (int i = 0; i < c1.getTimeSignatureCount(); ++i) {
                    std::pair<timeT, TimeSignature> t1 =
                        c1.getTimeSignatureChange(i);
                    std::pair<timeT, TimeSignature> t2 =
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
                    std::pair<timeT, tempoT> t1 = c1.getTempoChange(i);
                    std::pair<timeT, tempoT> t2 = c2.getTempoChange(i);
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
RosegardenMainWindow::slotCheckTransportStatus()
{
    ExternalTransport::TransportRequest req;
    RealTime rt;
    bool have = RosegardenSequencer::getInstance()->
        getNextTransportRequest(req, rt);

    if (have) {
        switch (req) {
        case ExternalTransport::TransportNoChange:    break;
        case ExternalTransport::TransportStop:        stop(); break;
        case ExternalTransport::TransportStart:       play(); break;
        case ExternalTransport::TransportPlay:        play(); break;
        case ExternalTransport::TransportRecord:      record(); break;
        case ExternalTransport::TransportJumpToTime:  jumpToTime(rt); break;
        case ExternalTransport::TransportStartAtTime: startAtTime(rt); break;
        case ExternalTransport::TransportStopAtTime:  stop(); jumpToTime(rt); break;
        }
    }

    TransportStatus status = RosegardenSequencer::getInstance()->
        getStatus();
    
    if (status == PLAYING || status == RECORDING) { //@@@ JAS orig ? KXMLGUIClient::StateReverse : KXMLGUIClient::StateNoReverse
        leaveActionState("not_playing");
    } else {
        enterActionState("not_playing");
    }

    if (m_seqManager) {

        m_seqManager->setTransportStatus(status);

        MappedComposition asynchronousQueue =
            RosegardenSequencer::getInstance()->pullAsynchronousMidiQueue();

        if (!asynchronousQueue.empty()) {
            
            m_seqManager->processAsynchronousMidi(asynchronousQueue, 0);

            if (m_view) {
                m_view->updateMeters();
            }
        }
    }
    
    return;
}

void
RosegardenMainWindow::slotUpdatePlaybackPosition()
{
    static int callbackCount = 0;

    // Either sequencer mappper or the sequence manager could be missing at
    // this point.
    //
    if (!m_seqManager) return;

    MappedEvent ev;
    bool haveEvent = SequencerDataBlock::getInstance()->getVisual(ev);
    if (haveEvent) getTransport()->setMidiOutLabel(&ev);

    RealTime position = SequencerDataBlock::getInstance()->getPositionPointer();

    //    std::cerr << "RosegardenMainWindow::slotUpdatePlaybackPosition: mapper pos = " << position << std::endl;

    Composition &comp = m_doc->getComposition();
    timeT elapsedTime = comp.getElapsedTimeForRealTime(position);

    //    std::cerr << "RosegardenMainWindow::slotUpdatePlaybackPosition: mapper timeT = " << elapsedTime << std::endl;

    if (m_seqManager->getTransportStatus() == RECORDING) {

        MappedComposition mC;
        if (SequencerDataBlock::getInstance()->getRecordedEvents(mC) > 0) {
            m_seqManager->processAsynchronousMidi(mC, 0);
            m_doc->insertRecordedMidi(mC);
        }

        m_doc->updateRecordingMIDISegment();
        m_doc->updateRecordingAudioSegments();
    }

    m_originatingJump = true;
    m_doc->slotSetPointerPosition(elapsedTime);
    m_originatingJump = false;

    if (m_audioMixer && m_audioMixer->isVisible()) m_audioMixer->updateMeters();
    if (m_midiMixer && m_midiMixer->isVisible()) m_midiMixer->updateMeters();
    m_view->updateMeters();

    if (++callbackCount == 60) {
        slotUpdateCPUMeter(true);
        callbackCount = 0;
    }

    //     if (elapsedTime >= comp.getEndMarker())
    //         slotStop();
}

void
RosegardenMainWindow::slotUpdateCPUMeter(bool playing)
{
    static std::ifstream *statstream = 0;
    static bool modified = false;
    static unsigned long lastBusy = 0, lastIdle = 0;

    if (playing) {

        if (!statstream) {
            statstream = new std::ifstream("/proc/stat", std::ios::in);
        }

        if (!statstream || !*statstream)
            return ;
        statstream->seekg(0, std::ios::beg);

        std::string cpu;
        unsigned long user, nice, sys, idle;
        *statstream >> cpu;
        *statstream >> user;
        *statstream >> nice;
        *statstream >> sys;
        *statstream >> idle;

        unsigned long busy = user + nice + sys;
        unsigned long count = 0;

        if (lastBusy > 0) {
            unsigned long bd = busy - lastBusy;
            unsigned long id = idle - lastIdle;
            if (bd + id > 0)
                count = bd * 100 / (bd + id);
            if (count > 100)
                count = 100;
        }

        lastBusy = busy;
        lastIdle = idle;

        if (m_progressBar) {
            if (!modified) {
                //m_progressBar->setTextEnabled(true);
                m_progressBar->setTextVisible(true);
                m_progressBar->setFormat("CPU%p%");
                
            }
            m_progressBar->setValue(count);
        }

        modified = true;

    } else if (modified) {
        if (m_progressBar) {
            m_progressBar->setTextVisible(false);
            m_progressBar->setFormat("%p%");
            m_progressBar->setValue(0);
        }
        modified = false;
    }
}

void
RosegardenMainWindow::slotUpdateMonitoring()
{
    if (m_audioMixer && m_audioMixer->isVisible())
        m_audioMixer->updateMonitorMeters();

    if (m_midiMixer && m_midiMixer->isVisible())
        m_midiMixer->updateMonitorMeter();

    m_view->updateMonitorMeters();

    slotUpdateCPUMeter(false);
}

void RosegardenMainWindow::slotSetPointerPosition(timeT t)
{
    Composition &comp = m_doc->getComposition();

    //    std::cerr << "RosegardenMainWindow::slotSetPointerPosition: t = " << t << std::endl;

    if (m_seqManager) {
        if (m_seqManager->getTransportStatus() == PLAYING ||
                m_seqManager->getTransportStatus() == RECORDING) {
            if (t > comp.getEndMarker()) {
                if (m_seqManager->getTransportStatus() == PLAYING) {
    
                    slotStop();
                    t = comp.getEndMarker();
                    m_doc->slotSetPointerPosition(t); //causes this method to be re-invoked
                    return ;
    
                } else { // if recording, increase composition duration
                    std::pair<timeT, timeT> timeRange = comp.getBarRangeForTime(t);
                    timeT barDuration = timeRange.second - timeRange.first;
                    timeT newEndMarker = t + 10 * barDuration;
                    comp.setEndMarker(newEndMarker);
                    getView()->getTrackEditor()->slotReadjustCanvasSize();
                    getView()->getTrackEditor()->updateRulers();
                }
            }
        }
    
        // cc 20050520 - jump at the sequencer even if we're not playing,
        // because we might be a transport master of some kind
        try {
            if (!m_originatingJump) {
                m_seqManager->sendSequencerJump(comp.getElapsedRealTime(t));
            }
        } catch (QString s) {
            QMessageBox::critical(this, "", s);
        }
    }

    // set the time sig
    getTransport()->setTimeSignature(comp.getTimeSignatureAt(t));

    // and the tempo
    getTransport()->setTempo(comp.getTempoAtTime(t));

    // and the time
    //
    TransportDialog::TimeDisplayMode mode =
        getTransport()->getCurrentMode();

    if (mode == TransportDialog::BarMode ||
            mode == TransportDialog::BarMetronomeMode) {

        slotDisplayBarTime(t);

    } else {

        RealTime rT(comp.getElapsedRealTime(t));

        if (getTransport()->isShowingTimeToEnd()) {
            rT = rT - comp.getElapsedRealTime(comp.getDuration());
        }

        if (mode == TransportDialog::RealMode) {

            getTransport()->displayRealTime(rT);

        } else if (mode == TransportDialog::SMPTEMode) {

            getTransport()->displaySMPTETime(rT);

        } else {

            getTransport()->displayFrameTime(rT);
        }
    }
    
    // handle transport mode configuration changes
    std::string modeAsString = getTransport()->getCurrentModeAsString();
    
    if (m_doc->getConfiguration().get<String>
            (DocumentConfiguration::TransportMode) != modeAsString) {

        m_doc->getConfiguration().set<String>
            (DocumentConfiguration::TransportMode, modeAsString);

        //m_doc->slotDocumentModified(); to avoid being prompted for a file change when merely changing the transport display
    }

    // Update position on the marker editor if it's available
    //
    if (m_markerEditor)
        m_markerEditor->updatePosition();
}

void RosegardenMainWindow::slotDisplayBarTime(timeT t)
{
    Composition &comp = m_doc->getComposition();

    int barNo = comp.getBarNumber(t);
    timeT barStart = comp.getBarStart(barNo);

    TimeSignature timeSig = comp.getTimeSignatureAt(t);
    timeT beatDuration = timeSig.getBeatDuration();

    int beatNo = (t - barStart) / beatDuration;
    int unitNo = (t - barStart) - (beatNo * beatDuration);

    if (getTransport()->isShowingTimeToEnd()) {
        barNo = barNo + 1 - comp.getNbBars();
        beatNo = timeSig.getBeatsPerBar() - 1 - beatNo;
        unitNo = timeSig.getBeatDuration() - 1 - unitNo;
    } else {
        // convert to 1-based display bar numbers
        barNo += 1;
        beatNo += 1;
    }

    // show units in hemidemis (or whatever), not in raw time ticks
    unitNo /= Note(Note::Shortest).getDuration();

    getTransport()->displayBarTime(barNo, beatNo, unitNo);
}

void RosegardenMainWindow::slotRefreshTimeDisplay()
{
    if (m_seqManager->getTransportStatus() == PLAYING ||
            m_seqManager->getTransportStatus() == RECORDING) {
        return ; // it'll be refreshed in a moment anyway
    }
    slotSetPointerPosition(m_doc->getComposition().getPosition());
}

bool
RosegardenMainWindow::isTrackEditorPlayTracking() const
{
    return m_view->getTrackEditor()->isTracking();
}

void RosegardenMainWindow::slotToggleTracking()
{
    m_view->getTrackEditor()->slotToggleTracking();
}

void RosegardenMainWindow::slotTestStartupTester()
{
    RG_DEBUG << "RosegardenMainWindow::slotTestStartupTester" << endl;

    if (!m_startupTester) {
        m_startupTester = new StartupTester();
        connect(m_startupTester, SIGNAL(newerVersionAvailable(QString)),
                this, SLOT(slotNewerVersionAvailable(QString)));
        m_startupTester->start();
        QTimer::singleShot(100, this, SLOT(slotTestStartupTester()));
        return ;
    }

    if (!m_startupTester->isReady()) {
        QTimer::singleShot(100, this, SLOT(slotTestStartupTester()));
        return ;
    }

    QStringList missingFeatures;
    QStringList allMissing;

    QStringList missing;
    bool have = m_startupTester->haveProjectPackager(&missing);

    if (have) { //@@@ JAS orig ? KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse
        enterActionState("have_project_packager");
    } else {
        leaveActionState("have_project_packager");

        missingFeatures.push_back(tr("Export and import of Rosegarden Project files"));
        if (missing.count() == 0) {
            allMissing.push_back(tr("The Rosegarden Project Packager helper script"));
        } else {
            for (int i = 0; i < missing.count(); ++i) {
//                if (missingFeatures.count() > 1) {
                    allMissing.push_back(tr("%1 - for project file support").arg(missing[i]));
//                } else {
//                    allMissing.push_back(missing[i]);
//                }
            }
        }
    }

    have = m_startupTester->haveLilyPondView(&missing);

    if (have) { //@@@ JAS orig ? KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse
        enterActionState("have_lilypondview");
    } else {
        leaveActionState("have_lilypondview");

        missingFeatures.push_back(tr("Notation previews through LilyPond"));
        if (missing.count() == 0) {
            allMissing.push_back(tr("The Rosegarden LilyPondView helper script"));
        } else {
            for (int i = 0; i < missing.count(); ++i) {
                if (missingFeatures.count() > 1) {
                    allMissing.push_back(tr("%1 - for LilyPond preview support").arg(missing[i]));
                } else {
                    allMissing.push_back(missing[i]);
                }
            }
        }
    }

#ifdef HAVE_LIBJACK
    if (m_seqManager && (m_seqManager->getSoundDriverStatus() & AUDIO_OK)) {

        m_haveAudioImporter = m_startupTester->haveAudioFileImporter(&missing);

        if (!m_haveAudioImporter) {
            missingFeatures.push_back(tr("General audio file import and conversion"));
            if (missing.count() == 0) {
                allMissing.push_back(tr("The Rosegarden Audio File Importer helper script"));
            } else {
                for (int i = 0; i < missing.count(); ++i) {
                    if (missingFeatures.count() > 1) {
                        allMissing.push_back(tr("%1 - for audio file import").arg(missing[i]));
                    } else {
                        allMissing.push_back(missing[i]);
                    }
                }
            }
        }
    }
#endif

    if (missingFeatures.count() > 0) {
        QString message = tr("<h3>Helper programs not found</h3><p>Rosegarden could not find one or more helper programs which it needs to provide some features.  The following features will not be available:</p>");
        message += tr("<ul>");
        for (int i = 0; i < missingFeatures.count(); ++i) {
            message += tr("<li>%1</li>").arg(missingFeatures[i]);
        }
        message += tr("</ul>");
        message += tr("<p>To fix this, you should install the following additional programs:</p>");
        message += tr("<ul>");
        for (int i = 0; i < allMissing.count(); ++i) {
            message += tr("<li>%1</li>").arg(allMissing[i]);
        }
        message += tr("</ul>");

        awaitDialogClearance();
        
        QMessageBox::information
            (m_view,
             message,
             tr("Helper programs not found"),
             "startup-helpers-missing");
    }

    delete m_startupTester;
    m_startupTester = 0;
}

void RosegardenMainWindow::slotDebugDump()
{
    Composition &comp = m_doc->getComposition();
    comp.dump(std::cerr);
}

bool RosegardenMainWindow::launchSequencer()
{
    if (!isUsingSequencer()) {
        RG_DEBUG << "RosegardenMainWindow::launchSequencer() - not using seq. - returning\n";
        return false; // no need to launch anything
    }

    if (isSequencerRunning()) {
        RG_DEBUG << "RosegardenMainWindow::launchSequencer() - sequencer already running - returning\n";
        if (m_seqManager) m_seqManager->checkSoundDriverStatus(false);
        return true;
    }

    m_sequencerThread = new SequencerThread();
    connect(m_sequencerThread, SIGNAL(finished()), this, SLOT(slotSequencerExited()));
    m_sequencerThread->start();

    // Sync current devices with the sequencer
    //
    if (m_doc) m_doc->syncDevices();

    if (m_doc && m_doc->getStudio().haveMidiDevices()) {
        enterActionState("got_midi_devices"); //@@@ JAS orig. 0
    } else {
        leaveActionState("got_midi_devices"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    }

    return true;
}

#ifdef HAVE_LIBJACK
bool RosegardenMainWindow::launchJack()
{
    QSettings settings;
    settings.beginGroup(SequencerOptionsConfigGroup);

    bool startJack = qStrToBool(settings.value("jackstart", "false")) ;
    settings.endGroup();

    if (!startJack)
        return true; // we don't do anything

    settings.beginGroup(SequencerOptionsConfigGroup);
    QString jackPath = settings.value("jackcommand", "").toString();

    emit startupStatusMessage(tr("Clearing down jackd..."));

    //setup "/usr/bin/killall" process
    QProcess *proc = new QProcess; // TODO: do it in a less clumsy way
    QStringList procArgs;

    procArgs << "-9";
    procArgs << "jackd";

    proc->execute("/usr/bin/killall", procArgs);

    if (proc->exitCode())
        RG_DEBUG << "couldn't kill any jackd processes" << endl;
    else
        RG_DEBUG << "killed old jackd processes" << endl;

    emit startupStatusMessage(tr("Starting jackd..."));

    if (jackPath != "") {

        RG_DEBUG << "starting jack \"" << jackPath << "\"" << endl;

        QStringList splitCommand;
        splitCommand = jackPath.split(" ", QString::SkipEmptyParts);

        RG_DEBUG << "RosegardenMainWindow::launchJack() : splitCommand length : "
        << splitCommand.size() << endl;

        // setup "jack" process
        m_jackProcess = new QProcess;

        m_jackProcess->start(splitCommand.takeFirst(), splitCommand);
    }
    settings.endGroup();

    return m_jackProcess != 0 ? m_jackProcess->state() == QProcess::Running : true;
}
#endif

void RosegardenMainWindow::slotDocumentDevicesResyncd()
{
    m_sequencerCheckedIn = true;
    m_trackParameterBox->populateDeviceLists();
}

void RosegardenMainWindow::slotSequencerExited()
{
    RG_DEBUG << "RosegardenMainWindow::slotSequencerExited Sequencer exited\n";

    StartupLogo::hideIfStillThere();

    if (m_sequencerCheckedIn) {

        QMessageBox::critical(this, "", tr("The Rosegarden sequencer process has exited unexpectedly.  Sound and recording will no longer be available for this session.\nPlease exit and restart Rosegarden to restore sound capability."));

    } else {

        QMessageBox::critical(this, "", tr("The Rosegarden sequencer could not be started, so sound and recording will be unavailable for this session.\nFor assistance with correct audio and MIDI configuration, go to http://rosegardenmusic.com."));
    }

    delete m_sequencerThread;
    m_sequencerThread = 0; // isSequencerRunning() will return false
    // but isUsingSequencer() will keep returning true
    // so pressing the play button may attempt to restart the sequencer
}

void RosegardenMainWindow::slotExportProject()
{
    TmpStatusMsg msg(tr("Exporting Rosegarden Project file..."), this);

    QString fileName = getValidWriteFileName
                       (tr("Rosegarden Project files") + " (*.rgp *.RGP)" + ";;" +
                        tr("All files") + " (*)",
                        tr("Export as..."));

    if (fileName.isEmpty())
        return ;

    QString rgFile = fileName;
    rgFile.replace(QRegExp(".rg.rgp$"), ".rg");
    rgFile.replace(QRegExp(".rgp$"), ".rg");

    CurrentProgressDialog::freeze();

    QString errMsg;
    if (!m_doc->saveDocument(rgFile, errMsg,
                             true)) { // pretend it's autosave
        /* was sorry */ QMessageBox::warning(this, "", tr("Saving Rosegarden file to package failed: %1").arg(errMsg));
        CurrentProgressDialog::thaw();
        return ;
    }

    //setup "rosegarden-project-package" process
    ProjectPackager *dialog = new ProjectPackager(this, ProjectPackager::Pack, fileName);
    if (dialog->exec() != QDialog::Accepted) {
        return;
    }

//    QProcess *proc = new QProcess;
//    QStringList procArgs;
//    procArgs << "--pack";
//    procArgs << rgFile;
//    procArgs << fileName;
//
//    proc->execute("rosegarden-project-package", procArgs);
//
//    if ((proc->exitStatus() != QProcess::NormalExit) || proc->exitCode()) {
//        /* was sorry */ QMessageBox::warning(this, "", tr("Failed to export to project file \"%1\"").arg(fileName));
//        CurrentProgressDialog::thaw();
//        delete proc;
//      return ;
//    }

//    delete proc;
}

void RosegardenMainWindow::slotExportMIDI()
{
    TmpStatusMsg msg(tr("Exporting MIDI file..."), this);

    QString fileName = getValidWriteFileName
                       (tr("Standard MIDI files") + " (*.mid *.midi *.MID *.MIDI)" + ";;" +
                        tr("All files") + " (*)",
                        tr("Export as..."));

    if (fileName.isEmpty())
        return ;

    exportMIDIFile(fileName);
}

void RosegardenMainWindow::exportMIDIFile(QString file)
{
    ProgressDialog progressDlg(tr("Exporting MIDI file..."),
                               100,
                               this);

    std::string fname(QFile::encodeName(file));

    MidiFile midiFile(fname,
                      &m_doc->getStudio());

    connect(&midiFile, SIGNAL(setValue(int)),
             m_progressBar, SLOT(setValue(int)));

    connect(&midiFile, SIGNAL(incrementProgress(int)),
             m_progressBar, SLOT(advance(int)));

    midiFile.convertToMidi(m_doc->getComposition());

    if (!midiFile.write()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, "", tr("Export failed.  The file could not be opened for writing."));
    }
}

void RosegardenMainWindow::slotExportCsound()
{
    TmpStatusMsg msg(tr("Exporting Csound score file..."), this);

    QString fileName = getValidWriteFileName
                       (tr("Csound files") + " (*.csd *.CSD)" + ";;" +
                        tr("All files") + " (*)",
                        tr("Export as..."));

    if (fileName.isEmpty())
        return ;

    exportCsoundFile(fileName);
}

void RosegardenMainWindow::exportCsoundFile(QString file)
{
    ProgressDialog progressDlg(tr("Exporting Csound score file..."),
                               100,
                               this);

    CsoundExporter e(this, &m_doc->getComposition(), std::string(QFile::encodeName(file)));

    connect(&e, SIGNAL(setValue(int)),
             m_progressBar, SLOT(setValue(int)));

    connect(&e, SIGNAL(incrementProgress(int)),
             m_progressBar, SLOT(advance(int)));

    if (!e.write()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, "", tr("Export failed.  The file could not be opened for writing."));
    }
}

void RosegardenMainWindow::slotExportMup()
{
    TmpStatusMsg msg(tr("Exporting Mup file..."), this);

    QString fileName = getValidWriteFileName
                       (tr("Mup files") + " (*.mup *.MUP)" + ";;" +
                        tr("All files") + " (*)",
                        tr("Export as..."));
    if (fileName.isEmpty())
        return ;

    exportMupFile(fileName);
}

void RosegardenMainWindow::exportMupFile(QString file)
{
    ProgressDialog progressDlg(tr("Exporting Mup file..."),
                               100,
                               this);

    MupExporter e(this, &m_doc->getComposition(), std::string(QFile::encodeName(file)));

    connect(&e, SIGNAL(setValue(int)),
             m_progressBar, SLOT(setValue(int)));

    connect(&e, SIGNAL(incrementProgress(int)),
             m_progressBar, SLOT(advance(int)));

    if (!e.write()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, "", tr("Export failed.  The file could not be opened for writing."));
    }
}

void RosegardenMainWindow::slotExportLilyPond()
{
    TmpStatusMsg msg(tr("Exporting LilyPond file..."), this);

    QString fileName = getValidWriteFileName
                       (tr("LilyPond files") + " (*.ly *.LY)" + ";;" +
                        tr("All files") + " (*)",
                        tr("Export as..."));

    if (fileName.isEmpty())
        return ;

    exportLilyPondFile(fileName);
}

std::map<QProcess *, QTemporaryFile *> RosegardenMainWindow::m_lilyTempFileMap;

void RosegardenMainWindow::slotPrintLilyPond()
{
    TmpStatusMsg msg(tr("Printing with LilyPond..."), this);
    QTemporaryFile *file = new QTemporaryFile("XXXXXX.ly");
    file->setAutoRemove(true);
    if (!file->open()) {
        CurrentProgressDialog::freeze();
        QMessageBox::warning(this, "", tr("<qt><p>Failed to open a temporary file for LilyPond export.</p>"
                                          "<p>This probably means you have run out of disk space on <pre>/tmp</pre></p></qt>"));
        delete file;
    }
    QString filename = file->fileName(); // must call this before close()
    file->close(); // we just want the filename
    if (!exportLilyPondFile(filename, true)) {
        return ;
    }
/*    //setup "rosegarden-lilypondview" process
    QProcess *proc = new QProcess;
    QStringList procArgs;
    procArgs << "--graphical";
    procArgs << "--print";
    procArgs << filename;
    connect(proc, SIGNAL(processExited(QProcess *)),
            this, SLOT(slotLilyPondViewProcessExited(QProcess *)));
    m_lilyTempFileMap[proc] = file;
    proc->start("rosegarden-lilypondview" ,procArgs); //@@@JAS KProcess::NotifyOnExit*/
    LilyPondProcessor *dialog = new LilyPondProcessor(this, LilyPondProcessor::Print, filename);
    if (dialog->exec() != QDialog::Accepted) {
        return;
    }
}

void RosegardenMainWindow::slotPreviewLilyPond()
{
    TmpStatusMsg msg(tr("Previewing LilyPond file..."), this);
    QTemporaryFile *file = new QTemporaryFile("XXXXXX.ly");
    file->setAutoRemove(true);
    if (!file->open()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, "", tr("Failed to open a temporary file for LilyPond export."));
        delete file;
    }
    QString filename = file->fileName(); // must call this before close()
    file->close(); // we just want the filename
    if (!exportLilyPondFile(filename, true)) {
        return ;
    }
    //setup "rosegarden-lilypondview" process
    /*QProcess *proc = new QProcess;
    QStringList procArgs;
    procArgs << "--graphical";
    procArgs << "--pdf";
    procArgs << filename;
    connect(proc, SIGNAL(processExited(QProcess *)),
            this, SLOT(slotLilyPondViewProcessExited(QProcess *)));
    m_lilyTempFileMap[proc] = file;
    proc->execute("rosegarden-lilypondview", procArgs);*/

    LilyPondProcessor *dialog = new LilyPondProcessor(this, LilyPondProcessor::Preview, filename);
    if (dialog->exec() != QDialog::Accepted) {
        return;
    }
}

void RosegardenMainWindow::slotLilyPondViewProcessExited(QProcess *p)
{
    delete m_lilyTempFileMap[p];
    m_lilyTempFileMap.erase(p);
    delete p;
}

bool RosegardenMainWindow::exportLilyPondFile(QString file, bool forPreview)
{
    QString caption = "", heading = "";
    if (forPreview) {
        caption = tr("LilyPond Preview Options");
        heading = tr("LilyPond preview options");
    }

    LilyPondOptionsDialog dialog(this, m_doc, caption, heading);
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    ProgressDialog progressDlg(tr("Exporting LilyPond file..."),
                               100,
                               this);

    LilyPondExporter e(this, m_doc, std::string(QFile::encodeName(file)));

    connect(&e, SIGNAL(setValue(int)),
             m_progressBar, SLOT(setValue(int)));
    //progressDlg.progressBar(), SLOT(setValue(int)));

    connect(&e, SIGNAL(incrementProgress(int)),
            m_progressBar, SLOT(advance(int)));

    if (!e.write()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, "", tr("Export failed.  The file could not be opened for writing."));
        return false;
    }

    return true;
}

void RosegardenMainWindow::slotExportMusicXml()
{
    TmpStatusMsg msg(tr("Exporting MusicXML file..."), this);

    QString fileName = getValidWriteFileName
                       (tr("XML files") + " (*.xml *.XML)" + ";;" +
                        tr("All files") + " (*)",
                        tr("Export as..."));

    if (fileName.isEmpty())
        return ;

    exportMusicXmlFile(fileName);
}

void RosegardenMainWindow::exportMusicXmlFile(QString file)
{
    ProgressDialog progressDlg(tr("Exporting MusicXML file..."),
                               100,
                               this);

    MusicXmlExporter e(this, m_doc, std::string(QFile::encodeName(file)));

    connect(&e, SIGNAL(setValue(int)),
             m_progressBar, SLOT(setValue(int)));
//    progressDlg.progressBar(), SLOT(setValue(int)));

    connect(&e, SIGNAL(incrementProgress(int)),
            m_progressBar, SLOT(advance(int)));

    if (!e.write()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, "", tr("Export failed.  The file could not be opened for writing."));
    }
}

void
RosegardenMainWindow::slotCloseTransport()
{
    findAction("show_transport")->setChecked(false);
    slotToggleTransport(); // hides the transport
}

void
RosegardenMainWindow::slotDeleteTransport()
{
    delete m_transport;
    m_transport = 0;
}

void
RosegardenMainWindow::slotActivateTool(QString toolName)
{
    if (toolName == SegmentSelector::ToolName) {
        findAction("select")->trigger();
    }
}

void
RosegardenMainWindow::slotToggleMetronome()
{
    Composition &comp = m_doc->getComposition();

    if (m_seqManager->getTransportStatus() == STARTING_TO_RECORD ||
            m_seqManager->getTransportStatus() == RECORDING ||
            m_seqManager->getTransportStatus() == RECORDING_ARMED) {
        if (comp.useRecordMetronome())
            comp.setRecordMetronome(false);
        else
            comp.setRecordMetronome(true);

        getTransport()->MetronomeButton()->setOn(comp.useRecordMetronome());
    } else {
        if (comp.usePlayMetronome())
            comp.setPlayMetronome(false);
        else
            comp.setPlayMetronome(true);

        getTransport()->MetronomeButton()->setOn(comp.usePlayMetronome());
    }
}

void
RosegardenMainWindow::slotRewindToBeginning()
{
    // ignore requests if recording
    //
    if (m_seqManager->getTransportStatus() == RECORDING)
        return ;

    m_seqManager->rewindToBeginning();
}

void
RosegardenMainWindow::slotFastForwardToEnd()
{
    // ignore requests if recording
    //
    if (m_seqManager->getTransportStatus() == RECORDING)
        return ;

    m_seqManager->fastForwardToEnd();
}

void
RosegardenMainWindow::slotSetPlayPosition(timeT time)
{
    RG_DEBUG << "RosegardenMainWindow::slotSetPlayPosition(" << time << ")" << endl;
    if (m_seqManager->getTransportStatus() == RECORDING)
        return ;

    m_doc->slotSetPointerPosition(time);

    if (m_seqManager->getTransportStatus() == PLAYING)
        return ;

    slotPlay();
}

void
RosegardenMainWindow::slotRecord()
{
    if (!isUsingSequencer())
        return ;

    if (!isSequencerRunning()) {

        // Try to launch sequencer and return if we fail
        //
        if (!launchSequencer()) return;
    }

    if (m_seqManager->getTransportStatus() == RECORDING) {
        slotStop();
        return ;
    } else if (m_seqManager->getTransportStatus() == PLAYING) {
        slotToggleRecord();
        return ;
    }

    // Attempt to start recording
    //
    try {
        m_seqManager->record(false);
    } catch (QString s) {
        // We should already be stopped by this point so just unset
        // the buttons after clicking the dialog.
        //
        QMessageBox::critical(this, "", s);

        getTransport()->MetronomeButton()->setOn(false);
        getTransport()->RecordButton()->setOn(false);
        getTransport()->PlayButton()->setOn(false);
        return ;
    } catch (AudioFileManager::BadAudioPathException e) {
            if (QMessageBox::warning
                    (this, tr("Warning"),
                 tr("The audio file path does not exist or is not writable.\nPlease set the audio file path to a valid directory in Document Properties before recording audio.\nWould you like to set it now?"),
                      QMessageBox::Yes | QMessageBox::Cancel, 
                               QMessageBox::Cancel) // default button
                 //tr("Set audio file path")) == QMessageBox::Continue) {
                == QMessageBox::Yes)
            {
                slotOpenAudioPathSettings();
            }//end if
        
        getTransport()->MetronomeButton()->setOn(false);
        getTransport()->RecordButton()->setOn(false);
        getTransport()->PlayButton()->setOn(false);
        return ;
    } catch (Exception e) {
        QMessageBox::critical(this, "", strtoqstr(e.getMessage()));

        getTransport()->MetronomeButton()->setOn(false);
        getTransport()->RecordButton()->setOn(false);
        getTransport()->PlayButton()->setOn(false);
        return ;
    }

    // plugin the keyboard shortcuts for focus on this dialog
    plugShortcuts(m_seqManager->getCountdownDialog(),
                     m_seqManager->getCountdownDialog()->getShortcuts());

    connect(m_seqManager->getCountdownDialog(), SIGNAL(stopped()),
            this, SLOT(slotStop()));

    // Start the playback timer - this fetches the current sequencer position &c
    //
    m_stopTimer->stop();
    m_playTimer->start(23); // avoid multiples of 10 just so as
    // to avoid always having the same digit
    // in one place on the transport.  How
    // shallow.)
}

void
RosegardenMainWindow::slotToggleRecord()
{
    if (!isUsingSequencer() || (!isSequencerRunning() && !launchSequencer()))
        return;

    try {
        m_seqManager->record(true);
    } catch (QString s) {
        QMessageBox::critical(this, "", s);
    } catch (AudioFileManager::BadAudioPathException e) {
        if (QMessageBox::warning
            (this, tr("Error"),
                 tr("The audio file path does not exist or is not writable.\nPlease set the audio file path to a valid directory in Document Properties before you start to record audio.\nWould you like to set it now?"),
                      QMessageBox::Ok | QMessageBox::Cancel, 
                       QMessageBox::Cancel
               ) == QMessageBox::Ok 
       ){
            //tr("Set audio file path")) == QMessageBox::Continue) {
        slotOpenAudioPathSettings();
        }
    } catch (Exception e) {
        QMessageBox::critical(this, "",  strtoqstr(e.getMessage()));
    }

}

void
RosegardenMainWindow::slotSetLoop(timeT lhs, timeT rhs)
{
    try {
        m_doc->slotDocumentModified();

        m_seqManager->setLoop(lhs, rhs);

        // toggle the loop button
        if (lhs != rhs) {
            getTransport()->LoopButton()->setOn(true);
            enterActionState("have_range"); //@@@ JAS orig. KXMLGUIClient::StateNoReverse
        } else {
            getTransport()->LoopButton()->setOn(false);
            leaveActionState("have_range"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        }
    } catch (QString s) {
        QMessageBox::critical(this, "", s);
    }
}

void RosegardenMainWindow::alive()
{
    if (m_doc)
        m_doc->syncDevices();

    if (m_doc && m_doc->getStudio().haveMidiDevices()) {
        enterActionState("got_midi_devices"); //@@@ JAS orig. 0
    } else {
        leaveActionState("got_midi_devices"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    }
}

void RosegardenMainWindow::slotPlay()
{
    if (!isUsingSequencer())
        return ;

    if (!isSequencerRunning()) {

        // Try to launch sequencer and return if it fails
        //
        if (!launchSequencer()) return;
    }

    if (!m_seqManager) return;

    // If we're armed and ready to record then do this instead (calling
    // slotRecord ensures we don't toggle the recording state in
    // SequenceManager)
    //
    if (m_seqManager->getTransportStatus() == RECORDING_ARMED) {
        slotRecord();
        return ;
    }

    // Send the controllers at start of playback if required
    //
    QSettings settings;
    settings.beginGroup(SequencerOptionsConfigGroup);

    bool sendControllers = qStrToBool(settings.value("alwayssendcontrollers", "false")) ;

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
    } catch (QString s) {
        QMessageBox::critical(this, "", s);
        m_playTimer->stop();
        m_stopTimer->start(100);
    } catch (Exception e) {
        QMessageBox::critical(this, "", strtoqstr(e.getMessage()));
        m_playTimer->stop();
        m_stopTimer->start(100);
    }

    settings.endGroup();
}

void RosegardenMainWindow::slotJumpToTime(RealTime rt)
{
    Composition *comp = &m_doc->getComposition();
    timeT t = comp->getElapsedTimeForRealTime(rt);
    m_doc->slotSetPointerPosition(t);
}

void RosegardenMainWindow::slotStartAtTime(RealTime rt)
{
    slotJumpToTime(rt);
    slotPlay();
}

void RosegardenMainWindow::slotStop()
{
    if (m_seqManager &&
        m_seqManager->getCountdownDialog()) {
        disconnect(m_seqManager->getCountdownDialog(), SIGNAL(stopped()),
                   this, SLOT(slotStop()));
        disconnect(m_seqManager->getCountdownDialog(), SIGNAL(completed()),
                   this, SLOT(slotStop()));
    }

    try {
        if (m_seqManager)
            m_seqManager->stopping();
    } catch (Exception e) {
        QMessageBox::critical(this, "", strtoqstr(e.getMessage()));
    }

    // stop the playback timer
    m_playTimer->stop();
    m_stopTimer->start(100);
}

void RosegardenMainWindow::slotRewind()
{
    // ignore requests if recording
    //
    if (m_seqManager->getTransportStatus() == RECORDING)
        return ;
    if (m_seqManager)
        m_seqManager->rewind();
}

void RosegardenMainWindow::slotFastforward()
{
    // ignore requests if recording
    //
    if (m_seqManager->getTransportStatus() == RECORDING)
        return ;

    if (m_seqManager)
        m_seqManager->fastforward();
}

void
RosegardenMainWindow::slotSetLoop()
{
    // restore loop
    m_doc->setLoop(m_storedLoopStart, m_storedLoopEnd);
}

void
RosegardenMainWindow::slotUnsetLoop()
{
    Composition &comp = m_doc->getComposition();

    // store the loop
    m_storedLoopStart = comp.getLoopStart();
    m_storedLoopEnd = comp.getLoopEnd();

    // clear the loop at the composition and propagate to the rest
    // of the display items
    m_doc->setLoop(0, 0);
}

void
RosegardenMainWindow::slotSetLoopStart()
{
    // Check so that start time is before endtime, othervise move upp the
    // endtime to that same pos.
    if (m_doc->getComposition().getPosition() < m_doc->getComposition().getLoopEnd()) {
        m_doc->setLoop(m_doc->getComposition().getPosition(), m_doc->getComposition().getLoopEnd());
    } else {
        m_doc->setLoop(m_doc->getComposition().getPosition(), m_doc->getComposition().getPosition());
    }
}

void
RosegardenMainWindow::slotSetLoopStop()
{
    // Check so that end time is after start time, othervise move upp the
    // start time to that same pos.
    if (m_doc->getComposition().getLoopStart() < m_doc->getComposition().getPosition()) {
        m_doc->setLoop(m_doc->getComposition().getLoopStart(), m_doc->getComposition().getPosition());
    } else {
        m_doc->setLoop(m_doc->getComposition().getPosition(), m_doc->getComposition().getPosition());
    }
}

void RosegardenMainWindow::slotToggleSolo(bool value)
{
    RG_DEBUG << "RosegardenMainWindow::slotToggleSolo value = " << value << endl;

    m_doc->getComposition().setSolo(value);
    getTransport()->SoloButton()->setOn(value);

    m_doc->slotDocumentModified();

    emit compositionStateUpdate();
}

void RosegardenMainWindow::slotTrackUp()
{
    Composition &comp = m_doc->getComposition();

    TrackId tid = comp.getSelectedTrack();
    TrackId pos = comp.getTrackById(tid)->getPosition();

    // If at top already
    if (pos == 0)
        return ;

    Track *track = comp.getTrackByPosition(pos - 1);

    // If the track exists
    if (track) {
        comp.setSelectedTrack(track->getId());
        m_view->slotSelectTrackSegments(comp.getSelectedTrack());
    }

}

void RosegardenMainWindow::slotTrackDown()
{
    Composition &comp = m_doc->getComposition();

    TrackId tid = comp.getSelectedTrack();
    TrackId pos = comp.getTrackById(tid)->getPosition();

    Track *track = comp.getTrackByPosition(pos + 1);

    // If the track exists
    if (track) {
        comp.setSelectedTrack(track->getId());
        m_view->slotSelectTrackSegments(comp.getSelectedTrack());
    }

}

void RosegardenMainWindow::slotMuteAllTracks()
{
    RG_DEBUG << "RosegardenMainWindow::slotMuteAllTracks" << endl;

    Composition &comp = m_doc->getComposition();

    Composition::trackcontainer tracks = comp.getTracks();
    Composition::trackiterator tit;
    for (tit = tracks.begin(); tit != tracks.end(); ++tit)
        m_view->slotSetMute((*tit).second->getInstrument(), true);
}

void RosegardenMainWindow::slotUnmuteAllTracks()
{
    RG_DEBUG << "RosegardenMainWindow::slotUnmuteAllTracks" << endl;

    Composition &comp = m_doc->getComposition();

    Composition::trackcontainer tracks = comp.getTracks();
    Composition::trackiterator tit;
    for (tit = tracks.begin(); tit != tracks.end(); ++tit)
        m_view->slotSetMute((*tit).second->getInstrument(), false);
}

void RosegardenMainWindow::slotToggleMutedCurrentTrack()
{
    Composition &comp = m_doc->getComposition();
    TrackId tid = comp.getSelectedTrack();
    Track *track = comp.getTrackById(tid);
    // If the track exists
    if (track) {
        bool isMuted = track->isMuted();
        m_view->slotSetMuteButton(tid, !isMuted);
    }
}

void RosegardenMainWindow::slotToggleRecordCurrentTrack()
{
    Composition &comp = m_doc->getComposition();
    TrackId tid = comp.getSelectedTrack();
    int pos = comp.getTrackPositionById(tid);
    m_view->getTrackEditor()->getTrackButtons()->slotToggleRecordTrack(pos);
}


void RosegardenMainWindow::slotConfigure()
{
    RG_DEBUG << "RosegardenMainWindow::slotConfigure\n";

    ConfigureDialog *configDlg =
        new ConfigureDialog(m_doc, this);

    connect(configDlg, SIGNAL(updateAutoSaveInterval(unsigned int)),
            this, SLOT(slotUpdateAutoSaveInterval(unsigned int)));
    connect(configDlg, SIGNAL(updateSidebarStyle(unsigned int)),
            this, SLOT(slotUpdateSidebarStyle(unsigned int)));

    configDlg->show();
}

void RosegardenMainWindow::slotEditDocumentProperties()
{
    RG_DEBUG << "RosegardenMainWindow::slotEditDocumentProperties\n";

    DocumentConfigureDialog *configDlg =
        new DocumentConfigureDialog(m_doc, this);

    configDlg->show();
}

void RosegardenMainWindow::slotOpenAudioPathSettings()
{
    RG_DEBUG << "RosegardenMainWindow::slotOpenAudioPathSettings\n";

    DocumentConfigureDialog *configDlg =
        new DocumentConfigureDialog(m_doc, this);

    configDlg->showAudioPage();
    configDlg->show();
}

void RosegardenMainWindow::slotEditKeys()
{
    //&&&
 //   KKeyDialog::configure(actionCollection());    //&&& disabled KKeyDialog for now
}

void RosegardenMainWindow::slotEditToolbars()
{
    //&&&
// KEditToolbar dlg(actionCollection(), "rosegardenui.rc");    //&&& disabled Toolbar config. no qt4 equival.
    /*
    QToolBar dlg;

    connect(&dlg, SIGNAL(newToolbarConfig()),
            SLOT(slotUpdateToolbars()));

    dlg.exec();
    */
}

void RosegardenMainWindow::slotUpdateToolbars()
{
    createGUI("rosegardenui.rc");
    findAction("show_stock_toolbar")->setChecked(!(findToolbar("Main Toolbar")->isHidden()));
}

void RosegardenMainWindow::slotEditTempo()
{
    slotEditTempo(this);
}

void RosegardenMainWindow::slotEditTempo(timeT atTime)
{
    slotEditTempo(this, atTime);
}

void RosegardenMainWindow::slotEditTempo(QWidget *parent)
{
    slotEditTempo(parent, m_doc->getComposition().getPosition());
}

void RosegardenMainWindow::slotEditTempo(QWidget *parent, timeT atTime)
{
    RG_DEBUG << "RosegardenMainWindow::slotEditTempo\n";

    TempoDialog tempoDialog(parent, m_doc);

    connect(&tempoDialog,
            SIGNAL(changeTempo(timeT,
                               tempoT,
                               tempoT,
                               TempoDialog::TempoDialogAction)),
            SLOT(slotChangeTempo(timeT,
                                 tempoT,
                                 tempoT,
                                 TempoDialog::TempoDialogAction)));

    tempoDialog.setTempoPosition(atTime);
    tempoDialog.exec();
}

void RosegardenMainWindow::slotEditTimeSignature()
{
    slotEditTimeSignature(this);
}

void RosegardenMainWindow::slotEditTimeSignature(timeT atTime)
{
    slotEditTimeSignature(this, atTime);
}

void RosegardenMainWindow::slotEditTimeSignature(QWidget *parent)
{
    slotEditTimeSignature(parent, m_doc->getComposition().getPosition());
}

void RosegardenMainWindow::slotEditTimeSignature(QWidget *parent,
        timeT time)
{
    Composition &composition(m_doc->getComposition());

    TimeSignature sig = composition.getTimeSignatureAt(time);

    TimeSignatureDialog dialog(parent, &composition, time, sig);

    if (dialog.exec() == QDialog::Accepted) {

        time = dialog.getTime();

        if (dialog.shouldNormalizeRests()) {
            CommandHistory::getInstance()->addCommand
            (new AddTimeSignatureAndNormalizeCommand
             (&composition, time, dialog.getTimeSignature()));
        } else {
            CommandHistory::getInstance()->addCommand
            (new AddTimeSignatureCommand
             (&composition, time, dialog.getTimeSignature()));
        }
    }
}

void RosegardenMainWindow::slotEditTransportTime()
{
    slotEditTransportTime(this);
}

void RosegardenMainWindow::slotEditTransportTime(QWidget *parent)
{
    TimeDialog dialog(parent, tr("Move playback pointer to time"),
                      &m_doc->getComposition(),
                      m_doc->getComposition().getPosition(),
                      true);
    if (dialog.exec() == QDialog::Accepted) {
        m_doc->slotSetPointerPosition(dialog.getTime());
    }
}

void RosegardenMainWindow::slotChangeZoom(int)
{
    double duration44 = TimeSignature(4, 4).getBarDuration();
    double value = double(m_zoomSlider->getCurrentSize());
    m_zoomLabel->setText(tr("%1%").arg(duration44 / value));

    RG_DEBUG << "RosegardenMainWindow::slotChangeZoom : zoom size = "
    << m_zoomSlider->getCurrentSize() << endl;

    // initZoomToolbar sets the zoom value. With some old versions of
    // Qt3.0, this can cause slotChangeZoom() to be called while the
    // view hasn't been initialized yet, so we need to check it's not
    // null
    //
    if (m_view)
        m_view->setZoomSize(m_zoomSlider->getCurrentSize());

    long newZoom = int(m_zoomSlider->getCurrentSize() * 1000.0);

    if (m_doc->getConfiguration().get<Int>
            (DocumentConfiguration::ZoomLevel) != newZoom) {

        m_doc->getConfiguration().set<Int>
        (DocumentConfiguration::ZoomLevel, newZoom);

        m_doc->slotDocumentModified();
    }
}

void
RosegardenMainWindow::slotZoomIn()
{
    m_zoomSlider->increment();
}

void
RosegardenMainWindow::slotZoomOut()
{
    m_zoomSlider->decrement();
}

void
RosegardenMainWindow::slotChangeTempo(timeT time,
                                  tempoT value,
                                  tempoT target,
                                  TempoDialog::TempoDialogAction action)
{
    //!!! handle target

    Composition &comp = m_doc->getComposition();

    // We define a macro command here and build up the command
    // label as we add commands on.
    //
    if (action == TempoDialog::AddTempo) {
        CommandHistory::getInstance()->addCommand
        (new AddTempoChangeCommand(&comp, time, value, target));
    } else if (action == TempoDialog::ReplaceTempo) {
        int index = comp.getTempoChangeNumberAt(time);

        // if there's no previous tempo change then just set globally
        //
        if (index == -1) {
            CommandHistory::getInstance()->addCommand
            (new AddTempoChangeCommand(&comp, 0, value, target));
            return ;
        }

        // get time of previous tempo change
        timeT prevTime = comp.getTempoChange(index).first;

        MacroCommand *macro =
            new MacroCommand(tr("Replace Tempo Change at %1").arg(time));

        macro->addCommand(new RemoveTempoChangeCommand(&comp, index));
        macro->addCommand(new AddTempoChangeCommand(&comp, prevTime, value,
                          target));

        CommandHistory::getInstance()->addCommand(macro);

    } else if (action == TempoDialog::AddTempoAtBarStart) {
        CommandHistory::getInstance()->addCommand(new
                                               AddTempoChangeCommand(&comp, comp.getBarStartForTime(time),
                                                                     value, target));
    } else if (action == TempoDialog::GlobalTempo ||
               action == TempoDialog::GlobalTempoWithDefault) {
        MacroCommand *macro = new MacroCommand(tr("Set Global Tempo"));

        // Remove all tempo changes in reverse order so as the index numbers
        // don't becoming meaningless as the command gets unwound.
        //
        for (int i = 0; i < comp.getTempoChangeCount(); i++)
            macro->addCommand(new RemoveTempoChangeCommand(&comp,
                              (comp.getTempoChangeCount() - 1 - i)));

        // add tempo change at time zero
        //
        macro->addCommand(new AddTempoChangeCommand(&comp, 0, value, target));

        // are we setting default too?
        //
        if (action == TempoDialog::GlobalTempoWithDefault) {
            macro->setName(tr("Set Global and Default Tempo"));
            macro->addCommand(new ModifyDefaultTempoCommand(&comp, value));
        }

        CommandHistory::getInstance()->addCommand(macro);

    } else {
        RG_DEBUG << "RosegardenMainWindow::slotChangeTempo() - "
        << "unrecognised tempo command" << endl;
    }
}

void
RosegardenMainWindow::slotMoveTempo(timeT oldTime,
                                timeT newTime)
{
    Composition &comp = m_doc->getComposition();
    int index = comp.getTempoChangeNumberAt(oldTime);

    if (index < 0)
        return ;

    MacroCommand *macro =
        new MacroCommand(tr("Move Tempo Change"));

    std::pair<timeT, tempoT> tc =
        comp.getTempoChange(index);
    std::pair<bool, tempoT> tr =
        comp.getTempoRamping(index, false);

    macro->addCommand(new RemoveTempoChangeCommand(&comp, index));
    macro->addCommand(new AddTempoChangeCommand(&comp,
                      newTime,
                      tc.second,
                      tr.first ? tr.second : -1));

    CommandHistory::getInstance()->addCommand(macro);
}

void
RosegardenMainWindow::slotDeleteTempo(timeT t)
{
    Composition &comp = m_doc->getComposition();
    int index = comp.getTempoChangeNumberAt(t);

    if (index < 0)
        return ;

    CommandHistory::getInstance()->addCommand(new RemoveTempoChangeCommand
                                           (&comp, index));
}

void
RosegardenMainWindow::slotAddMarker(timeT time)
{
    AddMarkerCommand *command =
        new AddMarkerCommand(&m_doc->getComposition(),
                             time,
                            qStrToStrUtf8(tr("new marker")),
                            qStrToStrUtf8(tr("no description")) );

    CommandHistory::getInstance()->addCommand(command);    
}

void
RosegardenMainWindow::slotDeleteMarker(int id, timeT time, QString name, QString description)
{
    RemoveMarkerCommand *command =
        new RemoveMarkerCommand(&m_doc->getComposition(),
                                id,
                                time,
                                qstrtostr(name),
                                qstrtostr(description));

    CommandHistory::getInstance()->addCommand(command);
}

void
RosegardenMainWindow::slotDocumentModified(bool m)
{
    RG_DEBUG << "RosegardenMainWindow::slotDocumentModified(" << m << ") - doc path = "
    << m_doc->getAbsFilePath() << endl;

    if (!m_doc->getAbsFilePath().isEmpty()) {
        //### JAS the qStrToCharPtrUtf8(s) is probably unnecessary.
        // slotStateChanged(qStrToCharPtrUtf8(QString("saved_file_modified")), m);
        slotStateChanged("saved_file_modified", m);
    } else {
        //### JAS the qStrToCharPtrUtf8(s) is probably unnecessary.
        // slotStateChanged(qStrToCharPtrUtf8(QString("new_file_modified")), m);
        slotStateChanged("new_file_modified", m);
    }

}

void
RosegardenMainWindow::slotStateChanged(QString s,
                                   bool noReverse)
{
    //     RG_DEBUG << "RosegardenMainWindow::slotStateChanged " << s << "," << noReverse << endl;

//    stateChanged(s, noReverse ? KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse); 
//@@@ JAS     rgTempQtIV->stateChanged(qStrToCharPtrUtf8(s), 0);  //### check later
//@@@ JAS Rewriting code as follows.
    if (noReverse) {
        //### JAS the qStrToCharPtrUtf8(s) is probably unnecessary.
        // enterActionState(qStrToCharPtrUtf8(s));
        enterActionState(qStrToCharPtrUtf8(s));
    } else {
        // leaveActionState(s);
        leaveActionState(s);
    }
}

void
RosegardenMainWindow::slotTestClipboard()
{
    // use qt4:
    //QClipboard *clipboard = QApplication::clipboard();
    //QString originalText = clipboard->text();

    if (m_clipboard->isEmpty()) {
        leaveActionState("have_clipboard"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        leaveActionState("have_clipboard_single_segment"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    } else {
        enterActionState("have_clipboard");
        if (m_clipboard->isSingleSegment()) {  //@@@ JAS orig. KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse
            enterActionState("have_clipboard_single_segment");
        } else {
            leaveActionState("have_clipboard_single_segment");
        }
    }
}

void
RosegardenMainWindow::plugShortcuts(QWidget *widget, QShortcut *acc)
{
    //
    // Shortcuts are now defined in *.rc files.
    //
    // new qt4: 
    // QWidget* sc_parent = this;
    // QShortcut* sc_tmp;
    
    // types:Qt::WidgetShortcut, Qt::ApplicationShortcut, Qt::WindowShortcut

    /**
     * Shortcuts for showing/hiding Transport.
     */
    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_T), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotToggleTransportVisibility()));
    
    /**
     * Shortcuts for playing.
     */
    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_MediaPlay), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotPlay()));
    
    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_Enter), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotPlay()));
    
    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_Return + Qt::CTRL), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotPlay()));

    /**
     * Shortcuts for stopping.
     */
    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_MediaStop), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotStop()));
    
    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_Insert), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotStop()));
    
    /**
     * Shortcuts for fast forwarding.
     */
    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_MediaNext), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotFastforward()));
    
    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_PageDown), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotFastforward()));
    
    /**
     * Shortcuts for rewinding.
     */
    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_MediaPrevious), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotRewind()));
    
    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_PageUp), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotRewind()));

    /**
     * Shortcuts for fast forwarding to end.
     */
    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_End), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotFastForwardToEnd()));

    /**
     * Shortcuts for rewinding to beginning.
     */
    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_Home), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotRewindToBeginning()));

    /**
     * Shortcuts for recording.
     */

    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_MediaRecord), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotToggleRecord()));
    
    // sc_tmp = new QShortcut(QKeySequence(Qt::Key_Space), sc_parent, 0, 0, Qt::ApplicationShortcut); 
    // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotToggleRecord()));
    


    TransportDialog *transport =
        dynamic_cast<TransportDialog*>(widget);

    if (transport) {
        
        // Should not use just Qt::Key_M, because it is used for "Open in MatrixView"
        // Are these needed, because we also have "Ctrl + 1" ... "Ctrl + 9" and "1" ... "9"
        // sc_tmp = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_M), sc_parent, 0, 0, Qt::ApplicationShortcut);
        //sc_tmp = m_jumpToQuickMarkerAction->shortcut();
        // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotJumpToQuickMarker()));
        
        // sc_tmp = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M), sc_parent, 0, 0, Qt::ApplicationShortcut);
        //sc_tmp = m_setQuickMarkerAction->shortcut();
        // connect(sc_tmp, SIGNAL(activated()), this, SLOT(slotSetQuickMarker()));
        

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

void
RosegardenMainWindow::setCursor(const QCursor& cursor)
{
    //KDockMainWindow::setCursor(cursor);
//    this->setCursor(cursor);    // note. this (qmainwindow) now contains main dock

    // play it safe, so we can use this class at anytime even very early in the app init
    if ((getView() &&
            getView()->getTrackEditor() &&
            getView()->getTrackEditor()->getCompositionView() &&
            getView()->getTrackEditor()->getCompositionView()->viewport())) {

        getView()->getTrackEditor()->getCompositionView()->viewport()->setCursor(cursor);
    }

    // view, main window...
    //
    getView()->setCursor(cursor);

    // toolbars...
    //
    /*
    //&&& what does this do? - disabled
    QPtrListIterator<KToolBar> tbIter = toolBarIterator();
    QToolBar* tb = 0;
    while ((tb = tbIter.current()) != 0) {
        tb->setCursor(cursor);
        ++tbIter;
    }
    */

    if (m_dockLeft) m_dockLeft->setCursor(cursor);
}

QVector<QString>
RosegardenMainWindow::createRecordAudioFiles(const QVector<InstrumentId> &recordInstruments)
{
    QVector<QString> qv;
    for (unsigned int i = 0; i < recordInstruments.size(); ++i) {
        AudioFile *aF = 0;
        try {
            aF = m_doc->getAudioFileManager().createRecordingAudioFile();
            if (aF) {
                // createRecordingAudioFile doesn't actually write to the disk,
                // and in principle it shouldn't fail
                qv.push_back(aF->getFilename().c_str());
                m_doc->addRecordAudioSegment(recordInstruments[i],
                                             aF->getId());
            } else {
                std::cerr << "ERROR: RosegardenMainWindow::createRecordAudioFiles: Failed to create recording audio file" << std::endl;
                return qv;
            }
        } catch (AudioFileManager::BadAudioPathException e) {
            delete aF;
            std::cerr << "ERROR: RosegardenMainWindow::createRecordAudioFiles: Failed to create recording audio file: " << e.getMessage() << std::endl;
            return qv;
        }
    }
    return qv;
}

QString
RosegardenMainWindow::getAudioFilePath()
{
    return QString(m_doc->getAudioFileManager().getAudioPath().c_str());
}

QVector<InstrumentId>
RosegardenMainWindow::getArmedInstruments()
{
    std::set
        <InstrumentId> iid;

    const Composition::recordtrackcontainer &tr =
        m_doc->getComposition().getRecordTracks();

    for (Composition::recordtrackcontainer::const_iterator i =
                tr.begin(); i != tr.end(); ++i) {
        TrackId tid = (*i);
        Track *track = m_doc->getComposition().getTrackById(tid);
        if (track) {
            iid.insert(track->getInstrument());
        } else {
            std::cerr << "Warning: RosegardenMainWindow::getArmedInstruments: Armed track " << tid << " not found in Composition" << std::endl;
        }
    }

    QVector<InstrumentId> iv;
    for (std::set
                <InstrumentId>::iterator ii = iid.begin();
                ii != iid.end(); ++ii) {
            iv.push_back(*ii);
        }
    return iv;
}

void
RosegardenMainWindow::showError(QString error)
{
    StartupLogo::hideIfStillThere();
    CurrentProgressDialog::freeze();

    // This is principally used for return values from DSSI plugin
    // configure() calls.  It seems some plugins return a string
    // telling you when everything's OK, as well as error strings, but
    // dssi.h does make it reasonably clear that configure() should
    // only return a string when there is actually a problem, so we're
    // going to stick with a sorry dialog here rather than an
    // information one

    /* was sorry */ QMessageBox::warning(0, "", error);

    CurrentProgressDialog::thaw();
}

void
RosegardenMainWindow::slotAudioManager()
{
    if (m_audioManagerDialog) {
        m_audioManagerDialog->show();
        m_audioManagerDialog->raise();
        m_audioManagerDialog->setActiveWindow();
        return ;
    }

    m_audioManagerDialog =
        new AudioManagerDialog(this, m_doc);

    connect(m_audioManagerDialog,
            SIGNAL(playAudioFile(AudioFileId,
                                 const RealTime &,
                                 const RealTime&)),
            SLOT(slotPlayAudioFile(AudioFileId,
                                   const RealTime &,
                                   const RealTime &)));

    connect(m_audioManagerDialog,
            SIGNAL(addAudioFile(AudioFileId)),
            SLOT(slotAddAudioFile(AudioFileId)));

    connect(m_audioManagerDialog,
            SIGNAL(deleteAudioFile(AudioFileId)),
            SLOT(slotDeleteAudioFile(AudioFileId)));

    //
    // Sync segment selection between audio man. dialog and main window
    //

    // from dialog to us...
    connect(m_audioManagerDialog,
            SIGNAL(segmentsSelected(const SegmentSelection&)),
            m_view,
            SLOT(slotPropagateSegmentSelection(const SegmentSelection&)));

    // and from us to dialog
    connect(this, SIGNAL(segmentsSelected(const SegmentSelection&)),
            m_audioManagerDialog,
            SLOT(slotSegmentSelection(const SegmentSelection&)));


    connect(m_audioManagerDialog,
            SIGNAL(deleteSegments(const SegmentSelection&)),
            SLOT(slotDeleteSegments(const SegmentSelection&)));

    connect(m_audioManagerDialog,
            SIGNAL(insertAudioSegment(AudioFileId,
                                      const RealTime&,
                                      const RealTime&)),
            m_view,
            SLOT(slotAddAudioSegmentDefaultPosition(AudioFileId,
                                                    const RealTime&,
                                                    const RealTime&)));
    connect(m_audioManagerDialog,
            SIGNAL(cancelPlayingAudioFile(AudioFileId)),
            SLOT(slotCancelAudioPlayingFile(AudioFileId)));

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
        m_seqManager->getSoundDriverStatus() & AUDIO_OK);

    plugShortcuts(m_audioManagerDialog,
                     m_audioManagerDialog->getShortcuts());

    m_audioManagerDialog->show();
}

void
RosegardenMainWindow::slotPlayAudioFile(unsigned int id,
                                    const RealTime &startTime,
                                    const RealTime &duration)
{
    AudioFile *aF = m_doc->getAudioFileManager().getAudioFile(id);

    if (aF == 0)
        return ;

    MappedEvent mE(m_doc->getStudio().
                   getAudioPreviewInstrument(),
                   id,
                   RealTime(-120, 0),
                   duration,                   // duration
                   startTime);                // start index

    StudioControl::sendMappedEvent(mE);

}

void
RosegardenMainWindow::slotAddAudioFile(unsigned int id)
{
    AudioFile *aF = m_doc->getAudioFileManager().getAudioFile(id);

    if (aF == 0) return;

    int result = RosegardenSequencer::getInstance()->
        addAudioFile(strtoqstr(aF->getFilename()), aF->getId());

    if (!result) {
        QMessageBox::critical(this, "", tr("Sequencer failed to add audio file %1").arg(aF->getFilename().c_str()));
    }
}

void
RosegardenMainWindow::slotDeleteAudioFile(unsigned int id)
{
    if (m_doc->getAudioFileManager().removeFile(id) == false)
        return ;

    int result = RosegardenSequencer::getInstance()->removeAudioFile(id);

    if (!result) {
        QMessageBox::critical(this, "", tr("Sequencer failed to remove audio file id %1").arg(id));
    }
}

void
RosegardenMainWindow::slotDeleteSegments(const SegmentSelection &selection)
{
    m_view->slotPropagateSegmentSelection(selection);
    slotDeleteSelectedSegments();
}

void
RosegardenMainWindow::slotCancelAudioPlayingFile(AudioFileId id)
{
    AudioFile *aF = m_doc->getAudioFileManager().getAudioFile(id);

    if (aF == 0)
        return ;

    MappedEvent mE(m_doc->getStudio().
                   getAudioPreviewInstrument(),
                   MappedEvent::AudioCancel,
                   id);

    StudioControl::sendMappedEvent(mE);
}

void
RosegardenMainWindow::slotDeleteAllAudioFiles()
{
    m_doc->getAudioFileManager().clear();

    // Clear at the sequencer
    //
    RosegardenSequencer::getInstance()->clearAllAudioFiles();
}

void
RosegardenMainWindow::slotRepeatingSegments()
{
    m_view->getTrackEditor()->slotTurnRepeatingSegmentToRealCopies();
}

void
RosegardenMainWindow::slotRelabelSegments()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection(m_view->getSelection());
    QString editLabel;

    if (selection.size() == 0)
        return ;
    else if (selection.size() == 1)
        editLabel = tr("Modify Segment label");
    else
        editLabel = tr("Modify Segments label");

    TmpStatusMsg msg(tr("Relabelling selection..."), this);

    // Generate label
    QString label = strtoqstr((*selection.begin())->getLabel());

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        if (strtoqstr((*i)->getLabel()) != label)
            label = "";
    }

    bool ok = false;

    QString newLabel = InputDialog::getText(dynamic_cast<QWidget*>(this), QString("Input"), tr("Enter new label"));

    if (ok) {
        CommandHistory::getInstance()->addCommand
        (new SegmentLabelCommand(selection, newLabel));
        m_view->getTrackEditor()->getCompositionView()->slotUpdateSegmentsDrawBuffer();
    }
}

void
RosegardenMainWindow::slotTransposeSegments()
{
    if (!m_view->haveSelection())
        return ;

    IntervalDialog intervalDialog(this, true, true);
    int ok = intervalDialog.exec();
    
    int semitones = intervalDialog.getChromaticDistance();
    int steps = intervalDialog.getDiatonicDistance();
    
    if (!ok || (semitones == 0 && steps == 0)) return;
    
    CommandHistory::getInstance()->addCommand
            (new SegmentTransposeCommand(m_view->getSelection(), intervalDialog.getChangeKey(), steps, semitones, intervalDialog.getTransposeSegmentBack()));
}

void
RosegardenMainWindow::slotChangeCompositionLength()
{
    CompositionLengthDialog dialog(this, &m_doc->getComposition());

    if (dialog.exec() == QDialog::Accepted) {
        ChangeCompositionLengthCommand *command
        = new ChangeCompositionLengthCommand(
              &m_doc->getComposition(),
              dialog.getStartMarker(),
              dialog.getEndMarker());

        m_view->getTrackEditor()->getCompositionView()->clearSegmentRectsCache(true);
        CommandHistory::getInstance()->addCommand(command);
    }
}

void
RosegardenMainWindow::slotManageMIDIDevices()
{
    if (m_devicesManagerNew != 0){
        RG_DEBUG << "Warning: m_devicesManagerNew was NOT null, in RosegardenMainWindow::slotOpenDeviceManagerNew() " << endl;
        //delete m_devicesManagerNew;
    }
    if (! m_devicesManagerNew){
        m_devicesManagerNew = new DeviceManagerDialog(this, getDocument());
        //m_devicesManagerNew->setupUi(dynamic_cast<QDialog*>(m_devicesManagerNew));
        
        //devMan->setAttribute(Qt::WA_DeleteOnClose);    // destroys dialog, if close event was accepted
        
        connect(m_devicesManagerNew, SIGNAL(editBanks(DeviceId)),
            this, SLOT(slotEditBanks(DeviceId)));
        
        connect(m_devicesManagerNew, SIGNAL(editControllers(DeviceId)),
            this, SLOT(slotEditControlParameters(DeviceId)));
        
        if (m_midiMixer) {
//             connect(m_devicesManagerNew, SIGNAL(deviceNamesChanged()),
//                         m_midiMixer, SLOT(slotSynchronise()));
        }
    }
    
    QToolButton *tb = findChild<QToolButton*>("manage_midi_devices");
    if(tb){
        tb->setDown(true);
    }
    
    m_devicesManagerNew->show();

// old slotManageMIDIDevices for old dialog that no longer exists, left in case
// any useful tricks remain to be discovered by looking through the old code.
/*    if (m_deviceManager) {
        m_deviceManager->show();
        m_deviceManager->raise();
        m_deviceManager->setActiveWindow();
        return ;
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

    connect(m_deviceManager, SIGNAL(editBanks(DeviceId)),
            this, SLOT(slotEditBanks(DeviceId)));

    connect(m_deviceManager, SIGNAL(editControllers(DeviceId)),
            this, SLOT(slotEditControlParameters(DeviceId)));

    if (m_midiMixer) {
        connect(m_deviceManager, SIGNAL(deviceNamesChanged()),
                m_midiMixer, SLOT(slotSynchronise()));

    }


    m_deviceManager->show();*/
}

void
RosegardenMainWindow::slotManageSynths()
{
    if (m_synthManager) {
        m_synthManager->show();
        m_synthManager->raise();
        m_synthManager->setActiveWindow();
        return ;
    }

    m_synthManager = new SynthPluginManagerDialog(this, m_doc, m_pluginGUIManager);

    connect(m_synthManager, SIGNAL(closing()),
            this, SLOT(slotSynthPluginManagerClosed()));

    connect(this, SIGNAL(documentAboutToChange()),
            m_synthManager, SLOT(close()));

    connect(m_synthManager,
            SIGNAL(pluginSelected(InstrumentId, int, int)),
            this,
            SLOT(slotPluginSelected(InstrumentId, int, int)));

    connect(m_synthManager,
            SIGNAL(showPluginDialog(QWidget *, InstrumentId, int)),
            this,
            SLOT(slotShowPluginDialog(QWidget *, InstrumentId, int)));

    connect(m_synthManager,
            SIGNAL(showPluginGUI(InstrumentId, int)),
            this,
            SLOT(slotShowPluginGUI(InstrumentId, int)));

    m_synthManager->show();
}

void
RosegardenMainWindow::slotOpenAudioMixer()
{
    if (m_audioMixer) {
        m_audioMixer->show();
        m_audioMixer->raise();
        m_audioMixer->setActiveWindow();
        return ;
    }

    m_audioMixer = new AudioMixerWindow(this, m_doc);

    connect(m_audioMixer, SIGNAL(windowActivated()),
            m_view, SLOT(slotActiveMainWindowChanged()));

    connect(m_view, SIGNAL(controllerDeviceEventReceived(MappedEvent *, const void *)),
            m_audioMixer, SLOT(slotControllerDeviceEventReceived(MappedEvent *, const void *)));

    connect(m_audioMixer, SIGNAL(closing()),
            this, SLOT(slotAudioMixerClosed()));

    connect(m_audioMixer, SIGNAL(selectPlugin(QWidget *, InstrumentId, int)),
            this, SLOT(slotShowPluginDialog(QWidget *, InstrumentId, int)));

    connect(this,
            SIGNAL(pluginSelected(InstrumentId, int, int)),
            m_audioMixer,
            SLOT(slotPluginSelected(InstrumentId, int, int)));

    connect(this,
            SIGNAL(pluginBypassed(InstrumentId, int, bool)),
            m_audioMixer,
            SLOT(slotPluginBypassed(InstrumentId, int, bool)));

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
    connect(m_audioMixer, SIGNAL(panic()),
            this, SLOT(slotPanic()));

    connect(m_audioMixer,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            this,
            SIGNAL(instrumentParametersChanged(InstrumentId)));

    connect(this,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            m_audioMixer,
            SLOT(slotUpdateInstrument(InstrumentId)));

    if (m_synthManager) {
        connect(m_synthManager,
                SIGNAL(pluginSelected(InstrumentId, int, int)),
                m_audioMixer,
                SLOT(slotPluginSelected(InstrumentId, int, int)));
    }

    plugShortcuts(m_audioMixer, m_audioMixer->getShortcuts());

    m_audioMixer->show();
}

void
RosegardenMainWindow::slotOpenMidiMixer()
{
    if (m_midiMixer) {
        m_midiMixer->show();
        m_midiMixer->raise();
        m_midiMixer->setActiveWindow();
        return ;
    }

    m_midiMixer = new MidiMixerWindow(this, m_doc);

    connect(m_midiMixer, SIGNAL(windowActivated()),
            m_view, SLOT(slotActiveMainWindowChanged()));

    connect(m_view, SIGNAL(controllerDeviceEventReceived(MappedEvent *, const void *)),
            m_midiMixer, SLOT(slotControllerDeviceEventReceived(MappedEvent *, const void *)));

    connect(m_midiMixer, SIGNAL(closing()),
            this, SLOT(slotMidiMixerClosed()));

    connect(this, SIGNAL(documentAboutToChange()),
            m_midiMixer, SLOT(close()));

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
    connect(m_midiMixer, SIGNAL(panic()),
            this, SLOT(slotPanic()));

    connect(m_midiMixer,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            this,
            SIGNAL(instrumentParametersChanged(InstrumentId)));

    connect(this,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            m_midiMixer,
            SLOT(slotUpdateInstrument(InstrumentId)));

    plugShortcuts(m_midiMixer, m_midiMixer->getShortcuts());

    m_midiMixer->show();
}

void
RosegardenMainWindow::slotEditControlParameters(DeviceId device)
{
    for (std::set
                <ControlEditorDialog *>::iterator i = m_controlEditors.begin();
                i != m_controlEditors.end(); ++i) {
            if ((*i)->getDevice() == device) {
                (*i)->show();
                (*i)->raise();
                (*i)->setActiveWindow();
                return ;
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

    
    controlEditor->resize(780, 360);
    controlEditor->move(50, 80);
    controlEditor->show();
}

void
RosegardenMainWindow::slotEditBanks()
{
    slotEditBanks(Device::NO_DEVICE);
}

void
RosegardenMainWindow::slotEditBanks(DeviceId device)
{
    if (m_bankEditor) {
        if (device != Device::NO_DEVICE)
            m_bankEditor->setCurrentDevice(device);
        m_bankEditor->show();
        m_bankEditor->raise();
        m_bankEditor->setActiveWindow();
        return ;
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
RosegardenMainWindow::slotManageTriggerSegments()
{
    if (m_triggerSegmentManager) {
        m_triggerSegmentManager->show();
        m_triggerSegmentManager->raise();
        m_triggerSegmentManager->setActiveWindow();
        return ;
    }

    m_triggerSegmentManager = new TriggerSegmentManager(this, m_doc);

    connect(m_triggerSegmentManager, SIGNAL(closing()),
            SLOT(slotTriggerManagerClosed()));

    connect(m_triggerSegmentManager, SIGNAL(editTriggerSegment(int)),
            m_view, SLOT(slotEditTriggerSegment(int)));

    m_triggerSegmentManager->show();
}

void
RosegardenMainWindow::slotTriggerManagerClosed()
{
    RG_DEBUG << "RosegardenMainWindow::slotTriggerManagerClosed" << endl;

    m_triggerSegmentManager = 0;
}

void
RosegardenMainWindow::slotEditMarkers()
{
    if (m_markerEditor) {
        m_markerEditor->show();
        m_markerEditor->raise();
        m_markerEditor->setActiveWindow();
        return ;
    }

    m_markerEditor = new MarkerEditor(this, m_doc);

    connect(m_markerEditor, SIGNAL(closing()),
            SLOT(slotMarkerEditorClosed()));

    connect(m_markerEditor, SIGNAL(jumpToMarker(timeT)),
            m_doc, SLOT(slotSetPointerPosition(timeT)));

    plugShortcuts(m_markerEditor, m_markerEditor->getShortcuts());

    m_markerEditor->show();
}

void
RosegardenMainWindow::slotMarkerEditorClosed()
{
    RG_DEBUG << "RosegardenMainWindow::slotMarkerEditorClosed" << endl;

    m_markerEditor = 0;
}

void
RosegardenMainWindow::slotEditTempos(timeT t)
{
    if (m_tempoView) {
        m_tempoView->show();
        m_tempoView->raise();
        m_tempoView->setActiveWindow();
        return ;
    }

    m_tempoView = new TempoView(m_doc, getView(), t);

    connect(m_tempoView, SIGNAL(closing()),
            SLOT(slotTempoViewClosed()));

    connect(m_tempoView, SIGNAL(windowActivated()),
            getView(), SLOT(slotActiveMainWindowChanged()));

    connect(m_tempoView,
            SIGNAL(changeTempo(timeT,
                               tempoT,
                               tempoT,
                               TempoDialog::TempoDialogAction)),
            this,
            SLOT(slotChangeTempo(timeT,
                                 tempoT,
                                 tempoT,
                                 TempoDialog::TempoDialogAction)));

    connect(m_tempoView, SIGNAL(saveFile()), this, SLOT(slotFileSave()));

    plugShortcuts(m_tempoView, m_tempoView->getShortcuts());

    m_tempoView->show();
}

void
RosegardenMainWindow::slotTempoViewClosed()
{
    RG_DEBUG << "RosegardenMainWindow::slotTempoViewClosed" << endl;

    m_tempoView = 0;
}

void
RosegardenMainWindow::slotControlEditorClosed()
{
    const QObject *s = sender();

    RG_DEBUG << "RosegardenMainWindow::slotControlEditorClosed" << endl;

    for (std::set
                <ControlEditorDialog *>::iterator i = m_controlEditors.begin();
                i != m_controlEditors.end(); ++i) {
            if (*i == s) {
                m_controlEditors.erase(i);
                RG_DEBUG << "removed control editor dialog, have " << m_controlEditors.size() << " left" << endl;
                return ;
            }
        }

    std::cerr << "WARNING: control editor " << s << " closed, but couldn't find it in our control editor list (we have " << m_controlEditors.size() << " editors)" << std::endl;
}

void
RosegardenMainWindow::slotShowPluginDialog(QWidget *parent,
                                       InstrumentId instrumentId,
                                       int index)
{
    if (!parent)
        parent = this;

    int key = (index << 16) + instrumentId;

    if (m_pluginDialogs[key]) {
        m_pluginDialogs[key]->show();
        m_pluginDialogs[key]->raise();
        m_pluginDialogs[key]->setActiveWindow();
        return ;
    }

    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenMainWindow::slotShowPluginDialog - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    // only create a dialog if we've got a plugin instance
    AudioPluginInstance *inst =
        container->getPlugin(index);

    if (!inst) {
        RG_DEBUG << "RosegardenMainWindow::slotShowPluginDialog - "
        << "no AudioPluginInstance found for index "
        << index << endl;
        return ;
    }

    // Create the plugin dialog
    //
    AudioPluginDialog *dialog =
        new AudioPluginDialog(parent,
                              m_doc->getPluginManager(),
                              m_pluginGUIManager,
                              container,
                              index);

    connect(dialog, SIGNAL(windowActivated()),
            m_view, SLOT(slotActiveMainWindowChanged()));

/* This feature isn't provided by the plugin dialog
    connect(m_view, SIGNAL(controllerDeviceEventReceived(MappedEvent *, const void *)),
            dialog, SLOT(slotControllerDeviceEventReceived(MappedEvent *, const void *)));
*/

    // Plug the new dialog into the standard keyboard shortcuts so
    // that we can use them still while the plugin has focus.
    //
    plugShortcuts(dialog, dialog->getShortcuts());

    connect(dialog,
            SIGNAL(pluginSelected(InstrumentId, int, int)),
            this,
            SLOT(slotPluginSelected(InstrumentId, int, int)));

    connect(dialog,
            SIGNAL(pluginPortChanged(InstrumentId, int, int)),
            this,
            SLOT(slotPluginPortChanged(InstrumentId, int, int)));

    connect(dialog,
            SIGNAL(pluginProgramChanged(InstrumentId, int)),
            this,
            SLOT(slotPluginProgramChanged(InstrumentId, int)));

    connect(dialog,
            SIGNAL(changePluginConfiguration(InstrumentId, int, bool, QString, QString)),
            this,
            SLOT(slotChangePluginConfiguration(InstrumentId, int, bool, QString, QString)));

    connect(dialog,
            SIGNAL(showPluginGUI(InstrumentId, int)),
            this,
            SLOT(slotShowPluginGUI(InstrumentId, int)));

    connect(dialog,
            SIGNAL(stopPluginGUI(InstrumentId, int)),
            this,
            SLOT(slotStopPluginGUI(InstrumentId, int)));

    connect(dialog,
            SIGNAL(bypassed(InstrumentId, int, bool)),
            this,
            SLOT(slotPluginBypassed(InstrumentId, int, bool)));

    connect(dialog,
            SIGNAL(destroyed(InstrumentId, int)),
            this,
            SLOT(slotPluginDialogDestroyed(InstrumentId, int)));

    connect(this, SIGNAL(documentAboutToChange()), dialog, SLOT(close()));

    m_pluginDialogs[key] = dialog;
    m_pluginDialogs[key]->show();

    // Set modified
    m_doc->slotDocumentModified();
}

void
RosegardenMainWindow::slotPluginSelected(InstrumentId instrumentId,
                                     int index, int plugin)
{
    const QObject *s = sender();

    bool fromSynthMgr = (s == m_synthManager);

    // It's assumed that ports etc will already have been set up on
    // the AudioPluginInstance before this is invoked.

    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenMainWindow::slotPluginSelected - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst =
        container->getPlugin(index);

    if (!inst) {
        RG_DEBUG << "RosegardenMainWindow::slotPluginSelected - "
        << "got index of unknown plugin!" << endl;
        return ;
    }

    if (plugin == -1) {
        // Destroy plugin instance
        //!!! seems iffy -- why can't we just unassign it?

        if (StudioControl::
                destroyStudioObject(inst->getMappedId())) {
            RG_DEBUG << "RosegardenMainWindow::slotPluginSelected - "
            << "cannot destroy Studio object "
            << inst->getMappedId() << endl;
        }

        inst->setAssigned(false);
    } else {
        // If unassigned then create a sequencer instance of this
        // AudioPluginInstance.
        //
        if (inst->isAssigned()) {
            RG_DEBUG << "RosegardenMainWindow::slotPluginSelected - "
            << " setting identifier for mapper id " << inst->getMappedId()
            << " to " << inst->getIdentifier() << endl;

            StudioControl::setStudioObjectProperty
            (inst->getMappedId(),
             MappedPluginSlot::Identifier,
             strtoqstr(inst->getIdentifier()));
        } else {
            // create a studio object at the sequencer
            MappedObjectId newId =
                StudioControl::createStudioObject
                (MappedObject::PluginSlot);

            RG_DEBUG << "RosegardenMainWindow::slotPluginSelected - "
            << " new MappedObjectId = " << newId << endl;

            // set the new Mapped ID and that this instance
            // is assigned
            inst->setMappedId(newId);
            inst->setAssigned(true);

            // set the instrument id
            StudioControl::setStudioObjectProperty
            (newId,
             MappedObject::Instrument,
             MappedObjectValue(instrumentId));

            // set the position
            StudioControl::setStudioObjectProperty
            (newId,
             MappedObject::Position,
             MappedObjectValue(index));

            // set the plugin id
            StudioControl::setStudioObjectProperty
            (newId,
             MappedPluginSlot::Identifier,
             strtoqstr(inst->getIdentifier()));
        }
    }

    int pluginMappedId = inst->getMappedId();

    //!!! much code duplicated here from RosegardenDocument::initialiseStudio

    inst->setConfigurationValue
    (qstrtostr(PluginIdentifier::RESERVED_PROJECT_DIRECTORY_KEY),
     m_doc->getAudioFileManager().getAudioPath());

    // Set opaque string configuration data (e.g. for DSSI plugin)
    //
    MappedObjectPropertyList config;
    for (AudioPluginInstance::ConfigMap::const_iterator
            i = inst->getConfiguration().begin();
            i != inst->getConfiguration().end(); ++i) {
        config.push_back(strtoqstr(i->first));
        config.push_back(strtoqstr(i->second));
    }

    QString error = StudioControl::setStudioObjectPropertyList
        (pluginMappedId,
         MappedPluginSlot::Configuration,
         config);

    if (error != "") showError(error);

    // Set the bypass
    //
    StudioControl::setStudioObjectProperty
    (pluginMappedId,
     MappedPluginSlot::Bypassed,
     MappedObjectValue(inst->isBypassed()));

    // Set the program
    //
    if (inst->getProgram() != "") {
        StudioControl::setStudioObjectProperty
        (pluginMappedId,
         MappedPluginSlot::Program,
         strtoqstr(inst->getProgram()));
    }

    // Set all the port values
    //
    PortInstanceIterator portIt;

    for (portIt = inst->begin();
            portIt != inst->end(); ++portIt) {
        StudioControl::setStudioPluginPort
        (pluginMappedId,
         (*portIt)->number,
         (*portIt)->value);
    }

    if (fromSynthMgr) {
        int key = (index << 16) + instrumentId;
        if (m_pluginDialogs[key]) {
            m_pluginDialogs[key]->updatePlugin(plugin);
        }
    } else if (m_synthManager) {
        m_synthManager->updatePlugin(instrumentId, plugin);
    }

    emit pluginSelected(instrumentId, index, plugin);

    // Set modified
    m_doc->slotDocumentModified();
}

void
RosegardenMainWindow::slotChangePluginPort(InstrumentId instrumentId,
                                       int pluginIndex,
                                       int portIndex,
                                       float value)
{
    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenMainWindow::slotChangePluginPort - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);
    if (!inst) {
        RG_DEBUG << "RosegardenMainWindow::slotChangePluginPort - "
        << "no plugin at index " << pluginIndex << " on " << instrumentId << endl;
        return ;
    }

    PluginPortInstance *port = inst->getPort(portIndex);
    if (!port) {
        RG_DEBUG << "RosegardenMainWindow::slotChangePluginPort - no port "
        << portIndex << endl;
        return ;
    }

    RG_DEBUG << "RosegardenMainWindow::slotPluginPortChanged - "
    << "setting plugin port (" << inst->getMappedId()
    << ", " << portIndex << ") from " << port->value
    << " to " << value << endl;

    port->setValue(value);

    StudioControl::setStudioPluginPort(inst->getMappedId(),
                                       portIndex, port->value);

    m_doc->slotDocumentModified();

    // This modification came from The Outside!
    int key = (pluginIndex << 16) + instrumentId;
    if (m_pluginDialogs[key]) {
        m_pluginDialogs[key]->updatePluginPortControl(portIndex);
    }
}

void
RosegardenMainWindow::slotPluginPortChanged(InstrumentId instrumentId,
                                        int pluginIndex,
                                        int portIndex)
{
    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenMainWindow::slotPluginPortChanged - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);
    if (!inst) {
        RG_DEBUG << "RosegardenMainWindow::slotPluginPortChanged - "
        << "no plugin at index " << pluginIndex << " on " << instrumentId << endl;
        return ;
    }

    PluginPortInstance *port = inst->getPort(portIndex);
    if (!port) {
        RG_DEBUG << "RosegardenMainWindow::slotPluginPortChanged - no port "
        << portIndex << endl;
        return ;
    }

    RG_DEBUG << "RosegardenMainWindow::slotPluginPortChanged - "
    << "setting plugin port (" << inst->getMappedId()
    << ", " << portIndex << ") to " << port->value << endl;

    StudioControl::setStudioPluginPort(inst->getMappedId(),
                                       portIndex, port->value);

    m_doc->slotDocumentModified();

    // This modification came from our own plugin dialog, so update
    // any external GUIs
    if (m_pluginGUIManager) {
        m_pluginGUIManager->updatePort(instrumentId,
                                       pluginIndex,
                                       portIndex);
    }
}

void
RosegardenMainWindow::slotChangePluginProgram(InstrumentId instrumentId,
        int pluginIndex,
        QString program)
{
    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenMainWindow::slotChangePluginProgram - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);
    if (!inst) {
        RG_DEBUG << "RosegardenMainWindow::slotChangePluginProgram - "
        << "no plugin at index " << pluginIndex << " on " << instrumentId << endl;
        return ;
    }

    RG_DEBUG << "RosegardenMainWindow::slotChangePluginProgram - "
    << "setting plugin program ("
    << inst->getMappedId() << ") from " << inst->getProgram()
    << " to " << program << endl;

    inst->setProgram(qstrtostr(program));

    StudioControl::
    setStudioObjectProperty(inst->getMappedId(),
                            MappedPluginSlot::Program,
                            program);

    PortInstanceIterator portIt;

    for (portIt = inst->begin();
            portIt != inst->end(); ++portIt) {
        float value = StudioControl::getStudioPluginPort
                      (inst->getMappedId(),
                       (*portIt)->number);
        (*portIt)->value = value;
    }

    // Set modified
    m_doc->slotDocumentModified();

    int key = (pluginIndex << 16) + instrumentId;
    if (m_pluginDialogs[key]) {
        m_pluginDialogs[key]->updatePluginProgramControl();
    }
}

void
RosegardenMainWindow::slotPluginProgramChanged(InstrumentId instrumentId,
        int pluginIndex)
{
    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenMainWindow::slotPluginProgramChanged - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);
    if (!inst) {
        RG_DEBUG << "RosegardenMainWindow::slotPluginProgramChanged - "
        << "no plugin at index " << pluginIndex << " on " << instrumentId << endl;
        return ;
    }

    QString program = strtoqstr(inst->getProgram());

    RG_DEBUG << "RosegardenMainWindow::slotPluginProgramChanged - "
    << "setting plugin program ("
    << inst->getMappedId() << ") to " << program << endl;

    StudioControl::
    setStudioObjectProperty(inst->getMappedId(),
                            MappedPluginSlot::Program,
                            program);

    PortInstanceIterator portIt;

    for (portIt = inst->begin();
            portIt != inst->end(); ++portIt) {
        float value = StudioControl::getStudioPluginPort
                      (inst->getMappedId(),
                       (*portIt)->number);
        (*portIt)->value = value;
    }

    // Set modified
    m_doc->slotDocumentModified();

    if (m_pluginGUIManager)
        m_pluginGUIManager->updateProgram(instrumentId,
                                          pluginIndex);
}

void
RosegardenMainWindow::slotChangePluginConfiguration(InstrumentId instrumentId,
        int index,
        bool global,
        QString key,
        QString value)
{
    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenMainWindow::slotChangePluginConfiguration - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(index);

    if (global && inst) {

        // Set the same configuration on other plugins in the same
        // instance group

        AudioPlugin *pl =
            m_pluginManager->getPluginByIdentifier(strtoqstr(inst->getIdentifier()));

        if (pl && pl->isGrouped()) {

            InstrumentList il =
                m_doc->getStudio().getAllInstruments();

            for (InstrumentList::iterator i = il.begin();
                    i != il.end(); ++i) {

                for (PluginInstanceIterator pli =
                            (*i)->beginPlugins();
                        pli != (*i)->endPlugins(); ++pli) {

                    if (*pli && (*pli)->isAssigned() &&
                            (*pli)->getIdentifier() == inst->getIdentifier() &&
                            (*pli) != inst) {

                        slotChangePluginConfiguration
                        ((*i)->getId(), (*pli)->getPosition(),
                         false, key, value);

                        m_pluginGUIManager->updateConfiguration
                        ((*i)->getId(), (*pli)->getPosition(), key);
                    }
                }
            }
        }
    }

    if (inst) {

        inst->setConfigurationValue(qstrtostr(key), qstrtostr(value));

        MappedObjectPropertyList config;
        for (AudioPluginInstance::ConfigMap::const_iterator
                i = inst->getConfiguration().begin();
                i != inst->getConfiguration().end(); ++i) {
            config.push_back(strtoqstr(i->first));
            config.push_back(strtoqstr(i->second));
        }

        RG_DEBUG << "RosegardenMainWindow::slotChangePluginConfiguration: setting new config on mapped id " << inst->getMappedId() << endl;

        QString error = StudioControl::setStudioObjectPropertyList
        (inst->getMappedId(),
         MappedPluginSlot::Configuration,
         config);

        if (error != "") showError(error);

        // Set modified
        m_doc->slotDocumentModified();

        int key = (index << 16) + instrumentId;
        if (m_pluginDialogs[key]) {
            m_pluginDialogs[key]->updatePluginProgramList();
        }
    }
}

void
RosegardenMainWindow::slotPluginDialogDestroyed(InstrumentId instrumentId,
        int index)
{
    int key = (index << 16) + instrumentId;
    m_pluginDialogs[key] = 0;
}

void
RosegardenMainWindow::slotPluginBypassed(InstrumentId instrumentId,
                                     int pluginIndex, bool bp)
{
    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenMainWindow::slotPluginBypassed - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);

    if (inst) {
        StudioControl::setStudioObjectProperty
        (inst->getMappedId(),
         MappedPluginSlot::Bypassed,
         MappedObjectValue(bp));

        // Set the bypass on the instance
        //
        inst->setBypass(bp);

        // Set modified
        m_doc->slotDocumentModified();
    }

    emit pluginBypassed(instrumentId, pluginIndex, bp);
}

void
RosegardenMainWindow::slotShowPluginGUI(InstrumentId instrument,
                                    int index)
{
    m_pluginGUIManager->showGUI(instrument, index);
}

void
RosegardenMainWindow::slotStopPluginGUI(InstrumentId instrument,
                                    int index)
{
    m_pluginGUIManager->stopGUI(instrument, index);
}

void
RosegardenMainWindow::slotPluginGUIExited(InstrumentId instrument,
                                      int index)
{
    int key = (index << 16) + instrument;
    if (m_pluginDialogs[key]) {
        m_pluginDialogs[key]->guiExited();
    }
}

void
RosegardenMainWindow::slotPlayList()
{
    if (!m_playList) {
        m_playList = new PlayListDialog(tr("Play List"), this);
        connect(m_playList, SIGNAL(closing()),
                SLOT(slotPlayListClosed()));
        connect(m_playList->getPlayList(), SIGNAL(play(QString)),
                SLOT(slotPlayListPlay(QString)));
    }

    m_playList->show();
}

void
RosegardenMainWindow::slotPlayListPlay(QString url)
{
    slotStop();
    openURL(url);
    slotPlay();
}

void
RosegardenMainWindow::slotPlayListClosed()
{
    RG_DEBUG << "RosegardenMainWindow::slotPlayListClosed()\n";
    m_playList = 0;
}

static void
invokeBrowser(QString url)
{
    // This is mostly lifted from Qt Assistant source code

    QProcess *process = new QProcess;
    QObject::connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));

    QStringList args;

#ifdef Q_OS_MAC
    args.append(url);
    process->start("open", args);
#else
#ifdef Q_OS_WIN32

    QString pf(getenv("ProgramFiles"));
    QString command = pf + QString("\\Internet Explorer\\IEXPLORE.EXE");

    args.append(url);
    process->start(command, args);

#else
#ifdef Q_WS_X11
    if (!qgetenv("KDE_FULL_SESSION").isEmpty()) {
        args.append("exec");
        args.append(url);
        process->start("kfmclient", args);
    } else if (!qgetenv("BROWSER").isEmpty()) {
        args.append(url);
        process->start(qgetenv("BROWSER"), args);
    } else {
        args.append(url);
        process->start("firefox", args);
    }
#endif
#endif
#endif
}

void
RosegardenMainWindow::slotTutorial()
{
    QString tutorialURL = tr("http://www.rosegardenmusic.com/tutorials/en/chapter-0.html");
    invokeBrowser(tutorialURL);
}

void
RosegardenMainWindow::slotBugGuidelines()
{
    QString glURL = tr("http://rosegarden.sourceforge.net/tutorial/bug-guidelines.html");
    invokeBrowser(glURL);
}

void
RosegardenMainWindow::slotBankEditorClosed()
{
    RG_DEBUG << "RosegardenMainWindow::slotBankEditorClosed()\n";

    if (m_doc->isModified()) {
        if (m_view)
            m_view->slotSelectTrackSegments(m_doc->getComposition().getSelectedTrack());
    }

    m_bankEditor = 0;
}

void
RosegardenMainWindow::slotDeviceManagerClosed()
{
// obsolete, or merely unimplemented for the new dialog?
//
/*    RG_DEBUG << "RosegardenMainWindow::slotDeviceManagerClosed()\n";

    if (m_doc->isModified()) {
        if (m_view)
            m_view->slotSelectTrackSegments(m_doc->getComposition().getSelectedTrack());
    }

    m_deviceManager = 0;*/
}

void
RosegardenMainWindow::slotSynthPluginManagerClosed()
{
    RG_DEBUG << "RosegardenMainWindow::slotSynthPluginManagerClosed()\n";

    m_synthManager = 0;
}

void
RosegardenMainWindow::slotAudioMixerClosed()
{
    RG_DEBUG << "RosegardenMainWindow::slotAudioMixerClosed()\n";

    m_audioMixer = 0;
}

void
RosegardenMainWindow::slotMidiMixerClosed()
{
    RG_DEBUG << "RosegardenMainWindow::slotMidiMixerClosed()\n";

    m_midiMixer = 0;
}

void
RosegardenMainWindow::slotAudioManagerClosed()
{
    RG_DEBUG << "RosegardenMainWindow::slotAudioManagerClosed()\n";

    if (m_doc->isModified()) {
        if (m_view)
            m_view->slotSelectTrackSegments(m_doc->getComposition().getSelectedTrack());
    }

    m_audioManagerDialog = 0;
}

void
RosegardenMainWindow::slotPanic()
{
    if (m_seqManager) {
        // Stop the transport before we send a panic as the
        // playback goes all to hell anyway.
        //
        slotStop();

        ProgressDialog progressDlg(tr("Queueing MIDI panic events for tranmission..."),
                                   100,
                                   this);
        CurrentProgressDialog::set
            (&progressDlg);
        ProgressDialog::processEvents();

        connect(m_seqManager, SIGNAL(setValue(int)),
                m_progressBar, SLOT(setValue(int)));
        connect(m_seqManager, SIGNAL(incrementProgress(int)),
                m_progressBar, SLOT(advance(int)));

        m_seqManager->panic();

    }
}

void
RosegardenMainWindow::slotPopulateTrackInstrumentPopup()
{
    RG_DEBUG << "RosegardenMainWindow::slotSetTrackInstrument\n";
    Composition &comp = m_doc->getComposition();
    Track *track = comp.getTrackById(comp.getSelectedTrack());

    if (!track) {
        RG_DEBUG << "Weird: no track available for instrument popup!" << endl;
        return ;
    }

    Instrument* instrument = m_doc->getStudio().getInstrumentById(track->getInstrument());

//    QPopupMenu* popup = dynamic_cast<QPopupMenu*>(factory()->container("set_track_instrument", this));
    QMenu* popup = this->findChild<QMenu*>("set_track_instrument");

    m_view->getTrackEditor()->getTrackButtons()->populateInstrumentPopup(instrument, popup);
}

void
RosegardenMainWindow::slotRemapInstruments()
{
    RG_DEBUG << "RosegardenMainWindow::slotRemapInstruments\n";
    RemapInstrumentDialog dialog(this, m_doc);

    connect(&dialog, SIGNAL(applyClicked()),
            m_view->getTrackEditor()->getTrackButtons(),
            SLOT(slotSynchroniseWithComposition()));

    if (dialog.exec() == QDialog::Accepted) {
        RG_DEBUG << "slotRemapInstruments - accepted\n";
    }

}

void
RosegardenMainWindow::slotSaveDefaultStudio()
{
    RG_DEBUG << "RosegardenMainWindow::slotSaveDefaultStudio\n";

    int reply = QMessageBox::warning
                (this, "", tr("Are you sure you want to save this as your default studio?"), 
                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return ;

    TmpStatusMsg msg(tr("Saving current document as default studio..."), this);

    QString autoloadFile = ResourceFinder().getAutoloadSavePath();
    
    RG_DEBUG << "RosegardenMainWindow::slotSaveDefaultStudio : saving studio in "
             << autoloadFile << endl;

    SetWaitCursor waitCursor;
    QString errMsg;
    bool res = m_doc->saveDocument(autoloadFile, errMsg);
    if (!res) {
        if (!errMsg.isEmpty())
            QMessageBox::critical(this, "", tr(qStrToCharPtrUtf8(QString("Could not auto-save document at %1\nError was : %2")
                                          .arg(autoloadFile).arg(errMsg))));
        else
            QMessageBox::critical(this, "", tr(qStrToCharPtrUtf8(QString("Could not auto-save document at %1")
                                          .arg(autoloadFile))));

    }
}

void
RosegardenMainWindow::slotImportDefaultStudio()
{
    int reply = QMessageBox::warning
            (this, "", tr("Are you sure you want to import your default studio and lose the current one?"), QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return ;

    QString autoloadFile = ResourceFinder().getAutoloadPath();

    QFileInfo autoloadFileInfo(autoloadFile);

    if (!autoloadFileInfo.isReadable()) {
        RG_DEBUG << "RosegardenDocument::slotImportDefaultStudio - "
        << "can't find autoload file - defaulting" << endl;
        return ;
    }

    slotImportStudioFromFile(autoloadFile);
}

void
RosegardenMainWindow::slotImportStudio()
{
    RG_DEBUG << "RosegardenMainWindow::slotImportStudio()\n";

    QString studioDir = ResourceFinder().getResourceDir("library");

    QUrl url = QFileDialog::getOpenFileName(this, tr("Import Studio from File"), studioDir,
               tr("All files") + " (*)", 0, 0);

    if (url.isEmpty())
        return ;

    QString target;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", tr("Cannot download file %1").arg(url.toString()));
        return ;
    }

    target = source.getLocalFilename();
    source.waitForData();

    slotImportStudioFromFile(target);
}

void
RosegardenMainWindow::slotImportStudioFromFile(const QString &file)
{
    RosegardenDocument *doc = new RosegardenDocument(this, 0, true); // skipAutoload

    Studio &oldStudio = m_doc->getStudio();
    Studio &newStudio = doc->getStudio();

    // Add some dummy devices for when we open the document.  We guess
    // that the file won't have more than 32 devices.
    //
    //    for (unsigned int i = 0; i < 32; i++) {
    //        newStudio.addDevice("", i, Device::Midi);
    //    }

    if (doc->openDocument(file, true)) { // true because we actually
        // do want to create devices
        // on the sequencer here

        MacroCommand *command = new MacroCommand(tr("Import Studio"));
        doc->syncDevices();

        // We actually only copy across MIDI play devices... for now
        std::vector<DeviceId> midiPlayDevices;

        for (DeviceList::const_iterator i =
                    oldStudio.begin(); i != oldStudio.end(); ++i) {

            MidiDevice *md =
                dynamic_cast<MidiDevice *>(*i);

            if (md && (md->getDirection() == MidiDevice::Play)) {
                midiPlayDevices.push_back((*i)->getId());
            }
        }

        std::vector<DeviceId>::iterator di(midiPlayDevices.begin());

        for (DeviceList::const_iterator i =
                    newStudio.begin(); i != newStudio.end(); ++i) {

            MidiDevice *md =
                dynamic_cast<MidiDevice *>(*i);

            if (md && (md->getDirection() == MidiDevice::Play)) {
                if (di != midiPlayDevices.end()) {
                    MidiDevice::VariationType variation
                    (md->getVariationType());
                    BankList bl(md->getBanks());
                    ProgramList pl(md->getPrograms());
                    ControlList cl(md->getControlParameters());

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

        CommandHistory::getInstance()->addCommand(command);
        m_doc->syncDevices();
        m_doc->initialiseStudio(); // The other document will have reset it
    }

    delete doc;
}

void
RosegardenMainWindow::slotResetMidiNetwork()
{
    if (m_seqManager) {

        m_seqManager->preparePlayback(true);

        m_seqManager->resetMidiNetwork();
    }

}

void
RosegardenMainWindow::slotModifyMIDIFilters()
{
    RG_DEBUG << "RosegardenMainWindow::slotModifyMIDIFilters" << endl;

    MidiFilterDialog dialog(this, m_doc);

    if (dialog.exec() == QDialog::Accepted) {
        RG_DEBUG << "slotModifyMIDIFilters - accepted" << endl;
    }
}

void
RosegardenMainWindow::slotManageMetronome()
{
    RG_DEBUG << "RosegardenMainWindow::slotManageMetronome" << endl;

    ManageMetronomeDialog dialog(this, m_doc);

    if (dialog.exec() == QDialog::Accepted) {
        RG_DEBUG << "slotManageMetronome - accepted" << endl;
    }
}

void
RosegardenMainWindow::slotAutoSave()
{
    if (!m_seqManager ||
        m_seqManager->getTransportStatus() == PLAYING ||
        m_seqManager->getTransportStatus() == RECORDING)
        return ;

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);

    if (! qStrToBool(settings.value("autosave", "true"))) {
        settings.endGroup();
        return ;
    }
    m_doc->slotAutoSave();

    settings.endGroup();
}

void
RosegardenMainWindow::slotUpdateAutoSaveInterval(unsigned int interval)
{
    RG_DEBUG << "RosegardenMainWindow::slotUpdateAutoSaveInterval - "
    << "changed interval to " << interval << endl;
    m_autoSaveTimer->changeInterval(int(interval) * 1000);
}

void
RosegardenMainWindow::slotUpdateSidebarStyle(unsigned int style)
{
    RG_DEBUG << "RosegardenMainWindow::slotUpdateSidebarStyle - "
    << "changed style to " << style << endl;
    if (m_parameterArea) m_parameterArea->setArrangement((RosegardenParameterArea::Arrangement) style);
}

void
RosegardenMainWindow::slotShowTip()
{
    RG_DEBUG << "RosegardenMainWindow::slotShowTip" << endl;
//    KTipDialog::showTip(this, locate("data", "rosegarden/tips"), true); //&&& showTip dialog deactivated.
}

void RosegardenMainWindow::slotShowToolHelp(const QString &s)
{
    QString msg = s;
    if (msg != "") msg = " " + msg;
    slotStatusMsg(msg);
}

void
RosegardenMainWindow::slotEnableMIDIThruRouting()
{
    m_seqManager->enableMIDIThruRouting(findAction("enable_midi_routing")->isChecked());
}

TransportDialog* RosegardenMainWindow::getTransport() 
{
    if (m_transport == 0)
        createAndSetupTransport();
    
    return m_transport;
}

RosegardenDocument *RosegardenMainWindow::getDocument() const
{
    return m_doc;
}

void
RosegardenMainWindow::awaitDialogClearance()
{
    std::cerr << "RosegardenMainWindow::awaitDialogClearance: entering" << std::endl;
    
    bool haveDialog = true;

    QDialog *c;
    QList<QDialog*> cl;
    int i;

    while (haveDialog) {

//        std::cerr << "RosegardenMainWindow::awaitDialogClearance: looping" << std::endl;
    
        cl = findChildren<QDialog*>();

        haveDialog = false;
        for(i=0; i < cl.size(); i++){
            c = cl.at(i);
            if(c->isVisible() && c->objectName() != "Rosegarden Transport"){
                haveDialog = true;
                break;    
            }
        }
    
        if (haveDialog) {
            qApp->processEvents(QEventLoop::AllEvents, 300);
        }
    }

    std::cerr << "RosegardenMainWindow::awaitDialogClearance: exiting" << std::endl;
}

void
RosegardenMainWindow::slotNewerVersionAvailable(QString v)
{
    //### DISABLE THIS FOR NOW, BECAUSE IT IS IRRITATING THE PISS OUT OF ME
    //HAVING TO CLICK ON THIS USELESS DIALOG EVERY TIME I RUN THIS THING!
    //
    return;
    //
    
    if (m_firstRun) return;
    StartupLogo::hideIfStillThere();
    CurrentProgressDialog::freeze();
    awaitDialogClearance();
    QMessageBox::information
            (this, tr("Newer version available"),
         tr("<h3>Newer version available</h3><p>A newer version of Rosegarden may be available.<br>Please consult the <a href=\"http://www.rosegardenmusic.com/getting/\">Rosegarden website</a> for more information.</p>").arg("version-%1-available-show").arg(v));
       //  QMessageBox::AllowLink);
    
    CurrentProgressDialog::thaw();
}

void
RosegardenMainWindow::slotSetQuickMarker()
{
    RG_DEBUG << "RosegardenMainWindow::slotSetQuickMarker" << endl;
    
    m_doc->setQuickMarker();
    getView()->getTrackEditor()->updateRulers();
}

void
RosegardenMainWindow::slotJumpToQuickMarker()
{
    RG_DEBUG << "RosegardenMainWindow::slotJumpToQuickMarker" << endl;

    m_doc->jumpToQuickMarker();
}

void RosegardenMainWindow::slotOpenDeviceManagerNew()
{
}

RosegardenMainWindow *RosegardenMainWindow::m_myself = 0;

}// end namespace Rosegarden

#include "RosegardenMainWindow.moc"
