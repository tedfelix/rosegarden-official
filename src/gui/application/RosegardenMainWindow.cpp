﻿/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[RosegardenMainWindow]"
#define RG_NO_DEBUG_PRINT

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
#include "base/NotationQuantizer.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/QEvents.h"
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
#include "commands/notation/InterpretCommand.h"
#include "commands/segment/AddTempoChangeCommand.h"
#include "commands/segment/AddTimeSignatureCommand.h"
#include "commands/segment/AudioSegmentAutoSplitCommand.h"
#include "commands/segment/AudioSegmentRescaleCommand.h"
#include "commands/segment/AudioSegmentSplitCommand.h"
#include "commands/segment/ChangeCompositionLengthCommand.h"
#include "commands/segment/CreateTempoMapFromSegmentCommand.h"
#include "commands/segment/CutRangeCommand.h"
#include "commands/segment/DeleteRangeCommand.h"
#include "commands/segment/EraseTempiInRangeCommand.h"
#include "commands/segment/ExpandFigurationCommand.h"
#include "commands/segment/FitToBeatsCommand.h"
#include "commands/segment/InsertRangeCommand.h"
#include "commands/segment/PasteConductorDataCommand.h"
#include "commands/segment/ModifyDefaultTempoCommand.h"
#include "commands/segment/MoveTracksCommand.h"
#include "commands/segment/PasteRangeCommand.h"
#include "commands/segment/RemoveTempoChangeCommand.h"
#include "commands/segment/RemoveTimeSignatureCommand.h"
#include "commands/segment/SegmentAutoSplitCommand.h"
#include "commands/segment/SegmentChangeTransposeCommand.h"
#include "commands/segment/SegmentJoinCommand.h"
#include "commands/segment/SegmentLabelCommand.h"
#include "commands/segment/SegmentReconfigureCommand.h"
#include "commands/segment/SegmentRescaleCommand.h"
#include "commands/segment/SegmentSplitByPitchCommand.h"
#include "commands/segment/SegmentSplitByRecordingSrcCommand.h"
#include "commands/segment/SegmentSplitByDrumCommand.h"
#include "commands/segment/SegmentSplitCommand.h"
#include "commands/segment/SegmentTransposeCommand.h"
#include "commands/segment/SegmentSyncCommand.h"
#include "commands/segment/UpdateFigurationCommand.h"
#include "commands/studio/CreateOrDeleteDeviceCommand.h"
#include "commands/studio/ModifyDeviceCommand.h"
#include "document/io/CsoundExporter.h"
//#include "document/io/HydrogenLoader.h"
#include "document/io/MusicXMLLoader.h"
#include "document/io/LilyPondExporter.h"
#include "document/CommandHistory.h"
#include "document/io/RG21Loader.h"
#include "document/io/MupExporter.h"
#include "document/io/MusicXmlExporter.h"
#include "document/RosegardenDocument.h"
#include "document/MetadataHelper.h"
#include "misc/ConfigGroups.h"
#include "misc/Preferences.h"
#include "gui/dialogs/AddTracksDialog.h"
#include "gui/dialogs/AboutDialog.h"
#include "gui/dialogs/AudioManagerDialog.h"
#include "gui/dialogs/AudioPluginDialog.h"
#include "gui/dialogs/AudioSplitDialog.h"
#include "gui/dialogs/BeatsBarsDialog.h"
#include "gui/dialogs/CompositionLengthDialog.h"
#include "gui/dialogs/CommentsPopupDialog.h"
#include "gui/dialogs/ConfigureDialog.h"
#include "gui/dialogs/CountdownDialog.h"
#include "gui/dialogs/DialogSuppressor.h"
#include "gui/dialogs/DocumentConfigureDialog.h"
#include "gui/dialogs/FileMergeDialog.h"
#include "gui/dialogs/IdentifyTextCodecDialog.h"
#include "gui/dialogs/InterpretDialog.h"
#include "gui/dialogs/IntervalDialog.h"
#include "gui/dialogs/LilyPondOptionsDialog.h"
#include "gui/dialogs/MusicXMLOptionsDialog.h"
#include "gui/dialogs/ManageMetronomeDialog.h"
#include "gui/dialogs/QuantizeDialog.h"
#include "gui/dialogs/RescaleDialog.h"
#include "gui/dialogs/ShortcutDialog.h"
#include "gui/dialogs/SplitByPitchDialog.h"
#include "gui/dialogs/SplitByRecordingSrcDialog.h"
#include "gui/dialogs/TimeDialog.h"
#include "gui/dialogs/TransportDialog.h"
#include "gui/editors/parameters/InstrumentParameterBox.h"
#include "gui/editors/parameters/RosegardenParameterArea.h"
#include "gui/editors/parameters/SegmentParameterBox.h"
#include "gui/editors/parameters/TrackParameterBox.h"
#include "gui/editors/segment/compositionview/CompositionView.h"
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
#include "gui/editors/tempo/TempoAndTimeSignatureEditor.h"
#include "gui/general/EditViewBase.h"
#include "gui/general/EditTempoController.h"
#include "gui/general/FileSource.h"
#include "gui/general/ResourceFinder.h"
#include "gui/general/AutoSaveFinder.h"
#include "gui/general/LilyPondProcessor.h"
#include "gui/general/ProjectPackager.h"
#include "gui/general/PresetHandlerDialog.h"
#include "gui/widgets/StartupLogo.h"
#include "gui/widgets/TmpStatusMsg.h"
#include "gui/widgets/WarningWidget.h"
#include "gui/seqmanager/MidiFilterDialog.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/studio/AudioMixerWindow2.h"
#include "gui/studio/AudioPlugin.h"
#include "gui/studio/AudioPluginManager.h"
#include "gui/studio/AudioPluginGUIManager.h"
#include "gui/studio/BankEditorDialog.h"
#include "gui/studio/DeviceManagerDialog.h"
#include "gui/studio/MidiMixerWindow.h"
#include "gui/studio/RemapInstrumentDialog.h"
#include "gui/studio/StudioControl.h"
#include "gui/studio/SynthPluginManagerDialog.h"
#include "gui/studio/ControlEditorDialog.h"
#include "gui/widgets/ProgressBar.h"
#include "gui/widgets/FileDialog.h"
#include "LircClient.h"
#include "LircCommander.h"
#include "RosegardenMainViewWidget.h"
#include "SetWaitCursor.h"
#include "sequencer/RosegardenSequencer.h"
#include "sequencer/SequencerThread.h"
#include "sound/AudioFile.h"
#include "sound/AudioFileManager.h"
#ifdef HAVE_LILV
#include "sound/LV2World.h"
#include "sound/LV2Utils.h"
#include "sound/LV2Worker.h"
#include "gui/studio/LV2Gtk.h"
#endif
#include "sound/MappedCommon.h"
#include "sound/MappedEventList.h"
#include "sound/MappedEvent.h"
#include "sound/MappedStudio.h"
#include "sound/MidiFile.h"
#include "sound/PluginIdentifier.h"
#include "sound/SequencerDataBlock.h"
#include "sound/SoundDriver.h"
#include "StartupTester.h"
#include "gui/studio/DeviceManagerDialog.h"
#include "gui/widgets/InputDialog.h"
#include "TranzportClient.h"

#include "rosegarden-version.h"

#include <QApplication>
#include <QDesktopServices>
#include <QSettings>
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
#include <QRegularExpression>
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
#include <QPrintDialog>
#include <QColorDialog>
#include <QFontDialog>
#include <QPageSetupDialog>
#include <QSharedPointer>
#include <QInputDialog>
#include <QThread>
#include <QStandardPaths>

// Ladish lv1 support
#include <cerrno>   // for errno
#include <climits>  // for LONG_MAX
#include <csignal>  // for sigaction()
#include <cstring>  // for strerror()
#include <unistd.h> // for pipe()
#include <QSocketNotifier>


namespace Rosegarden
{


RosegardenMainWindow *RosegardenMainWindow::m_myself = nullptr;

namespace
{
    // Auto-save timer interval in msecs.  See also m_autoSaveInterval.
    constexpr int autoSaveTimerInterval{1000};

    // Like QFileInfo::canonicalPath(), but returns the largest directory
    // path that actually exists.  E.g. if handed /a/b/c/d/e.txt and d does not
    // exist, this returns /a/b/c.
    QString existingDir(const QString &path)
    {
        RG_DEBUG << "existingDir" << path;

        QFileInfo dirInfo(path);
        // For each directory level...
        while (true) {
            RG_DEBUG << "existingDir" << dirInfo;
            if (dirInfo.isDir())
                return dirInfo.canonicalFilePath();
            // Remove a level.
            dirInfo.setFile(dirInfo.dir().path());
        }
    }

    void setFileSaveAsDirectory(const QString &directory)
    {
        QSettings settings;
        settings.beginGroup(LastUsedPathsConfigGroup);
        settings.setValue("save_file", directory);
    }

    QString getFileSaveAsDirectory()
    {
        QSettings settings;
        settings.beginGroup(LastUsedPathsConfigGroup);
        // Get the last Save As location, or DocumentsLocation if there is none.
        return settings.value(
                "save_file",
                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    }
}

RosegardenMainWindow::RosegardenMainWindow(
        bool enableSound, StartupLogo *startupLogo) :
    QMainWindow(nullptr),
    m_useSequencer(enableSound),
    m_autoSaveTimer(new QTimer(this)),
    m_clipboard(Clipboard::mainClipboard()),
    m_updateUITimer(new QTimer(this)),
    m_inputTimer(new QTimer(this)),
    m_cpuMeterTimer(new QTimer(this))
{
#ifdef THREAD_DEBUG
    RG_WARNING << "UI Thread gettid(): " << gettid();
#endif

    initStaticObjects();

    // the AudioPluginGUIManager must be created after initStaticObjects
    m_pluginGUIManager = new AudioPluginGUIManager(this);

    setAttribute(Qt::WA_DeleteOnClose);

    setObjectName("App");
    m_myself = this;

    if (startupLogo) {
        connect(this, &RosegardenMainWindow::startupStatusMessage,
                startupLogo, &StartupLogo::slotShowStatusMessage);
    }

    connect(EditTempoController::self(), &EditTempoController::editTempos,
            this, static_cast<void(RosegardenMainWindow::*)(timeT)>(
                    &RosegardenMainWindow::slotEditTempos));

    // Need to do this prior to launching the sequencer to
    // avoid ActionFileClient warnings in the debug log due
    // to menu actions not existing yet.  This makes sure they
    // exist.
    setupActions();

    if (m_useSequencer) {
        emit startupStatusMessage(tr("Starting sequencer..."));

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! From this point on, we are MULTITHREADED! !!!
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        // Launch the SequencerThread.
        launchSequencer();
    }

    // check for autosaved untitled document

    // note this must be done before the plugin manager is started as
    // the messagebox will hold up the initialization process and
    // plugins which require the sample rate for initialization will
    // not work correctly.
    bool loadAutoSaveFile = false;
    QString autoSaved = AutoSaveFinder().checkAutoSaveFile("");
    if (autoSaved != "") {

        // At this point the splash screen is definitely up, hide it
        // before showing the messagebox.
        StartupLogo::hideIfStillThere();

        // Ask the user if they want to use the auto-save file
        QMessageBox::StandardButton reply = QMessageBox::question(
                this,  // parent
                tr("Rosegarden"),  // title
                tr("An auto-save file for an unsaved document has been found.\n"
                   "Do you want to open it?"),  // text
                QMessageBox::Yes | QMessageBox::No,  // buttons
                QMessageBox::Yes);  // defaultButton

        if (reply == QMessageBox::Yes) {
            loadAutoSaveFile = true;
        } else {
            // user doesn't want the auto-save, so delete it
            // so it won't bother us again if we relaunch
            QString autoSaveFileName =
                AutoSaveFinder().getAutoSavePath("");
            RG_DEBUG << "ctor deleting autosave file" << autoSaveFileName;
            QFile::remove(autoSaveFileName);
        }

    }

    // Plugin manager
    //
    emit startupStatusMessage(tr("Initializing plugin manager..."));
    m_pluginManager.reset(new AudioPluginManager(enableSound));

    QString autoSaveFileName;
    if (loadAutoSaveFile) {
        autoSaveFileName = AutoSaveFinder().getAutoSavePath("");
        RG_DEBUG << "ctor loading autosave file" << autoSaveFileName;
    }
    RosegardenDocument *doc = newDocument(true, autoSaveFileName);  // permanent

    m_seqManager = new SequenceManager();

    m_parameterArea = new RosegardenParameterArea(this);
    m_parameterArea->setObjectName("RosegardenParameterArea");

    // Populate the parameter-box area with the respective
    // parameter box widgets.
    m_segmentParameterBox = new SegmentParameterBox(m_parameterArea);
    m_parameterArea->addRosegardenParameterBox(m_segmentParameterBox);
    m_trackParameterBox = new TrackParameterBox(m_parameterArea);
    m_parameterArea->addRosegardenParameterBox(m_trackParameterBox);
    m_instrumentParameterBox = new InstrumentParameterBox(m_parameterArea);
    m_parameterArea->addRosegardenParameterBox(m_instrumentParameterBox);

    // Lookup the configuration parameter that specifies the default
    // arrangement, and instantiate it.

    // call inits to invoke all other construction parts
    //
    emit startupStatusMessage(tr("Initializing view..."));
    initStatusBar();
    initZoomToolbar();

    // ??? TransportDialog should connect itself to SequenceManager.  Move
    //     all of this into TransportDialog.
    Q_ASSERT(m_transport);
    connect(m_seqManager, &SequenceManager::signalTempoChanged,
            m_transport, &TransportDialog::slotTempoChanged);
    connect(m_seqManager, &SequenceManager::signalMidiInLabel,
            m_transport, &TransportDialog::slotMidiInLabel);
    connect(m_seqManager, &SequenceManager::signalMidiOutLabel,
            m_transport, &TransportDialog::slotMidiOutLabel);
    connect(m_seqManager, &SequenceManager::signalPlaying,
            m_transport, &TransportDialog::slotPlaying);
    connect(m_seqManager, &SequenceManager::signalRecording,
            m_transport, &TransportDialog::slotRecording);
    connect(m_seqManager, &SequenceManager::signalMetronomeActivated,
            m_transport, &TransportDialog::slotMetronomeActivated);

    // Set up external controller.
    ExternalController::self().connectRMW(this);

    // Load the initial document (this includes doc's own autoload)
    //
    setDocument(doc);

    emit startupStatusMessage(tr("Starting sequence manager..."));
    m_seqManager->setDocument(RosegardenDocument::currentDocument);

    connect(m_seqManager,
            &SequenceManager::sendWarning,
            this,
            &RosegardenMainWindow::slotDisplayWarning);

    connect(CommandHistory::getInstance(),
            &CommandHistory::aboutToExecuteCommand,
            this,
            &RosegardenMainWindow::slotAboutToExecuteCommand);

    connect(CommandHistory::getInstance(),
            &CommandHistory::commandUndone,
            this,
            &RosegardenMainWindow::slotCommandUndone);

    connect(CommandHistory::getInstance(),
            &CommandHistory::commandRedone,
            this,
            &RosegardenMainWindow::slotCommandRedone);

    if (m_useSequencer) {
        // Check the sound driver status and warn the user of any
        // problems.  This warning has to happen early, in case it
        // affects the ability to load plugins etc from a file on the
        // command line.
        m_seqManager->checkSoundDriverStatus(true);
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
    m_seqManager->sendPreferences();

    // Now autoload
    //
    enterActionState("new_file"); //@@@ JAS orig. 0
    leaveActionState("have_segments"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    leaveActionState("have_selection"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    leaveActionState("have_clipboard_can_paste_as_links");
    slotTestClipboard();

    // Check for lack of MIDI devices and disable Studio options accordingly
    //
    if (!RosegardenDocument::currentDocument->getStudio().haveMidiDevices())
        leaveActionState("got_midi_devices"); //@@@ JAS orig. KXMLGUIClient::StateReverse

    emit startupStatusMessage(tr("Starting..."));

    readOptions();

    // All toolbars should be created before this is called
    //### implement or find alternative : rgTempQtIV->setAutoSaveSettings(MainWindowConfigGroup, true);

#ifdef HAVE_LIRC

    try {
        m_lircClient = new LircClient();
    } catch (const Exception &e) {
        //RG_DEBUG << e.getMessage().c_str();
        // continue without
        m_lircClient = nullptr;
    }
    if (m_lircClient) {
        m_lircCommander = new LircCommander(m_lircClient, this);
    }
#endif

    // Tranzport
    try
    {
        m_tranzport = new TranzportClient(this);
    }
    catch (const Exception &e)
    {
        m_tranzport = nullptr;
        //RG_DEBUG << e.getMessage().c_str();
    }

    enterActionState("have_project_packager");
    enterActionState("have_lilypondview");

    QTimer::singleShot(1000, this, &RosegardenMainWindow::slotTestStartupTester);

    // Restore window geometry and toolbar state
    //RG_DEBUG << "ctor: Restoring saved main window geometry...";
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    this->restoreGeometry(settings.value("Main_Window_Geometry").toByteArray());
    this->restoreState(settings.value("Main_Window_State").toByteArray());
    settings.endGroup();

    checkAudioPath();

    if (!installSignalHandlers())
        RG_WARNING << "ctor: Signal handlers not installed!";

    // Update UI time interval.
    settings.beginGroup("Performance_Testing");
    int updateUITime = settings.value("Update_UI_Time", 50).toInt();
    // Write it to the file to make it easier to find.
    settings.setValue("Update_UI_Time", updateUITime);
    settings.endGroup();

    // Connect the various timers to their handlers.
    connect(m_updateUITimer, &QTimer::timeout,
            this, &RosegardenMainWindow::slotUpdateUI);
    m_updateUITimer->start(updateUITime);
    connect(m_inputTimer, &QTimer::timeout, this, &RosegardenMainWindow::slotHandleInputs);
    m_inputTimer->start(20);
    connect(m_autoSaveTimer, &QTimer::timeout,
            this, &RosegardenMainWindow::slotAutoSave);
    connect(m_cpuMeterTimer, &QTimer::timeout,
            this, &RosegardenMainWindow::slotUpdateCPUMeter);
    m_cpuMeterTimer->start(1000);

    // Connect Typematic objects.
    connect(&m_rewindTypematic, &Typematic::click,
            this, &RosegardenMainWindow::slotRewind);
    connect(&m_fastForwardTypematic, &Typematic::click,
            this, &RosegardenMainWindow::slotFastforward);
}

RosegardenMainWindow::~RosegardenMainWindow()
{
    RG_DEBUG << "dtor...";

    delete m_tempoAndTimeSignatureEditor;
    m_tempoAndTimeSignatureEditor = nullptr;

    if (getView() &&
        getView()->getTrackEditor() &&
        getView()->getTrackEditor()->getCompositionView()) {
        getView()->getTrackEditor()->getCompositionView()->endAudioPreviewGeneration();
    }

    delete m_pluginGUIManager;
    m_pluginGUIManager = nullptr;

    if (isSequencerRunning()) {
        RosegardenSequencer::getInstance()->quit();
        // ??? Can we do better than this?  Can we wait for the thread to
        //     end inside of quit()?  QThread::wait()?
        usleep(300000);

        delete m_sequencerThread;
        m_sequencerThread = nullptr;
    }

    delete m_transport;
    m_transport = nullptr;

    delete m_seqManager;
    m_seqManager = nullptr;

#ifdef HAVE_LIRC
    delete m_lircCommander;
    delete m_lircClient;
#endif

    delete m_tranzport;
    m_tranzport = nullptr;

    delete RosegardenDocument::currentDocument;
    RosegardenDocument::currentDocument = nullptr;

    Profiles::getInstance()->dump();
}

void RosegardenMainWindow::initStaticObjects()
{
    // This will declare the static variables for the singletons in
    // the correct order. They are destroyed in the reverse order.
    RG_DEBUG << "initStaticObjects";
#ifdef HAVE_LILV
    LV2World::get();
    LV2Utils::getInstance();
    LV2Worker::getInstance();
#ifdef HAVE_GTK2
    LV2Gtk::getInstance();
#endif
#endif
    RosegardenSequencer::getInstance();
}

int RosegardenMainWindow::sigpipe[2];

/* Handler for system signals (SIGUSR1, SIGINT...)
 * Write a message to the pipe and leave as soon as possible
 */
void
RosegardenMainWindow::handleSignal(int sig)
{
    if (write(sigpipe[1], &sig, sizeof(sig)) == -1) {
        RG_WARNING << "handleSignal(): write() failed:" << std::strerror(errno);
    }
}

/* Install signal handlers (may be more than one; called from the
 * constructor of your MainWindow class*/
bool
RosegardenMainWindow::installSignalHandlers()
{
    /*install pipe to forward received system signals*/
    if (pipe(sigpipe) < 0) {
        RG_WARNING << "installSignalHandlers(): pipe() failed:" << std::strerror(errno);
        return false;
    }

    /*install notifier to handle pipe messages*/
    const QSocketNotifier *signalNotifier = new QSocketNotifier(sigpipe[0],
            QSocketNotifier::Read, this);
    connect(signalNotifier, &QSocketNotifier::activated,
            this, &RosegardenMainWindow::signalAction);

    /*install signal handlers*/
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handleSignal;

    if (sigaction(SIGUSR1, &action, nullptr) == -1) {
        RG_WARNING << "installSignalHandlers(): sigaction() failed:" << std::strerror(errno);
        return false;
    }

    return true;
}

/* Slot to give response to the incoming pipe message;
   e.g.: save current file */
void
RosegardenMainWindow::signalAction(int fd)
{
    int message;

    if (read(fd, &message, sizeof(message)) == -1) {
        RG_WARNING << "signalAction(): read() failed:" << std::strerror(errno);
        return;
    }

    switch (message) {
        case SIGUSR1:
            slotFileSave();
            break;
        default:
            RG_WARNING << "signalAction(): Unexpected signal received:" << message;
            break;
    }
}

void
RosegardenMainWindow::closeEvent(QCloseEvent *event)
{
    if (queryClose()) {
        // do some cleaning up
        emit documentAboutToChange();

        // Save window geometry and toolbar state
        //RG_DEBUG << "closeEvent(): Saving main window geometry...";
        QSettings settings;
        settings.beginGroup(WindowGeometryConfigGroup);
        settings.setValue("Main_Window_Geometry", this->saveGeometry());
        settings.setValue("Main_Window_State", this->saveState());
        settings.endGroup();

        settings.beginGroup(GeneralOptionsConfigGroup);
        settings.setValue("show_status_bar", !statusBar()->isHidden());
        settings.setValue("show_stock_toolbar", !findToolbar("Main Toolbar")->isHidden());
        settings.setValue("show_tools_toolbar", !findToolbar("Tools Toolbar")->isHidden());
        settings.setValue("show_tracks_toolbar", !findToolbar("Tracks Toolbar")->isHidden());
        settings.setValue("show_editors_toolbar", !findToolbar("Editors Toolbar")->isHidden());
        settings.setValue("show_transport_toolbar", !findToolbar("Transport Toolbar")->isHidden());
        settings.setValue("show_zoom_toolbar", !findToolbar("Zoom Toolbar")->isHidden());
        settings.setValue("show_transport", findAction("show_transport")->isChecked());

        if (m_transport) {
            settings.setValue("transport_flap_extended", m_transport->isExpanded());
        }

        settings.setValue("show_tracklabels", findAction("show_tracklabels")->isChecked());
        settings.setValue("show_rulers", findAction("show_rulers")->isChecked());
        settings.setValue("show_tempo_ruler", findAction("show_tempo_ruler")->isChecked());
        settings.setValue("show_chord_name_ruler", findAction("show_chord_name_ruler")->isChecked());
        settings.setValue("show_previews", findAction("show_previews")->isChecked());
        settings.setValue("show_segment_labels", findAction("show_segment_labels")->isChecked());
        settings.setValue("show_inst_segment_parameters", findAction("show_inst_segment_parameters")->isChecked());
        settings.endGroup();

        // Continue closing.
        event->accept();
    } else {
        // Do not close.
        event->ignore();
    }
}

void
RosegardenMainWindow::setupActions()
{
    createAction("file_new", &RosegardenMainWindow::slotFileNew);
    createAction("file_open", &RosegardenMainWindow::slotFileOpen);
    createAction("file_open_example", &RosegardenMainWindow::slotFileOpenExample);
    createAction("file_open_template", &RosegardenMainWindow::slotFileOpenTemplate);
    createAction("file_open_most_recent", &RosegardenMainWindow::slotFileOpenRecent);
    createAction("file_save", &RosegardenMainWindow::slotFileSave);
    createAction("file_save_as", &RosegardenMainWindow::slotFileSaveAs);
    createAction("file_save_as_template", &RosegardenMainWindow::slotFileSaveAsTemplate);
    createAction("file_revert", &RosegardenMainWindow::slotRevertToSaved);
    createAction("file_close", &RosegardenMainWindow::slotFileClose);
    createAction("file_quit", &RosegardenMainWindow::slotQuit);

    createAction("edit_cut", &RosegardenMainWindow::slotEditCut);
    createAction("edit_copy", &RosegardenMainWindow::slotEditCopy);
    createAction("edit_paste", &RosegardenMainWindow::slotEditPaste);
    //uncomment this when time comes to implement paste as links
    //createAction("edit_paste_as_links", &RosegardenMainWindow::slotEditPasteAsLinks);

    createAction("shortcuts_configure", &RosegardenMainWindow::slotConfigureShortcuts);
    createAction("options_configure", &RosegardenMainWindow::slotConfigure);

    createAction("file_import_project", &RosegardenMainWindow::slotImportProject);
    createAction("file_import_midi", &RosegardenMainWindow::slotImportMIDI);
    createAction("file_import_rg21", &RosegardenMainWindow::slotImportRG21);
//    createAction("file_import_hydrogen", &RosegardenMainWindow::slotImportHydrogen);
    createAction("file_import_musicxml", &RosegardenMainWindow::slotImportMusicXML);
    createAction("file_merge", &RosegardenMainWindow::slotMerge);
    createAction("file_merge_midi", &RosegardenMainWindow::slotMergeMIDI);
    createAction("file_merge_rg21", &RosegardenMainWindow::slotMergeRG21);
//    createAction("file_merge_hydrogen", &RosegardenMainWindow::slotMergeHydrogen);
    createAction("file_merge_musicxml", &RosegardenMainWindow::slotMergeMusicXML);
    createAction("file_export_project", &RosegardenMainWindow::slotExportProject);
    createAction("file_export_midi", &RosegardenMainWindow::slotExportMIDI);
    createAction("file_export_lilypond", &RosegardenMainWindow::slotExportLilyPond);
    createAction("file_export_musicxml", &RosegardenMainWindow::slotExportMusicXml);
    createAction("file_export_wav", &RosegardenMainWindow::slotExportWAV);
    createAction("file_export_csound", &RosegardenMainWindow::slotExportCsound);
    createAction("file_export_mup", &RosegardenMainWindow::slotExportMup);
    createAction("file_print_lilypond", &RosegardenMainWindow::slotPrintLilyPond);
    createAction("file_preview_lilypond", &RosegardenMainWindow::slotPreviewLilyPond);
    createAction("file_show_playlist", &RosegardenMainWindow::slotPlayList);

    // Help menu
    createAction("manual", &RosegardenMainWindow::slotHelp);
    createAction("tutorial", &RosegardenMainWindow::slotTutorial);
    createAction("guidelines", &RosegardenMainWindow::slotBugGuidelines);
    createAction("help_about_app", &RosegardenMainWindow::slotHelpAbout);
    createAction("help_about_qt", &RosegardenMainWindow::slotHelpAboutQt);
    createAction("donate", &RosegardenMainWindow::slotDonate);

    createAction("show_stock_toolbar", &RosegardenMainWindow::slotToggleToolBar);
    createAction("show_tools_toolbar", &RosegardenMainWindow::slotToggleToolsToolBar);
    createAction("show_tracks_toolbar", &RosegardenMainWindow::slotToggleTracksToolBar);
    createAction("show_editors_toolbar", &RosegardenMainWindow::slotToggleEditorsToolBar);
    createAction("show_transport_toolbar", &RosegardenMainWindow::slotToggleTransportToolBar);
    createAction("show_zoom_toolbar", &RosegardenMainWindow::slotToggleZoomToolBar);
    createAction("show_status_bar", &RosegardenMainWindow::slotToggleStatusBar);
    createAction("show_transport", &RosegardenMainWindow::slotUpdateTransportVisibility);
    createAction("show_tracklabels", &RosegardenMainWindow::slotToggleTrackLabels);
    createAction("show_rulers", &RosegardenMainWindow::slotToggleRulers);
    createAction("show_tempo_ruler", &RosegardenMainWindow::slotToggleTempoRuler);
    createAction("show_chord_name_ruler", &RosegardenMainWindow::slotToggleChordNameRuler);
    createAction("show_previews", &RosegardenMainWindow::slotTogglePreviews);
    createAction("show_inst_segment_parameters", &RosegardenMainWindow::slotHideShowParameterArea);
    createAction("full_screen", &RosegardenMainWindow::slotFullScreen);

    createAction("select", &RosegardenMainWindow::slotPointerSelected);
    createAction("draw", &RosegardenMainWindow::slotDrawSelected);
    createAction("erase", &RosegardenMainWindow::slotEraseSelected);
    createAction("move", &RosegardenMainWindow::slotMoveSelected);
    createAction("resize", &RosegardenMainWindow::slotResizeSelected);
    createAction("split", &RosegardenMainWindow::slotSplitSelected);
    createAction("join", &RosegardenMainWindow::slotJoinSelected);
    createAction("harmonize_selection", &RosegardenMainWindow::slotHarmonizeSelection);
    createAction("add_time_signature", &RosegardenMainWindow::slotEditTimeSignature);
    createAction("edit_tempos", &RosegardenMainWindow::slotEditTempos);
    createAction("cut_range", &RosegardenMainWindow::slotCutRange);
    createAction("copy_range", &RosegardenMainWindow::slotCopyRange);
    createAction("paste_range", &RosegardenMainWindow::slotPasteRange);
    createAction("delete_range", &RosegardenMainWindow::slotDeleteRange);
    createAction("insert_range", &RosegardenMainWindow::slotInsertRange);
    createAction("paste_conductor_data", &RosegardenMainWindow::slotPasteConductorData);
    createAction("erase_range_tempos", &RosegardenMainWindow::slotEraseRangeTempos);
    createAction("delete", &RosegardenMainWindow::slotDeleteSelectedSegments);
    createAction("select_all", &RosegardenMainWindow::slotSelectAll);
    createAction("add_tempo", &RosegardenMainWindow::slotEditTempo);
    createAction("change_composition_length", &RosegardenMainWindow::slotChangeCompositionLength);
    createAction("edit_markers", &RosegardenMainWindow::slotEditMarkers);
    createAction("edit_doc_properties", &RosegardenMainWindow::slotEditDocumentProperties);
    // throw a redundant copy on the View menu; even though it edits too, we
    // just call it "View -> Document Properties"  (I got this idea when I
    // noticed that some piece of configuration in OO.o was on two different
    // menus, when I looked for it in two different places, and found it in
    // both.  It seems reasonable to me if not overdone.)
    createAction("view_doc_properties", &RosegardenMainWindow::slotEditDocumentProperties);
    createAction("edit_default", &RosegardenMainWindow::slotEdit);
    createAction("edit_matrix", &RosegardenMainWindow::slotEditInMatrix);
    createAction("edit_percussion_matrix", &RosegardenMainWindow::slotEditInPercussionMatrix);
    createAction("edit_notation", &RosegardenMainWindow::slotEditAsNotation);
    createAction("edit_event_list", &RosegardenMainWindow::slotEditInEventList);
    createAction("edit_pitch_tracker", &RosegardenMainWindow::slotEditInPitchTracker);
    createAction("quantize_selection", &RosegardenMainWindow::slotQuantizeSelection);
    createAction("relabel_segment", &RosegardenMainWindow::slotRelabelSegments);
    createAction("transpose", &RosegardenMainWindow::slotTransposeSegments);
    createAction("transpose_semitones", &RosegardenMainWindow::slotTransposeSemitones);
    createAction("switch_preset", &RosegardenMainWindow::slotSwitchPreset);
    createAction("interpret", &RosegardenMainWindow::slotInterpret);
    createAction("repeat_quantize", &RosegardenMainWindow::slotRepeatQuantizeSelection);
    createAction("rescale", &RosegardenMainWindow::slotRescaleSelection);
    createAction("auto_split", &RosegardenMainWindow::slotAutoSplitSelection);
    createAction("split_by_pitch", &RosegardenMainWindow::slotSplitSelectionByPitch);
    createAction("split_by_recording", &RosegardenMainWindow::slotSplitSelectionByRecordedSrc);
    createAction("split_at_time", &RosegardenMainWindow::slotSplitSelectionAtTime);
    createAction("split_by_drum", &RosegardenMainWindow::slotSplitSelectionByDrum);
    createAction("jog_left", &RosegardenMainWindow::slotJogLeft);
    createAction("jog_right", &RosegardenMainWindow::slotJogRight);
    createAction("create_anacrusis", &RosegardenMainWindow::slotCreateAnacrusis);
    createAction("set_segment_start", &RosegardenMainWindow::slotSetSegmentStartTimes);
    createAction("set_segment_duration", &RosegardenMainWindow::slotSetSegmentDurations);
    createAction("join_segments", &RosegardenMainWindow::slotJoinSegments);
    createAction("expand_figuration", &RosegardenMainWindow::slotExpandFiguration);
    createAction("update_figurations", &RosegardenMainWindow::slotUpdateFigurations);
    createAction("repeats_to_real_copies", &RosegardenMainWindow::slotRepeatingSegments);
    createAction("links_to_real_copies", &RosegardenMainWindow::slotLinksToCopies);
    createAction("manage_trigger_segments", &RosegardenMainWindow::slotManageTriggerSegments);
    createAction("groove_quantize", &RosegardenMainWindow::slotGrooveQuantize);
    createAction("fit_beats", &RosegardenMainWindow::slotFitToBeats);
    createAction("set_tempo_to_segment_length", &RosegardenMainWindow::slotTempoToSegmentLength);
    createAction("audio_manager", &RosegardenMainWindow::slotAudioManager);
    createAction("show_segment_labels", &RosegardenMainWindow::slotToggleSegmentLabels);
    createAction("add_track", &RosegardenMainWindow::slotAddTrack);
    createAction("add_tracks", &RosegardenMainWindow::slotAddTracks);
    createAction("delete_track", &RosegardenMainWindow::slotDeleteTrack);
    createAction("move_track_down", &RosegardenMainWindow::slotMoveTrackDown);
    createAction("move_track_up", &RosegardenMainWindow::slotMoveTrackUp);
    createAction("select_next_track", &RosegardenMainWindow::slotSelectNextTrack);
    createAction("select_previous_track", &RosegardenMainWindow::slotSelectPreviousTrack);
    createAction("toggle_mute_track", &RosegardenMainWindow::slotToggleMute);
    createAction("toggle_arm_track", &RosegardenMainWindow::slotToggleRecordCurrentTrack);
    createAction("toggle_solo_track", &RosegardenMainWindow::slotToggleSoloCurrentTrack);
    createAction("mute_all_tracks", &RosegardenMainWindow::slotMuteAllTracks);
    createAction("unmute_all_tracks", &RosegardenMainWindow::slotUnmuteAllTracks);
    createAction("remap_instruments", &RosegardenMainWindow::slotRemapInstruments);
    createAction("audio_mixer", &RosegardenMainWindow::slotOpenAudioMixer);
    createAction("midi_mixer", &RosegardenMainWindow::slotOpenMidiMixer);
    createAction("manage_midi_devices", &RosegardenMainWindow::slotManageMIDIDevices);
    createAction("manage_synths", &RosegardenMainWindow::slotManageSynths);
    createAction("modify_midi_filters", &RosegardenMainWindow::slotModifyMIDIFilters);
    createAction("manage_metronome", &RosegardenMainWindow::slotManageMetronome);
    createAction("toggle_metronome", &RosegardenMainWindow::slotToggleMetronome);
    createAction("save_default_studio", &RosegardenMainWindow::slotSaveDefaultStudio);
    createAction("load_default_studio", &RosegardenMainWindow::slotImportDefaultStudio);
    createAction("load_studio", &RosegardenMainWindow::slotImportStudio);
    createAction("reset_midi_network", &RosegardenMainWindow::slotResetMidiNetwork);
    createAction("add_marker", &RosegardenMainWindow::slotAddMarker2);
    createAction("previous_marker", &RosegardenMainWindow::slotPreviousMarker);
    createAction("next_marker", &RosegardenMainWindow::slotNextMarker);
    createAction("set_quick_marker", &RosegardenMainWindow::slotSetQuickMarker);
    createAction("jump_to_quick_marker", &RosegardenMainWindow::slotJumpToQuickMarker);

    createAction("play", &RosegardenMainWindow::slotPlay);
    createAction("stop", &RosegardenMainWindow::slotStop);
    createAction("fast_forward", &RosegardenMainWindow::slotFastforward);
    createAction("rewind", &RosegardenMainWindow::slotRewind);
    createAction("recordtoggle", &RosegardenMainWindow::slotToggleRecord);
    createAction("record", &RosegardenMainWindow::slotRecord);
    createAction("rewindtobeginning", &RosegardenMainWindow::slotRewindToBeginning);
    createAction("fastforwardtoend", &RosegardenMainWindow::slotFastForwardToEnd);
    createAction("loop", &RosegardenMainWindow::slotLoop);
    createAction("scroll_to_follow", &RosegardenMainWindow::slotScrollToFollow);
    createAction("panic", &RosegardenMainWindow::slotPanic);
    createAction("debug_dump_segments", &RosegardenMainWindow::slotDebugDump);

    createAction("repeat_segment_onoff", &RosegardenMainWindow::slotToggleRepeat);

    createMenusAndToolbars("rosegardenmainwindow.rc");

    createAndSetupTransport();

    // Hook up for aboutToShow() so we can set up the menu when it is
    // needed.
    const QMenu *fileOpenRecentMenu = findMenu("file_open_recent");
    connect(fileOpenRecentMenu, &QMenu::aboutToShow,
            this, &RosegardenMainWindow::setupRecentFilesMenu);

    const QMenu *setTrackInstrumentMenu =
            findChild<QMenu *>("set_track_instrument");

    if (setTrackInstrumentMenu) {
        connect(setTrackInstrumentMenu, &QMenu::aboutToShow,
                this, &RosegardenMainWindow::slotPopulateTrackInstrumentPopup);
    } else {
        RG_WARNING << "setupActions() : couldn't find set_track_instrument menu - check rosegardenmainwindow.rc";
    }

    // Set the rewind and fast-forward buttons for auto-repeat.
    enableAutoRepeat("Transport Toolbar", "rewind");
    enableAutoRepeat("Transport Toolbar", "fast_forward");

    // Do an initial setup of the recent files so that Ctrl+R (or
    // other shortcut) will work at startup.
    // get the shortcuts for later use
    QAction* mostRecent = findAction("file_open_most_recent");
    m_mostRecentShortcuts = mostRecent->shortcuts();
    RG_DEBUG << "mostRecentShortcuts:" << m_mostRecentShortcuts;
    setupRecentFilesMenu();
}

void
RosegardenMainWindow::setupRecentFilesMenu()
{
    QMenu *fileOpenRecentMenu = findMenu("file_open_recent");
    if (!fileOpenRecentMenu) {
        RG_WARNING << "setupRecentFilesMenu(): WARNING: No recent files menu!";
        return;
    }

    // Start with a clean slate.
    fileOpenRecentMenu->clear();

    // Remove non-existent files if configured in the .conf.
    // This is problematic as it will cause files to be removed from
    // the recent files list when external storage is unmounted.
    QSettings settings;
    settings.beginGroup(RecentFilesConfigGroup);
    if (settings.value("cleanRecentFilesList", "false").toBool())
        m_recentFiles.removeNonExistent();

    bool first = true;

    // For each recent file, make a new action and add it to the menu.
    for (const QString &name : m_recentFiles.get()) {
        QAction *action = new QAction(name, this);
        action->setObjectName(name);
        connect(action, &QAction::triggered,
                this, &RosegardenMainWindow::slotFileOpenRecent);

        fileOpenRecentMenu->addAction(action);

        if (first) {
            first = false;
            action->setShortcuts(m_mostRecentShortcuts);
        }
    }
}

void
RosegardenMainWindow::initZoomToolbar()
{
    //### Zoom toolbar has already been created. Find it instead.
    // QToolBar *zoomToolbar = this->addToolBar("Zoom Toolbar");
    //
    QToolBar *zoomToolbar = findToolbar("Zoom Toolbar");
    if (!zoomToolbar) {
        RG_DEBUG << "initZoomToolbar() : "
        << "zoom toolbar not found";
        return ;
    }

    QLabel *label = new QLabel(tr("  Zoom:  "));
    zoomToolbar->addWidget(label);

    std::vector<double> zoomSizes; // in units-per-pixel
    double defaultBarWidth44 = 100.0;
    double duration44 = TimeSignature(4, 4).getBarDuration();
    static double factors[] = { 0.025, 0.05, 0.1, 0.2, 0.5,
                                1.0, 1.5, 2.5, 5.0, 10.0 , 20.0 };

    for (size_t i = 0; i < sizeof(factors) / sizeof(factors[0]); ++i) {
        zoomSizes.push_back(duration44 / (defaultBarWidth44 * factors[i]));
    }

    // zoom labels
    QString minZoom = QString("%1%").arg(factors[0] * 100.0);
    //QString maxZoom = QString("%1%").arg(factors[(sizeof(factors) / sizeof(factors[0])) - 1] * 100.0);

    m_zoomSlider = new ZoomSlider<double>
                   (zoomSizes, -1, Qt::Horizontal, zoomToolbar);
    m_zoomSlider->setTracking(true);
    m_zoomSlider->setFocusPolicy(Qt::NoFocus);
    m_zoomLabel = new QLabel(minZoom, zoomToolbar);
    m_zoomLabel->setIndent(10);

    connect(m_zoomSlider, &QAbstractSlider::valueChanged,
            this, &RosegardenMainWindow::slotChangeZoom);

    zoomToolbar->addWidget(m_zoomSlider);
    zoomToolbar->addWidget(m_zoomLabel);

    // set initial zoom - we might want to make this a settings option
    //    m_zoomSlider->setToDefault();

}

void
RosegardenMainWindow::initStatusBar()
{
    m_cpuBar = new ProgressBar(100, statusBar());
    m_cpuBar->setObjectName("Main Window progress bar"); // to help keep ProgressBar objects straight
    m_cpuBar->setFixedWidth(60);
    m_cpuBar->setFixedHeight(18);
    QFont font = m_cpuBar->font();
    font.setPixelSize(10);
    m_cpuBar->setFont(font);

    m_cpuBar->setTextVisible(false);
    statusBar()->addPermanentWidget(m_cpuBar);

    // status warning widget replaces a glob of annoying startup dialogs
    m_warningWidget = new WarningWidget(this);
    statusBar()->addPermanentWidget(m_warningWidget);
    statusBar()->setContentsMargins(0, 0, 0, 0);
}

void
RosegardenMainWindow::initView()
{
    //RG_DEBUG << "initView()...";

    Composition &comp = RosegardenDocument::currentDocument->getComposition();

    // Ensure that the start and end markers for the piece are set
    // to something reasonable
    //
    if (comp.getStartMarker() == 0 && comp.getEndMarker() == 0) {
        int endMarker = comp.getBarRange(100 + comp.getNbBars()).second;
        comp.setEndMarker(endMarker);
    }

    // The plan is to set the new central view via setCentralWidget() in
    // a moment, which schedules the old one for deletion later via
    // QObject::deleteLater().
    RosegardenMainViewWidget *oldView = m_view;

    // We need to make sure the parameter boxes don't send any
    // signals to the old view!
    disconnect(m_segmentParameterBox, nullptr, oldView, nullptr);
    disconnect(m_instrumentParameterBox, nullptr, oldView, nullptr);
    disconnect(m_trackParameterBox, nullptr, oldView, nullptr);

    RosegardenMainViewWidget *swapView = new RosegardenMainViewWidget
        (findAction("show_tracklabels")->isChecked(),
         m_segmentParameterBox,
         m_instrumentParameterBox,
         m_trackParameterBox,
         m_parameterArea,
         this);

    // Connect up this signal so that we can force tool mode
    // changes from the view
    connect(swapView, &RosegardenMainViewWidget::activateTool,
            this, &RosegardenMainWindow::slotActivateTool);

    connect(swapView,
            &RosegardenMainViewWidget::segmentsSelected,
            this, &RosegardenMainWindow::segmentsSelected);

    connect(swapView,
            &RosegardenMainViewWidget::addAudioFile,
            this, &RosegardenMainWindow::slotAddAudioFile);

    connect(swapView, &RosegardenMainViewWidget::toggleSolo,
            this, &RosegardenMainWindow::slotToggleSolo);

    RosegardenDocument::currentDocument->attachView(swapView);


    // Transport setup
    getTransport()->init();

    // Update the tempo in the SequenceManager which will in turn update
    // the tempo in the transport.
    m_seqManager->setTempo(comp.getCurrentTempo());


    // set the pointer position
    //
    slotSetPointerPosition(RosegardenDocument::currentDocument->getComposition().getPosition());

    // !!! The call to setCentralWidget() below will delete oldView.
    m_view = swapView;

    connect(m_view, &RosegardenMainViewWidget::stateChange,
            this, &RosegardenMainWindow::slotStateChanged);

    // We only check for the SequenceManager to make sure
    // we're not on the first pass though - we don't want
    // to send these toggles twice on initialisation.
    //
    // Clunky but we just about get away with it for the
    // moment.
    //
    if (m_seqManager) {
        slotToggleChordNameRuler();
        slotToggleRulers();
        slotToggleTempoRuler();
        slotTogglePreviews();
        slotToggleSegmentLabels();
    }

    //    delete m_playList;
    //    m_playList = 0;

    delete m_synthManager;
    m_synthManager = nullptr;

    // ??? Instead, AMW2 could connect for RMW::documentAboutToChange() which
    //     it should probably connect for anyway.  Then it could close in
    //     response.  That would remove these lines from RMW.  That's how the
    //     old AMW did it.  Or even better, we might not close at all.  Just
    //     handle the situation and stay up.
    if (m_audioMixerWindow2)
        m_audioMixerWindow2->close();

    delete m_bankEditor;
    m_bankEditor = nullptr;

    delete m_markerEditor;
    m_markerEditor = nullptr;

    delete m_tempoAndTimeSignatureEditor;
    m_tempoAndTimeSignatureEditor = nullptr;

    delete m_triggerSegmentManager;
    m_triggerSegmentManager = nullptr;

    // !!! This also deletes oldView (via QObject::deleteLater()).
    setCentralWidget(m_view);

    // set the highlighted track
    comp.notifyTrackSelectionChanged(comp.getSelectedTrack());
    m_view->slotSelectTrackSegments(comp.getSelectedTrack());

    // play tracking in the track editor is stored in the composition
    QAction *trackingAction = findAction("scroll_to_follow");
    if (trackingAction) {
        trackingAction->setChecked(comp.getMainFollowPlayback());
    }

    m_view->show();

    connect(m_view->getTrackEditor()->getCompositionView(),
            &CompositionView::showContextHelp,
            this,
            &RosegardenMainWindow::slotShowToolHelp);

    // We have to do this to make sure that the 2nd call ("select")
    // actually has any effect. Activating the same radio action
    // doesn't work the 2nd time (like pressing down the same radio
    // button twice - it doesn't have any effect), so if you load two
    // files in a row, on the 2nd file a new CompositionView will be
    // created but its tool won't be set, even though it will appear
    // to be selected.
    //
    QAction *moveAction = findAction(QString("move"));
    moveAction->trigger();

    if (RosegardenDocument::currentDocument->getComposition().getNbSegments() > 0){
        QAction *selectAction = findAction(QString("select"));
        selectAction->trigger();
    } else {
        QAction *drawAction = findAction(QString("draw"));
        drawAction->trigger();
    }

    int zoomLevel = RosegardenDocument::currentDocument->getConfiguration().get<Int>(DocumentConfiguration::ZoomLevel);
    m_zoomSlider->setSize(double(zoomLevel) / 1000.0);
    slotChangeZoom(zoomLevel);

    enterActionState("new_file"); //@@@ JAS orig. 0

    if (findAction("show_chord_name_ruler")->isChecked()) {
        SetWaitCursor swc;
        m_view->initChordNameRuler();
    } else {
        m_view->initChordNameRuler();
    }
}

void
RosegardenMainWindow::setDocument(RosegardenDocument *newDocument)
{
    if (RosegardenDocument::currentDocument == newDocument) return;

    // Save the modified status so that we can put it back after we
    // are done here.
    // ??? Probably worth investigating why this is necessary, and
    //     thinking through an alternate approach.  Perhaps this
    //     check/restore could be moved into the culprit at least.
    bool modified = newDocument->isModified();

    emit documentAboutToChange();
    qApp->processEvents(); // to make sure all opened dialogs (mixer, midi devices...) are closed

    // Take care of all subparts which depend on the document

    RosegardenDocument *oldDoc = RosegardenDocument::currentDocument;

    RosegardenDocument::currentDocument = newDocument;

    updateTitle();

    if (m_seqManager) // when we're called at startup, the seq. man. isn't created yet
        m_seqManager->setDocument(RosegardenDocument::currentDocument);

    if (m_markerEditor)
        m_markerEditor->setDocument(RosegardenDocument::currentDocument);

    delete m_tempoAndTimeSignatureEditor;
    m_tempoAndTimeSignatureEditor = nullptr;

    if (m_triggerSegmentManager)
        m_triggerSegmentManager->setDocument(RosegardenDocument::currentDocument);

    m_trackParameterBox->setDocument(RosegardenDocument::currentDocument);
    EditTempoController::self()->setDocument(RosegardenDocument::currentDocument);

    if (m_pluginGUIManager) {
        m_pluginGUIManager->stopAllGUIs();
        m_pluginGUIManager->setStudio(&RosegardenDocument::currentDocument->getStudio());
    }

    if (getView() &&
        getView()->getTrackEditor() &&
        getView()->getTrackEditor()->getCompositionView()) {
        getView()->getTrackEditor()->getCompositionView()->endAudioPreviewGeneration();
    }

    // connect needed signals

    connect(RosegardenDocument::currentDocument, &RosegardenDocument::pointerPositionChanged,
            this, &RosegardenMainWindow::slotSetPointerPosition);

    connect(RosegardenDocument::currentDocument, &RosegardenDocument::documentModified,
            this, &RosegardenMainWindow::slotDocumentModified);

    // connecting this independently of slotDocumentModified in the hope that it
    // will better reflect the true state of things
    connect(RosegardenDocument::currentDocument, &RosegardenDocument::documentModified,
            this, &RosegardenMainWindow::slotUpdateTitle);

    connect(RosegardenDocument::currentDocument,
                &RosegardenDocument::loopChanged,
            this, &RosegardenMainWindow::slotLoopChanged);

    connect(CommandHistory::getInstance(), &CommandHistory::commandExecuted,
            this, &RosegardenMainWindow::slotCommandExecuted);

    // use QueuedConnection here because the position update can
    // happen after the command is executed
    connect(CommandHistory::getInstance(),
                &CommandHistory::commandExecutedInitially,
            this, &RosegardenMainWindow::slotUpdatePosition,
            Qt::QueuedConnection);

    // start the autosave timer
    m_lastAutoSaveTime = QTime::currentTime();
    m_autoSaveInterval =
        RosegardenDocument::currentDocument->getAutoSavePeriod() * 1000;
    m_autoSaveTimer->start(autoSaveTimerInterval);

    connect(RosegardenDocument::currentDocument, &RosegardenDocument::devicesResyncd,
            this, &RosegardenMainWindow::slotDocumentDevicesResyncd);

    if (m_useSequencer) {
        // Connect the devices prior to calling initView() to make sure there
        // is connection information for the MIPP to display.
        RosegardenSequencer::getInstance()->connectSomething();

        newDocument->getStudio().resyncDeviceConnections();

        // Send out the channel setups.
        // Since the device connections might have changed due to the
        // connectSomething() call above, we need to send out the channel
        // setups at this point.  Otherwise if we load a Composition with
        // empty connections (like the example Vivaldi op.44), we get piano
        // on every track.
        newDocument->initialiseStudio();
    }

    // Create a new RosegardenMainViewWidget and set it as the central
    // widget.  The old RMVW instance will be scheduled for deletion later.
    initView();

    // This will delete all edit views.
    delete oldDoc;
    oldDoc = nullptr;

    // Make sure the view and the new document match.
    m_view->slotSynchroniseWithComposition();

    if (newDocument->getStudio().haveMidiDevices()) {
        enterActionState("got_midi_devices"); //@@@ JAS orig. 0
    } else {
        leaveActionState("got_midi_devices"); //@@@ JAS orig KXMLGUIClient::StateReverse
    }

    // Ensure the sequencer knows about any audio files
    // we've loaded as part of the new Composition
    //
    RosegardenDocument::currentDocument->prepareAudio();

    // Remove the audio segments from the clipboard as they point to
    // bogus file IDs now.
    m_clipboard->removeAudioSegments();

    // Do not reset instrument prog. changes after all.
    //     if (m_seqManager)
    //         m_seqManager->preparePlayback(true);

    emit documentLoaded(RosegardenDocument::currentDocument);

    // Make sure everyone is in sync with the loaded loop.
    // Note: This must be done *after* documentLoaded() is emitted.
    //       Otherwise TransportDialog will not be able to sync.
    if (m_seqManager)
        emit RosegardenDocument::currentDocument->loopChanged();

    // Restore the document's original modified status.
    if (modified)
        RosegardenDocument::currentDocument->slotDocumentModified();
    else
        RosegardenDocument::currentDocument->clearModifiedStatus();

    // Readjust canvas size
    //
    m_view->getTrackEditor()->updateCanvasSize();

    // Show notes about the composition in a pop up dialog if asked for
    new CommentsPopupDialog(RosegardenDocument::currentDocument, this);
}

void
RosegardenMainWindow::openFile(const QString& filePath, ImportType type)
{
    //RG_DEBUG << "openFile(): " << filePath;

    // If we're opening a .rgp file, delegate to importProject()
    if (type == ImportCheckType  &&  filePath.endsWith(".rgp")) {
        importProject(filePath);
        return;
    }

    // Are we just reloading the original file?  In that case, we'll need
    // to avoid locking/releasing the file.
    bool revert = false;

    if (RosegardenDocument::currentDocument) {
        QFileInfo newFileInfo(filePath);
        revert = (newFileInfo.absoluteFilePath() == RosegardenDocument::currentDocument->getAbsFilePath());
    }

    RosegardenDocument *doc = createDocument(
            filePath,
            type,  // importType
            true,  // permanent
            revert,  // revert
            true);  // clearHistory

    if (!doc)
        return;

    if (revert) {
        doc->stealLockFile(RosegardenDocument::currentDocument);
    }
    setDocument(doc);

    // fix #782, "SPB combo not updating after document swap"
    //RG_DEBUG << "openFile(): calling slotDocColoursChanged() in doc";
    doc->slotDocColoursChanged();


    // Preferences: Always use default studio when loading files

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    bool alwaysUseDefaultStudio =
            qStrToBool(settings.value("alwaysusedefaultstudio", "false"));
    settings.endGroup();

    if (alwaysUseDefaultStudio) {

        QString autoloadFile = ResourceFinder().getAutoloadPath();
        QFileInfo autoloadFileInfo(autoloadFile);

        if (autoloadFile != ""  &&  autoloadFileInfo.isReadable()) {

            //RG_DEBUG << "openFile(): Importing default studio from " << autoloadFile;

            slotImportStudioFromFile(autoloadFile);
        }
    }


    // Add to the MRU list
    QFileInfo fileInfo(filePath);
    m_recentFiles.add(fileInfo.absoluteFilePath());
    // Make sure Ctrl+R is correct.
    setupRecentFilesMenu();

    // As an empty composition can be saved, we need to look if
    // segments exist before enabling print options in menu
    // ??? This should be done in response to a Composition change
    //     notification.
    if (doc->getComposition().getSegments().size())
        enterActionState("have_segments");
    else
        leaveActionState("have_segments");
}

RosegardenDocument *
RosegardenMainWindow::createDocument(
        QString filePath, ImportType importType, bool permanent,
        bool revert, bool clearHistory)
{
    // ??? This and the create functions it calls might make more sense in
    //     RosegardenDocument.

    QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()) {
        // can happen with command-line arg, so...
        StartupLogo::hideIfStillThere();
        QMessageBox::warning(this, tr("Rosegarden"),
                tr("File \"%1\" does not exist").arg(filePath),
                QMessageBox::Ok, QMessageBox::Ok);
        return nullptr;
    }

    if (fileInfo.isDir()) {
        StartupLogo::hideIfStillThere();
        QMessageBox::warning(this, tr("Rosegarden"),
                tr("File \"%1\" is actually a directory").arg(filePath),
                QMessageBox::Ok, QMessageBox::Ok);
        return nullptr;
    }

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        StartupLogo::hideIfStillThere();
        QMessageBox::warning(this, tr("Rosegarden"),
                tr("You do not have read permission for \"%1\"").arg(filePath),
                QMessageBox::Ok, QMessageBox::Ok);
        return nullptr;
    }

    // If we need to import based on the filename extension
    if (importType == ImportCheckType) {
        QString extension = fileInfo.suffix().toLower();

        if (extension == "mid"  ||  extension == "midi")
            importType = ImportMIDI;
        else if (extension == "rg"  ||  extension == "rgt")
            importType = ImportRG4;
        else if (extension == "rgd")
            importType = ImportRGD;
        else if (extension == "rose")
            importType = ImportRG21;
        else if (extension == "xml")
            importType = ImportMusicXML;
        //else if (extension == "h2song")
        //    importType = ImportHydrogen;
    }

    if (importType == ImportRGD) {
        StartupLogo::hideIfStillThere();
        QMessageBox::warning(this, tr("Rosegarden"),
                tr("File \"%1\" is a Rosegarden Device, and must be imported using the MIDI device manager.").arg(filePath),
                QMessageBox::Ok, QMessageBox::Ok);
        return nullptr;
    }

    // If the sequencer is playing, stop it.
    if (m_seqManager  &&  m_seqManager->getTransportStatus() == PLAYING)
        slotStop();

    // Prevent playback while loading.
    // ??? This only disables the buttons on the TransportDialog.  It
    //     does not disable the toolbar buttons or the menu items.
    //     Luckily, we do not crash if we are playing and a document
    //     is loading.
    slotEnableTransport(false);

    RosegardenDocument *doc = nullptr;

    switch (importType) {
    case ImportMIDI:
        doc = createDocumentFromMIDIFile(filePath, permanent);
        break;

    case ImportRG21:
        doc = createDocumentFromRG21File(filePath);
        break;

    //case ImportHydrogen:
    //    doc = createDocumentFromHydrogenFile(filePath);
    //    break;

    case ImportMusicXML:
        doc = createDocumentFromMusicXMLFile(filePath, permanent);
        break;

    case ImportRG4:
    case ImportCheckType:
    default:
        doc = createDocumentFromRGFile(
                filePath,
                permanent,
                revert,
                clearHistory);
        break;

    case ImportRGD:  // Satisfy compiler warning.
        // Handled above.
        break;
    }

    slotEnableTransport(true);

    return doc;
}

RosegardenDocument *
RosegardenMainWindow::createDocumentFromRGFile(
        const QString &filePath, bool permanent, bool revert, bool clearHistory)
{
    // ??? This and its caller should probably be moved into
    //     RosegardenDocument as static factory functions.

    // The file we are going to open in the end.  This could be either
    // the requested file or the auto-saved version of that file.
    QString openFilePath = filePath;

    // Check for an auto-save file to recover
    QString autoSaveFileName = AutoSaveFinder().checkAutoSaveFile(filePath);

    bool recovering = (autoSaveFileName != ""  &&  !revert);

    if (recovering) {
        QFileInfo fileInfo(filePath);
        QFileInfo autoSaveFileInfo(autoSaveFileName);

        // If the auto-save file is more recent
        if (autoSaveFileInfo.lastModified() > fileInfo.lastModified()) {
            // For help with debugging Bug #1725.  Remove once that is closed.
            RG_WARNING << "createDocumentFromRGFile(): Performing recovery...";
            RG_WARNING << "       File date/time: " << fileInfo.lastModified();
            RG_WARNING << "  Auto-save date/time: " << autoSaveFileInfo.lastModified();

            // At this point the splash screen may still be there, hide it
            // before showing the messagebox.
            StartupLogo::hideIfStillThere();

            // Ask the user if they want to use the auto-save file
            QMessageBox::StandardButton reply = QMessageBox::question(
                    this, tr("Rosegarden"),
                    tr("An auto-save file for this document has been found\nDo you want to open it instead ?"),
                    QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                // open the auto-save file instead
                openFilePath = autoSaveFileName;
            } else {
                // user doesn't want the auto-save, so delete it
                // so it won't bother us again if we reload
                QFile::remove(autoSaveFileName);
                recovering = false;
            }
        } else {  // Auto-save file is older.  Ignore it.
            recovering = false;
        }
    }

    // Create a new blank document
    RosegardenDocument *newDoc = new RosegardenDocument(
            this,  // parent
            m_pluginManager,  // audioPluginManager
            true,  // skipAutoload
            clearHistory,  // clearCommandHistory
            m_useSequencer);  // enableSound

    // Read the document from the file.
    bool readOk = newDoc->openDocument(
            openFilePath,  // filename
            permanent,  // permanent
            false,  // squelchProcessDialog
            !revert);  // enableLock

    // If the read failed, bail.
    if (!readOk) {
        delete newDoc;
        return nullptr;
    }

    if (recovering) {
        newDoc->slotDocumentModified();

        // Replace the auto-save filepath with the filepath of
        // the original file.
        QFileInfo fileInfo(filePath);
        newDoc->setAbsFilePath(fileInfo.absoluteFilePath());
        newDoc->setTitle(fileInfo.fileName());
    }

    return newDoc;
}

void
RosegardenMainWindow::readOptions()
{
    QSettings settings;

    // Statusbar and toolbars toggling action status
    //
    bool opt;

    settings.beginGroup(GeneralOptionsConfigGroup);

    opt = qStrToBool(settings.value("show_status_bar", "true"));
    findAction("show_status_bar")->setChecked(opt);
    slotToggleStatusBar();

    opt = qStrToBool(settings.value("show_stock_toolbar", "true"));
    findAction("show_stock_toolbar")->setChecked(opt);
    slotToggleToolBar();

    opt = qStrToBool(settings.value("show_tools_toolbar", "true"));
    findAction("show_tools_toolbar")->setChecked(opt);
    slotToggleToolsToolBar();

    opt = qStrToBool(settings.value("show_tracks_toolbar", "true"));
    findAction("show_tracks_toolbar")->setChecked(opt);
    slotToggleTracksToolBar();

    opt = qStrToBool(settings.value("show_editors_toolbar", "true"));
    findAction("show_editors_toolbar")->setChecked(opt);
    slotToggleEditorsToolBar();

    opt = qStrToBool(settings.value("show_transport_toolbar", "true"));
    findAction("show_transport_toolbar")->setChecked(opt);
    slotToggleTransportToolBar();

    opt = qStrToBool(settings.value("show_zoom_toolbar", "true"));
    findAction("show_zoom_toolbar")->setChecked(opt);
    slotToggleZoomToolBar();

    opt = qStrToBool(settings.value("show_transport", "true")) ;
    findAction("show_transport")->setChecked(opt);
    slotUpdateTransportVisibility();

    opt = qStrToBool(settings.value("transport_flap_extended", "true")) ;

#ifdef SETTING_LOG_DEBUG
    RG_DEBUG << "SETTING 3 : transport flap extended = " << opt;
#endif

    if (opt)
        getTransport()->slotPanelOpenButtonClicked();
    else
        getTransport()->slotPanelCloseButtonClicked();

    opt = qStrToBool(settings.value("show_tracklabels", "true")) ;

#ifdef SETTING_LOG_DEBUG
    RG_DEBUG << "SETTING 3 : show track labels = " << opt;
#endif

    findAction("show_tracklabels")->setChecked(opt);
    slotToggleTrackLabels();

    opt = qStrToBool(settings.value("show_rulers", "true")) ;
    findAction("show_rulers")->setChecked(opt);
    slotToggleRulers();

    opt = qStrToBool(settings.value("show_tempo_ruler", "true")) ;
    findAction("show_tempo_ruler")->setChecked(opt);
    slotToggleTempoRuler();

    opt = qStrToBool(settings.value("show_chord_name_ruler", "false")) ;
    findAction("show_chord_name_ruler")->setChecked(opt);
    slotToggleChordNameRuler();

    opt = qStrToBool(settings.value("show_previews", "true")) ;
    findAction("show_previews")->setChecked(opt);
    slotTogglePreviews();

    opt = qStrToBool(settings.value("show_segment_labels", "true")) ;
    findAction("show_segment_labels")->setChecked(opt);
    slotToggleSegmentLabels();

    opt = qStrToBool(settings.value("show_inst_segment_parameters", true));
    findAction("show_inst_segment_parameters")->setChecked(opt);
    slotHideShowParameterArea();

    settings.endGroup();

    m_actionsSetup = true;
}

/* unused
void
RosegardenMainWindow::saveGlobalProperties()
{
    QSettings settings;
    //@@@ JAS Do we need a settings.startGroup() here?

    if (RosegardenDocument::currentDocument->getTitle() != tr("Untitled") && !RosegardenDocument::currentDocument->isModified()) {
        // saving to tempfile not necessary
    } else {
        QString filename = RosegardenDocument::currentDocument->getAbsFilePath();
        settings.setValue("filename", filename);
        settings.setValue("modified", RosegardenDocument::currentDocument->isModified());

        QString tempname = AutoSaveFinder().getAutoSavePath(filename);
        if (tempname != "") {
            QString errMsg;
            bool res = RosegardenDocument::currentDocument->saveDocument(tempname, errMsg);
            if (!res) {
                if (!errMsg.isEmpty()) {
                    QMessageBox::critical(this, tr("Rosegarden"), tr("Could not save document at %1\nError was : %2").arg(tempname).arg(errMsg));
                } else {
                    QMessageBox::critical(this, tr("Rosegarden"), tr("Could not save document at %1").arg(tempname));
                }
            }
        }
    }
}
*/

#if 0
void
RosegardenMainWindow::readGlobalProperties()
{
    QSettings settings;
    //@@@ JAS Do we need a settings.startGroup() here?

    QString filename = settings.value("filename", "").toString();
    bool modified = qStrToBool(settings.value("modified", "false")) ;

    if (modified) {

        QString tempname = AutoSaveFinder().checkAutoSaveFile(filename);

        if (tempname != "") {
            slotEnableTransport(false);
            RosegardenDocument::currentDocument->openDocument(tempname);
            slotEnableTransport(true);
            RosegardenDocument::currentDocument->slotDocumentModified();
            QFileInfo info(filename);
            RosegardenDocument::currentDocument->setAbsFilePath(info.absoluteFilePath());
            RosegardenDocument::currentDocument->setTitle(info.fileName());
        }
    } else {
        if (!filename.isEmpty()) {
            slotEnableTransport(false);
            RosegardenDocument::currentDocument->openDocument(filename);
            slotEnableTransport(true);
        }
    }

    updateTitle();
}
#endif

void
RosegardenMainWindow::showEvent(QShowEvent *e)
{
    //RG_DEBUG << "showEvent()";

    getTransport()->raise();

    QMainWindow::showEvent(e);
}

bool
RosegardenMainWindow::queryClose()
{
    // If we are recording, don't let the user close.
    if (m_seqManager->getTransportStatus() == RECORDING)
        return false;

    // Let the user save any unsaved changes.
    return saveIfModified();
}

void
RosegardenMainWindow::slotFileNew()
{
// RG_DEBUG << "slotFileNew()\n";

    TmpStatusMsg msg(tr("Creating new document..."), this);

    bool makeNew = false;

    if (!RosegardenDocument::currentDocument->isModified()) {
        makeNew = true;
        // RosegardenDocument::currentDocument->closeDocument();
    } else if (saveIfModified()) {
        makeNew = true;
    }

    if (makeNew) {
        // do some cleaning up
        emit documentAboutToChange();
        setDocument(newDocument(
                true));  // permanent
        leaveActionState("have_segments");
    }
}

void
RosegardenMainWindow::slotUpdateTitle(bool modified)
{
    // NB: I seems like using doc->isModified() would be a more accurate state
    // test than the value of "modified", but in practice there is a lag factor
    // of a few ms where we have gotten one value in "modified" here, and
    // isModified() is in the opposite state briefly.  I don't think there's
    // any real concern there, so I just switched everything over to use the
    // state of the bool passed with the signal and ignore isModified()
    //RG_DEBUG << "slotUpdateTitle(" << modified << ")";

    // Preferences: Show full path in window titles.
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    bool showFullPath =
            settings.value("long_window_titles", false).toBool();
    settings.endGroup();

    QString filename;

    if (showFullPath) {
        // If it's not "Untitled", use the full path.
        if (RosegardenDocument::currentDocument->getAbsFilePath() != "")
            filename = RosegardenDocument::currentDocument->getAbsFilePath();
        else  // Otherwise use the title.  Which is probably "Untitled".
            filename = RosegardenDocument::currentDocument->getTitle();
    } else {
        filename = RosegardenDocument::currentDocument->getTitle();
    }

    setWindowTitle(
            tr("%1%2 - %3").
                arg((modified ? "*" : "")).
                arg(filename).
                arg(qApp->applicationName()));
}

void
RosegardenMainWindow::updateTitle()
{
    slotUpdateTitle(RosegardenDocument::currentDocument->isModified());
}

void
RosegardenMainWindow::slotOpenDroppedURL(QString url)
{
    qApp->processEvents(QEventLoop::AllEvents, 100);

    // ??? This is redundant.  openURL() does this again.
    if (!saveIfModified())
        return;

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle(tr("Rosegarden"));
    msgBox.setText(tr("Replace or Merge?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    QAbstractButton *buttonY = msgBox.button(QMessageBox::Yes);
    buttonY->setText(tr("Replace"));
    QAbstractButton *buttonN = msgBox.button(QMessageBox::No);
    buttonN->setText(tr("Merge"));
    const int reply = msgBox.exec();
    const bool replace = (reply == QMessageBox::Yes);

    openURL(QUrl(url), replace);
}

void
RosegardenMainWindow::openURL(const QString& url)
{
    //RG_DEBUG << "openURL(): url =" << url;

    openURL(QUrl(url), true);  // replace
}

bool
RosegardenMainWindow::openURL(const QUrl &url, bool replace)
{
    SetWaitCursor waitCursor;

    //RG_DEBUG << "openURL(): url =" << url;

    if (!url.isValid()) {
        QMessageBox::warning(this, tr("Rosegarden"),
                tr("Malformed URL\n%1").arg(url.toString()));

        return false;
    }

    FileSource source(url);

    if (!source.isAvailable()) {
        QMessageBox::critical(this, tr("Rosegarden"),
                tr("Cannot open file %1").arg(url.toString()));
        return false;
    }

    //RG_DEBUG << "openURL(): local filename =" << source.getLocalFilename();

    // Let the user save the current document if it's been modified.
    // ??? rename: safeToClobber()?  Might be too geared toward the result.
    if (!saveIfModified())
        return false;

    // In case the source is remote, wait for the file to be downloaded to
    // the local file.
    source.waitForData();

    // Needed for mergeFile which expects a list for multiple merge
    const QStringList fileList = { source.getLocalFilename() };

    if (replace)
        openFile(source.getLocalFilename());
    else
        // this only merges the first file for now
        mergeFile(fileList, ImportCheckType);

    return true;
}

void
RosegardenMainWindow::openFileDialogAt(const QString &target)
{
    slotStatusHelpMsg(tr("Opening file..."));

    QSettings settings;
    QString directory;

    // if target is empty, this is a generic file open dialog, otherwise
    // we open the specified target (examples, templates)
    if (target.isEmpty()) {
        // Get the last used path for File > Open.
        settings.beginGroup(LastUsedPathsConfigGroup);
        directory = settings.value("open_file", QDir::homePath()).toString();
        directory = existingDir(directory);
        settings.endGroup();
    } else {
        directory = target;
    }

    // Launch the Open File dialog.
    QString fname = FileDialog::getOpenFileName(this, tr("Open File"), directory,
                    tr("All supported files") + " (*.rg *.RG *.rgt *.RGT *.rgp *.RGP *.mid *.MID *.midi *.MIDI)" + ";;" +
                    tr("Rosegarden files") + " (*.rg *.RG *.rgp *.RGP *.rgt *.RGT)" + ";;" +
                    tr("MIDI files") + " (*.mid *.MID *.midi *.MIDI)" + ";;" +
                    tr("All files") + " (*)", nullptr);

    // If the user has cancelled, bail.
    if (fname.isEmpty())
        return;

    // If a document is currently loaded
    if (RosegardenDocument::currentDocument) {
        // Check to see if the user needs/wants to save the current document.
        // ??? openURL() does this too.  Should we defer to it?
        bool okToOpen = saveIfModified();

        // If the user has cancelled, bail.
        if (!okToOpen)
            return;
    }

    // Continue opening the file.
    const bool success = openURL(QUrl::fromLocalFile(fname), true);  // replace

    // Only update the .conf directory if we were successful.
    if (target.isEmpty()  &&  success) {
        // Update the last used path for File > Open.
        directory = existingDir(fname);
        settings.beginGroup(LastUsedPathsConfigGroup);
        settings.setValue("open_file", directory);
        settings.endGroup();
    }
}

void
RosegardenMainWindow::slotFileOpen()
{
    openFileDialogAt("");
}

QString
RosegardenMainWindow::getDataLocation()
{
#if QT_VERSION >= 0x050000
    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
#else
    QString dataLocation = QDesktopServices::storageLocation(QDesktopServices::HomeLocation) + "/.local/share/";
#endif
    RG_DEBUG << "getDataLocation(): returning: " << dataLocation;
    return dataLocation;
}

void
RosegardenMainWindow::slotFileOpenExample()
{
    QString target = getDataLocation() + "/rosegarden/examples";
    openFileDialogAt(target);
}

void
RosegardenMainWindow::slotFileOpenTemplate()
{
    QString target = getDataLocation() + "/rosegarden/templates";
    openFileDialogAt(target);
}

void
RosegardenMainWindow::slotMerge()
{
    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    QString directory = settings.value("merge_file", QDir::homePath()).toString();
    directory = existingDir(directory);

    const QStringList fileList = FileDialog::getOpenFileNames(
            this, tr("Select File(s)"), directory,
               tr("Rosegarden files") + " (*.rg *.RG)" + ";;" +
               tr("All files") + " (*)", nullptr);
    if (fileList.isEmpty())
        return;

    mergeFile(fileList, ImportCheckType);

    directory = existingDir(fileList[0]);
    settings.setValue("merge_file", directory);
    settings.endGroup();
}

void
RosegardenMainWindow::slotFileOpenRecent()
{
    QObject *obj = sender();
    QAction *action = dynamic_cast<QAction *>(obj);

    if (!action) {
        RG_WARNING << "slotFileOpenRecent(): WARNING: sender is not an action";
        return;
    }

    QString pathOrUrl = action->objectName();
    if (pathOrUrl.isEmpty()) return;

    TmpStatusMsg msg(tr("Opening file..."), this);

    if (RosegardenDocument::currentDocument) {
        if (!saveIfModified()) {
            return ;
        }
    }

    openURL(QUrl::fromUserInput(pathOrUrl), true);  // replace
}

void
RosegardenMainWindow::slotFileSave()
{
    // No document to save?  Bail.
    if (!RosegardenDocument::currentDocument)
        return;

    TmpStatusMsg msg(tr("Saving file..."), this);

    // If it's a new file (no file path), or an imported file
    // (file path doesn't end with .rg), do a "save as".
    if (!RosegardenDocument::currentDocument->isRegularDotRGFile()) {
        fileSaveAs(false);
        return;
    }

    const QString& docFilePath =
            RosegardenDocument::currentDocument->getAbsFilePath();

    QString errMsg;
    bool success;

    {
        SetWaitCursor setWaitCursor;

        // An attempt to address Bug #1725.  Stop the auto-save timer to make
        // sure no auto-saves sneak in while we are saving.
        m_autoSaveTimer->stop();

        success = RosegardenDocument::currentDocument->saveDocument(
                docFilePath, errMsg);

        // Restart the auto-save timer at the current time so that the next
        // auto-save happens as far into the future as possible.
        m_lastAutoSaveTime = QTime::currentTime();
        m_autoSaveTimer->start(autoSaveTimerInterval);
    }

    if (!success) {
        if (!errMsg.isEmpty())
            QMessageBox::critical(this, tr("Rosegarden"), tr("Could not save document at %1\nError was : %2")
                                  .arg(docFilePath).arg(errMsg));
        else
            QMessageBox::critical(this, tr("Rosegarden"), tr("Could not save document at %1")
                                  .arg(docFilePath));
    }

    // If save was successful, update the Save As directory.
    if (success)
        setFileSaveAsDirectory(existingDir(docFilePath));

    // Let the audio file manager know we've just saved so it can prompt the
    // user for an audio file location if it needs one.
    RosegardenDocument::currentDocument->getAudioFileManager().save();
}

QString
RosegardenMainWindow::launchSaveAsDialog(QString filter,
                                         QString label)
{

    const QFileInfo originalFileInfo(
            RosegardenDocument::currentDocument->getAbsFilePath());

    // Extract first extension listed in filter, for instance,
    // ".rg" from "Rosegarden files (*.rg)", or ".mid" from
    // "MIDI Files (*.mid *.midi)"
    const int left = filter.indexOf("*.");
    const int right = filter.indexOf(
            QRegularExpression("[ ]"),left);
    const QString filterExtension =
            filter.mid(left + 1, right - left - 1);

    QString directory;

    // Most applications (e.g. OpenOffice.org and the GIMP) use the document's
    // directory for Save As... and Export rather than the last directory
    // the user saved to.  This code implements that approach along with
    // remembering the previous save directory for unnamed files.

    // If the file is unnamed...
    if (RosegardenDocument::currentDocument->getAbsFilePath().isEmpty()) {
        directory = existingDir(getFileSaveAsDirectory());
    } else {  // File has a name.  Go with its directory.
        directory = originalFileInfo.absolutePath();
    }

    // Launch the Save As dialog.
    QString name = FileDialog::getSaveFileName(
            this,  // parent
            label,  // caption
            directory,  // dir
            originalFileInfo.baseName(),  // defaultName
            filter,  // filter
            nullptr,  // selectedFilter
            FileDialog::DontConfirmOverwrite);  // options

    if (name.isEmpty())
        return name;

    // If we have a filter extension...
    if (!filterExtension.isEmpty()) {
        static QRegularExpression extensionRegEx("\\..{1,4}$");
        // If the file name has no extension, add the filter extension.
        if (!extensionRegEx.match(name).hasMatch())
            name += filterExtension;
    }

    const QFileInfo info(name);

    // Confirm overwrite.
    if (info.exists()) {
        int overwrite = QMessageBox::question(
                this,
                tr("Rosegarden"),
                tr("The specified file exists.  Overwrite?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

        if (overwrite != QMessageBox::Yes)
            return "";
    }

    return name;
}

bool
RosegardenMainWindow::fileSaveAs(bool asTemplate)
{
    if (!RosegardenDocument::currentDocument)
        return false;

    // Display a message on the status bar.
    TmpStatusMsg msg(tr("Saving file%1with a new filename...",
                        "'file%1with' is correct. %1 will either become ' ' or ' as a template ' at runtime").
                        arg(asTemplate ? tr(" as a template ") : " "),  // msg
                     this);  // window

    const QString fileType(asTemplate ? tr("Rosegarden templates") : tr("Rosegarden files"));
    const QString fileExtension(asTemplate ? " (*.rgt *.RGT)" : " (*.rg *.RG)");
    const QString dialogMessage(asTemplate ? tr("Save as template...") : tr("Save as..."));

    const QString newName = launchSaveAsDialog(
            fileType + fileExtension + ";;" + tr("All files") + " (*)",
            dialogMessage);
    if (newName.isEmpty())
        return false;

    SetWaitCursor waitCursor;

    QString errMsg;

    // An attempt to address Bug #1725.  Stop the auto-save timer to
    // make sure no auto-saves sneak in while we are saving.
    m_autoSaveTimer->stop();

    const bool success =
            RosegardenDocument::currentDocument->saveAs(newName, errMsg);

    // Restart the auto-save timer at the current time so that the next
    // auto-save happens as far into the future as possible.
    m_lastAutoSaveTime = QTime::currentTime();
    m_autoSaveTimer->start(autoSaveTimerInterval);

    // save template as read-only, even though this is largely pointless
    if (asTemplate) {
        QFileInfo saveAsInfo(newName);
        QFile chmod(saveAsInfo.absoluteFilePath());
        chmod.setPermissions(QFile::ReadOwner |
                             QFile::ReadUser  | /* for potential platform-independence */
                             QFile::ReadGroup |
                             QFile::ReadOther);
    }

    if (!success) {
        if (!errMsg.isEmpty())
            QMessageBox::critical(this, tr("Rosegarden"), tr("Could not save document at %1\nError was : %2")
                                  .arg(newName).arg(errMsg));
        else
            QMessageBox::critical(this, tr("Rosegarden"), tr("Could not save document at %1")
                                  .arg(newName));

        // Indicate failure.
        return false;
    }

    setFileSaveAsDirectory(existingDir(newName));

    if (!asTemplate) {
        // Let the audio file manager know we've just saved so it can prompt the
        // user for an audio file location if it needs one.
        RosegardenDocument::currentDocument->getAudioFileManager().save();
    }

    m_recentFiles.add(newName);
    // Make sure Ctrl+R is correct.
    setupRecentFilesMenu();

    updateTitle();

    // Remove any autosave file for Untitled
    QString autoSaveFileName =
        AutoSaveFinder().checkAutoSaveFile("");
    RG_DEBUG << "fileSaveAs deleting autosave file" << autoSaveFileName;
    if (autoSaveFileName != "") QFile::remove(autoSaveFileName);

    // Indicate success.
    return true;
}

void
RosegardenMainWindow::slotFileClose()
{
    RG_DEBUG << "slotFileClose()";

    if (!RosegardenDocument::currentDocument)
        return ;

    TmpStatusMsg msg(tr("Closing file..."), this);

    if (saveIfModified()) {
        setDocument(newDocument(
                true));  // permanent
    }

    // Don't close the whole view (i.e. Quit), just close the doc.
    //    close();
}

void
RosegardenMainWindow::slotQuit()
{
    slotStatusMsg(tr("Exiting..."));

    Profiles::getInstance()->dump();

    close(); // this calls closeEvent
}

void
RosegardenMainWindow::slotEditCut()
{
    if (!m_view->haveSelection())
        return ;
    TmpStatusMsg msg(tr("Cutting selection..."), this);

    SegmentSelection selection(m_view->getSelection());
    CommandHistory::getInstance()->addCommand(
            new CutCommand(selection, m_clipboard));
}

void
RosegardenMainWindow::slotEditCopy()
{
    if (!m_view->haveSelection())
        return ;
    TmpStatusMsg msg(tr("Copying selection to clipboard..."), this);

    SegmentSelection selection(m_view->getSelection());
    CommandHistory::getInstance()->addCommand(
            new CopyCommand(selection, m_clipboard));
}

void
RosegardenMainWindow::slotEditPaste()
{
    if (m_clipboard->isEmpty()) {
        TmpStatusMsg msg(tr("Clipboard is empty"), this);
        return ;
    }
    TmpStatusMsg msg(tr("Inserting clipboard contents..."), this);

    // for now, but we could paste at the time of the first copied
    // segment and then do ghosting drag or something
    timeT insertionTime = RosegardenDocument::currentDocument->getComposition().getPosition();
    CommandHistory::getInstance()->addCommand
    (new PasteSegmentsCommand(&RosegardenDocument::currentDocument->getComposition(),
                              m_clipboard, insertionTime,
                              RosegardenDocument::currentDocument->getComposition().getSelectedTrack(),
                              false));

    // User preference? Update song pointer position on paste
    RosegardenDocument::currentDocument->slotSetPointerPosition(RosegardenDocument::currentDocument->getComposition().getPosition());
}

/* unused
void
RosegardenMainWindow::slotEditPasteAsLinks()
{
    //this contains a copy of slotEditPaste() - as and when the time comes to
    //implement this properly, the code below needs uncommenting and adapting

    */ /*
    if (m_clipboard->isEmpty()) {
        TmpStatusMsg msg(tr("Clipboard is empty"), this);
        return ;
    }
    TmpStatusMsg msg(tr("Pasting clipboard contents as linked segments..."), this);

    // for now, but we could paste at the time of the first copied
    // segment and then do ghosting drag or something
    timeT insertionTime = RosegardenDocument::currentDocument->getComposition().getPosition();
    CommandHistory::getInstance()->addCommand
    (new PasteSegmentsAsLinksCommand(&RosegardenDocument::currentDocument->getComposition(),
                                     m_clipboard, insertionTime,
                                     RosegardenDocument::currentDocument->getComposition().getSelectedTrack(),
                                     false));
    // User preference? Update song pointer position on paste
    RosegardenDocument::currentDocument->slotSetPointerPosition(RosegardenDocument::currentDocument->getComposition().getPosition());
    */ /*
}
*/

void
RosegardenMainWindow::slotCutRange()
{
    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();

    const timeT loopStart = composition.getLoopStart();
    const timeT loopStop = composition.getLoopEnd();

    if (loopStart == loopStop)
        return;

    CommandHistory::getInstance()->addCommand(new CutRangeCommand(
            &composition, loopStart, loopStop, m_clipboard));
}

void
RosegardenMainWindow::slotCopyRange()
{
    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();

    const timeT loopStart = composition.getLoopStart();
    const timeT loopEnd = composition.getLoopEnd();

    if (loopStart == loopEnd)
        return;

    CommandHistory::getInstance()->addCommand(new CopyCommand(
            &composition, loopStart, loopEnd, m_clipboard));
}

void
RosegardenMainWindow::slotPasteRange()
{
    if (m_clipboard->isEmpty())
        return ;

    CommandHistory::getInstance()->addCommand
    (new PasteRangeCommand(&RosegardenDocument::currentDocument->getComposition(), m_clipboard,
                           RosegardenDocument::currentDocument->getComposition().getPosition()));
}

void
RosegardenMainWindow::slotDeleteRange()
{
    // ??? Dead Code.  There is no reference to the delete_range action in
    //     the .rc file.

    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();

    const timeT loopStart = composition.getLoopStart();
    const timeT loopEnd = composition.getLoopEnd();

    if (loopStart == loopEnd)
        return;

    CommandHistory::getInstance()->addCommand(new DeleteRangeCommand(
            &composition, loopStart, loopEnd));
}

void
RosegardenMainWindow::slotInsertRange()
{
    timeT t0 = RosegardenDocument::currentDocument->getComposition().getPosition();
    std::pair<timeT, timeT> r = RosegardenDocument::currentDocument->
            getComposition().getBarRangeForTime(t0);

    TimeDialog dialog(m_view,  // parent
                      tr("Duration of empty range to insert"),  // title
                      t0,  // startTime
                      r.second - r.first,  // defaultDuration
                      1,  // minimumDuration
                      false);  // constrainToCompositionDuration

    if (dialog.exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand
            (new InsertRangeCommand(&RosegardenDocument::currentDocument->getComposition(), t0, dialog.getTime()));
    }
}

void
RosegardenMainWindow::slotPasteConductorData()
{
    if (m_clipboard->isEmpty())
        return ;

    CommandHistory::getInstance()->addCommand
    (new PasteConductorDataCommand(&RosegardenDocument::currentDocument->getComposition(), m_clipboard,
                                   RosegardenDocument::currentDocument->getComposition().getPosition()));
}

void
RosegardenMainWindow::slotEraseRangeTempos()
{
    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();

    const timeT loopStart = composition.getLoopStart();
    const timeT loopEnd = composition.getLoopEnd();

    if (loopStart == loopEnd)
        return;

    CommandHistory::getInstance()->addCommand(new EraseTempiInRangeCommand(
            &composition, loopStart, loopEnd));
}

void
RosegardenMainWindow::slotSelectAll()
{
    m_view->slotSelectAllSegments();
}

void
RosegardenMainWindow::slotDeleteSelectedSegments()
{
    m_view->getTrackEditor()->deleteSelectedSegments();
}

void
RosegardenMainWindow::slotQuantizeSelection()
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

void
RosegardenMainWindow::slotRepeatQuantizeSelection()
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
                             "Quantize Dialog Grid", // no tr (config group name)
                             EventQuantizeCommand::QUANTIZE_NORMAL));
    }

    m_view->slotAddCommandToHistory(command);
}

void
RosegardenMainWindow::slotGrooveQuantize()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();

    if (selection.size() != 1) {
        QMessageBox::warning(this, tr("Rosegarden"), tr("This function needs no more than one segment to be selected."));
        return ;
    }

    Segment *s = *selection.begin();
    m_view->slotAddCommandToHistory(new CreateTempoMapFromSegmentCommand(s));
}

void
RosegardenMainWindow::slotFitToBeats()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();

    if (selection.size() != 1) {
        QMessageBox::warning(this, tr("Rosegarden"), tr("This function needs no more than one segment to be selected."));
        return ;
    }

    Segment *s = *selection.begin();
    m_view->slotAddCommandToHistory(new FitToBeatsCommand(s));
}

void
RosegardenMainWindow::slotJoinSegments()
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
            QMessageBox::warning(this, tr("Rosegarden"), tr("Can't join Audio segments"));
            return ;
        }
    }

    m_view->slotAddCommandToHistory(new SegmentJoinCommand(selection));
    m_view->updateSelectedSegments();
}

void
RosegardenMainWindow::slotExpandFiguration()
{
    if (!m_view->haveSelection())
        { return; }

    //!!! this should all be in rosegardenguiview
    //!!! should it?

    SegmentSelection selection = m_view->getSelection();
    if (selection.size() < 2)
        { return; }

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        if ((*i)->getType() != Segment::Internal) {
            QMessageBox::warning(this, tr("Rosegarden"),
                                 tr("Can't expand Audio segments with figuration"));
            return ;
        }
    }

    m_view->slotAddCommandToHistory(new ExpandFigurationCommand(selection));
    m_view->updateSelectedSegments();
}

void
RosegardenMainWindow::slotUpdateFigurations()
{
    m_view->slotAddCommandToHistory(new UpdateFigurationCommand());
}

void
RosegardenMainWindow::slotRescaleSelection()
{
    // ??? This routine might make more sense in RosegardenMainViewWidget.

    // Nothing selected?  Bail.
    if (!m_view->haveSelection())
        return;

    const SegmentSelection selection = m_view->getSelection();

    // Find the time range for the selection.

    timeT startTime = LONG_MAX;
    timeT endTime = 0;
    bool haveAudioSegment = false;

    // For each segment
    for (SegmentSelection::const_iterator i = selection.begin();
         i != selection.end(); ++i) {
        const Segment *segment = (*i);

        if (segment->getStartTime() < startTime)
            startTime = segment->getStartTime();

        if (segment->getEndMarkerTime() > endTime)
            endTime = segment->getEndMarkerTime();

        if (segment->getType() == Segment::Audio)
            haveAudioSegment = true;
    }

    // If there's an audio segment, make sure the audio path is ok.
    if (haveAudioSegment)
        testAudioPath(tr("rescaling an audio file"));

    RescaleDialog dialog(
            m_view,  // parent
            startTime,  // startTime
            endTime - startTime,  // originalDuration
            Note(Note::Shortest).getDuration(),  // minimumDuration
            false,  // showCloseGapOption
            false);  // constrainToCompositionDuration

    if (dialog.exec() != QDialog::Accepted)
        return;

    // Just the audio rescale commands for various housekeeping.
    std::vector<AudioSegmentRescaleCommand *> audioRescaleCommands;

    int multiplier = dialog.getNewDuration();
    int divisor = endTime - startTime;
    double ratio = static_cast<double>(multiplier) / divisor;

    RG_DEBUG << "slotRescaleSelection(): multiplier = " << multiplier << ", divisor = " << divisor << ", ratio = " << ratio;

    // All of the rescale commands, both MIDI and Audio are added to this
    // macro command.
    MacroCommand *command = new MacroCommand(
            SegmentRescaleCommand::getGlobalName());

    // For each selected segment
    for (SegmentSelection::iterator i = selection.begin();
         i != selection.end(); ++i) {
        Segment *segment = *i;

        if (segment->getType() == Segment::Audio) {
            AudioSegmentRescaleCommand *asrc =
                    new AudioSegmentRescaleCommand(RosegardenDocument::currentDocument, segment, ratio);
            command->addCommand(asrc);

            audioRescaleCommands.push_back(asrc);
        } else {
            command->addCommand(
                    new SegmentRescaleCommand(segment, multiplier, divisor));
        }
    }

    // Progress Dialog
    // Note: The label text and range will be set later as needed.
    // ??? We should make the label text the more generic
    //     "Rescaling segment...".  Then this is OK for audio or MIDI
    //     segments.
    QProgressDialog progressDialog(
            tr("Rescaling audio file..."),  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 0,  // min, max
            this);  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // Don't want to auto close since this is a multi-step
    // process.  Any of the steps may set progress to 100.  We
    // will close anyway when this object goes out of scope.
    progressDialog.setAutoClose(false);
    // Just force the progress dialog up.
    // Both Qt4 and Qt5 have bugs related to delayed showing of progress
    // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
    // based distros might lock up.  See Bug #1546.
    progressDialog.show();

    // For each AudioSegmentRescaleCommand, pass on the progress dialog.
    for (size_t i = 0; i < audioRescaleCommands.size(); ++i) {
        audioRescaleCommands[i]->setProgressDialog(&progressDialog);
    }

    m_view->slotAddCommandToHistory(command);

    if (progressDialog.wasCanceled())
        return;

    if (!audioRescaleCommands.empty()) {
        RosegardenDocument::currentDocument->getAudioFileManager().setProgressDialog(&progressDialog);

        // For each AudioSegmentRescaleCommand
        for (size_t i = 0; i < audioRescaleCommands.size(); ++i) {
            int fileId = audioRescaleCommands[i]->getNewAudioFileId();
            if (fileId < 0)
                continue;

            // Add to the sequencer
            slotAddAudioFile(fileId);

            // Generate a preview
            RosegardenDocument::currentDocument->getAudioFileManager().generatePreview(fileId);

            if (progressDialog.wasCanceled())
                return;
        }
    }
}

bool
RosegardenMainWindow::testAudioPath(const QString &operation)
{
    try {
        RosegardenDocument::currentDocument->getAudioFileManager().testAudioPath();
    } catch (const AudioFileManager::BadAudioPathException &) {
        // Changing the parent to nullptr fixes a nasty style problem cheaply.
        // ??? What style problem?
        if (QMessageBox::warning(
                nullptr,  // parent
                tr("Warning"),  // title
                tr("The audio file path does not exist or is not writable.\nYou must set the audio file path to a valid directory in Document Properties before %1.\nWould you like to set it now?",
                        operation.toStdString().c_str()),  // text
                QMessageBox::Yes | QMessageBox::Cancel,  // buttons
                QMessageBox::Cancel) ==  // defaultButton
                        QMessageBox::Yes) {
            openAudioPathSettings();
        }
        return false;
    }
    return true;
}

void
RosegardenMainWindow::slotAutoSplitSelection()
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
            AudioSplitDialog aSD(this, (*i), RosegardenDocument::currentDocument);

            if (aSD.exec() == QDialog::Accepted) {
                // split to threshold
                //
                command->addCommand(
                    new AudioSegmentAutoSplitCommand(RosegardenDocument::currentDocument,
                                                     *i,
                                                     aSD.getThreshold()));
            }
        } else {
            command->addCommand(new SegmentAutoSplitCommand(*i));
        }
    }

    m_view->slotAddCommandToHistory(command);
}

void
RosegardenMainWindow::slotJogLeft()
{
    RG_DEBUG << "slotJogLeft";
    jogSelection(-Note(Note::Demisemiquaver).getDuration());
}

void
RosegardenMainWindow::slotJogRight()
{
    RG_DEBUG << "slotJogRight";
    jogSelection(Note(Note::Demisemiquaver).getDuration());
}

void
RosegardenMainWindow::jogSelection(timeT amount)
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();

    SegmentReconfigureCommand *command =
            new SegmentReconfigureCommand(tr("Jog Selection"),
                                          &RosegardenDocument::currentDocument->getComposition());

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {

        command->addSegment((*i),
                            (*i)->getStartTime() + amount,
                            (*i)->getEndMarkerTime(false) + amount,
                            (*i)->getTrack());
    }

    m_view->slotAddCommandToHistory(command);
}

void
RosegardenMainWindow::createAndSetupTransport()
{
    m_transport = new TransportDialog(this);

    // Transport button callbacks.

    connect(m_transport->PlayButton(),
            &QAbstractButton::clicked,
            this,
            &RosegardenMainWindow::slotPlay);

    connect(m_transport->StopButton(),
            &QAbstractButton::clicked,
            this,
            &RosegardenMainWindow::slotStop);

    connect(m_transport->FfwdButton(),
            &QAbstractButton::clicked,
            this, &RosegardenMainWindow::slotFastforward);

    connect(m_transport->RewindButton(),
            &QAbstractButton::clicked,
            this,
            &RosegardenMainWindow::slotRewind);

    connect(m_transport->RecordButton(),
            &QAbstractButton::clicked,
            this,
            &RosegardenMainWindow::slotRecord);

    connect(m_transport->RewindEndButton(),
            &QAbstractButton::clicked,
            this,
            &RosegardenMainWindow::slotRewindToBeginning);

    connect(m_transport->FfwdEndButton(),
            &QAbstractButton::clicked,
            this,
            &RosegardenMainWindow::slotFastForwardToEnd);

    connect(m_transport->MetronomeButton(),
            &QAbstractButton::clicked,
            this,
            &RosegardenMainWindow::slotToggleMetronome);

    connect(m_transport->SoloButton(),
            &QAbstractButton::clicked,
            this,
            &RosegardenMainWindow::slotToggleSolo);

    connect(m_transport->TimeDisplayButton(),
            &QAbstractButton::clicked,
            this,
            &RosegardenMainWindow::slotRefreshTimeDisplay);

    connect(m_transport->ToEndButton(),
            &QAbstractButton::clicked,
            this, &RosegardenMainWindow::slotRefreshTimeDisplay);

    // Ensure that the checkbox is unchecked if the dialog
    // is closed
    connect(m_transport, &TransportDialog::closed,
            this, &RosegardenMainWindow::slotCloseTransport);

    connect(m_transport, &TransportDialog::panic, this, &RosegardenMainWindow::slotPanic);

    connect(m_transport, &TransportDialog::editTimeSignature,
            this, static_cast<void(RosegardenMainWindow::*)(QWidget *)>(
                    &RosegardenMainWindow::slotEditTimeSignature));

    connect(m_transport, &TransportDialog::editTransportTime,
            this, static_cast<void(RosegardenMainWindow::*)(QWidget *)>(
                    &RosegardenMainWindow::slotEditTransportTime));
}

void
RosegardenMainWindow::slotSplitSelectionByPitch()
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
              (SegmentSplitByPitchCommand::SplitStrategy)
              dialog.getStrategy(),
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

    SplitByRecordingSrcDialog dialog(m_view, RosegardenDocument::currentDocument);
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

    timeT now = RosegardenDocument::currentDocument->getComposition().getPosition();

    QString title = tr("Split %n Segment(s) at Time", "",
                         selection.size());

    TimeDialog dialog(
            m_view,  // parent
            title,
            now,  // defaultTime
            true);  // constrainToCompositionDuration

    MacroCommand *command = new MacroCommand(title);

    if (dialog.exec() == QDialog::Accepted) {
        int segmentCount = 0;
        for (SegmentSelection::iterator i = selection.begin();
                i != selection.end(); ++i) {

            if ((*i)->getType() == Segment::Audio) {
                AudioSegmentSplitCommand *subCommand =
                    new AudioSegmentSplitCommand(*i, dialog.getTime());
                if (subCommand->isValid()) {
                    command->addCommand(subCommand);
                    ++segmentCount;
                }
            } else {
                SegmentSplitCommand *subCommand =
                    new SegmentSplitCommand(*i, dialog.getTime());
                if (subCommand->isValid()) {
                    command->addCommand(subCommand);
                    ++segmentCount;
                }
            }
        }

        if (segmentCount) {
            // Change the command's name to indicate how many segments were
            // actually split.
            title = tr("Split %n Segment(s) at Time", "", segmentCount);
            command->setName(title);

            m_view->slotAddCommandToHistory(command);
        } else {
            QMessageBox::information(this, tr("Rosegarden"),
                tr("Split time is not within a selected segment.\n"
                   "No segment will be split."));
        }
    }
}

void
RosegardenMainWindow::slotSplitSelectionByDrum()
{
    if (!m_view->haveSelection()) return;

    SegmentSelection selection = m_view->getSelection();
    if (selection.empty()) return;

//    timeT now = RosegardenDocument::currentDocument->getComposition().getPosition();

    QString title = tr("Split %n Segment(s) by Drum", "", selection.size());

//    TimeDialog dialog(m_view, title,
//                      &RosegardenDocument::currentDocument->getComposition(),
//                      now, true);

    MacroCommand *command = new MacroCommand(title);

    //TODO there may be an options dialog where you set up where to write what
    // pitches...  also considering adding a new field to the percussion key map
    // to show indicated pitch for each trigger pitch, eg. several hi-hat
    // variants all have G on top of staff pitch...
    //
    // also this magic thingie could hack the events while it's at it, to set up
    // special percussion-related properties that do not as yet actually exist,
    // like whether to draw with an X head, and also stuff that will be needed
    // for LilyPond export to handle things correctly, like this is what in
    // LilyPond and this is what else in LilyPond, built into the events as new
    // properties or something
    //
    // haven't gotten that far in my planning yet...  figure it's better to get
    // something started so I end up shamed into seeing it through, because I
    // can chat with myself in design documents for eternity without
    // accomplishing anything
    //
    //
//    if (dialog.exec() == QDialog::Accepted) {
        int segmentCount = 0;
        for (SegmentSelection::iterator i = selection.begin();
                i != selection.end(); ++i) {

            if ((*i)->getType() == Segment::Audio) {
                return;

                // message box to inform user that this only works on MIDI
                // segments?  if we do, we should only show it one time
            } else {


                // get percussion key map, if available, or a fat 0 otherwise
                Composition &comp = RosegardenDocument::currentDocument->getComposition();
                Track *track = comp.getTrackById((*i)->getTrack());
                Instrument *inst = RosegardenDocument::currentDocument->getStudio().getInstrumentById(track->getInstrument());
                const MidiKeyMapping *keyMap = inst->getKeyMapping();

                SegmentSplitByDrumCommand *subCommand = new SegmentSplitByDrumCommand(*i, keyMap);
                command->addCommand(subCommand);
                ++segmentCount;
            }
        }

        if (segmentCount) {
            // Change the command's name to indicate how many segments were
            // actually split.
            title = tr("Split %n Segment(s) by Drum", "", segmentCount);
            command->setName(title);

            m_view->slotAddCommandToHistory(command);
        } else {
            QMessageBox::information(this, tr("Rosegarden"),
                tr("No segment was split."));
        }
//    }
}

void
RosegardenMainWindow::slotCreateAnacrusis()
{
    // ??? Replace all this with a Composition > Shift Left (Anacrusis)
    //     command that acts on all Segments and allows shifts of arbitrary
    //     amounts.  It should handle expanding Composition start in a
    //     sensible manner.  E.g. do it when the Composition is empty
    //     or when the Segments need it.

    // No selection?  Bail.
    if (!m_view->haveSelection())
        return;

    SegmentSelection selection = m_view->getSelection();
    if (selection.empty())
        return;

    Composition &comp = RosegardenDocument::currentDocument->getComposition();

    bool haveBeginningSegment = false;
    const timeT origCompStart = comp.getStartMarker();
    const timeT compositionEnd = comp.getEndMarker();

    // Make sure at least one Segment is at the Composition start.
    for (Segment *segment : selection) {
        if (segment->getStartTime() == origCompStart)
            haveBeginningSegment = true;
    }

    if (!haveBeginningSegment) {
        QMessageBox::information(
                this,
                tr("Rosegarden"),
                tr("<qt><p>In order to create anacrusis, at least one of the segments in your selection must start at the beginning of the composition.</p></qt>"));
        return;
    }

    const timeT defaultDuration = Note(Note::QuarterNote).getDuration();
    TimeDialog dialog(
            m_view,  // parent
            tr("Anacrusis Amount"),  // title
            origCompStart - defaultDuration,  // startTime
            defaultDuration,  // defaultDuration
            Note(Note::SixtyFourthNote).getDuration(),  // minimumDuration
            false);  // constrainToCompositionDuration

    if (dialog.exec() != QDialog::Accepted)
        return;

    const timeT anacrusisAmount = dialog.getTime();
    const timeT barOneDuration = comp.getBarEnd(1) - comp.getBarStart(1);

    if (anacrusisAmount >= barOneDuration)
        return;

    MacroCommand *macro = new MacroCommand(tr("Create Anacrusis"));

    // Move the composition start back one bar.

    // New composition start is one bar prior.
    const timeT newCompStart = origCompStart - barOneDuration;

    ChangeCompositionLengthCommand *changeLengthCommand =
            new ChangeCompositionLengthCommand(
                    &comp,  // composition
                    newCompStart,  // startTime
                    compositionEnd,  // endTime
                    comp.autoExpandEnabled());  // autoExpand

    macro->addCommand(changeLengthCommand);

    // Move the Segments back by the anacrusis duration.

    // ??? Do we really need this for a command within a macro?
    const bool plural = (selection.size() > 1);
    const QString name = plural ?
            tr("Set Segment Start Times") :
            tr("Set Segment Start Time");

    SegmentReconfigureCommand *reconfigureCommand =
            new SegmentReconfigureCommand(name, &comp);

    // For each Segment in the selection, add to the command.
    for (Segment *segment : selection) {
        const timeT newStartTime = segment->getStartTime() - anacrusisAmount;
        const timeT newEndMarkerTime = segment->getEndMarkerTime(false) -
                segment->getStartTime() + newStartTime;
        reconfigureCommand->addSegment(
                segment,
                newStartTime,
                newEndMarkerTime,
                segment->getTrack());  // newTrack
    }

    macro->addCommand(reconfigureCommand);

    // Move initial tempo to bar 0 (new composition start).

    macro->addCommand(new AddTempoChangeCommand(
            &comp,
            newCompStart,  // time
            comp.getTempoAtTime(origCompStart)));  // tempo
    macro->addCommand(new RemoveTempoChangeCommand(&comp, 1));

    // ??? We need to move the remaining tempos back anacrusisAmount.
    //
    //     Maybe a new MoveTemposAndTimeSignaturesCommand?  Modifying
    //     Event times is perilous, though.  Might want to go with
    //     a delete/add approach to avoid modifying immutable Events.

    // Move initial time signature to bar 0 (new composition start).

    macro->addCommand(new AddTimeSignatureCommand(
            &comp,
            newCompStart,  // time
            comp.getTimeSignatureAt(origCompStart)));  // timeSig
    macro->addCommand(new RemoveTimeSignatureCommand(&comp, 1));

    // ??? We need to move the remaining time signatures back
    //     anacrusisAmount.

    CommandHistory::getInstance()->addCommand(macro);
}

void
RosegardenMainWindow::slotSetSegmentStartTimes()
{
    if (!m_view->haveSelection()) return ;

    SegmentSelection selection = m_view->getSelection();
    if (selection.empty()) return ;

    timeT someTime = (*selection.begin())->getStartTime();

    TimeDialog dialog(
            m_view,  // parent
            tr("Segment Start Time"),  // title
            someTime,  // defaultTime
            false);  // constrainToCompositionDuration

    if (dialog.exec() == QDialog::Accepted) {

        bool plural = (selection.size() > 1);

        SegmentReconfigureCommand *command =
            new SegmentReconfigureCommand(plural ?
                                              tr("Set Segment Start Times") :
                                              tr("Set Segment Start Time"),
                                          &RosegardenDocument::currentDocument->getComposition());

        for (SegmentSelection::iterator i = selection.begin();
                i != selection.end(); ++i) {

            command->addSegment
            (*i, dialog.getTime(),
             (*i)->getEndMarkerTime(false) - (*i)->getStartTime() + dialog.getTime(),
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

    TimeDialog dialog(m_view,
                      tr("Segment Duration"),
                      someTime,
                      someDuration,
                      Note(Note::Shortest).getDuration(),
                      false);

    if (dialog.exec() == QDialog::Accepted) {

        bool plural = (selection.size() > 1);

        SegmentReconfigureCommand *command =
            new SegmentReconfigureCommand(plural ?
                                              tr("Set Segment Durations") :
                                              tr("Set Segment Duration"),
                                          &RosegardenDocument::currentDocument->getComposition());

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

void
RosegardenMainWindow::slotToggleRepeat()
{
    if (m_segmentParameterBox)
        m_segmentParameterBox->toggleRepeat();
}

void
RosegardenMainWindow::slotHarmonizeSelection()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();
    //!!! This should be somewhere else too

    CompositionTimeSliceAdapter adapter(&RosegardenDocument::currentDocument->getComposition(),
                                        &selection);

    AnalysisHelper helper;
    Segment *segment = new Segment;
    helper.guessHarmonies(adapter, *segment);

    //!!! do nothing with the results yet
    delete segment;
}

void
RosegardenMainWindow::slotTempoToSegmentLength()
{
    slotTempoToSegmentLength(this);
}

void
RosegardenMainWindow::slotTempoToSegmentLength(QWidget *parent)
{
    RG_DEBUG << "slotTempoToSegmentLength";

    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();

    // Only set for a single selection
    //
    if (selection.size() == 1 &&
            (*selection.begin())->getType() == Segment::Audio) {
        Composition &comp = RosegardenDocument::currentDocument->getComposition();
        Segment *seg = *selection.begin();

        TimeSignature timeSig =
            comp.getTimeSignatureAt(seg->getStartTime());

        // unused warning fix
//        timeT endTime = seg->getEndTime();
//        if (seg->getRawEndMarkerTime())
//            endTime = seg->getEndMarkerTime();

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

            RG_DEBUG << "slotTempoToSegmentLength - beats = " << beats
            << " mode = " << ((dialog.getMode() == 0) ? "bars" : "beats") << endl
            << " beats per bar = " << timeSig.getBeatsPerBar()
            << " user quantity = " << dialog.getQuantity()
            << " user mode = " << dialog.getMode() << endl;
#endif

        } else {
            RG_DEBUG << "slotTempoToSegmentLength - BeatsBarsDialog aborted";
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

        RG_DEBUG << "slotTempoToSegmentLength info: "
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

void
RosegardenMainWindow::slotToggleSegmentLabels()
{
    QAction *act = this->findAction("show_segment_labels");

    if (act) {
        m_view->slotShowSegmentLabels(act->isChecked());
    }
}

void
RosegardenMainWindow::slotEdit()
{
    m_view->slotEditSegment(nullptr);
}

void
RosegardenMainWindow::slotEditAsNotation()
{
    m_view->slotEditSegmentNotation(nullptr);
}

void
RosegardenMainWindow::slotEditInMatrix()
{
    m_view->slotEditSegmentMatrix(nullptr);
}

void
RosegardenMainWindow::slotEditInPercussionMatrix()
{
    m_view->slotEditSegmentPercussionMatrix(nullptr);
}

void
RosegardenMainWindow::slotEditInEventList()
{
    m_view->slotEditSegmentEventList(nullptr);
}

void
RosegardenMainWindow::slotEditInPitchTracker()
{
    m_view->slotEditSegmentPitchTracker(nullptr);
}

void
RosegardenMainWindow::slotEditTempos()
{
    slotEditTempos(RosegardenDocument::currentDocument->getComposition().getPosition());
}

void
RosegardenMainWindow::slotToggleToolBar()
{
    TmpStatusMsg msg(tr("Toggle the toolbar..."), this);

    if (findAction("show_stock_toolbar")->isChecked())
        findToolbar("Main Toolbar")->show();
    else
        findToolbar("Main Toolbar")->hide();
}

void
RosegardenMainWindow::slotToggleToolsToolBar()
{
    TmpStatusMsg msg(tr("Toggle the tools toolbar..."), this);

    if (findAction("show_tools_toolbar")->isChecked())
        findToolbar("Tools Toolbar")->show();
    else
        findToolbar("Tools Toolbar")->hide();
}

void
RosegardenMainWindow::slotToggleTracksToolBar()
{
    TmpStatusMsg msg(tr("Toggle the tracks toolbar..."), this);

    if (findAction("show_tracks_toolbar")->isChecked())
        findToolbar("Tracks Toolbar")->show();
    else
        findToolbar("Tracks Toolbar")->hide();
}

void
RosegardenMainWindow::slotToggleEditorsToolBar()
{
    TmpStatusMsg msg(tr("Toggle the editor toolbar..."), this);

    if (findAction("show_editors_toolbar")->isChecked())
        findToolbar("Editors Toolbar")->show();
    else
        findToolbar("Editors Toolbar")->hide();
}

void
RosegardenMainWindow::slotToggleTransportToolBar()
{
    TmpStatusMsg msg(tr("Toggle the transport toolbar..."), this);

    if (findAction("show_transport_toolbar")->isChecked())
        findToolbar("Transport Toolbar")->show();
    else
        findToolbar("Transport Toolbar")->hide();
}

void
RosegardenMainWindow::slotToggleZoomToolBar()
{
    TmpStatusMsg msg(tr("Toggle the zoom toolbar..."), this);

    if (findAction("show_zoom_toolbar")->isChecked())
        findToolbar("Zoom Toolbar")->show();
    else
        findToolbar("Zoom Toolbar")->hide();
}

void
RosegardenMainWindow::slotUpdateTransportVisibility()
{
    TmpStatusMsg msg(tr("Toggle the Transport"), this);

    if (findAction("show_transport")->isChecked()) {
        getTransport()->show();
        getTransport()->raise();
        // Put the window where it belongs.
        // ??? We shouldn't need to do this, but...
        //     If you hide the transport with the menu item or the "T"
        //     shortcut, the window crawls up the screen.  This doesn't
        //     happen if you hide it with its close button.  It appears
        //     to be some sort of X or window manager problem.
        getTransport()->loadGeo();
    } else {  // Hide
        // Save the window location for when we show it again.
        // ??? We shouldn't need to do this.  See comments above.
        // ??? Also see TransportDialog's dtor which depends on this
        //     saveGeo() call.
        getTransport()->saveGeo();
        getTransport()->hide();
    }
}

void
RosegardenMainWindow::slotToggleTransportVisibility()
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
    slotUpdateTransportVisibility();
}

void
RosegardenMainWindow::slotToggleTrackLabels()
{
    if (findAction("show_tracklabels")->isChecked()) {
#ifdef SETTING_LOG_DEBUG
        RG_DEBUG << "toggle track labels on";
#endif

        m_view->getTrackEditor()->getTrackButtons()->
                changeLabelDisplayMode(TrackLabel::ShowTrack);
    } else {
#ifdef SETTING_LOG_DEBUG
        RG_DEBUG << "toggle track labels off";
#endif

        m_view->getTrackEditor()->getTrackButtons()->
                changeLabelDisplayMode(TrackLabel::ShowInstrument);
    }
}

void
RosegardenMainWindow::slotToggleRulers()
{
    m_view->slotShowRulers(findAction("show_rulers")->isChecked());
}

void
RosegardenMainWindow::slotToggleTempoRuler()
{
    m_view->slotShowTempoRuler(findAction("show_tempo_ruler")->isChecked());
}

void
RosegardenMainWindow::slotToggleChordNameRuler()
{
    m_view->slotShowChordNameRuler(findAction("show_chord_name_ruler")->isChecked());
}

void
RosegardenMainWindow::slotTogglePreviews()
{
    m_view->slotShowPreviews(findAction("show_previews")->isChecked());
}

void
RosegardenMainWindow::slotHideShowParameterArea()
{
    m_parameterArea->setVisible(findAction("show_inst_segment_parameters")->isChecked());
}

/* unused
void
RosegardenMainWindow::slotParameterAreaHidden()
{
    RG_DEBUG << "slotParameterAreaHidden(): who called this?  Is this the amnesia source?";

    // Since the parameter area is now hidden, clear the checkbox in the
    // menu to keep things in sync.
    findAction("show_inst_segment_parameters")->setChecked(false);
}
*/

void
RosegardenMainWindow::slotToggleStatusBar()
{
    TmpStatusMsg msg(tr("Toggle the statusbar..."), this);

    if (!findAction("show_status_bar")->isChecked())
        statusBar()->hide();
    else
        statusBar()->show();
}

void
RosegardenMainWindow::slotFullScreen()
{
    const bool fullScreen = findAction("full_screen")->isChecked();

    if (fullScreen)
        showFullScreen();
    else
        showNormal();
}

void
RosegardenMainWindow::slotStatusMsg(QString text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clearMessage();
//    statusBar()->changeItem(text, EditViewBase::ID_STATUS_MSG);
//    statusBar()->showMessage(text, EditViewBase::ID_STATUS_MSG);
    statusBar()->showMessage(text, 0);    // note: last param == timeout
}

void
RosegardenMainWindow::slotStatusHelpMsg(QString text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->showMessage(text, 2000);
}

void
RosegardenMainWindow::slotEnableTransport(bool enable)
{
    // ??? What about the menu items and the toolbar buttons?  Also
    //     note that the TransportDialog buttons do not gray or
    //     indicate that they've been disabled in any way.

    if (m_transport)
        getTransport()->setEnabled(enable);
}

void
RosegardenMainWindow::slotPointerSelected()
{
    m_view->selectTool(SegmentSelector::ToolName());
}

void
RosegardenMainWindow::slotEraseSelected()
{
    m_view->selectTool(SegmentEraser::ToolName());
}

void
RosegardenMainWindow::slotDrawSelected()
{
    m_view->selectTool(SegmentPencil::ToolName());
}

void
RosegardenMainWindow::slotMoveSelected()
{
    m_view->selectTool(SegmentMover::ToolName());
}

void
RosegardenMainWindow::slotResizeSelected()
{
    m_view->selectTool(SegmentResizer::ToolName());
}

void
RosegardenMainWindow::slotJoinSelected()
{
    QMessageBox::information(this,
                             tr("The join tool isn't implemented yet.  Instead please highlight "
                                  "the segments you want to join and then use the menu option:\n\n"
                                  "        Segments->Collapse Segments.\n"),
                             tr("Join tool not yet implemented"));

    m_view->selectTool(SegmentJoiner::ToolName());
}

void
RosegardenMainWindow::slotSplitSelected()
{
    m_view->selectTool(SegmentSplitter::ToolName());
}

void
RosegardenMainWindow::slotAddTrack()
{
    if (!m_view)
        return;

    RosegardenDocument *document = RosegardenDocument::currentDocument;
    if (!document)
        return;

    const InstrumentId foundInstrumentID =
            document->getStudio().getAvailableMIDIInstrument();

    Composition &comp = document->getComposition();
    const TrackId trackId = comp.getSelectedTrack();
    const Track *track = comp.getTrackById(trackId);

    int pos = -1;
    if (track)
        pos = track->getPosition() + 1;

    m_view->addTrack(foundInstrumentID, pos);

    // Move the selected Track to the new Track so that repeated pressings
    // of Ctrl+T yields a series of new Tracks in correct Instrument order.
    TrackId newTrackID = comp.getTrackByPosition(pos)->getId();
    comp.setSelectedTrack(newTrackID);
    // Make sure everything updates appropriately.
    comp.notifyTrackSelectionChanged(newTrackID);
    // Note that we don't call m_view->slotSelectTrackSegments(newTrackId)
    // because there are no segments on this new track, so there is no point.
    // Track selection and Segment selection might get out of sync.  Not
    // sure if that is a problem.
    //m_view->slotSelectTrackSegments(newTrackId);
    // Make sure the Instrument Parameter Panel is updated.
    document->emitDocumentModified();

}

void
RosegardenMainWindow::slotAddTracks()
{
    AddTracksDialog dialog(this);
    dialog.exec();
}

void
RosegardenMainWindow::slotDeleteTrack()
{
    if (!m_view)
        return ;

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    TrackId trackId = comp.getSelectedTrack();
    Track *track = comp.getTrackById(trackId);

    if (track == nullptr)
        return ;

    // Don't let the user delete the last track.
    if (comp.getNbTracks() == 1)
        return ;

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
        RG_DEBUG << "slotDeleteTrack - "
                 << "can't select a highlighted track after delete";
    }

    comp.setSelectedTrack(trackId);
    comp.notifyTrackSelectionChanged(trackId);
    m_view->slotSelectTrackSegments(trackId);
    RosegardenDocument::currentDocument->emitDocumentModified();

// unused:
//    Instrument *inst = RosegardenDocument::currentDocument->getStudio().
//                       getInstrumentById(comp.getTrackById(trackId)->getInstrument());

    //VLADA
    //    m_view->slotSelectTrackSegments(trackId);
    //VLADA
}

void
RosegardenMainWindow::slotMoveTrackDown()
{
    //RG_DEBUG << "slotMoveTrackDown";

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    Track *srcTrack = comp.getTrackById(comp.getSelectedTrack());

    // Check for track object
    //
    if (srcTrack == nullptr)
        return ;

    // Check destination track exists
    //
    Track *destTrack =
        comp.getTrackByPosition(srcTrack->getPosition() + 1);

    if (destTrack == nullptr)
        return ;

    MoveTracksCommand *command =
        new MoveTracksCommand(&comp, srcTrack->getId(), destTrack->getId());

    CommandHistory::getInstance()->addCommand(command);

    // make sure we're showing the right selection
    comp.notifyTrackSelectionChanged(comp.getSelectedTrack());
    // ??? This doesn't work right when the user uses the keyboard shortcut
    //     "Shift + Down Arrow".  Since "Shift" is being held down, that
    //     modifies the behavior of this call.  Keep pressing Shift + Down
    //     and note that the segments alternate between selected and
    //     unselected.
    if (m_view)
        m_view->slotSelectTrackSegments(comp.getSelectedTrack());

}

void
RosegardenMainWindow::slotMoveTrackUp()
{
    //RG_DEBUG << "slotMoveTrackUp";

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    Track *srcTrack = comp.getTrackById(comp.getSelectedTrack());

    // Check for track object
    //
    if (srcTrack == nullptr)
        return ;

    // Check we're not at the top already
    //
    if (srcTrack->getPosition() == 0)
        return ;

    // Check destination track exists
    //
    Track *destTrack =
        comp.getTrackByPosition(srcTrack->getPosition() - 1);

    if (destTrack == nullptr)
        return ;

    MoveTracksCommand *command =
        new MoveTracksCommand(&comp, srcTrack->getId(), destTrack->getId());

    CommandHistory::getInstance()->addCommand(command);

    // make sure we're showing the right selection
    comp.notifyTrackSelectionChanged(comp.getSelectedTrack());
    // ??? This doesn't work right when the user uses the keyboard shortcut
    //     "Shift + Up Arrow".  Since "Shift" is being held down, that
    //     modifies the behavior of this call.  Keep pressing Shift + Up
    //     and note that the segments alternate between selected and
    //     unselected.
    if (m_view)
        m_view->slotSelectTrackSegments(comp.getSelectedTrack());
}

void
RosegardenMainWindow::slotRevertToSaved()
{
    RG_DEBUG << "slotRevertToSaved";

    // No changes, no point.  Bail.
    if (!RosegardenDocument::currentDocument->isModified())
        return;

    const int revert = QMessageBox::question(this, tr("Rosegarden"),
            tr("Revert modified document to previous saved version?"));
    if (revert == QMessageBox::No)
        return;

    // Re-open the file.
    // Further down, we will figure out this is a revert based on the
    // filename.  This will disable locking and autosave.
    openFile(RosegardenDocument::currentDocument->getAbsFilePath());
}

void
RosegardenMainWindow::slotImportProject()
{
    if (RosegardenDocument::currentDocument && !saveIfModified())
        return ;

    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    QString directory = settings.value("import_project", QDir::homePath()).toString();
    directory = existingDir(directory);

    const QString file = FileDialog::getOpenFileName(this, tr("Import Rosegarden Project File"), directory,
               tr("Rosegarden Project files") + " (*.rgp *.RGP)" + ";;" +
               tr("All files") + " (*)", nullptr);

    if (file.isEmpty())
        return;

    importProject(file);

    directory = existingDir(file);
    settings.setValue("import_project", directory);
    settings.endGroup();
}

void
RosegardenMainWindow::importProject(const QString& filePath)
{
    // Launch the project packager script-in-a-dialog in Unpack mode:
    ProjectPackager *dialog = new ProjectPackager(this, RosegardenDocument::currentDocument, ProjectPackager::Unpack, filePath);
    if (dialog->exec() != QDialog::Accepted) {
        return;
    }

    // open the true filename contained within and extracted from the package (foo.rgp might have
    // contained bar.rg)
    openURL(dialog->getTrueFilename());
}

void
RosegardenMainWindow::slotImportMIDI()
{
    if (RosegardenDocument::currentDocument && !saveIfModified())
        return ;

    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    QString directory = settings.value("import_midi", QDir::homePath()).toString();
    directory = existingDir(directory);

    const QString file = FileDialog::getOpenFileName(this, tr("Open MIDI File"), directory,
               tr("MIDI files") + " (*.mid *.midi *.MID *.MIDI)" + ";;" +
               tr("All files") + " (*)", nullptr);

    if (file.isEmpty())
        return;

    openFile(file, ImportMIDI); // does everything including setting the document

    directory = existingDir(file);
    settings.setValue("import_midi", directory);
    settings.endGroup();
}

void
RosegardenMainWindow::slotMergeMIDI()
{
    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    QString directory = settings.value("merge_midi", QDir::homePath()).toString();
    directory = existingDir(directory);

    const QStringList fileList = FileDialog::getOpenFileNames(
            this, tr("Select MIDI File(s)"), directory,
               tr("MIDI files") + " (*.mid *.midi *.MID *.MIDI)" + ";;" +
               tr("All files") + " (*)", nullptr);

    if (fileList.isEmpty())
        return;

    mergeFile(fileList, ImportMIDI);

    settings.setValue("merge_midi", directory);
    directory = existingDir(fileList[0]);
    settings.endGroup();
}

QTextCodec *
RosegardenMainWindow::guessTextCodec(std::string text)
{
    QTextCodec *codec = nullptr;

    for (int c = 0; c < int(text.length()); ++c) {
        if (text[c] & 0x80) {
            StartupLogo::hideIfStillThere();

            IdentifyTextCodecDialog dialog(nullptr, text);
            dialog.exec();

            QString codecName = dialog.getCodec();

            if (codecName != "") {
                codec = QTextCodec::codecForName(codecName.toLatin1());
            }
            break;
        }
    }

    return codec;
}

void
RosegardenMainWindow::fixTextEncodings(Composition *c)

{
    QTextCodec *codec = nullptr;

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

    for (Composition::TrackMap::iterator i =
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

RosegardenDocument *
RosegardenMainWindow::createDocumentFromMIDIFile(
        const QString &filePath,
        bool permanent)
{
    //if (!merge && !saveIfModified()) return;

    // Create new document (autoload is inherent)
    //
    RosegardenDocument *newDoc = newDocument(permanent);

    MidiFile midiFile;

    StartupLogo::hideIfStillThere();

    // Progress Dialog
    // Note: The label text and range will be set later as needed.
    QProgressDialog progressDialog(
            tr("Importing MIDI file..."),  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 100,  // min, max
            this);  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // Don't want to auto close since this is a multi-step
    // process.  Any of the steps may set progress to 100.  We
    // will close anyway when this object goes out of scope.
    progressDialog.setAutoClose(false);
    // Just force the progress dialog up.
    // Both Qt4 and Qt5 have bugs related to delayed showing of progress
    // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
    // based distros might lock up.  See Bug #1546.
    progressDialog.show();

    midiFile.setProgressDialog(&progressDialog);

    if (!midiFile.convertToRosegarden(filePath, newDoc)) {
        // NOTE: Someone flagged midiFile.getError() with a warning about tr().
        // This stuff either gets translated at the source, if we own it, or it
        // doesn't get translated at all, if we don't (eg. errors from the
        // underlying filesystem, a library, etc.)
        QMessageBox::critical(this, tr("Rosegarden"), strtoqstr(midiFile.getError()));
        delete newDoc;
        return nullptr;
    }

    fixTextEncodings(&newDoc->getComposition());

    // Set modification flag
    //
    newDoc->slotDocumentModified();

    // Set the caption
    //
    newDoc->setTitle(QFileInfo(filePath).fileName());
    newDoc->setAbsFilePath(QFileInfo(filePath).absoluteFilePath());

    // Clean up for notation purposes (after reinitialise, because that
    // sets the composition's end marker time which is needed here)

    progressDialog.setLabelText(tr("Calculating notation..."));
    progressDialog.setValue(0);
    qApp->processEvents();

    Composition *comp = &newDoc->getComposition();

    for (Composition::iterator i = comp->begin();
         i != comp->end(); ++i) {

        Segment &segment = **i;
        SegmentNotationHelper helper(segment);
        segment.insert(helper.guessClef(segment.begin(),
                                        segment.getEndMarker())
                       .getAsEvent(segment.getStartTime()));
    }

    progressDialog.setValue(10);

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

    progressDialog.setValue(20);

    int segmentCount = 1;
    const int nbSegments = comp->getNbSegments();

    double progressPerSegment = 80;
    if (nbSegments > 0)
        progressPerSegment = 80.0 / nbSegments;

    MacroCommand *command = new MacroCommand(tr("Calculate Notation"));

    // For each segment in the composition.
    for (Composition::iterator i = comp->begin(); i != comp->end(); ++i) {
        Segment &segment = **i;

        timeT startTime(segment.getStartTime());
        timeT endTime(segment.getEndMarkerTime());

        //RG_DEBUG << "segment: start time " << segment.getStartTime() << ", end time " << segment.getEndTime() << ", end marker time " << segment.getEndMarkerTime() << ", events " << segment.size();

        EventQuantizeCommand *subCommand = new EventQuantizeCommand
            (segment, startTime, endTime, NotationOptionsConfigGroup,
             EventQuantizeCommand::QUANTIZE_NOTATION_ONLY);
        subCommand->setProgressDialog(&progressDialog);

        // Compute progress so far.
        double totalProgress = 20 + segmentCount * progressPerSegment;
        ++segmentCount;

        //RG_DEBUG << "createDocumentFromMIDIFile() totalProgress: " << totalProgress;

        subCommand->setProgressTotal(totalProgress, progressPerSegment + 1);

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

void
RosegardenMainWindow::slotImportRG21()
{
    if (RosegardenDocument::currentDocument && !saveIfModified())
        return ;

    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    QString directory = settings.value("import_relic", QDir::homePath()).toString();
    directory = existingDir(directory);

    const QString file = FileDialog::getOpenFileName(this, tr("Open X11 Rosegarden File"), directory,
               tr("X11 Rosegarden files") + " (*.rose)" + ";;" +
               tr("All files") + " (*)", nullptr);

    if (file.isEmpty())
        return;

    openFile(file, ImportRG21);

    directory = existingDir(file);
    settings.setValue("import_relic", directory);
    settings.endGroup();
}

void
RosegardenMainWindow::slotMergeRG21()
{
    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    QString directory = settings.value("merge_relic", QDir::homePath()).toString();
    directory = existingDir(directory);

    const QStringList fileList = FileDialog::getOpenFileNames(
            this, tr("Select X11 Rosegarden File(s)"), directory,
               tr("X11 Rosegarden files") + " (*.rose)" + ";;" +
               tr("All files") + " (*)", nullptr);

    if (fileList.isEmpty())
        return;

    mergeFile(fileList, ImportRG21);

    directory = existingDir(fileList[0]);
    settings.setValue("merge_relic", directory);
    settings.endGroup();
}

RosegardenDocument *
RosegardenMainWindow::createDocumentFromRG21File(QString file)
{
    StartupLogo::hideIfStillThere();

    // Progress Dialog
    QProgressDialog progressDialog(
            tr("Importing X11 Rosegarden file..."),  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 0,  // min, max
            this);  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // We will close anyway when this object goes out of scope.
    progressDialog.setAutoClose(false);
    // Remove the cancel button since RG21Loader doesn't support cancel.
    progressDialog.setCancelButton(nullptr);
    // Just force the progress dialog up.
    // Both Qt4 and Qt5 have bugs related to delayed showing of progress
    // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
    // based distros might lock up.  See Bug #1546.
    progressDialog.show();

    // Inherent autoload
    //
    RosegardenDocument *newDoc = newDocument(
            true);  // permanent

    RG21Loader rg21Loader(&newDoc->getStudio());

    if (!rg21Loader.load(file, newDoc->getComposition())) {
        QMessageBox::critical(this, tr("Rosegarden"),
                           tr("Can't load X11 Rosegarden file.  It appears to be corrupted."));
        delete newDoc;
        return nullptr;
    }

    // Set modification flag
    //
    newDoc->slotDocumentModified();

    // Set the caption and add recent
    //
    newDoc->setTitle(QFileInfo(file).fileName());
    newDoc->setAbsFilePath(QFileInfo(file).absoluteFilePath());

    return newDoc;
}

#if 0
void
RosegardenMainWindow::slotImportHydrogen()
{
    if (RosegardenDocument::currentDocument && !saveIfModified())
        return ;

    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    QString directory = settings.value("import_hydrogen", QDir::homePath()).toString();
    directory = existingDir(directory);

    const QString file = FileDialog::getOpenFileName(this, tr("Open Hydrogen File"), directory,
               tr("All files") + " (*)", 0, 0);
    if (file.isEmpty())
        return;

    openFile(file, ImportHydrogen);

    directory = existingDir(file);
    settings.setValue("import_hydrogen", directory);
    settings.endGroup();
}

void
RosegardenMainWindow::slotMergeHydrogen()
{
    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    QString directory = settings.value("merge_hydrogen", QDir::homePath()).toString();
    directory = existingDir(directory);

    const QString file = FileDialog::getOpenFileName(this, tr("Open Hydrogen File"), directory,
               tr("All files") + " (*)", 0, 0);
    if (file.isEmpty())
        return;

    mergeFile(file, ImportHydrogen);

    directory = existingDir(file);
    settings.setValue("merge_hydrogen", directory);
    settings.endGroup();
}


RosegardenDocument *
RosegardenMainWindow::createDocumentFromHydrogenFile(QString file)
{
    StartupLogo::hideIfStillThere();

    // Progress Dialog
    QProgressDialog progressDialog(
            tr("Importing Hydrogen file..."),  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 0,  // min, max
            this);  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // We will close anyway when this object goes out of scope.
    progressDialog.setAutoClose(false);
    // Remove the cancel button since HydrogenLoader doesn't support cancel.
    progressDialog.setCancelButton(nullptr);
    // Just force the progress dialog up.
    // Both Qt4 and Qt5 have bugs related to delayed showing of progress
    // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
    // based distros might lock up.  See Bug #1546.
    progressDialog.show();

    // Inherent autoload
    //
    RosegardenDocument *newDoc = newDocument();

    HydrogenLoader hydrogenLoader(&newDoc->getStudio());

    if (!hydrogenLoader.load(file, newDoc->getComposition())) {
        QMessageBox::critical(this, tr("Rosegarden"),
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
    newDoc->setAbsFilePath(QFileInfo(file).absoluteFilePath());

    return newDoc;
}
#endif

void
RosegardenMainWindow::slotImportMusicXML()
{
    if (RosegardenDocument::currentDocument && !saveIfModified())
        return ;

    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    QString directory = settings.value("import_musicxml", QDir::homePath()).toString();
    directory = existingDir(directory);

    const QString file = FileDialog::getOpenFileName(this, tr("Open MusicXML File"), directory,
               tr("XML files") + " (*.xml *.XML)" + ";;" +
               tr("All files") + " (*)", nullptr);
    if (file.isEmpty())
        return;

    openFile(file, ImportMusicXML);

    directory = existingDir(file);
    settings.setValue("import_musicxml", directory);
    settings.endGroup();
}

void
RosegardenMainWindow::slotMergeMusicXML()
{
    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    QString directory = settings.value("merge_musicxml", QDir::homePath()).toString();
    directory = existingDir(directory);

    const QStringList fileList = FileDialog::getOpenFileNames(
            this, tr("Select File(s)"), directory,
               tr("XML files") + " (*.xml *.XML)" + ";;" +
               tr("All files") + " (*)", nullptr);
    if (fileList.isEmpty())
        return;

    mergeFile(fileList, ImportMusicXML);

    directory = existingDir(fileList[0]);
    settings.setValue("merge_musicxml", directory);
    settings.endGroup();
}

RosegardenDocument *
RosegardenMainWindow::createDocumentFromMusicXMLFile(const QString& file,
                                                     const bool permanent)
{
    StartupLogo::hideIfStillThere();

    // Progress Dialog
    QProgressDialog progressDialog(
            tr("Importing MusicXML file..."),  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 0,  // min, max
            this);  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // We will close anyway when this object goes out of scope.
    progressDialog.setAutoClose(false);
    // Remove the cancel button since MusicXMLLoader doesn't support cancel.
    progressDialog.setCancelButton(nullptr);
    // Just force the progress dialog up.
    // Both Qt4 and Qt5 have bugs related to delayed showing of progress
    // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
    // based distros might lock up.  See Bug #1546.
    progressDialog.show();

    // Inherent autoload
    //
    RosegardenDocument *newDoc = newDocument(permanent);

    MusicXMLLoader musicxmlLoader;

    if (!musicxmlLoader.load(file, newDoc)) {
        QMessageBox::critical(this, tr("Rosegarden"),
                           tr("Can't load MusicXML file:\n")+
                              musicxmlLoader.errorMessage());
        delete newDoc;
        return nullptr;
    }

    // Set modification flag
    //
    newDoc->slotDocumentModified();

    // Set the caption and add recent
    //
    newDoc->setTitle(QFileInfo(file).fileName());
    newDoc->setAbsFilePath(QFileInfo(file).absoluteFilePath());

    return newDoc;

}

void
RosegardenMainWindow::mergeFile(const QStringList &filePathList, ImportType type)
{
    if (!RosegardenDocument::currentDocument)
        return;

    for (int i = 0; i < filePathList.size(); ++i) {
        RosegardenDocument *srcDoc = createDocument(
                filePathList[i],
                type,  // importType
                false,  // permanent
                false,  // revert
                false);  // clearHistory
        if (!srcDoc)
            return;

        if (filePathList.size() < 2) {
            // Just one file. Usual merge behaviour
            const Composition &srcComp = srcDoc->getComposition();
            const Composition &destComp =
                    RosegardenDocument::currentDocument->getComposition();
            const bool timingsDiffer =
                    !srcComp.compareSignaturesAndTempos(destComp);

            FileMergeDialog dialog(this, timingsDiffer);
            if (dialog.exec() == QDialog::Accepted) {
                RosegardenDocument::currentDocument->mergeDocument(
                        srcDoc,
                        dialog.getMergeAtEnd(),
                        dialog.getMergeTimesAndTempos());
            }

            delete srcDoc;
        } else {
            // more than 1 file, so multiple merge
            RosegardenDocument::currentDocument->mergeDocument(
                    srcDoc,
                    true,       // assume merge at end
                    false       // assume ignore tempo and time
                    );
        }
    }
}

void RosegardenMainWindow::processRecordedEvents()
{
    if (!m_seqManager)
        return;
    // Only if we're recording
    if (m_seqManager->getTransportStatus() != RECORDING)
        return;
    if (!RosegardenDocument::currentDocument)
        return;

    // Gather the recorded events and put them where they belong in the
    // document.

    MappedEventList mC;
    if (SequencerDataBlock::getInstance()->getRecordedEvents(mC) > 0) {
        m_seqManager->processAsynchronousMidi(mC, nullptr);
        RosegardenDocument::currentDocument->insertRecordedMidi(mC);
    }

    RosegardenDocument::currentDocument->updateRecordingMIDISegment();
    RosegardenDocument::currentDocument->updateRecordingAudioSegments();
}

void
RosegardenMainWindow::slotHandleInputs()
{
    // ??? This routine does more than handle inputs.  How much of this
    //     is critical?  This routine is constantly called, so some
    //     parts of the system might depend on all of this.

    // Update the document from incoming data.
    processRecordedEvents();

    // Handle transport requests

    // ??? These could be implemented more directly as QEvents sent to
    //     RosegardenMainWindow's event queue.  See customEvent().
    //     We would need to analyze each caller of
    //     RosegardenSequencer::transportChange() and
    //     RosegardenSequencer::transportJump() and have them call
    //     postEvent() instead.  See AlsaDriver::handleTransportCCs()
    //     which uses both approaches right now.  Transitioning AlsaDriver
    //     to QEvent should be relatively smooth.  JackDriver requires
    //     the ability to wait on completion, so it might require
    //     additional work to transition to QEvent.

    RosegardenSequencer::TransportRequest req;
    RealTime rt;
    bool have = RosegardenSequencer::getInstance()->
        getNextTransportRequest(req, rt);

    if (have) {
        switch (req) {
        case RosegardenSequencer::TransportNoChange:
            break;
        case RosegardenSequencer::TransportStop:
            slotStop();
            break;
        case RosegardenSequencer::TransportStart:
            slotPlay();
            break;
        case RosegardenSequencer::TransportPlay:
            slotPlay();
            break;
        case RosegardenSequencer::TransportRecord:
            slotToggleRecord();
            break;
        case RosegardenSequencer::TransportJumpToTime:
            slotJumpToTime(rt);
            break;
        case RosegardenSequencer::TransportStartAtTime:
            slotStartAtTime(rt);
            break;
        case RosegardenSequencer::TransportStopAtTime:
            slotStop();
            slotJumpToTime(rt);
            break;
        }
    }

    TransportStatus status = RosegardenSequencer::getInstance()->
        getStatus();

    // ??? Wouldn't it make more sense to do this when playback or
    //     recording is started/stopped?  E.g. in slotStop(), slotPlay(), and
    //     slotRecord().  That would avoid constantly polling this.
    if (status == PLAYING || status == RECORDING) { //@@@ JAS orig ? KXMLGUIClient::StateReverse : KXMLGUIClient::StateNoReverse
        if (m_notPlaying)
            leaveActionState("not_playing");
    } else {
        if (!m_notPlaying)
            enterActionState("not_playing");
    }

    if (m_seqManager) {

        // ??? Why are we constantly setting this?  Why are we a conduit
        //     from RosegardenSequencer to SequenceManager?  Isn't there
        //     a more direct route?
        m_seqManager->setTransportStatus(status);

        // ??? Again, why are we a conduit from RosegardenSequencer to
        //     SequenceManager?  This also incurs a lock.  Would it be
        //     possible to keep this in the sequencer thread and avoid
        //     the lock?

        MappedEventList asynchronousQueue =
            RosegardenSequencer::getInstance()->pullAsynchronousMidiQueue();

        if (!asynchronousQueue.empty())
            m_seqManager->processAsynchronousMidi(asynchronousQueue, nullptr);
    }
}

void
RosegardenMainWindow::slotUpdateUI()
{
    TransportStatus status = RosegardenSequencer::getInstance()->getStatus();

    // If we're stopped
    if (status != PLAYING  &&  status != RECORDING) {
        // Keep the meters going for monitoring
        slotUpdateMonitoring();
        return;
    }

    // Either sequencer mapper or the sequence manager could be missing at
    // this point.
    //
    if (!m_seqManager) return;
    if (!RosegardenDocument::currentDocument) return;

    // Update display of the current MIDI out event on the transport

    MappedEvent ev;
    bool haveEvent = SequencerDataBlock::getInstance()->getVisual(ev);
    if (haveEvent) getTransport()->slotMidiOutLabel(&ev);


    // Update the playback position pointer

    RealTime position = SequencerDataBlock::getInstance()->getPositionPointer();
    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    timeT elapsedTime = comp.getElapsedTimeForRealTime(position);

    // We don't want slotSetPointerPosition() to affect the sequencer.
    // Setting m_originatingJump to true causes slotSetPointerPosition()
    // to not tell the sequencer to jump to this new position.  This
    // might be renamed m_seqJump and reverse its value.
    // ??? This should just be an argument to slotSetPointerPosition().
    //   slotSetPointerPosition(elapsedTime, bool jumpSequencer = true);
    //   (Can we have default args in a slot?  Seems unlikely.)
    m_originatingJump = true;
    // Move the pointer to the current position.
    RosegardenDocument::currentDocument->slotSetPointerPosition(elapsedTime);
    // Future moves (jumps) won't be coming from here.
    m_originatingJump = false;


    // Update the VU meters
    if (m_view) m_view->updateMeters();
}

void
RosegardenMainWindow::slotUpdateCPUMeter()
{
    // Set to true when CPU % has been displayed.
    static bool modified = false;

    TransportStatus status = RosegardenSequencer::getInstance()->getStatus();

    // If we're playing, display the CPU %
    if (status == PLAYING  ||  status == RECORDING) {

        static std::ifstream *statstream = nullptr;
        static unsigned long lastBusy = 0, lastIdle = 0;
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

        // correct use of m_cpuBar; it's the CPU meter, and from now on,
        // nothing else (use QProgressDialog for reporting any kind of progress)
        if (m_cpuBar) {
            if (!modified) {
                m_cpuBar->setTextVisible(true);
                m_cpuBar->setFormat("CPU %p%");
            }
            m_cpuBar->setValue(count);
        }

        modified = true;

    } else if (modified) {  // If CPU has been displayed, clear it.
        if (m_cpuBar) {
            m_cpuBar->setTextVisible(false);
            m_cpuBar->setFormat("%p%");
            m_cpuBar->setValue(0);
        }
        // Indicate CPU % display is now clear.
        modified = false;
    }
}

void
RosegardenMainWindow::slotUpdateMonitoring()
{
    m_view->updateMonitorMeters();
}

void
RosegardenMainWindow::slotSetPointerPosition(timeT t)
{
    Composition &comp = RosegardenDocument::currentDocument->getComposition();

    if (m_seqManager) {
        // Normally we stop at composition end.
        timeT stopTime = comp.getEndMarker();

        // If "stop at end of last Segment" is enabled, use the latest
        // Segment end time.
        // ??? rename: getStopAtSegmentEnd()?
        if (Preferences::getStopAtSegmentEnd())
            stopTime = comp.getDuration(true);

        // If we're playing and we're past the end...
        if (m_seqManager->getTransportStatus() == PLAYING  &&
            t > stopTime) {

            // Stop - automatic stop - not triggered by the user
            doStop(true);

            // Limit the end to the end of the composition.
            // RECURSION: Causes this method to be re-invoked.
            RosegardenDocument::currentDocument->
                slotSetPointerPosition(stopTime);

            return;

        }
        // If we're recording and we're near the end...
        if (m_seqManager->getTransportStatus() == RECORDING  &&
            t > comp.getEndMarker() - timebase) {

            // Compute bar duration
            std::pair<timeT, timeT> timeRange = comp.getBarRangeForTime(t);
            const timeT barDuration = timeRange.second - timeRange.first;

            // Add on ten bars.
            const timeT newEndMarker = comp.getEndMarker() + 10 * barDuration;
            comp.setEndMarker(newEndMarker);

            // Update UI
            getView()->getTrackEditor()->updateCanvasSize();
            getView()->getTrackEditor()->updateRulers();

        }

        // cc 20050520 - jump at the sequencer even if we're not playing,
        // because we might be a transport master of some kind
        try {
            if (!m_originatingJump) {
                m_seqManager->jumpTo(comp.getElapsedRealTime(t));
            }
        } catch (const QString &s) {
            QMessageBox::critical(this, tr("Rosegarden"), s);
        }
    }

    // set the time sig
    getTransport()->setTimeSignature(comp.getTimeSignatureAt(t));

    // and the tempo
    m_seqManager->setTempo(comp.getTempoAtTime(t));

    // and the time
    //
    TransportDialog::TimeDisplayMode mode =
        getTransport()->getCurrentMode();

    if (mode == TransportDialog::BarMode ||
            mode == TransportDialog::BarMetronomeMode) {

        displayBarTime(t);

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

    // Update position on the marker editor if it's available
    //
    if (m_markerEditor)
        m_markerEditor->updatePosition();
}

void
RosegardenMainWindow::displayBarTime(timeT t)
{
    Composition &comp = RosegardenDocument::currentDocument->getComposition();

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

void
RosegardenMainWindow::slotRefreshTimeDisplay()
{
    if (m_seqManager->getTransportStatus() == PLAYING ||
            m_seqManager->getTransportStatus() == RECORDING) {
        return ; // it'll be refreshed in a moment anyway
    }
    slotSetPointerPosition(RosegardenDocument::currentDocument->getComposition().getPosition());
}

void
RosegardenMainWindow::slotScrollToFollow()
{
    m_view->getTrackEditor()->scrollToFollow();
}

void
RosegardenMainWindow::slotLoop()
{
    RosegardenDocument::currentDocument->loopButton(
            findAction("loop")->isChecked());
}

void
RosegardenMainWindow::slotTestStartupTester()
{
    if (!m_startupTester) {
        m_startupTester = new StartupTester();
        connect(m_startupTester, &StartupTester::newerVersionAvailable,
                this, &RosegardenMainWindow::slotNewerVersionAvailable);
        m_startupTester->start();
        QTimer::singleShot(100, this, &RosegardenMainWindow::slotTestStartupTester);
        return ;
    }

    if (!m_startupTester->isReady()) {
        QTimer::singleShot(100, this, &RosegardenMainWindow::slotTestStartupTester);
        return ;
    }

    m_startupTester->wait();
    delete m_startupTester;
    m_startupTester = nullptr;
}

void
RosegardenMainWindow::slotDebugDump()
{
    RosegardenDocument::currentDocument->getComposition().dump();
}

bool
RosegardenMainWindow::launchSequencer()
{
    if (!isUsingSequencer()) {
        RG_DEBUG << "launchSequencer() - not using seq. - returning\n";
        return false; // no need to launch anything
    }

    if (isSequencerRunning()) {
        RG_DEBUG << "launchSequencer() - sequencer already running - returning\n";
        if (m_seqManager) m_seqManager->checkSoundDriverStatus(false);
        return true;
    }

    m_sequencerThread = new SequencerThread();
    connect(m_sequencerThread, &QThread::finished, this, &RosegardenMainWindow::slotSequencerExited);
    m_sequencerThread->start();

    //RG_DEBUG << "launchSequencer: Sequencer thread is" << m_sequencerThread;

    if (RosegardenDocument::currentDocument && RosegardenDocument::currentDocument->getStudio().haveMidiDevices()) {
        enterActionState("got_midi_devices"); //@@@ JAS orig. 0
    } else {
        leaveActionState("got_midi_devices"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    }

    return true;
}

void
RosegardenMainWindow::slotDocumentDevicesResyncd()
{
    //!DEVPUSH how to replace this?
    m_sequencerCheckedIn = true;
    m_trackParameterBox->devicesChanged();
}

void
RosegardenMainWindow::slotSequencerExited()
{
    RG_DEBUG << "slotSequencerExited Sequencer exited\n";

    StartupLogo::hideIfStillThere();

    if (m_sequencerCheckedIn) {

        QMessageBox::critical(this, tr("Rosegarden"), tr("The Rosegarden sequencer process has exited unexpectedly.  Sound and recording will no longer be available for this session.\nPlease exit and restart Rosegarden to restore sound capability."));

    } else {

        QMessageBox::critical(this, tr("Rosegarden"), tr("The Rosegarden sequencer could not be started, so sound and recording will be unavailable for this session.\nFor assistance with correct audio and MIDI configuration, go to http://rosegardenmusic.com."));
    }

    delete m_sequencerThread;
    m_sequencerThread = nullptr; // isSequencerRunning() will return false
    // but isUsingSequencer() will keep returning true
    // so pressing the play button may attempt to restart the sequencer
}

void
RosegardenMainWindow::slotExportProject()
{
    TmpStatusMsg msg(tr("Exporting Rosegarden Project file..."), this);

    QString fileName = launchSaveAsDialog
                       (tr("Rosegarden Project files") + " (*.rgp *.RGP)" + ";;" +
                        tr("All files") + " (*)",
                        tr("Export as..."));

    if (fileName.isEmpty())
        return ;

    QString rgFile = fileName;
    rgFile.replace(QRegularExpression(".rg.rgp$"), ".rg");
    rgFile.replace(QRegularExpression(".rgp$"), ".rg");

    // I have a ton of weird files and suspect problems with this, but maybe
    // not:
    RG_DEBUG << "getValidWriteFileName() returned " << fileName.toStdString()
             << "                         rgFile: " << fileName.toStdString();

    QString errMsg;
    if (!RosegardenDocument::currentDocument->saveDocument(rgFile, errMsg,
                             true)) { // pretend it's autosave
        QMessageBox::warning(this, tr("Rosegarden"), tr("Saving Rosegarden file to package failed: %1").arg(errMsg));
        return ;
    }

    // Launch the project packager script-in-a-dialog in Pack mode:
    ProjectPackager *dialog = new ProjectPackager(this, RosegardenDocument::currentDocument, ProjectPackager::Pack, fileName);
    if (dialog->exec() != QDialog::Accepted) {
        return;
    }

    // ProjectPackager has no success() routine.  Stick with existingDir()
    // just in case we have a bad dir name by this point.
    setFileSaveAsDirectory(existingDir(fileName));
}

void
RosegardenMainWindow::slotExportMIDI()
{
    TmpStatusMsg msg(tr("Exporting MIDI file..."), this);

    QString fileName = launchSaveAsDialog
                       (tr("Standard MIDI files") + " (*.mid *.midi *.MID *.MIDI)" + ";;" +
                        tr("All files") + " (*)",
                        tr("Export as..."));

    if (fileName.isEmpty())
        return ;

    const bool success = exportMIDIFile(fileName);

    if (success)
        setFileSaveAsDirectory(existingDir(fileName));
}

bool
RosegardenMainWindow::exportMIDIFile(QString file)
{
    // Progress Dialog
    QProgressDialog progressDialog(
            tr("Exporting MIDI file..."),  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 100,  // min, max
            this);  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // No sense in auto close since we will close anyway when
    // this object goes out of scope.
    progressDialog.setAutoClose(false);
    // Just force the progress dialog up.
    // Both Qt4 and Qt5 have bugs related to delayed showing of progress
    // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
    // based distros might lock up.  See Bug #1546.
    progressDialog.show();

    MidiFile midiFile;

    midiFile.setProgressDialog(&progressDialog);

    if (!midiFile.convertToMidi(RosegardenDocument::currentDocument, file)) {
        QMessageBox::warning(this, tr("Rosegarden"),
                tr("Export failed.  The file could not be opened for writing."));

        return false;
    }

    return true;
}

void
RosegardenMainWindow::slotExportCsound()
{
    TmpStatusMsg msg(tr("Exporting Csound score file..."), this);

    QString fileName = launchSaveAsDialog
                       (tr("Csound files") + " (*.csd *.CSD)" + ";;" +
                        tr("All files") + " (*)",
                        tr("Export as..."));

    if (fileName.isEmpty())
        return ;

    const bool success = exportCsoundFile(fileName);

    if (success)
        setFileSaveAsDirectory(existingDir(fileName));
}

bool
RosegardenMainWindow::exportCsoundFile(const QString &file)
{
    // Progress Dialog
    // ??? The Csound export process is so fast, this never has a
    //     chance to appear.  Even with a huge composition.  I'm
    //     just going to leave this as an indeterminate progress
    //     dialog.  It can probably just be removed.
    QProgressDialog progressDialog(
            tr("Exporting Csound score file..."),  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 0,  // min, max
            this);  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // No sense in auto close since we will close anyway when
    // this object goes out of scope.
    progressDialog.setAutoClose(false);
    // Get rid of the cancel button for now.
    progressDialog.setCancelButton(nullptr);
    // Just force the progress dialog up.
    // Both Qt4 and Qt5 have bugs related to delayed showing of progress
    // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
    // based distros might lock up.  See Bug #1546.
    progressDialog.show();

    CsoundExporter csoundExporter(
            this,  // parent
            &RosegardenDocument::currentDocument->getComposition(),  // composition
            std::string(file.toLocal8Bit()));  // fileName

    if (!csoundExporter.write()) {
        QMessageBox::warning(this, tr("Rosegarden"),
                tr("Export failed.  The file could not be opened for writing."));
        return false;
    }

    return true;
}

void
RosegardenMainWindow::slotExportMup()
{
    TmpStatusMsg msg(tr("Exporting Mup file..."), this);

    QString fileName = launchSaveAsDialog
                       (tr("Mup files") + " (*.mup *.MUP)" + ";;" +
                        tr("All files") + " (*)",
                        tr("Export as..."));
    if (fileName.isEmpty())
        return ;

    const bool success = exportMupFile(fileName);

    if (success)
        setFileSaveAsDirectory(existingDir(fileName));
}

bool
RosegardenMainWindow::exportMupFile(const QString &file)
{
    // Progress Dialog
    QProgressDialog progressDialog(
            tr("Exporting Mup file..."),  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 0,  // min, max
            this);  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // No sense in auto close since we will close anyway when
    // this object goes out of scope.
    progressDialog.setAutoClose(false);
    // Get rid of the cancel button for now.
    progressDialog.setCancelButton(nullptr);
    // Just force the progress dialog up.
    // Both Qt4 and Qt5 have bugs related to delayed showing of progress
    // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
    // based distros might lock up.  See Bug #1546.
    progressDialog.show();

    MupExporter mupExporter(
            this,  // parent
            &RosegardenDocument::currentDocument->getComposition(),  // composition
            std::string(file.toLocal8Bit()));  // fileName

    if (!mupExporter.write()) {
        QMessageBox::warning(this, tr("Rosegarden"),
                tr("Export failed.  The file could not be opened for writing."));

        return false;
    }

    return true;
}

void
RosegardenMainWindow::slotExportLilyPond()
{
    TmpStatusMsg msg(tr("Exporting LilyPond file..."), this);

    QString fileName = launchSaveAsDialog
                       (tr("LilyPond files") + " (*.ly *.LY)" + ";;" +
                        tr("All files") + " (*)",
                        tr("Export as..."));

    if (fileName.isEmpty())
        return ;

    const bool success = exportLilyPondFile(fileName);

    if (success)
        setFileSaveAsDirectory(existingDir(fileName));
}


void
RosegardenMainWindow::slotPrintLilyPond()
{
    TmpStatusMsg msg(tr("Printing with LilyPond..."), this);

    QString filename = getLilyPondTmpFilename();
    if (filename.isEmpty()) return;

    if (!exportLilyPondFile(filename, true)) {
        return ;
    }

    LilyPondProcessor *dialog = new LilyPondProcessor(
            this, LilyPondProcessor::Mode::Print, filename);
    dialog->exec();
}

void
RosegardenMainWindow::slotPreviewLilyPond()
{
    TmpStatusMsg msg(tr("Previewing LilyPond file..."), this);

    QString filename = getLilyPondTmpFilename();
    if (filename.isEmpty()) return;

    if (!exportLilyPondFile(filename, true)) {
        return ;
    }

    LilyPondProcessor *dialog = new LilyPondProcessor(
            this, LilyPondProcessor::Mode::Preview, filename);
    dialog->exec();
}

QString
RosegardenMainWindow::getLilyPondTmpFilename()
{
    // ??? Can we combine with NotationView::getLilyPondTmpFilename()?  Maybe
    //     move them both to LilyPondProcess?

    QTemporaryFile file(QDir::tempPath() + "/rosegarden_tmp_XXXXXX.ly");

    // Must call open() to generate the guaranteed unique file name.
    if (!file.open()) {
        QMessageBox::warning(
                this,
                tr("Rosegarden"),
                tr("<qt><p>Failed to open a temporary file for LilyPond export.</p>"
                   "<p>This probably means you have run out of disk space on <pre>%1</pre></p></qt>").
                           arg(QDir::tempPath()));
        return "";
    }

    return file.fileName();
}

bool
RosegardenMainWindow::exportLilyPondFile(const QString &file, bool forPreview)
{
    QString caption;
    QString heading;

    if (forPreview) {
        caption = tr("LilyPond Preview Options");
        heading = tr("LilyPond preview options");
    }

    LilyPondOptionsDialog dialog(this, RosegardenDocument::currentDocument, caption, heading);
    if (dialog.exec() != QDialog::Accepted)
        return false;

    // Progress Dialog
    QProgressDialog progressDialog(
            tr("Exporting LilyPond file..."),  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 100,  // min, max
            this);  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // No sense in auto close since we will close anyway when
    // this object goes out of scope.
    progressDialog.setAutoClose(false);
    // Just force the progress dialog up.
    // Both Qt4 and Qt5 have bugs related to delayed showing of progress
    // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
    // based distros might lock up.  See Bug #1546.
    progressDialog.show();

    LilyPondExporter lilyPondExporter(
            RosegardenDocument::currentDocument,  // document
            m_view->getSelection(),  // selection
            std::string(QFile::encodeName(file)));  // fileName

    lilyPondExporter.setProgressDialog(&progressDialog);

    if (!lilyPondExporter.write()) {
        if (!progressDialog.wasCanceled()) {
            QMessageBox::warning(this, tr("Rosegarden"),
                    lilyPondExporter.getMessage());
        }

        return false;
    }

    return true;
}

void
RosegardenMainWindow::slotExportMusicXml()
{
    TmpStatusMsg msg(tr("Exporting MusicXML file..."), this);

    QString fileName = launchSaveAsDialog
                       (tr("XML files") + " (*.xml *.XML)" + ";;" +
                        tr("All files") + " (*)",
                        tr("Export as..."));

    if (fileName.isEmpty())
        return ;

    const bool success = exportMusicXmlFile(fileName);

    if (success)
        setFileSaveAsDirectory(existingDir(fileName));
}

void
RosegardenMainWindow::slotExportWAV()
{
    RG_DEBUG << "slotExportWAV()";

    if (!m_seqManager)
        return;

    if (!(m_seqManager->getSoundDriverStatus() & AUDIO_OK)) {
        QMessageBox::information(
                    this,  // parent
                    tr("Rosegarden"),  // title
                    tr("Unable to export WAV without JACK running."));  // text
        return;
    }

    QString fileName = FileDialog::getSaveFileName(
            this,  // parent
            tr("Rosegarden"),  // caption
            "",  // dir
            "",  // defaultName
            tr("WAV files") + " (*.wav)");  // filter

    if (fileName.isEmpty())
        return;

    if (fileName.right(4).toLower() != ".wav")
        fileName += ".wav";

    QString msg = tr(
            "Press play to start exporting to\n"
            "%1\n"
            "Press stop to stop export.\n"
            "Only audio and synth plugin tracks will be exported").arg(fileName);

    QMessageBox::information(
            this,  // parent
            tr("Rosegarden"),  // title
            msg);  // text

    m_seqManager->setExportWavFile(fileName);
}

bool
RosegardenMainWindow::exportMusicXmlFile(const QString &file)
{
    MusicXMLOptionsDialog dialog(this, RosegardenDocument::currentDocument, "", "");

    if (dialog.exec() != QDialog::Accepted)
        return false;

    // Progress Dialog
    // Note: Label text will be set later in the process.
    QProgressDialog progressDialog(
            "...",  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 100,  // min, max
            this);  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // No sense in auto close since we will close anyway when
    // this object goes out of scope.
    progressDialog.setAutoClose(false);
    // Get rid of the cancel button for now.
    progressDialog.setCancelButton(nullptr);
    // Just force the progress dialog up.
    // Both Qt4 and Qt5 have bugs related to delayed showing of progress
    // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
    // based distros might lock up.  See Bug #1546.
    progressDialog.show();

    MusicXmlExporter musicXmlExporter(
            this,  // parent
            RosegardenDocument::currentDocument,  // document
            std::string(file.toLocal8Bit()));  // fileName

    musicXmlExporter.setProgressDialog(&progressDialog);

    if (!musicXmlExporter.write()) {
        QMessageBox::warning(this,
                tr("Rosegarden"),
                tr("Export failed.  The file could not be opened for writing."));

        return false;
    }

    return true;
}

void
RosegardenMainWindow::slotCloseTransport()
{
    findAction("show_transport")->setChecked(false);
    slotUpdateTransportVisibility();
}

void
RosegardenMainWindow::slotDeleteTransport()
{
    delete m_transport;
    m_transport = nullptr;
}

void
RosegardenMainWindow::slotActivateTool(const QString& toolName)
{
    if (toolName == SegmentSelector::ToolName()) {
        findAction("select")->trigger();
    }
}

void
RosegardenMainWindow::slotToggleMetronome()
{
    Composition &comp = RosegardenDocument::currentDocument->getComposition();

    if (m_seqManager->getTransportStatus() == STARTING_TO_RECORD ||
            m_seqManager->getTransportStatus() == RECORDING ||
            m_seqManager->getTransportStatus() == RECORDING_ARMED) {
        if (comp.useRecordMetronome())
            comp.setRecordMetronome(false);
        else
            comp.setRecordMetronome(true);

        getTransport()->MetronomeButton()->setChecked(comp.useRecordMetronome());
    } else {
        if (comp.usePlayMetronome())
            comp.setPlayMetronome(false);
        else
            comp.setPlayMetronome(true);

        getTransport()->MetronomeButton()->setChecked(comp.usePlayMetronome());
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
    RG_DEBUG << "slotSetPlayPosition(" << time << ")";
    if (m_seqManager->getTransportStatus() == RECORDING)
        return ;

    RosegardenDocument::currentDocument->slotSetPointerPosition(time);

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
        // Punch-In
        slotToggleRecord();
        return ;
    }

    // Attempt to start recording
    //
    try {
        m_seqManager->record(false);
    } catch (const QString &s) {
        // We should already be stopped by this point so just unset
        // the buttons after clicking the dialog.
        //
        QMessageBox::critical(this, tr("Rosegarden"), s);

        getTransport()->MetronomeButton()->setChecked(false);
        getTransport()->RecordButton()->setChecked(false);
        getTransport()->PlayButton()->setChecked(false);
        return ;
    } catch (const AudioFileManager::BadAudioPathException &e) {
            if (QMessageBox::warning
                    (nullptr, tr("Warning"),
                 tr("The audio file path does not exist or is not writable.\nPlease set the audio file path to a valid directory in Document Properties before recording audio.\nWould you like to set it now?"),
                      QMessageBox::Yes | QMessageBox::Cancel,
                               QMessageBox::Cancel) // default button
                == QMessageBox::Yes)
            {
                openAudioPathSettings();
            }//end if

        getTransport()->MetronomeButton()->setChecked(false);
        getTransport()->RecordButton()->setChecked(false);
        getTransport()->PlayButton()->setChecked(false);
        return ;
    } catch (const Exception &e) {
        QMessageBox::critical(this, tr("Rosegarden"), strtoqstr(e.getMessage()));

        getTransport()->MetronomeButton()->setChecked(false);
        getTransport()->RecordButton()->setChecked(false);
        getTransport()->PlayButton()->setChecked(false);
        return ;
    }

    connect(m_seqManager->getCountdownDialog(), &CountdownDialog::stopped,
            this, &RosegardenMainWindow::slotStop);
}

void
RosegardenMainWindow::slotToggleRecord()
{
    // Not using sequencer?  Bail.
    if (!m_useSequencer)
        return;

    // If the sequencer thread isn't running, try launching it.
    if (!isSequencerRunning()  &&  !launchSequencer())
        return;

    // If we are stopped, start recording.
    // This is to satisfy the MIDI spec description of MMC RECORD STROBE.
    // RECORD STROBE needs to punch-in/out and it needs to start recording
    // if the transport is stopped.
    if (m_seqManager->getTransportStatus() == STOPPED) {
        slotRecord();
        return;
    }

    try {

        m_seqManager->record(true);

    } catch (const QString &s) {
        QMessageBox::critical(this, tr("Rosegarden"), s);
    } catch (const AudioFileManager::BadAudioPathException &e) {
        if (QMessageBox::warning
            (this, tr("Error"),
                 tr("The audio file path does not exist or is not writable.\nPlease set the audio file path to a valid directory in Document Properties before you start to record audio.\nWould you like to set it now?"),
                      QMessageBox::Yes | QMessageBox::Cancel,
                       QMessageBox::Cancel
               ) == QMessageBox::Yes
       ){
            //tr("Set audio file path")) == QMessageBox::Continue) {
        openAudioPathSettings();
        }
    } catch (const Exception &e) {
        QMessageBox::critical(this, tr("Rosegarden"),  strtoqstr(e.getMessage()));
    }

}

void
RosegardenMainWindow::slotLoopChanged()
{
    Composition &composition =
        RosegardenDocument::currentDocument->getComposition();

    // ??? Should RD do this on its own in response to loopChanged()?
    RosegardenDocument::currentDocument->slotDocumentModified();

    // If the user can see the loop range, let them edit with it.

    // Advanced Looping
    if (Preferences::getAdvancedLooping()) {
        // With advanced looping, the range is always visible.
        if (composition.getLoopStart() != composition.getLoopEnd()) {
            enterActionState("have_range");
        } else {
            leaveActionState("have_range");
        }
    } else {  // Classic Looping
        // With classic looping, the range is only visible with LoopOn
        if (composition.getLoopMode() == Composition::LoopOn  &&
            composition.getLoopStart() != composition.getLoopEnd()) {
            enterActionState("have_range");
        } else {
            leaveActionState("have_range");
        }
    }

    findAction("loop")->setChecked(
            (composition.getLoopMode() != Composition::LoopOff));
}

bool
RosegardenMainWindow::isUsingSequencer()
{
    return m_useSequencer;
}

bool
RosegardenMainWindow::isSequencerRunning()
{
    //RG_DEBUG << "isSequencerRunning: m_useSequencer = "
    //         << m_useSequencer << ", m_sequencerThread = " << m_sequencerThread;
    return m_useSequencer && (m_sequencerThread != nullptr);
}

void
RosegardenMainWindow::slotPlay()
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

    try {
        m_seqManager->play(); // this will stop playback (pause) if it's already running
    } catch (const QString &s) {
        QMessageBox::critical(this, tr("Rosegarden"), s);
    } catch (const Exception &e) {
        QMessageBox::critical(this, tr("Rosegarden"), strtoqstr(e.getMessage()));
    }
}

void
RosegardenMainWindow::slotJumpToTime(RealTime rt)
{
    Composition *comp = &RosegardenDocument::currentDocument->getComposition();
    timeT t = comp->getElapsedTimeForRealTime(rt);
    RosegardenDocument::currentDocument->slotSetPointerPosition(t);
}

void
RosegardenMainWindow::slotStartAtTime(const RealTime& rt)
{
    slotJumpToTime(rt);
    slotPlay();
}

void
RosegardenMainWindow::slotStop()
{
    doStop(false);
}

void
RosegardenMainWindow::doStop(bool autoStop)
{
    if (m_seqManager &&
        m_seqManager->getCountdownDialog()) {
        disconnect(m_seqManager->getCountdownDialog(), &CountdownDialog::stopped,
                   this, &RosegardenMainWindow::slotStop);
        disconnect(m_seqManager->getCountdownDialog(), &CountdownDialog::completed,
                   this, &RosegardenMainWindow::slotStop);
    }

    try {
        if (m_seqManager)
            m_seqManager->stop(autoStop);
    } catch (const Exception &e) {
        QMessageBox::critical(this, tr("Rosegarden"), strtoqstr(e.getMessage()));
    }
}

void
RosegardenMainWindow::slotRewind()
{
    if (!m_seqManager)
        return;

    // Rewind is not allowed when recording.
    if (m_seqManager->getTransportStatus() == RECORDING)
        return;

    m_seqManager->rewind();
}

void
RosegardenMainWindow::slotFastforward()
{
    if (!m_seqManager)
        return;

    // Fast Forward is not allowed when recording.
    if (m_seqManager->getTransportStatus() == RECORDING)
        return;

    m_seqManager->fastforward();
}

void
RosegardenMainWindow::toggleLoop()
{
    RosegardenDocument *document = RosegardenDocument::currentDocument;
    Composition &composition = document->getComposition();

    // No loop to toggle?  Bail.
    if (composition.getLoopStart() == composition.getLoopEnd())
        return;

    // Toggle the loop.
    if (composition.getLoopMode() == Composition::LoopOff)
        composition.setLoopMode(Composition::LoopOn);
    else  // LoopOn or LoopAll -> off
        composition.setLoopMode(Composition::LoopOff);

    emit document->loopChanged();
}

void
RosegardenMainWindow::slotToggleSolo(bool)
{
    // Delegate to TrackButtons.
    getView()->getTrackEditor()->getTrackButtons()->toggleSolo();
}

void
RosegardenMainWindow::slotToggleSoloCurrentTrack()
{
    // Delegate to TrackButtons.
    getView()->getTrackEditor()->getTrackButtons()->toggleSolo();
}

void
RosegardenMainWindow::slotSelectPreviousTrack()
{
    if (!RosegardenDocument::currentDocument)
        return;

    Composition &comp = RosegardenDocument::currentDocument->getComposition();

    TrackId tid = comp.getSelectedTrack();
    int pos = comp.getTrackById(tid)->getPosition();

    // If at top already
    if (pos == 0)
        return ;

    Track *track = comp.getTrackByPosition(pos - 1);

    if (!track)
        return;

    comp.setSelectedTrack(track->getId());
    comp.notifyTrackSelectionChanged(comp.getSelectedTrack());
    if (m_view)
        m_view->slotSelectTrackSegments(comp.getSelectedTrack());

    RosegardenDocument::currentDocument->emitDocumentModified();
}

void
RosegardenMainWindow::slotSelectNextTrack()
{
    if (!RosegardenDocument::currentDocument)
        return;

    Composition &comp = RosegardenDocument::currentDocument->getComposition();

    TrackId tid = comp.getSelectedTrack();
    int pos = comp.getTrackById(tid)->getPosition();

    Track *track = comp.getTrackByPosition(pos + 1);

    if (!track)
        return;

    comp.setSelectedTrack(track->getId());
    comp.notifyTrackSelectionChanged(comp.getSelectedTrack());
    if (m_view)
        m_view->slotSelectTrackSegments(comp.getSelectedTrack());

    RosegardenDocument::currentDocument->emitDocumentModified();
}

void
RosegardenMainWindow::muteAllTracks(bool mute)
{
    //RG_DEBUG << "muteAllTracks()";

    if (!RosegardenDocument::currentDocument)
        return;

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    Composition::TrackMap tracks = comp.getTracks();

    for (Composition::TrackMap::iterator trackIt = tracks.begin();
            trackIt != tracks.end(); ++trackIt) {
        Track *track = trackIt->second;

        if (!track)
            continue;

        // Mute or unmute the track
        track->setMuted(mute);

        // Notify observers
        comp.notifyTrackChanged(track);
    }

    RosegardenDocument::currentDocument->slotDocumentModified();
}

void
RosegardenMainWindow::slotMuteAllTracks()
{
    muteAllTracks();
}

void
RosegardenMainWindow::slotUnmuteAllTracks()
{
    muteAllTracks(false);
}

void
RosegardenMainWindow::slotToggleMute()
{
    if (!RosegardenDocument::currentDocument)
        return;

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    Track *track = comp.getTrackById(comp.getSelectedTrack());

    if (!track)
        return;

    // Toggle the mute state
    track->setMuted(!track->isMuted());

    // Notify observers
    comp.notifyTrackChanged(track);
    RosegardenDocument::currentDocument->slotDocumentModified();
}

void
RosegardenMainWindow::slotToggleRecordCurrentTrack()
{
    if (!RosegardenDocument::currentDocument)
        return;

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    TrackId tid = comp.getSelectedTrack();

    Track *track = comp.getTrackById(tid);

    if (!track)
        return;

    // Toggle
    bool state = !comp.isTrackRecording(tid);

    // Update the Track
    comp.setTrackRecording(tid, state);
    comp.notifyTrackChanged(track);

    RosegardenDocument::currentDocument->checkAudioPath(track);
}

void
RosegardenMainWindow::slotConfigureShortcuts()
{
    RG_DEBUG << "slotConfigureShortcuts";
    ShortcutDialog sdlg(this);
    sdlg.exec();
}

void
RosegardenMainWindow::slotConfigure()
{
    RG_DEBUG << "slotConfigure\n";

    if (!m_configDlg) {
        m_configDlg = new ConfigureDialog(RosegardenDocument::currentDocument, this);

        connect(m_configDlg, &ConfigureDialog::updateAutoSaveInterval,
                this, &RosegardenMainWindow::slotUpdateAutoSaveInterval);

        // Close the dialog if the document is changed : fix a potential crash
        connect(this, &RosegardenMainWindow::documentAboutToChange,
                m_configDlg, &ConfigureDialog::slotCancelOrClose);

        // Clear m_configDlg if the dialog is destroyed
        connect(m_configDlg, &QObject::destroyed,
                this, &RosegardenMainWindow::slotResetConfigDlg);

        m_configDlg->show();
    }
}

void
RosegardenMainWindow::slotResetConfigDlg()
{
    // The configuration dialog has been closed.
    // Qt should have removed the connections.
    m_configDlg = nullptr;
}

void
RosegardenMainWindow::slotEditDocumentProperties()
{
    RG_DEBUG << "slotEditDocumentProperties\n";

    // Don't create a dialog if there is already one
    if (!m_docConfigDlg) {
        m_docConfigDlg = new DocumentConfigureDialog(this);

        // Close the dialog if the document is changed : fix #1462
        connect(this, &RosegardenMainWindow::documentAboutToChange,
                m_docConfigDlg, &DocumentConfigureDialog::slotCancelOrClose);

        // Clear m_docConfigDlg if the dialog is destroyed
        connect(m_docConfigDlg, &QObject::destroyed,
                this, &RosegardenMainWindow::slotResetDocConfigDlg);
    }

    m_docConfigDlg->show();
}

void
RosegardenMainWindow::openAudioPathSettings()
{
    RG_DEBUG << "slotOpenAudioPathSettings\n";

    // Don't create a dialog if there is already one
    if (!m_docConfigDlg) {
        m_docConfigDlg = new DocumentConfigureDialog(this);

        // Close the dialog if the document is changed : fix #1462
        connect(this, &RosegardenMainWindow::documentAboutToChange,
                m_docConfigDlg, &DocumentConfigureDialog::slotCancelOrClose);

        // Clear m_docConfigDlg if the dialog is destroyed
        connect(m_docConfigDlg, &QObject::destroyed,
                this, &RosegardenMainWindow::slotResetDocConfigDlg);
    }

    m_docConfigDlg->showAudioPage();
    m_docConfigDlg->show();
}

void
RosegardenMainWindow::slotResetDocConfigDlg()
{
    // The document configuration dialog has been closed.
    // Qt should have removed the connections.
    m_docConfigDlg = nullptr;
}

/* unused
void
RosegardenMainWindow::slotUpdateToolbars()
{
    findAction("show_stock_toolbar")->setChecked(!(findToolbar("Main Toolbar")->isHidden()));
}
*/

void
RosegardenMainWindow::slotEditTempo()
{
    slotEditTempo(this);
}

void
RosegardenMainWindow::slotEditTempo(timeT atTime)
{
    slotEditTempo(this, atTime);
}

void
RosegardenMainWindow::slotEditTempo(QWidget *parent)
{
    slotEditTempo(parent, RosegardenDocument::currentDocument->getComposition().getPosition());
}

void
RosegardenMainWindow::slotEditTempo(QWidget *parent, timeT atTime)
{
    RG_DEBUG << "slotEditTempo";
    EditTempoController::self()->editTempo(
            parent,
            atTime,
            false);  // timeEditable
}

void
RosegardenMainWindow::slotEditTimeSignature()
{
    slotEditTimeSignature(this);
}

void
RosegardenMainWindow::slotEditTimeSignature(timeT atTime)
{
    slotEditTimeSignature(this, atTime);
}

void
RosegardenMainWindow::slotEditTimeSignature(QWidget *parent)
{
    slotEditTimeSignature(parent, RosegardenDocument::currentDocument->getComposition().getPosition());
}

void
RosegardenMainWindow::slotEditTimeSignature(QWidget *parent,
        timeT atTime)
{
    EditTempoController::self()->editTimeSignature(parent, atTime);
}

void
RosegardenMainWindow::slotEditTransportTime()
{
    slotEditTransportTime(this);
}

void
RosegardenMainWindow::slotEditTransportTime(QWidget *parent)
{
    TimeDialog dialog(
            parent,
            tr("Move playback pointer to time"),  // title
            RosegardenDocument::currentDocument->getComposition().getPosition(),  // defaultTime
            true);  // constrainToCompositionDuration
    if (dialog.exec() == QDialog::Accepted) {
        RosegardenDocument::currentDocument->slotSetPointerPosition(dialog.getTime());
    }
}

void
RosegardenMainWindow::slotChangeZoom(int)
{
    double duration44 = TimeSignature(4, 4).getBarDuration();
    double value = double(m_zoomSlider->getCurrentSize());
    m_zoomLabel->setText(tr("%1%").arg(duration44 / value));

    //RG_DEBUG << "slotChangeZoom(): zoom size = " << m_zoomSlider->getCurrentSize();

    // initZoomToolbar sets the zoom value. With some old versions of
    // Qt3.0, this can cause slotChangeZoom() to be called while the
    // view hasn't been initialized yet, so we need to check it's not
    // null
    //
    if (m_view)
        m_view->setZoomSize(m_zoomSlider->getCurrentSize());

    long newZoom = int(m_zoomSlider->getCurrentSize() * 1000.0);

    if (RosegardenDocument::currentDocument->getConfiguration().get<Int>
            (DocumentConfiguration::ZoomLevel) != newZoom) {

        RosegardenDocument::currentDocument->getConfiguration().set<Int>
        (DocumentConfiguration::ZoomLevel, newZoom);

        RosegardenDocument::currentDocument->slotDocumentModified();
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
RosegardenMainWindow::slotAddMarker(timeT time)
{
    AddMarkerCommand *command =
        new AddMarkerCommand(&RosegardenDocument::currentDocument->getComposition(),
                             time,
                            qStrToStrUtf8(tr("new marker")),
                            qStrToStrUtf8(tr("no description")) );

    CommandHistory::getInstance()->addCommand(command);
}

void
RosegardenMainWindow::slotDeleteMarker(int id,
                                       timeT time,
                                       const QString& name,
                                       const QString& description)
{
    RemoveMarkerCommand *command =
        new RemoveMarkerCommand(&RosegardenDocument::currentDocument->getComposition(),
                                id,
                                time,
                                qstrtostr(name),
                                qstrtostr(description));

    CommandHistory::getInstance()->addCommand(command);
}

void
RosegardenMainWindow::slotDocumentModified(bool modified)
{
    RG_DEBUG << "slotDocumentModified(" << modified << ") - doc path = " << RosegardenDocument::currentDocument->getAbsFilePath();

    if (!RosegardenDocument::currentDocument->getAbsFilePath().isEmpty()) {
        slotStateChanged("saved_file_modified", modified);
    } else {
        slotStateChanged("new_file_modified", modified);
    }
}

void
RosegardenMainWindow::slotStateChanged(const QString& s,
                                       bool noReverse)
{
    if (noReverse) {
        enterActionState(s);
    } else {
        leaveActionState(s);
    }
}

void
RosegardenMainWindow::updateActions()
{
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    bool enableEditingDuringPlayback =
            settings.value("enableEditingDuringPlayback", false).toBool();


    // not_playing && have_selection

    findAction("delete")->setEnabled(
            (enableEditingDuringPlayback || m_notPlaying)  &&  m_haveSelection);
    findAction("edit_cut")->setEnabled(
            (enableEditingDuringPlayback || m_notPlaying)  &&  m_haveSelection);
    // ??? This doesn't prevent Ctrl+Resize on the edges of a Segment.  CRASH.
    findAction("rescale")->setEnabled(m_notPlaying  &&  m_haveSelection);
    findAction("auto_split")->setEnabled(
            (enableEditingDuringPlayback || m_notPlaying)  &&  m_haveSelection);
    findAction("split_by_pitch")->setEnabled(
            (enableEditingDuringPlayback || m_notPlaying)  &&  m_haveSelection);
    findAction("split_by_recording")->setEnabled(
            (enableEditingDuringPlayback || m_notPlaying)  &&  m_haveSelection);
    // ??? This doesn't prevent the split tool from causing a CRASH.
    findAction("split_at_time")->setEnabled(
            (enableEditingDuringPlayback || m_notPlaying)  &&  m_haveSelection);
    findAction("split_by_drum")->setEnabled(
            (enableEditingDuringPlayback || m_notPlaying)  &&  m_haveSelection);
    findAction("join_segments")->setEnabled(m_notPlaying  &&  m_haveSelection);


    // not_playing && have_range

    findAction("cut_range")->setEnabled(m_notPlaying  &&  m_haveRange);
}

void
RosegardenMainWindow::enterActionState(QString stateName)
{
   if (stateName == "not_playing") {
      m_notPlaying = true;
      CommandHistory::getInstance()->enableUndo(true);
   }

   if (stateName == "have_selection")
      m_haveSelection = true;
   if (stateName == "have_range")
      m_haveRange = true;

   updateActions();

   // Let baseclass take a shot as well.
   ActionFileClient::enterActionState(stateName);
}

void
RosegardenMainWindow::leaveActionState(QString stateName)
{
   if (stateName == "not_playing") {
      m_notPlaying = false;

      QSettings settings;
      settings.beginGroup(GeneralOptionsConfigGroup);
      bool enableEditingDuringPlayback =
              settings.value("enableEditingDuringPlayback", false).toBool();

      if (!enableEditingDuringPlayback)
          CommandHistory::getInstance()->enableUndo(false);
   }

   if (stateName == "have_selection")
      m_haveSelection = false;
   if (stateName == "have_range")
      m_haveRange = false;

   updateActions();

   // Let baseclass take a shot as well.
   ActionFileClient::leaveActionState(stateName);
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
        //leaveActionState("have_clipboard_can_paste_as_links");
    } else {
        enterActionState("have_clipboard");
        if (m_clipboard->isSingleSegment()) {  //@@@ JAS orig. KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse
            enterActionState("have_clipboard_single_segment");
        } else {
            leaveActionState("have_clipboard_single_segment");
        }
        //if (m_clipboard->getCanPasteAsLinks()) {
        //    enterActionState("have_clipboard_can_paste_as_links");
        //} else {
        //    leaveActionState("have_clipboard_can_paste_as_links");
        //}
    }
}

QVector<QString>
RosegardenMainWindow::createRecordAudioFiles(const QVector<InstrumentId> &recordInstruments)
{
    QVector<QString> qv;

    // Refuse to start recording to "Untitled" for the user's own good.  (Want
    // proof?  Look at my 3.9G ~/rosegarden full of meaningless files not
    // associated with anything particular.  The ones since 2005, at least I
    // know the date I recorded them, but that isn't very helpful.)
    //
    // I weighed the user inconvenience carefully, and decided to do this to
    // maximize the value of the new filenames that include the title and
    // instrument alias.  It's worthless if they all say "Untitled" and there's
    // no way to make "pick this and set that up before you hit this" intuitive,
    // so I've had to throw instructions in their face.

    // If the user has not yet saved the composition, let them know that
    // they need to save first.  Otherwise all of their audio files will
    // be called "Untitled".
    if (RosegardenDocument::currentDocument->getTitle() == tr("Untitled")) {
        QMessageBox::StandardButton selected = QMessageBox::information(
                this,
                tr("Rosegarden"),
        // TRANSLATOR: you may change "doc:audio-filename-en" to a page in your
        // language if you wish.  The n in <i>n</i>.wav refers to an unknown
        // number, such as might be used in a mathematical equation
                tr("<qt><p>You must choose a filename for this composition before recording audio.</p><p>Audio files will be saved to <b>%1</b> as <b>rg-[<i>filename</i>]-[<i>instrument</i>]-<i>date</i>_<i>time</i>-<i>n</i>.wav</b>.  You may wish to rename audio instruments before recording as well.  For more information, please see the <a style=\"color:gold\" href=\"http://www.rosegardenmusic.com/wiki/doc:audio-filenames-en\">Rosegarden Wiki</a>.</p></qt>")
                        .arg(RosegardenDocument::currentDocument->getAudioFileManager().getAbsoluteAudioPath()),
                QMessageBox::Ok | QMessageBox::Cancel,
                QMessageBox::Ok);

        // User wants to cancel?  Abort the recording.
        if (selected == QMessageBox::Cancel)
            return qv;

        slotFileSave();

        // If they cancelled the save, bail.
        if (RosegardenDocument::currentDocument->getTitle() == tr("Untitled"))
            return qv;
    }

    for (int i = 0; i < recordInstruments.size(); ++i) {
        AudioFile *aF = nullptr;
        try {
            std::string alias = "";
            Instrument *ins = RosegardenDocument::currentDocument->getStudio().getInstrumentById(recordInstruments[i]);
            if (ins) alias = ins->getAlias();
            aF = RosegardenDocument::currentDocument->getAudioFileManager().createRecordingAudioFile(RosegardenDocument::currentDocument->getTitle(),
                                                                       strtoqstr(alias));
            if (aF) {
                // createRecordingAudioFile doesn't actually write to the disk,
                // and in principle it shouldn't fail
                qv.push_back(aF->getAbsoluteFilePath());
                RosegardenDocument::currentDocument->addRecordAudioSegment(recordInstruments[i],
                                             aF->getId());
            } else {
                RG_WARNING << "createRecordAudioFiles(): WARNING: Failed to create recording audio file";
                return qv;
            }
        } catch (const AudioFileManager::BadAudioPathException &e) {
            delete aF;
            RG_WARNING << "createRecordAudioFiles(): ERROR: Failed to create recording audio file: " << e.getMessage();
            return qv;
        }
    }
    return qv;
}

/* unused
QString
RosegardenMainWindow::getAudioFilePath()
{
    return RosegardenDocument::currentDocument->getAudioFileManager().getAbsoluteAudioPath();
}
*/

QVector<InstrumentId>
RosegardenMainWindow::getArmedInstruments()
{
    std::set
        <InstrumentId> iid;

    const Composition::TrackIdSet &tr =
        RosegardenDocument::currentDocument->getComposition().getRecordTracks();

    for (Composition::TrackIdSet::const_iterator i =
                tr.begin(); i != tr.end(); ++i) {
        TrackId tid = (*i);
        Track *track = RosegardenDocument::currentDocument->getComposition().getTrackById(tid);
        if (track) {
            iid.insert(track->getInstrument());
        } else {
            RG_WARNING << "getArmedInstruments(): Warning: Armed track " << tid << " not found in Composition";
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

    // This is principally used for return values from DSSI plugin
    // configure() calls.  It seems some plugins return a string
    // telling you when everything's OK, as well as error strings, but
    // dssi.h does make it reasonably clear that configure() should
    // only return a string when there is actually a problem, so we're
    // going to stick with a warning dialog here rather than an
    // information one

    QMessageBox::warning(nullptr, tr("Rosegarden"), error);
}

void
RosegardenMainWindow::slotAudioManager()
{
    if (m_audioManagerDialog) {
        m_audioManagerDialog->show();
        m_audioManagerDialog->raise();
        m_audioManagerDialog->activateWindow();
        return ;
    }

    m_audioManagerDialog =
        new AudioManagerDialog(this, RosegardenDocument::currentDocument);

    if (m_view->haveSelection()) {
        SegmentSelection selection(m_view->getSelection());
        m_audioManagerDialog->slotSegmentSelection(selection);
    }
    connect(m_audioManagerDialog, &AudioManagerDialog::playAudioFile,
            this, &RosegardenMainWindow::slotPlayAudioFile);

    connect(m_audioManagerDialog,
            static_cast<void(AudioManagerDialog::*)(AudioFileId)>(
                    &AudioManagerDialog::addAudioFile),
            this, &RosegardenMainWindow::slotAddAudioFile);

    connect(m_audioManagerDialog,
            &AudioManagerDialog::deleteAudioFile,
            this, &RosegardenMainWindow::slotDeleteAudioFile);

    //
    // Sync segment selection between audio man. dialog and main window
    //

    // from dialog to us...
    connect(m_audioManagerDialog,
            &AudioManagerDialog::segmentsSelected,
            m_view,
            &RosegardenMainViewWidget::slotPropagateSegmentSelection);

    // and from us to dialog
    connect(this, &RosegardenMainWindow::segmentsSelected,
            m_audioManagerDialog,
            &AudioManagerDialog::slotSegmentSelection);


    connect(m_audioManagerDialog,
            &AudioManagerDialog::deleteSegments,
            this, &RosegardenMainWindow::slotDeleteSegments);

    connect(m_audioManagerDialog,
            &AudioManagerDialog::insertAudioSegment,
            m_view,
            &RosegardenMainViewWidget::slotAddAudioSegmentDefaultPosition);
    connect(m_audioManagerDialog,
            &AudioManagerDialog::cancelPlayingAudioFile,
            this, &RosegardenMainWindow::slotCancelAudioPlayingFile);

    connect(m_audioManagerDialog,
            &AudioManagerDialog::deleteAllAudioFiles,
            this, &RosegardenMainWindow::slotDeleteAllAudioFiles);

    // Make sure we know when the audio man. dialog is closing
    //
    connect(m_audioManagerDialog,
            &AudioManagerDialog::closing,
            this, &RosegardenMainWindow::slotAudioManagerClosed);

    // And that it goes away when the current document is changing
    //
    connect(this, &RosegardenMainWindow::documentAboutToChange,
            m_audioManagerDialog, &QWidget::close);

    m_audioManagerDialog->setAudioSubsystemStatus(
        m_seqManager->getSoundDriverStatus() & AUDIO_OK);

    m_audioManagerDialog->show();
}

void
RosegardenMainWindow::slotPlayAudioFile(unsigned int id,
                                    const RealTime &startTime,
                                    const RealTime &duration)
{
    // Make sure id is valid.
    const AudioFile *aF = RosegardenDocument::currentDocument->
            getAudioFileManager().getAudioFile(id);
    if (!aF)
        return;

    MappedEvent mE;
    mE.setType(MappedEvent::Audio);
    mE.setInstrumentId(RosegardenDocument::currentDocument->getStudio().
            getAudioPreviewInstrument());
    mE.setAudioFileID(id);
    mE.setEventTime(RealTime(-120, 0));  // two minutes into the past
    mE.setDuration(duration);
    mE.setAudioStartMarker(startTime);

    StudioControl::sendMappedEvent(mE);
}

void
RosegardenMainWindow::slotAddAudioFile(unsigned int id)
{
    const AudioFile *aF = RosegardenDocument::currentDocument->
            getAudioFileManager().getAudioFile(id);
    if (!aF)
        return;

    const int result = RosegardenSequencer::getInstance()->addAudioFile(
            aF->getAbsoluteFilePath(), aF->getId());

    if (!result) {
        QMessageBox::critical(this, tr("Rosegarden"), tr("Sequencer failed to add audio file %1").arg(aF->getAbsoluteFilePath()));
    }
}

void
RosegardenMainWindow::slotDeleteAudioFile(unsigned int id)
{
    if (RosegardenDocument::currentDocument->getAudioFileManager().removeFile(id) == false)
        return ;

    int result = RosegardenSequencer::getInstance()->removeAudioFile(id);

    if (!result) {
        QMessageBox::critical(this, tr("Rosegarden"), tr("Sequencer failed to remove audio file id %1").arg(id));
    }
}

void
RosegardenMainWindow::slotDeleteSegments(const SegmentSelection &selection)
{
    m_view->slotPropagateSegmentSelection(selection);
    slotDeleteSelectedSegments();
}

void
RosegardenMainWindow::slotCancelAudioPlayingFile(AudioFileId audioFileID)
{
    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    if (!doc)
        return;

    // Make sure the audio file ID is valid.
    if (!doc->getAudioFileManager().getAudioFile(audioFileID))
        return;

    MappedEvent mE;
    mE.setInstrumentId(doc->getStudio().getAudioPreviewInstrument());
    mE.setType(MappedEvent::AudioCancel);
    mE.setData1(audioFileID);

    StudioControl::sendMappedEvent(mE);
}

void
RosegardenMainWindow::slotDeleteAllAudioFiles()
{
    // Clear at the sequencer
    RosegardenSequencer::getInstance()->clearAllAudioFiles();
}

void
RosegardenMainWindow::slotRepeatingSegments()
{
    m_view->getTrackEditor()->turnRepeatingSegmentToRealCopies();
}

void
RosegardenMainWindow::slotLinksToCopies()
{
    m_view->getTrackEditor()->turnLinkedSegmentsToRealCopies();
}

void
RosegardenMainWindow::slotRelabelSegments()
{
    if (!m_view->haveSelection())
        return;

    SegmentSelection selection(m_view->getSelection());

    // Generate default label based on the first selected Segment.
    QString oldLabel = strtoqstr((*selection.begin())->getLabel());

    // For each Segment in the SegmentSelection...
    for (Segment *segment : selection) {
        // If they don't all have the same name, go with blank.
        if (strtoqstr(segment->getLabel()) != oldLabel) {
            oldLabel = "";
            break;
        }
    }

    bool ok = false;

    QString newLabel = InputDialog::getText(
            dynamic_cast<QWidget*>(this),  // parent
            tr("Relabel Segment"),  // title
            tr("New segment label"),  // label
            LineEdit::Normal,  // mode
            oldLabel,  // text
            &ok);

    if (ok) {
        CommandHistory::getInstance()->addCommand(
                new SegmentLabelCommand(selection, newLabel));
        m_view->getTrackEditor()->getCompositionView()->slotUpdateAll();
    }
}

void
RosegardenMainWindow::slotTransposeSegments()
{
    if (!m_view->haveSelection()) return ;

    IntervalDialog intervalDialog(this, true, true);
    int ok = intervalDialog.exec();

    int semitones = intervalDialog.getChromaticDistance();
    int steps = intervalDialog.getDiatonicDistance();

    if (!ok || (semitones == 0 && steps == 0)) return;

    CommandHistory::getInstance()->addCommand(
            new SegmentTransposeCommand(
                    m_view->getSelection(),
                    intervalDialog.getChangeKey(),
                    steps,
                    semitones,
                    intervalDialog.getTransposeSegmentBack()));
}

void
RosegardenMainWindow::slotTransposeSemitones()
{
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);

    int lastTranspose = settings.value("main_last_transpose", 0).toInt() ;

    bool ok = false;
    int semitones = QInputDialog::getInt(
            this,  // parent
            tr("Transpose"),  // title
            tr("By number of semitones: "),  // label
            lastTranspose,  // value
            -127,  // minValue
            127,  // maxValue
            1,  // step
            &ok);

    if (!ok  ||  semitones == 0)
        return;

    settings.setValue("main_last_transpose", semitones);

    SegmentSelection selection = m_view->getSelection();

    MacroCommand *command = new MacroCommand(TransposeCommand::getGlobalName());

    // for each selected Segment
    for (SegmentSelection::iterator i = selection.begin();
         i != selection.end();
         ++i) {

        Segment &segment = **i;

        // Create an EventSelection with all events in the Segment.
        // ??? MEMORY LEAK (confirmed)  TransposeCommand stores a pointer
        //     to this, so we can't delete it.  Annoying.
        //     Make TransposeCommand take a QSharedPointer<EventSelection>.
        //     Then it could hang on to it as long as it likes.  It can even
        //     call reset() when it is really finished if it wants.
        EventSelection *eventSelection = new EventSelection(
                    segment,
                    segment.getStartTime(),
                    segment.getEndMarkerTime());

        command->addCommand(new TransposeCommand(semitones, *eventSelection));

    }

    m_view->slotAddCommandToHistory(command);
}

void
RosegardenMainWindow::slotChangeCompositionLength()
{
    CompositionLengthDialog dialog(this, &RosegardenDocument::currentDocument->getComposition());

    if (dialog.exec() == QDialog::Accepted) {
        ChangeCompositionLengthCommand *command
        = new ChangeCompositionLengthCommand(
              &RosegardenDocument::currentDocument->getComposition(),
              dialog.getStartMarker(),
              dialog.getEndMarker(),
              dialog.autoExpandEnabled());

        m_view->getTrackEditor()->getCompositionView()->deleteCachedPreviews();
        CommandHistory::getInstance()->addCommand(command);

        // If you change the composition length to 50 while the pointer is still
        // at 100 and save, the pointer position is saved, and the document
        // re-extends itself to 100 when you reload.  I was going to bother to
        // do some nice "if the pointer is beyond the new end then move the
        // pointer" logic here, but it's more than a oneliner, and so we'll just
        // rewind the damn cursor to the beginning as insurance against this,
        // and dust off our hands.
        slotRewindToBeginning();
    }
}

void
RosegardenMainWindow::slotManageMIDIDevices()
{
    if (m_deviceManager) {
        m_deviceManager->show();
        m_deviceManager->raise();
        m_deviceManager->activateWindow();
        return;
    }

    m_deviceManager = new DeviceManagerDialog(this);

    connect(m_deviceManager, &DeviceManagerDialog::editBanks,
            this, static_cast<void(RosegardenMainWindow::*)(DeviceId)>(
                    &RosegardenMainWindow::slotEditBanks));

    connect(m_deviceManager.data(), &DeviceManagerDialog::editControllers,
            this, &RosegardenMainWindow::slotEditControlParameters);

    connect(this, &RosegardenMainWindow::documentAboutToChange,
            m_deviceManager.data(), &DeviceManagerDialog::slotCloseButtonPress);

    if (m_midiMixer) {
         connect(m_deviceManager.data(), &DeviceManagerDialog::deviceNamesChanged,
                 m_midiMixer, &MidiMixerWindow::slotSynchronise);
    }

    connect(m_deviceManager.data(), &DeviceManagerDialog::deviceNamesChanged,
                 m_trackParameterBox, &TrackParameterBox::devicesChanged);

    QToolButton *tb = findChild<QToolButton*>("manage_midi_devices");
    if(tb){
        tb->setDown(true);
    }

    // Prevent the device manager from being resized larger or smaller than the
    // size it first renders at.  I tried to correct this in Designer, but long
    // story short, the only way I could get the form preview to behave properly
    // was to fix its size in the .ui file.  That can't possibly work well, so
    // we'll try fixing its size here, after it has been calculated against the
    // user environment.
    QSize renderedSize = m_deviceManager->size();
    m_deviceManager->setMinimumSize(renderedSize);
    m_deviceManager->setMaximumSize(renderedSize);
    m_deviceManager->show();
}

void
RosegardenMainWindow::slotManageSynths()
{
    if (m_synthManager) {
        m_synthManager->show();
        m_synthManager->raise();
        m_synthManager->activateWindow();
        return ;
    }

    m_synthManager = new SynthPluginManagerDialog(this, RosegardenDocument::currentDocument, m_pluginGUIManager);

    connect(m_synthManager, &SynthPluginManagerDialog::closing,
            this, &RosegardenMainWindow::slotSynthPluginManagerClosed);

    connect(this, &RosegardenMainWindow::documentAboutToChange,
            m_synthManager, &QWidget::close);

    connect(m_synthManager,
            &SynthPluginManagerDialog::pluginSelected,
            this,
            &RosegardenMainWindow::slotPluginSelected);

    connect(m_synthManager,
            &SynthPluginManagerDialog::showPluginDialog,
            this,
            &RosegardenMainWindow::slotShowPluginDialog);

    connect(m_synthManager,
            &SynthPluginManagerDialog::showPluginGUI,
            this,
            &RosegardenMainWindow::slotShowPluginGUI);

    m_synthManager->show();
}

void
RosegardenMainWindow::slotOpenAudioMixer()
{
    if (m_audioMixerWindow2) {
        m_audioMixerWindow2->activateWindow();
        m_audioMixerWindow2->raise();
        return;
    }

    m_audioMixerWindow2 = new AudioMixerWindow2;
}

void
RosegardenMainWindow::slotOpenMidiMixer()
{
    if (m_midiMixer) {
        m_midiMixer->activateWindow();
        m_midiMixer->raise();
        return;
    }

    m_midiMixer = new MidiMixerWindow;
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
                (*i)->activateWindow();
                return ;
            }
        }

    ControlEditorDialog *controlEditor = new ControlEditorDialog(this, RosegardenDocument::currentDocument,
                                         device);
    m_controlEditors.insert(controlEditor);

    RG_DEBUG << "inserting control editor dialog, have " << m_controlEditors.size() << " now";

    connect(controlEditor, &ControlEditorDialog::closing,
            this, &RosegardenMainWindow::slotControlEditorClosed);

    connect(this, &RosegardenMainWindow::documentAboutToChange,
            controlEditor, &QWidget::close);

    connect(RosegardenDocument::currentDocument,
            &RosegardenDocument::devicesResyncd,
            controlEditor,
            static_cast<void(ControlEditorDialog::*)()>(
                    &ControlEditorDialog::slotUpdate));


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
        m_bankEditor->activateWindow();
        return ;
    }

    m_bankEditor = new BankEditorDialog(this, RosegardenDocument::currentDocument, device);

    connect(m_bankEditor, &BankEditorDialog::closing,
            this, &RosegardenMainWindow::slotBankEditorClosed);

    connect(this, &RosegardenMainWindow::documentAboutToChange,
            m_bankEditor, &BankEditorDialog::slotFileClose);

    // Cheating way of updating the track/instrument list
    //
    connect(m_bankEditor, &BankEditorDialog::deviceNamesChanged,
            m_view, &RosegardenMainViewWidget::slotSynchroniseWithComposition);

    // Assume a m_device manager at this point, but check
    // Need to refresh the device manager as well
    connect(m_bankEditor, &BankEditorDialog::deviceNamesChanged,
            m_deviceManager.data(), &DeviceManagerDialog::slotResyncDevicesReceived);
    m_bankEditor->show();

    connect(m_bankEditor, &BankEditorDialog::deviceNamesChanged,
            m_trackParameterBox, &TrackParameterBox::devicesChanged);
}

void
RosegardenMainWindow::slotManageTriggerSegments()
{
    if (m_triggerSegmentManager) {
        m_triggerSegmentManager->show();
        m_triggerSegmentManager->raise();
        m_triggerSegmentManager->activateWindow();
        return ;
    }

    m_triggerSegmentManager = new TriggerSegmentManager(this, RosegardenDocument::currentDocument);

    connect(m_triggerSegmentManager, &TriggerSegmentManager::closing,
            this, &RosegardenMainWindow::slotTriggerManagerClosed);

    connect(m_triggerSegmentManager, &TriggerSegmentManager::editTriggerSegment,
            m_view, &RosegardenMainViewWidget::slotEditTriggerSegment);

    m_triggerSegmentManager->show();
}

void
RosegardenMainWindow::slotTriggerManagerClosed()
{
    RG_DEBUG << "slotTriggerManagerClosed()";

    m_triggerSegmentManager = nullptr;
}

void
RosegardenMainWindow::slotEditMarkers()
{
    if (m_markerEditor) {
        m_markerEditor->show();
        m_markerEditor->raise();
        m_markerEditor->activateWindow();
        return ;
    }

    m_markerEditor = new MarkerEditor(this, RosegardenDocument::currentDocument);

    connect(m_markerEditor, &MarkerEditor::closing,
            this, &RosegardenMainWindow::slotMarkerEditorClosed);

    connect(m_markerEditor, &MarkerEditor::jumpToMarker,
            RosegardenDocument::currentDocument, &RosegardenDocument::slotSetPointerPosition);

    m_markerEditor->show();
}

void
RosegardenMainWindow::slotMarkerEditorClosed()
{
    RG_DEBUG << "slotMarkerEditorClosed()";

    m_markerEditor = nullptr;
}

void
RosegardenMainWindow::slotEditTempos(timeT openAtTime)
{
    if (m_tempoAndTimeSignatureEditor) {
        m_tempoAndTimeSignatureEditor->show();
        m_tempoAndTimeSignatureEditor->raise();
        m_tempoAndTimeSignatureEditor->activateWindow();
        return ;
    }

    m_tempoAndTimeSignatureEditor = new TempoAndTimeSignatureEditor(openAtTime);

    connect(m_tempoAndTimeSignatureEditor, &TempoAndTimeSignatureEditor::closing,
            this, &RosegardenMainWindow::slotTempoViewClosed);

    connect(m_tempoAndTimeSignatureEditor, &EditViewBase::saveFile, this, &RosegardenMainWindow::slotFileSave);

    m_tempoAndTimeSignatureEditor->show();
}

void
RosegardenMainWindow::slotTempoViewClosed()
{
    RG_DEBUG << "slotTempoViewClosed()";

    m_tempoAndTimeSignatureEditor = nullptr;
}

void
RosegardenMainWindow::slotControlEditorClosed()
{
    RG_DEBUG << "slotControlEditorClosed()";

    // Make sure changes get to the UI.
    uiUpdateKludge();

    const QObject *s = sender();

    for (std::set<ControlEditorDialog *>::iterator i = m_controlEditors.begin();
         i != m_controlEditors.end(); ++i) {
        if (*i == s) {
            m_controlEditors.erase(i);
            RG_DEBUG << "removed control editor dialog, have " << m_controlEditors.size() << " left";
            return ;
        }
    }

    RG_WARNING << "WARNING: control editor " << s << " closed, but couldn't find it in our control editor list (we have " << m_controlEditors.size() << " editors)";
}

void
RosegardenMainWindow::slotShowPluginDialog(QWidget *parent,
                                       InstrumentId instrumentId,
                                       int index)
{
    RG_DEBUG << "slotShowPluginDialog(" << parent << ", " << instrumentId << ", " << index << ")";

    // ??? AudioPluginDialog should be simplified to the point where this
    //     routine is only a single line launching it.  Don't hold on
    //     to the dialogs (AudioPluginDialog should do this).  Don't connect
    //     all these signals (AudioPluginDialog should take care of itself).
    //     Etc...  Then let all who are connected to this slot launch
    //     AudioPluginDialog directly on their own.  Get rid of this routine.

    if (!parent)
        parent = this;

    int key = (index << 16) + instrumentId;
    RG_DEBUG << "  key:" << key;

    // If we already have a dialog for this plugin, show it.
    if (m_pluginDialogs[key]) {
        m_pluginDialogs[key]->show();
        m_pluginDialogs[key]->raise();
        m_pluginDialogs[key]->activateWindow();
        return ;
    }

    PluginContainer *container = RosegardenDocument::currentDocument->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "slotShowPluginDialog - "
        << "no instrument or buss of id " << instrumentId;
        return ;
    }

    // only create a dialog if we've got a plugin instance
    const AudioPluginInstance *inst = container->getPlugin(index);

    if (!inst) {
        RG_DEBUG << "slotShowPluginDialog - no AudioPluginInstance found for index " << index;
        return;
    }

    // Create the plugin dialog
    //
    AudioPluginDialog *dialog =
        new AudioPluginDialog(parent,
                              RosegardenDocument::currentDocument->getPluginManager(),
                              m_pluginGUIManager,
                              container,
                              index);

    connect(dialog,
            &AudioPluginDialog::pluginSelected,
            this,
            &RosegardenMainWindow::slotPluginSelected);

    connect(dialog,
            &AudioPluginDialog::pluginPortChanged,
            this,
            &RosegardenMainWindow::slotPluginPortChanged);

    connect(dialog,
            &AudioPluginDialog::pluginProgramChanged,
            this,
            &RosegardenMainWindow::slotPluginProgramChanged);

    connect(dialog,
            &AudioPluginDialog::changePluginConfiguration,
            this,
            &RosegardenMainWindow::slotChangePluginConfiguration);

    connect(dialog,
            &AudioPluginDialog::showPluginGUI,
            this,
            &RosegardenMainWindow::slotShowPluginGUI);

    connect(dialog,
            &AudioPluginDialog::stopPluginGUI,
            this,
            &RosegardenMainWindow::slotStopPluginGUI);

    connect(dialog,
            &AudioPluginDialog::bypassed,
            this,
            &RosegardenMainWindow::slotPluginBypassed);

    connect(dialog,
            &AudioPluginDialog::destroyed,
            this,
            &RosegardenMainWindow::slotPluginDialogDestroyed);

    connect(this, &RosegardenMainWindow::documentAboutToChange, dialog, &QWidget::close);

    // Hold onto this dialog so we don't have to create it again.
    m_pluginDialogs[key] = dialog;
    m_pluginDialogs[key]->show();

    // Set modified
    RosegardenDocument::currentDocument->slotDocumentModified();
}

void
RosegardenMainWindow::slotPluginSelected(InstrumentId instrumentId,
                                         int index, int plugin)
{
    const QObject *s = sender();

    bool fromSynthMgr = (s == m_synthManager);

    // It's assumed that ports etc will already have been set up on
    // the AudioPluginInstance before this is invoked.

    PluginContainer *container = RosegardenDocument::currentDocument->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "slotPluginSelected - "
        << "no instrument or buss of id " << instrumentId;
        return ;
    }

    AudioPluginInstance *inst =
        container->getPlugin(index);

    if (!inst) {
        RG_DEBUG << "slotPluginSelected - "
        << "got index of unknown plugin!";
        return ;
    }

    if (plugin == -1) {
        // Destroy plugin instance
        //!!! seems iffy -- why can't we just unassign it?

        if (StudioControl::
                destroyStudioObject(inst->getMappedId())) {
            RG_DEBUG << "slotPluginSelected - "
            << "cannot destroy Studio object "
            << inst->getMappedId();
        }

        inst->setAssigned(false);
    } else {
        // If unassigned then create a sequencer instance of this
        // AudioPluginInstance.
        //
        if (inst->isAssigned()) {
            RG_DEBUG << "slotPluginSelected - "
            << " setting identifier for mapper id " << inst->getMappedId()
            << " to " << strtoqstr(inst->getIdentifier());

            StudioControl::setStudioObjectProperty(inst->getMappedId(),
                                                   MappedPluginSlot::Identifier,
                                                   strtoqstr(inst->getIdentifier()));
        } else {
            // create a studio object at the sequencer
            MappedObjectId newId =
                StudioControl::createStudioObject
                (MappedObject::PluginSlot);

            RG_DEBUG << "slotPluginSelected - "
                     << " new MappedObjectId = " << newId;

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
     qstrtostr(RosegardenDocument::currentDocument->getAudioFileManager().getAbsoluteAudioPath()));

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
    RosegardenDocument::currentDocument->slotDocumentModified();
}

void
RosegardenMainWindow::slotChangePluginPort(InstrumentId instrumentId,
                                           int pluginIndex,
                                           int portIndex,
                                           float value)
{
    PluginContainer *container = RosegardenDocument::currentDocument->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "slotChangePluginPort - "
        << "no instrument or buss of id " << instrumentId;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);
    if (!inst) {
        RG_DEBUG << "slotChangePluginPort - "
        << "no plugin at index " << pluginIndex << " on " << instrumentId;
        return ;
    }

    PluginPortInstance *port = inst->getPort(portIndex);
    if (!port) {
        RG_DEBUG << "slotChangePluginPort - no port "
        << portIndex;
        return ;
    }

    RG_DEBUG << "slotPluginPortChanged - "
             << "setting plugin port (" << inst->getMappedId()
             << ", " << portIndex << ") from " << port->value
             << " to " << value;

    port->setValue(value);

    StudioControl::setStudioPluginPort(inst->getMappedId(),
                                       portIndex, port->value);

    RosegardenDocument::currentDocument->slotDocumentModified();

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
    PluginContainer *container = RosegardenDocument::currentDocument->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "slotPluginPortChanged - "
                 << "no instrument or buss of id " << instrumentId;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);
    if (!inst) {
        RG_DEBUG << "slotPluginPortChanged - "
                 << "no plugin at index " << pluginIndex << " on " << instrumentId;
        return ;
    }

    PluginPortInstance *port = inst->getPort(portIndex);
    if (!port) {
        RG_DEBUG << "slotPluginPortChanged - no port "
                 << portIndex;
        return ;
    }

    RG_DEBUG << "slotPluginPortChanged - "
             << "setting plugin port (" << inst->getMappedId()
             << ", " << portIndex << ") to " << port->value;

    StudioControl::setStudioPluginPort(inst->getMappedId(),
                                       portIndex, port->value);

    RosegardenDocument::currentDocument->slotDocumentModified();

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
    PluginContainer *container = RosegardenDocument::currentDocument->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "slotChangePluginProgram - "
        << "no instrument or buss of id " << instrumentId;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);
    if (!inst) {
        RG_DEBUG << "slotChangePluginProgram - "
        << "no plugin at index " << pluginIndex << " on " << instrumentId;
        return ;
    }

    RG_DEBUG << "slotChangePluginProgram - "
             << "setting plugin program ("
             << inst->getMappedId() << ") from " << strtoqstr(inst->getProgram())
             << " to " << program;

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
    RosegardenDocument::currentDocument->slotDocumentModified();

    int key = (pluginIndex << 16) + instrumentId;
    if (m_pluginDialogs[key]) {
        m_pluginDialogs[key]->updatePluginProgramControl();
    }
}

void
RosegardenMainWindow::slotPluginProgramChanged(InstrumentId instrumentId,
        int pluginIndex)
{
    PluginContainer *container = RosegardenDocument::currentDocument->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "slotPluginProgramChanged - "
        << "no instrument or buss of id " << instrumentId;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);
    if (!inst) {
        RG_DEBUG << "slotPluginProgramChanged - "
        << "no plugin at index " << pluginIndex << " on " << instrumentId;
        return ;
    }

    QString program = strtoqstr(inst->getProgram());

    RG_DEBUG << "slotPluginProgramChanged - "
    << "setting plugin program ("
    << inst->getMappedId() << ") to " << program;

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
    RosegardenDocument::currentDocument->slotDocumentModified();

    if (m_pluginGUIManager)
        m_pluginGUIManager->updateProgram(instrumentId,
                                          pluginIndex);
}

void
RosegardenMainWindow::slotChangePluginConfiguration(InstrumentId instrumentId,
                                                    int index,
                                                    bool global,
                                                    const QString &configKey,
                                                    const QString &configValue)
{
    PluginContainer *container = RosegardenDocument::currentDocument->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "slotChangePluginConfiguration - "
        << "no instrument or buss of id " << instrumentId;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(index);

    if (global && inst) {

        // Set the same configuration on other plugins in the same
        // instance group

        QSharedPointer<AudioPlugin> pl =
            m_pluginManager->getPluginByIdentifier(strtoqstr(inst->getIdentifier()));

        if (pl && pl->isGrouped()) {

            InstrumentVector il =
                RosegardenDocument::currentDocument->getStudio().getAllInstruments();

            for (InstrumentVector::iterator i = il.begin();
                    i != il.end(); ++i) {

                for (AudioPluginVector::iterator pli =
                            (*i)->beginPlugins();
                        pli != (*i)->endPlugins(); ++pli) {

                    if (*pli && (*pli)->isAssigned() &&
                            (*pli)->getIdentifier() == inst->getIdentifier() &&
                            (*pli) != inst) {

                        slotChangePluginConfiguration
                        ((*i)->getId(), (*pli)->getPosition(),
                         false, configKey, configValue);

                        m_pluginGUIManager->updateConfiguration
                        ((*i)->getId(), (*pli)->getPosition(), configKey);
                    }
                }
            }
        }
    }

    if (inst) {

        inst->setConfigurationValue(qstrtostr(configKey), qstrtostr(configValue));

        MappedObjectPropertyList config;
        for (AudioPluginInstance::ConfigMap::const_iterator
                i = inst->getConfiguration().begin();
                i != inst->getConfiguration().end(); ++i) {
            config.push_back(strtoqstr(i->first));
            config.push_back(strtoqstr(i->second));
        }

        RG_DEBUG << "slotChangePluginConfiguration: setting new config on mapped id " << inst->getMappedId();

        QString error = StudioControl::setStudioObjectPropertyList
        (inst->getMappedId(),
         MappedPluginSlot::Configuration,
         config);

        if (error != "") showError(error);

        // Set modified
        RosegardenDocument::currentDocument->slotDocumentModified();

        const int dialogKey = (index << 16) + instrumentId;
        if (m_pluginDialogs[dialogKey]) {
            m_pluginDialogs[dialogKey]->updatePluginProgramList();
        }
    }
}

void
RosegardenMainWindow::slotPluginDialogDestroyed(InstrumentId instrumentId,
        int index)
{
    RG_DEBUG << "slotPluginDialogDestroyed()";

    int key = (index << 16) + instrumentId;

    RG_DEBUG << "  key:" << key;

    // We can't simply call erase() here to prevent the map from
    // growing.  We would also need to change every check for
    // m_pluginDialogs[key] to use find() instead.
    m_pluginDialogs[key] = nullptr;
}

void
RosegardenMainWindow::slotPluginBypassed(InstrumentId instrumentId,
                                         int pluginIndex, bool bypassed)
{
    PluginContainer *container = RosegardenDocument::currentDocument->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "slotPluginBypassed - "
        << "no instrument or buss of id " << instrumentId;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);

    if (inst) {
        StudioControl::setStudioObjectProperty
        (inst->getMappedId(),
         MappedPluginSlot::Bypassed,
         MappedObjectValue(bypassed));

        // Set the bypass on the instance
        //
        inst->setBypass(bypassed);

        // Set modified
        RosegardenDocument::currentDocument->slotDocumentModified();
    }

    emit pluginBypassed(instrumentId, pluginIndex, bypassed);
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
        connect(m_playList, &PlayListDialog::closing, this, &RosegardenMainWindow::slotPlayListClosed);

        connect(m_playList->getPlayList(), &PlayList::play, this, &RosegardenMainWindow::slotPlayListPlay);

    }

    m_playList->show();
}

void
RosegardenMainWindow::slotPlayListPlay(const QString& url)
{
//     RG_DEBUG << "slotPlayListPlay() - called with: " << url;
    slotStop();
    openURL(url);
    slotPlay();
}

void
RosegardenMainWindow::slotPlayListClosed()
{
    RG_DEBUG << "slotPlayListClosed()\n";
    if( m_playList ){
//         delete m_playList;
        m_playList = nullptr;
    }
}

void
RosegardenMainWindow::slotHelp()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:manual-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:manual-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
RosegardenMainWindow::slotTutorial()
{
    QString tutorialURL = tr("http://rosegardenmusic.com/tutorials/");
    QDesktopServices::openUrl(QUrl(tutorialURL));
}

void
RosegardenMainWindow::slotBugGuidelines()
{
    QString glURL = tr("http://rosegarden.sourceforge.net/tutorial/bug-guidelines.html");
    QDesktopServices::openUrl(QUrl(glURL));
}

void
RosegardenMainWindow::slotHelpAbout()
{
    new AboutDialog(this);
}

void
RosegardenMainWindow::slotHelpAboutQt()
{
    QMessageBox::aboutQt(this, tr("Rosegarden"));
}

void
RosegardenMainWindow::slotDonate()
{
    QDesktopServices::openUrl(QUrl(
            "https://www.rosegardenmusic.com/wiki/donations"));
}


void
RosegardenMainWindow::slotBankEditorClosed()
{
    RG_DEBUG << "slotBankEditorClosed()";

    if (RosegardenDocument::currentDocument->isModified()) {
        if (m_view) {
            // ??? Can't just remove this when the time comes.
            // ??? I suspect this call is being made because the Track and
            //     Instrument Parameters boxes need to be updated.  But
            //     they should have already updated in response to the
            //     RosegardenDocument::documentModified() signal(s) sent
            //     while modifying the banks.
            m_view->slotSelectTrackSegments(RosegardenDocument::currentDocument->getComposition().getSelectedTrack());
        }
    }

    m_bankEditor = nullptr;
    slotManageMIDIDevices(); // this will raise the device manager
}

void
RosegardenMainWindow::slotSynthPluginManagerClosed()
{
    RG_DEBUG << "slotSynthPluginManagerClosed()\n";

    m_synthManager = nullptr;
}

void
RosegardenMainWindow::slotMidiMixerClosed()
{
    m_midiMixer = nullptr;
}

void
RosegardenMainWindow::slotAudioManagerClosed()
{
    RG_DEBUG << "slotAudioManagerClosed()";

    m_audioManagerDialog = nullptr;
}

void
RosegardenMainWindow::slotPanic()
{
    if (!m_seqManager)
        return;

    // Stop the transport before we send a panic as the
    // playback goes all to hell anyway.
    //
    slotStop();

    m_seqManager->panic();
}

void
RosegardenMainWindow::slotPopulateTrackInstrumentPopup()
{
    RG_DEBUG << "slotSetTrackInstrument\n";
    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    Track *track = comp.getTrackById(comp.getSelectedTrack());

    if (!track) {
        RG_DEBUG << "Weird: no track available for instrument popup!";
        return ;
    }

    Instrument *instrument = RosegardenDocument::currentDocument->getStudio().getInstrumentById(track->getInstrument());

//    QPopupMenu *popup = dynamic_cast<QPopupMenu*>(factory()->container("set_track_instrument", this));
    QMenu *popup = this->findChild<QMenu*>("set_track_instrument");

    m_view->getTrackEditor()->getTrackButtons()->populateInstrumentPopup(instrument, popup);
}

void
RosegardenMainWindow::slotRemapInstruments()
{
    RG_DEBUG << "slotRemapInstruments\n";
    RemapInstrumentDialog dialog(this, RosegardenDocument::currentDocument);

    connect(&dialog, &RemapInstrumentDialog::applyClicked,
            m_view->getTrackEditor()->getTrackButtons(),
            &TrackButtons::slotSynchroniseWithComposition);

    if (dialog.exec() == QDialog::Accepted) {
        RG_DEBUG << "slotRemapInstruments - accepted\n";
    }

}

void
RosegardenMainWindow::slotSaveDefaultStudio()
{
    RG_DEBUG << "slotSaveDefaultStudio\n";

    int reply = QMessageBox::warning
                (this, tr("Rosegarden"), tr("Are you sure you want to save this as your default studio?"),
                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return ;

    TmpStatusMsg msg(tr("Saving current document as default studio..."), this);

    QString autoloadFile = ResourceFinder().getAutoloadSavePath();

    RG_DEBUG << "slotSaveDefaultStudio : saving studio in "
             << autoloadFile;

    SetWaitCursor waitCursor;
    QString errMsg;
    bool res = RosegardenDocument::currentDocument->saveDocument(autoloadFile, errMsg);
    if (!res) {
        if (!errMsg.isEmpty())
            QMessageBox::critical(this, tr("Rosegarden"), tr("Could not auto-save document at %1\nError was : %2")
                                  .arg(autoloadFile).arg(errMsg));
        else
            QMessageBox::critical(this, tr("Rosegarden"), tr("Could not auto-save document at %1")
                                  .arg(autoloadFile));

    }
}

void
RosegardenMainWindow::slotImportDefaultStudio()
{
    int reply = QMessageBox::warning
            (this, tr("Rosegarden"), tr("Are you sure you want to import your default studio and lose the current one?"), QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return ;

    QString autoloadFile = ResourceFinder().getAutoloadPath();

    QFileInfo autoloadFileInfo(autoloadFile);

    if (!autoloadFileInfo.isReadable()) {
        RG_DEBUG << "RosegardenDocument::slotImportDefaultStudio - "
        << "can't find autoload file - defaulting";
        return ;
    }

    slotImportStudioFromFile(autoloadFile);
}

void
RosegardenMainWindow::slotImportStudio()
{
    RG_DEBUG << "slotImportStudio()\n";

    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    QString directory = settings.value("import_studio",
            ResourceFinder().getResourceDir("library")).toString();

    const QString file = FileDialog::getOpenFileName(this, tr("Import Studio from File"), directory,
                    tr("All supported files") + " (*.rg *.RG *.rgt *.RGT *.rgp *.RGP)" + ";;" +
                    tr("All files") + " (*)", nullptr);
    if (file.isEmpty())
        return;

    slotImportStudioFromFile(file);

    directory = existingDir(file);
    settings.setValue("import_studio", directory);
    settings.endGroup();
}

void
RosegardenMainWindow::slotImportStudioFromFile(const QString &file)
{
    // We're only using this document temporarily, so we don't want to let it
    // obliterate the command history!
    bool clearCommandHistory = false, skipAutoload = true;
    RosegardenDocument *doc = new RosegardenDocument(this, {}, skipAutoload, clearCommandHistory, m_useSequencer);

    Studio &oldStudio = RosegardenDocument::currentDocument->getStudio();
    Studio &newStudio = doc->getStudio();

    // Add some dummy devices for when we open the document.  We guess
    // that the file won't have more than 32 devices.
    //
    //    for (unsigned int i = 0; i < 32; i++) {
    //        newStudio.addDevice("", i, Device::Midi);
    //    }

    //!DEVPUSH review this

    if (doc->openDocument(file, true)) { // true because we actually
                                         // do want to create devices
                                         // on the sequencer here

        MacroCommand *command = new MacroCommand(tr("Import Studio"));

        // We actually only copy across MIDI play devices... for now
        std::vector<DeviceId> midiPlayDevices;

        for (DeviceVector::const_iterator i =
                 oldStudio.begin(); i != oldStudio.end(); ++i) {

            MidiDevice *md = dynamic_cast<MidiDevice *>(*i);

            if (md && (md->getDirection() == MidiDevice::Play)) {
                midiPlayDevices.push_back((*i)->getId());
            }
        }

        std::vector<DeviceId>::iterator di(midiPlayDevices.begin());

        for (DeviceVector::const_iterator i = newStudio.begin();
             i != newStudio.end(); ++i) {

            MidiDevice *md = dynamic_cast<MidiDevice *>(*i);

            if (md && (md->getDirection() == MidiDevice::Play)) {
                RG_DEBUG << "newStudio device" << md->getName();
                if (di != midiPlayDevices.end()) {
                    MidiDevice::VariationType variation
                    (md->getVariationType());
                    BankList bl(md->getBanks());
                    ProgramList pl(md->getPrograms());
                    ControlList cl(md->getControlParameters());

                    RG_DEBUG << "modify device" << md->getName() << *di;
                    ModifyDeviceCommand *mdCommand =
                        new ModifyDeviceCommand(&oldStudio,
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
                } else {
                    RG_DEBUG << "new device" << md->getName();
                    command->addCommand(new CreateOrDeleteDeviceCommand
                                        (&oldStudio,
                                         md->getName(),
                                         md->getType(),
                                         md->getDirection(),
                                         "",
                                         true,
                                         md->getLibrarianName(),
                                         md->getLibrarianEmail(),
                                         md->getVariationType(),
                                         md->getBanks(),
                                         md->getPrograms(),
                                         md->getControlParameters(),
                                         md->getKeyMappings()
                                         ));
                }
            }
        }

        while (di != midiPlayDevices.end()) {
            RG_DEBUG << "add device" << *di;
            command->addCommand(new CreateOrDeleteDeviceCommand
                                (&oldStudio,
                                 *di));
            ++di;
        }

        oldStudio.setMIDIThruFilter(newStudio.getMIDIThruFilter());
        oldStudio.setMIDIRecordFilter(newStudio.getMIDIRecordFilter());

        CommandHistory::getInstance()->addCommand(command);
        RosegardenDocument::currentDocument->initialiseStudio(); // The other document will have reset it

        if (m_view) {
            // ??? Can't just remove this when the time comes.
            // ??? I suspect this call is being made because the Track and
            //     Instrument Parameters boxes need to be updated.  It
            //     would be better if the various boxes responded to
            //     RosegardenDocument::documentModified() which should
            //     have already been emitted.
            m_view->slotSelectTrackSegments
                (RosegardenDocument::currentDocument->getComposition().getSelectedTrack());
        }
    }
    delete doc;
}

void
RosegardenMainWindow::slotResetMidiNetwork()
{
    if (!RosegardenDocument::currentDocument)
        return;

    // Send out BS/PC/CCs for each Track.
    RosegardenDocument::currentDocument->sendChannelSetups(false);  // do not send resets
}

void
RosegardenMainWindow::slotModifyMIDIFilters()
{
    RG_DEBUG << "slotModifyMIDIFilters";

    MidiFilterDialog dialog(this, RosegardenDocument::currentDocument);

    if (dialog.exec() == QDialog::Accepted) {
        RG_DEBUG << "slotModifyMIDIFilters - accepted";
    }
}

void
RosegardenMainWindow::slotManageMetronome()
{
    RG_DEBUG << "slotManageMetronome";

    ManageMetronomeDialog dialog(this, RosegardenDocument::currentDocument);

    if (dialog.exec() == QDialog::Accepted) {
        RG_DEBUG << "slotManageMetronome - accepted";
    }
}

void
RosegardenMainWindow::slotAutoSave()
{
    // Don't autosave when playing or recording.
    // ??? Shouldn't this be "m_seqManager && !..."?  That way if there
    //     is no sequence manager (e.g. when the sequencer isn't launched)
    //     we still will autosave.
    if (!m_seqManager  ||
        m_seqManager->getTransportStatus() == PLAYING  ||
        m_seqManager->getTransportStatus() == RECORDING)
        return;

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    // If autosave is disabled, bail.
    if (!settings.value("autosave", "true").toBool())
        return;

    const QTime now{QTime::currentTime()};
    // Not time to autosave?  Bail.
    if (m_lastAutoSaveTime.msecsTo(now) <= (int)m_autoSaveInterval)
        return;

    RG_DEBUG << "slotAutoSave(): saving" << m_lastAutoSaveTime << m_autoSaveInterval << now;

    RosegardenDocument::currentDocument->autoSave();

    m_lastAutoSaveTime = now;
}

void
RosegardenMainWindow::slotUpdateAutoSaveInterval(unsigned int interval)
{
    RG_DEBUG << "slotUpdateAutoSaveInterval - " << "changed interval to " << interval;

    m_autoSaveInterval = interval * 1000;
}

void RosegardenMainWindow::slotShowToolHelp(const QString &s)
{
    QString msg = s;
    if (msg != "") msg = " " + msg;
    slotStatusMsg(msg);
}

TransportDialog *RosegardenMainWindow::getTransport()
{
    if (m_transport == nullptr)
        createAndSetupTransport();

    return m_transport;
}

void
RosegardenMainWindow::awaitDialogClearance() const
{
    bool haveDialog = true;

    while (haveDialog) {

        QList<QDialog *> childList = findChildren<QDialog *>();

        haveDialog = false;

        // For each child dialog...
        for (int i = 0; i < childList.size(); ++i) {
            QDialog *child = childList.at(i);
            // If it's visible and it's not the TransportDialog, we need
            // to keep waiting.
            if (child->isVisible()  &&
                child->objectName() != "Rosegarden Transport") {
                haveDialog = true;
                break;
            }
        }

        if (haveDialog)
            qApp->processEvents(QEventLoop::AllEvents, 300);
    }
}

void
RosegardenMainWindow::slotNewerVersionAvailable(QString v)
{
    QString text(tr("<h3>Newer version available</h3>"));
    QString informativeText(tr("<p>You are using version %1.  Version %2 is now available.</p><p>Please consult the <a style=\"color:gold\" href=\"http://www.rosegardenmusic.com/getting/\">Rosegarden website</a> for more information.</p>").arg(VERSION).arg(v));
    slotDisplayWarning(WarningWidget::Info, text, informativeText);
}

void
RosegardenMainWindow::slotAddMarker2()
{
    AddMarkerCommand *command =
        new AddMarkerCommand(&RosegardenDocument::currentDocument->getComposition(),
                             RosegardenDocument::currentDocument->getComposition().getPosition(),
                             "new marker",
                             "no description");

    m_view->slotAddCommandToHistory(command);
}

void
RosegardenMainWindow::slotPreviousMarker()
{
    const Composition::MarkerVector &markers =
            RosegardenDocument::currentDocument->getComposition().getMarkers();

    timeT currentTime = RosegardenDocument::currentDocument->getComposition().getPosition();
    timeT time = currentTime;

    // For each marker...
    for (const Marker *marker : markers) {
        if (marker->getTime() >= currentTime)
            break;
        time = marker->getTime();
    }

    // If a jump is needed, jump.
    if (time != currentTime)
        RosegardenDocument::currentDocument->slotSetPointerPosition(time);
}

void
RosegardenMainWindow::slotNextMarker()
{
    const Composition::MarkerVector &markers =
            RosegardenDocument::currentDocument->getComposition().getMarkers();

    timeT currentTime = RosegardenDocument::currentDocument->getComposition().getPosition();
    timeT time = currentTime;

    // For each marker...
    for (const Marker *marker : markers) {
        if (marker->getTime() > currentTime) {
            time = marker->getTime();
            break;
        }
    }

    // If a jump is needed, jump.
    if (time != currentTime)
        RosegardenDocument::currentDocument->slotSetPointerPosition(time);
}

void
RosegardenMainWindow::slotSetQuickMarker()
{
    RG_DEBUG << "slotSetQuickMarker()";

    RosegardenDocument::currentDocument->setQuickMarker();
    getView()->getTrackEditor()->updateRulers();
}

void
RosegardenMainWindow::slotJumpToQuickMarker()
{
    RG_DEBUG << "slotJumpToQuickMarker()";

    RosegardenDocument::currentDocument->jumpToQuickMarker();
}

void
RosegardenMainWindow::slotDisplayWarning(int type,
                                         QString text,
                                         QString informativeText)
{
    RG_WARNING << "slotDisplayWarning(): MAIN WINDOW DISPLAY WARNING:  type " << type << " text" << text;

    // queue up the message, which trips the warning or info icon in so doing
    m_warningWidget->queueMessage(type, text, informativeText);

    // set up the error state for the appropriate icon...  this should probably
    // be managed some other way, but that's organic growth for you
    switch (type) {
        case WarningWidget::Midi: m_warningWidget->setMidiWarning(true); break;
        case WarningWidget::Audio: m_warningWidget->setAudioWarning(true); break;
        case WarningWidget::Timer: m_warningWidget->setTimerWarning(true); break;
        case WarningWidget::Other:
        case WarningWidget::Info:
        default: break;
    }

}

void
RosegardenMainWindow::slotAboutToExecuteCommand()
{
    // save the pointer position to the command history
    timeT pointerPos =
        RosegardenDocument::currentDocument->getComposition().getPosition();
    RG_DEBUG << "about to execute a command" << pointerPos;
    CommandHistory::getInstance()->setPointerPosition(pointerPos);
}

void
RosegardenMainWindow::slotCommandUndone()
{
    // reset the pointer position from the command history
    timeT pointerPos = CommandHistory::getInstance()->getPointerPosition();
    RG_DEBUG << "command undone" << pointerPos;
    RosegardenDocument::currentDocument->slotSetPointerPosition(pointerPos);
}

void
RosegardenMainWindow::slotCommandRedone()
{
    // reset the pointer position from the command history
    timeT pointerPos = CommandHistory::getInstance()->getPointerPosition();
    RG_DEBUG << "command redone" << pointerPos;
    RosegardenDocument::currentDocument->slotSetPointerPosition(pointerPos);
}

void
RosegardenMainWindow::slotUpdatePosition()
{
    // set the pointer position in the command history
    timeT pointerPos =
        RosegardenDocument::currentDocument->getComposition().getPosition();
    RG_DEBUG << "update position in command history" << pointerPos;
    CommandHistory::getInstance()->setPointerPositionForRedo(pointerPos);
}

void
RosegardenMainWindow::slotSwitchPreset()
{
    if (!m_view->haveSelection()) return ;

    // Code pasted from NotationView.cpp with very coarse adaptation performed.
    // Really, I think both places need some more thought about what "selected
    // segments" and "all segments on this track" mean.  Definitely more work
    // could be done here.  TODO
    PresetHandlerDialog dialog(this, true);

    if (dialog.exec() != QDialog::Accepted) return;

    if (dialog.getConvertAllSegments()) {
        // get all segments for this track and convert them.
        Composition& comp = RosegardenDocument::currentDocument->getComposition();
        TrackId selectedTrack = comp.getSelectedTrack();

        // satisfy #1885251 the way that seems most reasonble to me at the
        // moment, only changing track parameters when acting on all segments on
        // this track from the notation view
        //
        //!!! This won't be undoable, and I'm not sure if that's seriously
        // wrong, or just mildly wrong, but I'm betting somebody will tell me
        // about it if this was inappropriate
        Track *track = comp.getTrackById(selectedTrack);
        track->setPresetLabel( qstrtostr(dialog.getName()) );
        track->setClef(dialog.getClef());
        track->setTranspose(dialog.getTranspose());
        track->setLowestPlayable(dialog.getLowRange());
        track->setHighestPlayable(dialog.getHighRange());

        CommandHistory::getInstance()->addCommand(new SegmentSyncCommand(
                            comp.getSegments(), selectedTrack,
                            dialog.getTranspose(),
                            dialog.getLowRange(),
                            dialog.getHighRange(),
                            clefIndexToClef(dialog.getClef())));

        comp.notifyTrackChanged(track);

    } else {
        CommandHistory::getInstance()->addCommand(new SegmentSyncCommand(
                            m_view->getSelection(),
                            dialog.getTranspose(),
                            dialog.getLowRange(),
                            dialog.getHighRange(),
                            clefIndexToClef(dialog.getClef())));
    }

    RosegardenDocument::currentDocument->slotDocumentModified();
}

void RosegardenMainWindow::slotInterpret()
{
    InterpretDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        const int interpretations = dialog.getInterpretations();

        const SegmentSelection segmentSelection = m_view->getSelection();

        MacroCommand *command = new MacroCommand(tr("Interpret segments"));

        // Keep track of these so we can delete them.
        std::vector<EventSelection *> selections;

        // For each Segment...
        for (Segment *segment : segmentSelection) {

            // Can't interpret an audio Segment.
            if (segment->getType() == Segment::Audio) continue;

            EventSelection *selection = new EventSelection(
                    *segment,
                    segment->getStartTime(),
                    segment->getEndMarkerTime());
            selections.push_back(selection);

            const NotationQuantizer *quantizer =
                    RosegardenDocument::currentDocument->getComposition().
                            getNotationQuantizer();

            command->addCommand(new InterpretCommand(
                    *selection,
                    quantizer,
                    interpretations));
        }

        m_view->slotAddCommandToHistory(command);

        // Should be safe to delete now.
        for (EventSelection *selection : selections) {
            delete selection;
        }
    }
}

void
RosegardenMainWindow::checkAudioPath()
{
    QString  audioPath = RosegardenDocument::currentDocument->getAudioFileManager().getAbsoluteAudioPath();
    QDir dir(audioPath);
    QString text(tr("<h3>Invalid audio path</h3>"));
    QString correctThis(tr("<p>You will not be able to record audio or drag and drop audio files onto Rosegarden until you correct this in <b>View -> Document Properties -> Audio</b>.</p>"));

    if (!dir.exists()) {

        text = tr("<h3>Created audio path</h3>");
        slotDisplayWarning(WarningWidget::Info, text,
                tr("<qt><p>Rosegarden created the audio path \"%1\" to use for audio recording, and to receive dropped audio files.</p><p>If you wish to use a different path, change this in <b>View -> Document Properties -> Audio</b>.</p></qt>").arg(audioPath));

        if (!dir.mkpath(audioPath)) {
            RG_DEBUG << "RosegardenDocument::testAudioPath() - audio path did not exist.  Tried to create it, and failed.";

            slotDisplayWarning(WarningWidget::Audio, text,
                    tr("<qt><p>The audio path \"%1\" did not exist, and could not be created.</p>%2</qt>").arg(audioPath).arg(correctThis));
        }
    } else {
        QTemporaryFile tmp(audioPath);
        bool failure = false;
        if (tmp.open()) {
            if (tmp.write("0", 1) == -1) {
                std::cout << "could not write file" << std::endl;
                failure = true;
            }
        } else {
            failure = true;
        }

        if (failure) {
            slotDisplayWarning(WarningWidget::Audio, text,
                    tr("<qt><p>The audio path \"%1\" exists, but is not writable.</p>%2</qt>").arg(audioPath).arg(correctThis));
        }

        if (tmp.isOpen())
            tmp.close();
    }

// This is all more convenient than intentionally breaking things in my system
// to trigger warning conditions.  It's not a real test of function, but it
// serves to test form.
//#define WARNING_WIDGET_WORKOUT
#ifdef WARNING_WIDGET_WORKOUT
    slotDisplayWarning(WarningWidget::Audio, "Audio warning!", "Informative audio warning!");
    slotDisplayWarning(WarningWidget::Midi, "MIDI warning!", "Informative MIDI warning!");
    slotDisplayWarning(WarningWidget::Timer, "Timer warning!", "Informative timer warning!");
    slotDisplayWarning(WarningWidget::Other, "Misc. warning!", "Informative misc. warning!");
    slotDisplayWarning(WarningWidget::Info, "Information", "Informative information!");
#endif
}

bool RosegardenMainWindow::saveIfModified()
{
    //RG_DEBUG << "saveIfModified()";

    // If the current document hasn't been modified, it's ok to continue.
    if (!RosegardenDocument::currentDocument->isModified())
        return true;

    // The current document has been modified.

    bool completed = true;

    // Ask the user if they want to save changes to the current document.
    int wantSave = QMessageBox::warning( this, tr("Rosegarden - Warning"),
            tr("<qt><p>The current file has been modified.</p><p>Do you want to save it?</p></qt>"),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel );

    //RG_DEBUG << "saveIfModified(): wantSave = " << wantSave;

    switch (wantSave) {

    case QMessageBox::Yes:

        if (!RosegardenDocument::currentDocument->isRegularDotRGFile()) {

            //RG_DEBUG << "saveIfModified() : new or imported file";

            completed = fileSaveAs(false);

        } else {

            //RG_DEBUG << "saveIfModified() : regular file";

            QString errMsg;

            // An attempt to address Bug #1725.  Stop the auto-save timer to
            // make sure no auto-saves sneak in while we are saving.
            m_autoSaveTimer->stop();

            completed = RosegardenDocument::currentDocument->saveDocument(
                    RosegardenDocument::currentDocument->getAbsFilePath(),
                    errMsg);

            // Restart the auto-save timer at the current time so that the next
            // auto-save happens as far into the future as possible.
            m_lastAutoSaveTime = QTime::currentTime();
            m_autoSaveTimer->start(autoSaveTimerInterval);

            if (!completed) {
                if (!errMsg.isEmpty()) {
                    QMessageBox::critical(this, tr("Rosegarden"), tr("Could not save document at %1\n(%2)").arg(RosegardenDocument::currentDocument->getAbsFilePath()).arg(errMsg));
                } else {
                    QMessageBox::critical(this, tr("Rosegarden"), tr("Could not save document at %1").arg(RosegardenDocument::currentDocument->getAbsFilePath()));
                }
            }
        }

        break;

    case QMessageBox::No:
        // delete the autosave file so it won't annoy
        // the user when reloading the file.
        RosegardenDocument::currentDocument->deleteAutoSaveFile();

        completed = true;
        break;

    case QMessageBox::Cancel:
        completed = false;
        break;

    default:
        completed = false;
        break;
    }

    // Clean up audio files.
    if (completed) {
        completed = RosegardenDocument::currentDocument->deleteOrphanedAudioFiles(wantSave == QMessageBox::No);
        if (completed) {
            RosegardenDocument::currentDocument->getAudioFileManager().resetRecentlyCreatedFiles();
        }
    }

    // If all's well, mark the document as ok to toss.
    if (completed)
        RosegardenDocument::currentDocument->clearModifiedStatus();

    return completed;
}


void
RosegardenMainWindow::uiUpdateKludge()
{
    // Force an update to the UI.
    // ??? This is a kludge.  Callers to this should be using
    //     RosegardenDocument::documentModified() to cause a
    //     refresh of the UI.
    m_view->slotSelectTrackSegments(
                RosegardenDocument::currentDocument->getComposition().getSelectedTrack());
}

RosegardenDocument *RosegardenMainWindow::newDocument(bool permanent,
                                                      const QString& path)
{
    // if the path is set this will load a file so autolaod should be skipped
    bool skipAutoLoad = false;
    if (path != "") skipAutoLoad = true;
    return new RosegardenDocument(
            this,  // parent
            m_pluginManager,  // audioPluginManager
            skipAutoLoad,  // skipAutoload
            true,  // clearCommandHistory
            m_useSequencer && permanent,  // enableSound
            path); // file to load
}

void
RosegardenMainWindow::changeEvent(QEvent *event)
{
    // Let baseclass handle first.
    QWidget::changeEvent(event);

    // We only care about this if the external controller port is
    // in Rosegarden native mode.
    if (!ExternalController::self().isNative())
        return;

    // We only care about activation changes.
    if (event->type() != QEvent::ActivationChange)
        return;

    // If we aren't the active window, bail.
    if (!isActiveWindow())
        return;

    ExternalController::self().activeWindow =
            ExternalController::Main;

    // Send CCs for current Track to external controller.

    // Get the selected Track's Instrument.
    InstrumentId instrumentId =
            RosegardenDocument::currentDocument->getComposition().getSelectedInstrumentId();

    if (instrumentId == NoInstrument)
        return;

    Instrument *instrument =
            RosegardenDocument::currentDocument->getStudio().getInstrumentById(instrumentId);

    if (!instrument)
        return;

    ExternalController::sendAllCCs(instrument, 0);

    // Clear out channels 1-15 for external controller.
    // ??? Why not provide the next 15 tracks here?  Then the user can
    //     always have control of 16 out of the full set of tracks.
    //     Sounds potentially handy.
    for (MidiByte channel = 1; channel < 16; ++channel) {
        ExternalController::send(channel, MIDI_CONTROLLER_VOLUME, 0);
        ExternalController::send(
                channel, MIDI_CONTROLLER_PAN, MidiMidValue);
    }
}

void
RosegardenMainWindow::openWindow(ExternalController::Window window)
{
    switch (window) {
    case ExternalController::Main:
        show();
        activateWindow();
        raise();
        break;

    case ExternalController::AudioMixer:
        slotOpenAudioMixer();
        break;

    case ExternalController::MidiMixer:
        slotOpenMidiMixer();
        break;

    default:
        RG_WARNING << "openwindow(): Unexpected window.";
        break;
    }
}

void
// cppcheck-suppress unusedFunction
RosegardenMainWindow::customEvent(QEvent *event)
{
    // See AlsaDriver::handleTransportCCs().

    // ??? This seems redundant with RosegardenSequencer::transportChange()
    //     which ends up triggering RosegardenMainWindow::slotHandleInputs().
    //     Review both approaches and see if we can consolidate down to the
    //     best one.  Using QEvent is thread-safe, but slow (message queue).
    //     Is transportChange() thread-safe and faster?  Can transportChange()
    //     be simplified to make it better than both?

    // ??? switch?

    if (event->type() == PreviousTrack) {
        slotSelectPreviousTrack();
        return;
    }

    if (event->type() == NextTrack) {
        slotSelectNextTrack();
        return;
    }

    if (event->type() == Loop) {
        toggleLoop();
        return;
    }

    if (event->type() == Stop) {
        slotStop();
        return;
    }

    if (event->type() == Rewind) {
        ButtonEvent *buttonEvent = dynamic_cast<ButtonEvent *>(event);
        if (!buttonEvent)
            return;

        m_rewindTypematic.press(buttonEvent->pressed);

        return;
    }
    if (event->type() == FastForward) {
        ButtonEvent *buttonEvent = dynamic_cast<ButtonEvent *>(event);
        if (!buttonEvent)
            return;

        m_fastForwardTypematic.press(buttonEvent->pressed);

        return;
    }

    if (event->type() == AddMarker) {
        slotAddMarker2();
        return;
    }

    if (event->type() == PreviousMarker) {
        slotPreviousMarker();
        return;
    }

    if (event->type() == NextMarker) {
        slotNextMarker();
        return;
    }
}

void
RosegardenMainWindow::slotCommandExecuted()
{
    // Refresh the display.
    update();

    // Update clipboard action states.
    slotTestClipboard();

    // Refresh the TransportDialog's time display.
    // This is needed for time signature and tempo changes.
    // See Bug #1723.
    // ??? This breaks the matrix editor.  When adding notes, the note previews
    //     last a very long time with this in place.
    //slotSetPointerPosition(
    //        RosegardenDocument::currentDocument->getComposition().getPosition());
}


}  // end namespace Rosegarden
