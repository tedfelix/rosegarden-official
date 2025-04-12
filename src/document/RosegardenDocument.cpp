/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[RosegardenDocument]"
#define RG_NO_DEBUG_PRINT

#include "RosegardenDocument.h"

#include "CommandHistory.h"
#include "RoseXmlHandler.h"
#include "GzipFile.h"

#include "base/AudioDevice.h"
#include "base/AudioPluginInstance.h"
#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/Configuration.h"
#include "base/Device.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/SegmentLinker.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/RealTime.h"
#include "base/RecordIn.h"
#include "base/Segment.h"
#include "base/SoftSynthDevice.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "base/XmlExportable.h"
#include "commands/edit/EventQuantizeCommand.h"
#include "commands/notation/NormalizeRestsCommand.h"
#include "commands/segment/AddTracksCommand.h"
#include "commands/segment/SegmentInsertCommand.h"
#include "commands/segment/SegmentRecordCommand.h"
#include "commands/segment/ChangeCompositionLengthCommand.h"
#include "commands/segment/MergeFileCommand.h"
#include "gui/editors/segment/TrackEditor.h"
#include "gui/editors/segment/TrackButtons.h"
#include "gui/general/ClefIndex.h"
#include "gui/application/TransportStatus.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/application/RosegardenMainViewWidget.h"
#include "gui/dialogs/UnusedAudioSelectionDialog.h"
#include "gui/editors/segment/compositionview/AudioPeaksThread.h"
#include "gui/editors/segment/TrackLabel.h"
// !!! DO NOT REMOVE THIS!
//     qDeleteAll() needs this in order to do its work correctly.
//     Without EditViewBase.h, we crash on close whenever an editor is up.
#include "gui/general/EditViewBase.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/ResourceFinder.h"
#include "gui/widgets/StartupLogo.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/studio/AudioPluginManager.h"
#include "gui/studio/StudioControl.h"
#include "gui/general/AutoSaveFinder.h"
#include "gui/studio/AudioPluginGUIManager.h"
#include "sequencer/RosegardenSequencer.h"
#include "sound/AudioFile.h"
#include "sound/AudioFileManager.h"
#include "sound/ExternalController.h"
#include "sound/MappedCommon.h"
#include "sound/MappedEventList.h"
#include "sound/MappedDevice.h"
#include "sound/MappedInstrument.h"
#include "sound/MappedEvent.h"
#include "sound/MappedStudio.h"
#include "sound/PluginIdentifier.h"
#include "sound/SoundDriver.h"
#include "sound/Midi.h"
#include "misc/AppendLabel.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "document/Command.h"
#include "document/io/XMLReader.h"
#include "misc/ConfigGroups.h"
#include "misc/Preferences.h"

#include "rosegarden-version.h"

#include <QApplication>
#include <QSettings>
#include <QMessageBox>
#include <QProcess>
#include <QTemporaryFile>
#include <QByteArray>
#include <QDataStream>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QWidget>
#include <QHostInfo>
#include <QLockFile>

// ??? Get rid of this.
using namespace Rosegarden::BaseProperties;


namespace Rosegarden
{


RosegardenDocument *RosegardenDocument::currentDocument{};

RosegardenDocument::RosegardenDocument(
        QObject *parent,
        QSharedPointer<AudioPluginManager> audioPluginManager,
        bool skipAutoload,
        bool clearCommandHistory,
        bool enableSound,
        const QString& path) :
    QObject(parent),
    m_modified(false),
    m_autoSaved(false),
    m_lockFile(nullptr),
    m_audioFileManager(this),
    m_audioPeaksThread(&m_audioFileManager),
    m_seqManager(nullptr),
    m_pluginManager(audioPluginManager),
    m_audioRecordLatency(0, 0),
    m_quickMarkerTime(-1),
    m_autoSavePeriod(0),
    m_beingDestroyed(false),
    m_clearCommandHistory(clearCommandHistory),
    m_soundEnabled(enableSound)
{
    connect(CommandHistory::getInstance(), &CommandHistory::commandExecuted,
            this, &RosegardenDocument::slotDocumentModified);

    connect(CommandHistory::getInstance(), &CommandHistory::documentRestored,
            this, &RosegardenDocument::slotDocumentRestored);

    // autoload a new document
    if (!skipAutoload)
        performAutoload();

    // now set it up as a "new document"
    newDocument(path);
}

RosegardenDocument::~RosegardenDocument()
{
    RG_DEBUG << "dtor";

    m_beingDestroyed = true;

    m_audioPeaksThread.finish();
    m_audioPeaksThread.wait();

    deleteEditViews();

    //     ControlRulerCanvasRepository::clear();

    if (m_clearCommandHistory)
        CommandHistory::getInstance()->clear(); // before Composition is deleted

    release();
}

unsigned int
RosegardenDocument::getAutoSavePeriod() const
{
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    unsigned int ret;
    ret = settings.value("autosaveinterval", 60).toUInt();

    settings.endGroup();        // corresponding to: settings.beginGroup( GeneralOptionsConfigGroup );
    return ret;
}

void RosegardenDocument::attachView(RosegardenMainViewWidget *view)
{
    m_viewList.append(view);
}

/* unused
void RosegardenDocument::detachView(RosegardenMainViewWidget *view)
{
    m_viewList.removeOne(view);
}
*/

void RosegardenDocument::attachEditView(EditViewBase *view)
{
    m_editViewList.append(view);
}

void RosegardenDocument::detachEditView(EditViewBase *view)
{
    // auto-deletion is disabled, as
    // the editview detaches itself when being deleted
    m_editViewList.removeOne(view);
}

void RosegardenDocument::deleteEditViews()
{
    QList<EditViewBase*> views = m_editViewList;
    m_editViewList.clear();
    qDeleteAll(views);
}

void RosegardenDocument::setAbsFilePath(const QString &filename)
{
    m_absFilePath = filename;
}

void RosegardenDocument::setTitle(const QString &title)
{
    m_title = title;
}

const QString &RosegardenDocument::getAbsFilePath() const
{
    return m_absFilePath;
}

void RosegardenDocument::deleteAutoSaveFile()
{
    QFile::remove(getAutoSaveFileName());
}

const QString& RosegardenDocument::getTitle() const
{
    return m_title;
}

void RosegardenDocument::slotUpdateAllViews(RosegardenMainViewWidget *sender)
{
    RG_DEBUG << "RosegardenDocument::slotUpdateAllViews";
    for (int i = 0; i < int(m_viewList.size()); ++i ){
        if (m_viewList.at(i) != sender) {
            // try to fix another crash, though I don't really understand
            // exactly what m_viewList is, etc.
            if (m_viewList.at(i)) {
                m_viewList.at(i)->update();
            }
        }
    }
}

void RosegardenDocument::setModified()
{
    // Already set?  Bail.
    if (m_modified)
        return;

    m_modified = true;
    m_autoSaved = false;

    // Make sure the star (*) appears in the title bar.
    RosegardenMainWindow::self()->slotUpdateTitle(true);
}

void RosegardenDocument::clearModifiedStatus()
{
    m_modified = false;
    m_autoSaved = true;

    emit documentModified(false);
}

void RosegardenDocument::slotDocumentModified()
{
    m_modified = true;
    m_autoSaved = false;

    m_composition.invalidateDurationCache();

    emit documentModified(true);
}

void RosegardenDocument::slotDocumentRestored()
{
    RG_DEBUG << "RosegardenDocument::slotDocumentRestored()";
    m_modified = false;

    // if we hit the bottom of the undo stack, emit this so the modified flag
    // will be cleared from assorted title bars
    emit documentModified(false);
}

void
RosegardenDocument::setQuickMarker()
{
    RG_DEBUG << "RosegardenDocument::setQuickMarker";

    m_quickMarkerTime = getComposition().getPosition();
}

void
RosegardenDocument::jumpToQuickMarker()
{
    RG_DEBUG << "RosegardenDocument::jumpToQuickMarker";

    if (m_quickMarkerTime >= 0)
        slotSetPointerPosition(m_quickMarkerTime);
}

QString RosegardenDocument::getAutoSaveFileName()
{
    QString filename = getAbsFilePath();
    //!!! NB this should _not_ use the new TempDirectory class -- that
    //!!! is for files that are more temporary than this.  Its files
    //!!! are cleaned up after a crash, the next time RG is started,
    //!!! so they aren't appropriate for recovery purposes.

    QString autoSaveFileName = AutoSaveFinder().getAutoSavePath(filename);
    RG_DEBUG << "RosegardenDocument::getAutoSaveFilename(): returning "
             << autoSaveFileName;

    return autoSaveFileName;
}

void RosegardenDocument::slotAutoSave()
{
    //     RG_DEBUG << "RosegardenDocument::slotAutoSave()";

    if (isAutoSaved() || !isModified())
        return ;

    QString autoSaveFileName = getAutoSaveFileName();

    RG_DEBUG << "RosegardenDocument::slotAutoSave() - doc modified - saving '"
    << getAbsFilePath() << "' as"
    << autoSaveFileName;

    QString errMsg;

    saveDocument(autoSaveFileName, errMsg, true);

}

bool RosegardenDocument::isRegularDotRGFile() const
{
    return getAbsFilePath().right(3).toLower() == ".rg";
}

bool
RosegardenDocument::deleteOrphanedAudioFiles(bool documentWillNotBeSaved)
{
    std::vector<QString> recordedOrphans;
    std::vector<QString> derivedOrphans;

    if (documentWillNotBeSaved) {

        // All audio files recorded or derived in this session are
        // about to become orphans

        for (AudioFileVector::const_iterator i =
                    m_audioFileManager.cbegin();
                i != m_audioFileManager.cend(); ++i) {

            if (m_audioFileManager.wasAudioFileRecentlyRecorded((*i)->getId())) {
                recordedOrphans.push_back((*i)->getAbsoluteFilePath());
            }

            if (m_audioFileManager.wasAudioFileRecentlyDerived((*i)->getId())) {
                derivedOrphans.push_back((*i)->getAbsoluteFilePath());
            }
        }
    }

    // Whether we save or not, explicitly orphaned (i.e. recorded in
    // this session and then unloaded) recorded files are orphans.
    // Make sure they are actually unknown to the audio file manager
    // (i.e. they haven't been loaded more than once, or reloaded
    // after orphaning).

    for (std::vector<QString>::iterator i = m_orphanedRecordedAudioFiles.begin();
            i != m_orphanedRecordedAudioFiles.end(); ++i) {

        bool stillHave = false;

        for (AudioFileVector::const_iterator j =
                 m_audioFileManager.cbegin();
                j != m_audioFileManager.cend(); ++j) {
            if ((*j)->getAbsoluteFilePath() == *i) {
                stillHave = true;
                break;
            }
        }

        if (!stillHave) recordedOrphans.push_back(*i);
    }

    // Derived orphans get deleted whatever happens
    //!!! Should we orphan any file derived during this session that
    //is not currently used in a segment?  Probably: we have no way to
    //reuse them

    for (std::vector<QString>::iterator i = m_orphanedDerivedAudioFiles.begin();
            i != m_orphanedDerivedAudioFiles.end(); ++i) {

        bool stillHave = false;

        for (AudioFileVector::const_iterator j =
                 m_audioFileManager.cbegin();
                j != m_audioFileManager.cend(); ++j) {
            if ((*j)->getAbsoluteFilePath() == *i) {
                stillHave = true;
                break;
            }
        }

        if (!stillHave) derivedOrphans.push_back(*i);
    }

    for (size_t i = 0; i < derivedOrphans.size(); ++i) {
        QFile file(derivedOrphans[i]);
        if (!file.remove()) {
            std::cerr << "WARNING: Failed to remove orphaned derived audio file \"" << derivedOrphans[i] << std::endl;
        }
        QFile peakFile(QString("%1.pk").arg(derivedOrphans[i]));
        peakFile.remove();
    }

    m_orphanedDerivedAudioFiles.clear();

    if (recordedOrphans.empty())
        return true;

    if (documentWillNotBeSaved) {

        int reply = QMessageBox::warning
            (dynamic_cast<QWidget *>(parent()), "Warning",
             tr("Delete the %n audio file(s) recorded during the unsaved session?", "", recordedOrphans.size()),
             QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
             QMessageBox::Cancel);

        switch (reply) {

        case QMessageBox::Yes:
            break;

        case QMessageBox::No:
            return true;

        default:
        case QMessageBox::Cancel:
            return false;
        }

    } else {

        UnusedAudioSelectionDialog *dialog =
            new UnusedAudioSelectionDialog
            (dynamic_cast<QWidget *>(parent()),
             tr("The following audio files were recorded during this session but have been unloaded\nfrom the audio file manager, and so are no longer in use in the document you are saving.\n\nYou may want to clean up these files to save disk space.\n\nPlease select any you wish to delete permanently from the hard disk.\n"),
             recordedOrphans);

        if (dialog->exec() != QDialog::Accepted) {
            delete dialog;
            return true;
        }

        recordedOrphans = dialog->getSelectedAudioFileNames();
        delete dialog;
    }

    if (recordedOrphans.empty())
        return true;

    QString question =
        tr("<qt>About to delete %n audio file(s) permanently from the hard disk.<br>There will be no way to recover the file(s).<br>Are you sure?</qt>", "", recordedOrphans.size());

    int reply = QMessageBox::warning(dynamic_cast<QWidget *>(parent()), "Warning", question, QMessageBox::Ok | QMessageBox::Cancel);

    if (reply == QMessageBox::Ok) {
        for (size_t i = 0; i < recordedOrphans.size(); ++i) {
            QFile file(recordedOrphans[i]);
            if (!file.remove()) {
                QMessageBox::critical(dynamic_cast<QWidget *>(parent()), tr("Rosegarden"), tr("File %1 could not be deleted.")
                                    .arg(recordedOrphans[i]));
            }

            QFile peakFile(QString("%1.pk").arg(recordedOrphans[i]));
            peakFile.remove();
        }
    }

    return true;
}

void RosegardenDocument::newDocument(const QString& path)
{
    m_modified = false;
    if (path != "") {
        openDocument(path);
        m_modified = true;
    }
    setAbsFilePath(QString());
    setTitle(tr("Untitled"));
    if (m_clearCommandHistory) CommandHistory::getInstance()->clear();
}

void RosegardenDocument::performAutoload()
{
    QString autoloadFile = ResourceFinder().getAutoloadPath();

    QFileInfo autoloadFileInfo(autoloadFile);

    if (autoloadFile == "" || !autoloadFileInfo.isReadable()) {
        std::cerr << "WARNING: RosegardenDocument::performAutoload - "
                  << "can't find autoload file - defaulting" << std::endl;
        return ;
    }

    openDocument(
            autoloadFile,  // filename
            m_soundEnabled,  // permanent
            true,  // squelchProgressDialog
            false);  // enableLock
}

bool RosegardenDocument::openDocument(const QString &filename,
                                      bool permanent,
                                      bool squelchProgressDialog,
                                      bool enableLock)
{
    RG_DEBUG << "openDocument(" << filename << ")";

    if (filename.isEmpty())
        return false;

    newDocument();

    QFileInfo fileInfo(filename);
    setTitle(fileInfo.fileName());

    // If the file cannot be read, or it's a directory
    if (!fileInfo.isReadable() || fileInfo.isDir()) {
        StartupLogo::hideIfStillThere();

        QString msg(tr("Can't open file '%1'").arg(filename));
        QMessageBox::warning(dynamic_cast<QWidget *>(parent()),
                             tr("Rosegarden"), msg);

        return false;
    }

    // Progress Dialog
    // Note: The label text and range will be set later as needed.
    QProgressDialog progressDialog(
            tr("Reading file..."),  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 100,  // min, max
            RosegardenMainWindow::self());  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // Don't want to auto close since this is a multi-step
    // process.  Any of the steps may set progress to 100.  We
    // will close anyway when this object goes out of scope.
    progressDialog.setAutoClose(false);
    // We're usually a bit late to the game here as it is.  Shave off a
    // couple of seconds to make up for it.
    // ??? We should move the progress dialog further up the call chain
    //     to include the additional time.
    //progressDialog.setMinimumDuration(2000);

    m_progressDialog = &progressDialog;

    if (squelchProgressDialog) {
        m_progressDialog = nullptr;
    } else {
        // Just force the progress dialog up.
        // Both Qt4 and Qt5 have bugs related to delayed showing of progress
        // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
        // based distros might lock up.  See Bug #1546.
        progressDialog.show();
    }

    setAbsFilePath(fileInfo.absoluteFilePath());

    // Lock.

    if (permanent  &&  enableLock) {
        if (!lock()) {
            // Avoid deleting the lock file by clearing out this document.
            newDocument();

            return false;
        }
    }

    // Load.

    QString fileContents;

    // Unzip
    bool okay = GzipFile::readFromFile(filename, fileContents);

    QString errMsg;
    bool cancelled = false;

    if (!okay) {
        errMsg = tr("Could not open Rosegarden file");
    } else {
        // Parse the XML
        okay = xmlParse(fileContents,
                        errMsg,
                        permanent,
                        cancelled);
    }

    if (!okay) {
        StartupLogo::hideIfStillThere();

        QString msg(tr("Error when parsing file '%1':<br />\"%2\"")
                     .arg(filename)
                     .arg(errMsg));
        QMessageBox::warning(dynamic_cast<QWidget *>(parent()), tr("Rosegarden"), msg);

        return false;
    }

    if (cancelled) {
        // Don't leave a lock file around.
        release();

        newDocument();

        return false;
    }

    RG_DEBUG << "openDocument() - "
             << "m_composition : " << &m_composition
             << " - m_composition->getNbSegments() : "
             << m_composition.getNbSegments()
             << " - m_composition->getDuration() : "
             << m_composition.getDuration();

    if (m_composition.begin() != m_composition.end()) {
        RG_DEBUG << "First segment starts at " << (*m_composition.begin())->getStartTime();
    }

    m_audioFileManager.setProgressDialog(m_progressDialog);

    try {
        // generate any audio previews after loading the files
        m_audioFileManager.generatePreviews();
    } catch (const Exception &e) {
        StartupLogo::hideIfStillThere();
        QMessageBox::critical(dynamic_cast<QWidget *>(parent()), tr("Rosegarden"), strtoqstr(e.getMessage()));
    }

    RG_DEBUG << "openDocument(): Successfully opened document \"" << filename << "\"";

    return true;
}

void
RosegardenDocument::stealLockFile(RosegardenDocument *other)
{
    Q_ASSERT(!m_lockFile);
    m_lockFile = other->m_lockFile;
    other->m_lockFile = nullptr;
}

void
RosegardenDocument::mergeDocument(RosegardenDocument *srcDoc,
                                  bool mergeAtEnd,
                                  bool mergeTimesAndTempos)
{
    // Merging from the merge source (srcDoc) into the merge destination
    // (this).

    CommandHistory::getInstance()->addCommand(
            new MergeFileCommand(
                    srcDoc,
                    mergeAtEnd,
                    mergeTimesAndTempos));
}

void RosegardenDocument::sendChannelSetups(bool reset)
{
    std::set<DeviceId> devicesSeen;
    std::set<InstrumentId> instrumentsSeen;

    // For each track in the composition, send the channel setup
    for (Composition::TrackMap::const_iterator i =
             m_composition.getTracks().begin();
         i != m_composition.getTracks().end();
         ++i) {

        //const TrackId trackId = i->first;
        const Track *track = i->second;

        const InstrumentId instrumentId = track->getInstrument();

        // If we've already seen this instrument, try the next track.
        if (instrumentsSeen.find(instrumentId) != instrumentsSeen.end())
            continue;

        instrumentsSeen.insert(instrumentId);

        Instrument *instrument = m_studio.getInstrumentById(instrumentId);

        // If this instrument doesn't exist, try the next track.
        if (!instrument)
            continue;

        // If this isn't a MIDI instrument, try the next track.
        if (instrument->getType() != Instrument::Midi)
            continue;

        // The reset feature is experimental and unused (all callers
        // specify reset == false).  I suspect it might
        // cause trouble with some synths.  It's probably also
        // ignored by some.  Might want to make this configurable
        // via the .conf or maybe pop up a dialog to ask whether
        // to send resets.

        const DeviceId deviceId = instrument->getDevice()->getId();

        // If we've not seen this Device before
        if (reset  &&  devicesSeen.find(deviceId) == devicesSeen.end())
        {
            // Send a Reset
            // ??? Would it be better to send the resets, wait a few seconds,
            //     then send the channel setups?  Some hardware might need
            //     time to respond to a reset.

            MappedEvent mappedEvent;
            mappedEvent.setInstrumentId(instrumentId);
            mappedEvent.setType(MappedEvent::MidiSystemMessage);
            mappedEvent.setData1(MIDI_SYSTEM_RESET);

            StudioControl::sendMappedEvent(mappedEvent);

            // Add Device to devices we've seen.
            devicesSeen.insert(deviceId);
        }

        // If this instrument isn't in fixed channel mode, try the next track.
        if (!instrument->hasFixedChannel())
            continue;

        // Call Instrument::sendChannelSetup() to make sure the program
        // change for this track has been sent out.
        // The test case (MIPP #35) for this is a bit esoteric:
        //   1. Load a composition and play it.
        //   2. Load a different composition, DO NOT play it.
        //   3. Select tracks in the new composition and play the MIDI
        //      input keyboard.
        //   4. Verify that you hear the programs for the new composition.
        // Without the following, you'll hear the programs for the old
        // composition.
        instrument->sendChannelSetup();
    }
}

void RosegardenDocument::initialiseStudio()
{
    //Profiler profiler("initialiseStudio", true);

    RG_DEBUG << "initialiseStudio() begin...";

    // stop any running guis
    RosegardenMainWindow::self()->getPluginGUIManager()->stopAllGUIs();

    // Destroy all the mapped objects in the studio.
    RosegardenSequencer::getInstance()->clearStudio();

    // To reduce the number of DCOP calls at this stage, we put some
    // of the float property values in a big list and commit in one
    // single call at the end.  We can only do this with properties
    // that aren't depended on by other port, connection, or non-float
    // properties during the initialisation process.
    MappedObjectIdList ids;
    MappedObjectPropertyList properties;
    MappedObjectValueList values;

    // All the softsynths, audio instruments, and busses.
    std::vector<PluginContainer *> pluginContainers;

    BussList busses = m_studio.getBusses();

    // For each buss (first one is master)
    for (size_t i = 0; i < busses.size(); ++i) {

        MappedObjectId mappedId =
            StudioControl::createStudioObject(MappedObject::AudioBuss);

        StudioControl::setStudioObjectProperty(mappedId,
                                               MappedAudioBuss::BussId,
                                               MappedObjectValue(i));

        // Level
        ids.push_back(mappedId);
        properties.push_back(MappedAudioBuss::Level);
        values.push_back(MappedObjectValue(busses[i]->getLevel()));

        // Pan
        ids.push_back(mappedId);
        properties.push_back(MappedAudioBuss::Pan);
        values.push_back(MappedObjectValue(busses[i]->getPan()) - 100.0);

        busses[i]->setMappedId(mappedId);

        pluginContainers.push_back(busses[i]);
    }

    RecordInList recordIns = m_studio.getRecordIns();

    // For each record in
    for (size_t i = 0; i < recordIns.size(); ++i) {

        MappedObjectId mappedId =
            StudioControl::createStudioObject(MappedObject::AudioInput);

        StudioControl::setStudioObjectProperty(mappedId,
                                               MappedAudioInput::InputNumber,
                                               MappedObjectValue(i));

        recordIns[i]->mappedId = mappedId;
    }

    InstrumentList list = m_studio.getAllInstruments();

    // For each instrument
    for (InstrumentList::iterator it = list.begin();
         it != list.end();
         ++it) {
        Instrument &instrument = **it;

        if (instrument.getType() == Instrument::Audio  ||
            instrument.getType() == Instrument::SoftSynth) {

            MappedObjectId mappedId =
                StudioControl::createStudioObject(MappedObject::AudioFader);

            instrument.setMappedId(mappedId);

            //RG_DEBUG << "initialiseStudio(): Setting mapped object ID = " << mappedId << " - on Instrument " << (*it)->getId();

            StudioControl::setStudioObjectProperty(
                    mappedId,
                    MappedObject::Instrument,
                    MappedObjectValue(instrument.getId()));

            // Fader level
            ids.push_back(mappedId);
            properties.push_back(MappedAudioFader::FaderLevel);
            values.push_back(static_cast<MappedObjectValue>(instrument.getLevel()));

            // Fader record level
            ids.push_back(mappedId);
            properties.push_back(MappedAudioFader::FaderRecordLevel);
            values.push_back(static_cast<MappedObjectValue>(instrument.getRecordLevel()));

            // Channels
            ids.push_back(mappedId);
            properties.push_back(MappedAudioFader::Channels);
            values.push_back(static_cast<MappedObjectValue>(instrument.getNumAudioChannels()));

            // Pan
            ids.push_back(mappedId);
            properties.push_back(MappedAudioFader::Pan);
            values.push_back(static_cast<MappedObjectValue>(instrument.getPan()) - 100.0f);

            // Set up connections

            // Clear any existing connections (shouldn't be necessary, but)
            StudioControl::disconnectStudioObject(mappedId);

            // Handle the output connection.
            BussId outputBuss = instrument.getAudioOutput();
            if (outputBuss < (unsigned int)busses.size()) {
                MappedObjectId bussMappedId = busses[outputBuss]->getMappedId();

                if (bussMappedId > 0)
                    StudioControl::connectStudioObjects(mappedId, bussMappedId);
            }

            // Handle the input connection.
            bool isBuss;
            int channel;
            int input = instrument.getAudioInput(isBuss, channel);

            MappedObjectId inputMappedId = 0;

            if (isBuss) {
                if (input < static_cast<int>(busses.size()))
                    inputMappedId = busses[input]->getMappedId();
            } else {
                if (input < static_cast<int>(recordIns.size()))
                    inputMappedId = recordIns[input]->mappedId;
            }

            ids.push_back(mappedId);
            properties.push_back(MappedAudioFader::InputChannel);
            values.push_back(MappedObjectValue(channel));

            if (inputMappedId > 0)
                StudioControl::connectStudioObjects(inputMappedId, mappedId);

            pluginContainers.push_back(&instrument);

        }
    }

    sendChannelSetups(false);  // no resets

    RG_DEBUG << "initialiseStudio(): Have " << pluginContainers.size() << " plugin container(s)";

    // For each softsynth, audio instrument, and buss
    for (std::vector<PluginContainer *>::iterator pluginContainerIter =
                 pluginContainers.begin();
         pluginContainerIter != pluginContainers.end();
         ++pluginContainerIter) {

        PluginContainer &pluginContainer = **pluginContainerIter;

        // Initialise all the plugins for this Instrument or Buss

        // For each plugin within this instrument or buss
        for (AudioPluginVector::iterator pli = pluginContainer.beginPlugins();
             pli != pluginContainer.endPlugins();
             ++pli) {

            AudioPluginInstance &plugin = **pli;

            RG_DEBUG << "initialiseStudio(): Container id " << pluginContainer.getId()
                     << ", plugin position " << plugin.getPosition()
                     << ", identifier " << plugin.getIdentifier()
                     << ", assigned " << plugin.isAssigned();

            if (!plugin.isAssigned())
                continue;

            RG_DEBUG << "initialiseStudio(): Found an assigned plugin";

            // Plugin Slot
            MappedObjectId pluginMappedId =
                    StudioControl::createStudioObject(
                            MappedObject::PluginSlot);

            plugin.setMappedId(pluginMappedId);

            RG_DEBUG << "initialiseStudio(): Creating plugin ID = " << pluginMappedId;

            // Position
            StudioControl::setStudioObjectProperty(
                    pluginMappedId,
                    MappedObject::Position,
                    MappedObjectValue(plugin.getPosition()));

            // Instrument
            StudioControl::setStudioObjectProperty(
                    pluginMappedId,
                    MappedObject::Instrument,
                    pluginContainer.getId());

            // Identifier
            StudioControl::setStudioObjectProperty(
                    pluginMappedId,
                    MappedPluginSlot::Identifier,
                    plugin.getIdentifier().c_str());

            plugin.setConfigurationValue(
                    qstrtostr(PluginIdentifier::RESERVED_PROJECT_DIRECTORY_KEY),
                    qstrtostr(getAudioFileManager().getAbsoluteAudioPath()));

            // Set opaque string configuration data (e.g. for DSSI plugin)

            MappedObjectPropertyList config;

            for (AudioPluginInstance::ConfigMap::const_iterator i =
                         plugin.getConfiguration().begin();
                 i != plugin.getConfiguration().end();
                 ++i) {
                config.push_back(strtoqstr(i->first));
                config.push_back(strtoqstr(i->second));

                RG_DEBUG << "initialiseStudio(): plugin configuration: " << i->first << " -> " << i->second;
            }

            RG_DEBUG << "initialiseStudio(): plugin configuration: " << config.size() << " values";

            QString error = StudioControl::setStudioObjectPropertyList(
                    pluginMappedId,
                    MappedPluginSlot::Configuration,
                    config);

            if (error != "") {
                StartupLogo::hideIfStillThere();
//                if (m_progressDialog)
//                    m_progressDialog->hide();
                QMessageBox::warning(dynamic_cast<QWidget *>(parent()), tr("Rosegarden"), error);
//                if (m_progressDialog)
//                    m_progressDialog->show();
            }

            // Bypassed
            ids.push_back(pluginMappedId);
            properties.push_back(MappedPluginSlot::Bypassed);
            values.push_back(MappedObjectValue(plugin.isBypassed()));

            // Port Values

            for (PortInstanceIterator portIt = plugin.begin();
                 portIt != plugin.end();
                 ++portIt) {
                StudioControl::setStudioPluginPort(pluginMappedId,
                                                   (*portIt)->number,
                                                   (*portIt)->value);
            }

            // Program
            if (plugin.getProgram() != "") {
                StudioControl::setStudioObjectProperty(
                        pluginMappedId,
                        MappedPluginSlot::Program,
                        strtoqstr(plugin.getProgram()));
            }

            // Set the post-program port values
            // ??? Why?
            for (PortInstanceIterator portIt = plugin.begin();
                 portIt != plugin.end();
                 ++portIt) {
                if ((*portIt)->changedSinceProgramChange) {
                    StudioControl::setStudioPluginPort(pluginMappedId,
                                                       (*portIt)->number,
                                                       (*portIt)->value);
                }
            }
        }
    }

    // Now commit all the remaining changes
    StudioControl::setStudioObjectProperties(ids, properties, values);

    QSettings settings;
    settings.beginGroup(SequencerOptionsConfigGroup);

    bool faderOuts = qStrToBool( settings.value("audiofaderouts", "false") ) ;
    bool submasterOuts = qStrToBool( settings.value("audiosubmasterouts", "false") ) ;
    unsigned int audioFileFormat = settings.value("audiorecordfileformat", 1).toUInt() ;

    settings.endGroup();

    // Send System Audio Ports Event

    MidiByte ports = 0;

    if (faderOuts)
        ports |= MappedEvent::FaderOuts;

    if (submasterOuts)
        ports |= MappedEvent::SubmasterOuts;

    MappedEvent mEports;
    mEports.setInstrumentId(MidiInstrumentBase);  // ??? needed?
    mEports.setType(MappedEvent::SystemAudioPorts);
    mEports.setData1(ports);
    StudioControl::sendMappedEvent(mEports);

    // Send System Audio File Format Event

    MappedEvent mEff;
    mEff.setInstrumentId(MidiInstrumentBase);  // ??? needed?
    mEff.setType(MappedEvent::SystemAudioFileFormat);
    mEff.setData1(audioFileFormat);

    StudioControl::sendMappedEvent(mEff);
}

SequenceManager *
RosegardenDocument::getSequenceManager()
{
    return m_seqManager;
}

void RosegardenDocument::setSequenceManager(SequenceManager *sm)
{
    m_seqManager = sm;
}


// FILE FORMAT VERSION NUMBERS
//
// These should be updated when the file format changes.  The
// intent is to warn the user that they are loading a file that
// was saved with a newer version of Rosegarden, and data might
// be lost as a result.  See RoseXmlHandler::startElement().
//
// Increment the major version number only for updates so
// substantial that we shouldn't bother even trying to read a file
// saved with a newer major version number than our own.  Older
// versions of Rosegarden *will not* try to load files with
// newer major version numbers.  Basically, this should be done
// only as a last resort to lock out all older versions of
// Rosegarden from reading in and completely mangling the contents
// of a file.
//
// Increment the minor version number for updates that may break
// compatibility such that we should warn when reading a file
// that was saved with a newer minor version than our own.
//
// Increment the point version number for updates that shouldn't
// break compatibility in either direction, just for informational
// purposes.
//
// When updating major, reset minor to zero; when updating minor,
// reset point to zero.
//
int RosegardenDocument::FILE_FORMAT_VERSION_MAJOR = 1;
int RosegardenDocument::FILE_FORMAT_VERSION_MINOR = 6;
// Version 10 introduces LV2 plugins and provides values that
// older versions will interpret as plugins that weren't found.
// Older versions will issue helpful "plugin not found" messages.
int RosegardenDocument::FILE_FORMAT_VERSION_POINT = 10;

bool RosegardenDocument::saveDocument(const QString& filename,
                                    QString& errMsg,
                                    bool autosave)
{
    QFileInfo fileInfo(filename);

    if (!fileInfo.exists()) { // safe to write directly
        return saveDocumentActual(filename, errMsg, autosave);
    }

    if (fileInfo.exists()  &&  !fileInfo.isWritable()) {
        errMsg = tr("'%1' is read-only.  Please save to a different file.").arg(filename);
        return false;
    }

    QTemporaryFile temp(filename + ".");
    //!!! was: KTempFile temp(filename + ".", "", 0644); // will be umask'd

    temp.setAutoRemove(false);

    temp.open(); // This creates the file and opens it atomically

    if ( temp.error() ) {
        errMsg = tr("Could not create temporary file in directory of '%1': %2").arg(filename).arg(temp.errorString());        //### removed .arg(strerror(status))
        return false;
    }

    QString tempFileName = temp.fileName(); // Must do this before temp.close()

    // The temporary file is now open: close it (without removing it)
    temp.close();

    if( temp.error() ){
        //status = temp.status();
        errMsg = tr("Failure in temporary file handling for file '%1': %2")
            .arg(tempFileName).arg(temp.errorString()); // .arg(strerror(status))
        return false;
    }

    bool success = saveDocumentActual(tempFileName, errMsg, autosave);

    if (!success) {
        // errMsg should be already set
        return false;
    }

    QDir dir(QFileInfo(tempFileName).dir());
    // According to  http://doc.trolltech.com/4.4/qdir.html#rename
    // some systems fail, if renaming over an existing file.
    // Therefore, delete first the existing file.
    if (dir.exists(filename)) dir.remove(filename);
    if (!dir.rename(tempFileName, filename)) {
        errMsg = tr("Failed to rename temporary output file '%1' to desired output file '%2'").arg(tempFileName).arg(filename);
        return false;
    }

    return true;
}


bool RosegardenDocument::saveDocumentActual(const QString& filename,
                                          QString& errMsg,
                                          bool autosave)
{
    //Profiler profiler("RosegardenDocument::saveDocumentActual");

    RG_DEBUG << "RosegardenDocument::saveDocumentActual(" << filename << ")";

    QString outText;
    QTextStream outStream(&outText, QIODevice::WriteOnly);
//    outStream.setEncoding(QTextStream::UnicodeUTF8); qt3
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    // qt6 default codec is UTF-8
#else
    outStream.setCodec("UTF-8");
#endif

    // output XML header
    //
    outStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    << "<!DOCTYPE rosegarden-data>\n"
    << "<rosegarden-data version=\"" << VERSION
    << "\" format-version-major=\"" << FILE_FORMAT_VERSION_MAJOR
    << "\" format-version-minor=\"" << FILE_FORMAT_VERSION_MINOR
    << "\" format-version-point=\"" << FILE_FORMAT_VERSION_POINT
    << "\">\n";

    // First make sure all MIDI devices know their current connections
    //
    m_studio.resyncDeviceConnections();

    // tell plugins to save state
    RosegardenSequencer::getInstance()->savePluginState();

    // Send out Composition (this includes Tracks, Instruments, Tempo
    // and Time Signature changes and any other sub-objects)
    //
    outStream << strtoqstr(getComposition().toXmlString())
              << "\n\n";

    outStream << strtoqstr(getAudioFileManager().toXmlString())
              << "\n\n";

    outStream << strtoqstr(getConfiguration().toXmlString())
              << "\n\n";

    long totalEvents = 0;
    for (Composition::iterator segitr = m_composition.begin();
         segitr != m_composition.end(); ++segitr) {
        totalEvents += (long)(*segitr)->size();
    }

    for (Composition::TriggerSegmentSet::iterator ci =
             m_composition.getTriggerSegments().begin();
         ci != m_composition.getTriggerSegments().end(); ++ci) {
        totalEvents += (long)(*ci)->getSegment()->size();
    }

    // output all elements
    //
    // Iterate on segments
    long eventCount = 0;

    // Put a break in the file
    //
    outStream << "\n\n";

    for (Composition::iterator segitr = m_composition.begin();
         segitr != m_composition.end(); ++segitr) {

        Segment *segment = *segitr;

        // Fix #1446 : Replace isLinked() with isTrulyLinked().
        // Maybe this fix will need to be removed some day if the
        // LinkTransposeParams come to be used.
        if (segment->isTrulyLinked()) {
            QString attsString = QString("linkerid=\"%1\" ");
            attsString += QString("linkertransposechangekey=\"%2\" ");
            attsString += QString("linkertransposesteps=\"%3\" ");
            attsString += QString("linkertransposesemitones=\"%4\" ");
            attsString += QString("linkertransposesegmentback=\"%5\" ");
            QString linkedSegAtts = QString(attsString)
              .arg(segment->getLinker()->getSegmentLinkerId())
              .arg(segment->getLinkTransposeParams().m_changeKey ? "true" :
                                                                   "false")
              .arg(segment->getLinkTransposeParams().m_steps)
              .arg(segment->getLinkTransposeParams().m_semitones)
              .arg(segment->getLinkTransposeParams().m_transposeSegmentBack
                                                         ? "true" : "false");

            saveSegment(outStream, segment, totalEvents,
                                            eventCount, linkedSegAtts);
        } else {
            saveSegment(outStream, segment, totalEvents, eventCount);
        }

    }

    // Put a break in the file
    //
    outStream << "\n\n";

    for (Composition::TriggerSegmentSet::iterator ci =
                m_composition.getTriggerSegments().begin();
            ci != m_composition.getTriggerSegments().end(); ++ci) {

        QString triggerAtts = QString
                              ("triggerid=\"%1\" triggerbasepitch=\"%2\" triggerbasevelocity=\"%3\" triggerretune=\"%4\" triggeradjusttimes=\"%5\" ")
                              .arg((*ci)->getId())
                              .arg((*ci)->getBasePitch())
                              .arg((*ci)->getBaseVelocity())
                              .arg((*ci)->getDefaultRetune())
                              .arg(strtoqstr((*ci)->getDefaultTimeAdjust()));

        Segment *segment = (*ci)->getSegment();
        saveSegment(outStream, segment, totalEvents, eventCount, triggerAtts);
    }

    // Put a break in the file
    //
    outStream << "\n\n";

    // Send out the studio - a self contained command
    //
    outStream << strtoqstr(m_studio.toXmlString()) << "\n\n";

    // Send out the appearance data
    outStream << "<appearance>\n";
    outStream << strtoqstr(getComposition().getSegmentColourMap().toXmlString("segmentmap"));
    outStream << strtoqstr(getComposition().getGeneralColourMap().toXmlString("generalmap"));
    outStream << "</appearance>\n\n\n";

    // close the top-level XML tag
    //
    outStream << "</rosegarden-data>\n";

    bool okay = GzipFile::writeToFile(filename, outText);
    if (!okay) {
        errMsg = tr("Error while writing on '%1'").arg(filename);
        return false;
    }

    RG_DEBUG << "RosegardenDocument::saveDocument() finished";

    if (!autosave) {
        m_modified = false;
        emit documentModified(false);
        CommandHistory::getInstance()->documentSaved();
    }

    setAutoSaved(true);

    return true;
}

bool RosegardenDocument::exportStudio(const QString& filename,
                                      QString &errMsg,
                                      std::vector<DeviceId> devices)
{
    Profiler profiler("RosegardenDocument::exportStudio");
    RG_DEBUG << "RosegardenDocument::exportStudio(" << filename << ")";

    QString outText;
    QTextStream outStream(&outText, QIODevice::WriteOnly);
//    outStream.setEncoding(QTextStream::UnicodeUTF8); qt3
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    // qt6 default codec is UTF-8
#else
    outStream.setCodec("UTF-8");
#endif

    // output XML header
    //
    outStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
              << "<!DOCTYPE rosegarden-data>\n"
              << "<rosegarden-data version=\"" << VERSION << "\">\n";

    // Send out the studio - a self contained command
    //
    outStream << strtoqstr(m_studio.toXmlString(devices)) << "\n\n";

    // close the top-level XML tag
    //
    outStream << "</rosegarden-data>\n";

    bool okay = GzipFile::writeToFile(filename, outText);
    if (!okay) {
        errMsg = tr("Could not open file '%1' for writing").arg(filename);
        return false;
    }

    RG_DEBUG << "RosegardenDocument::exportStudio() finished";
    return true;
}

void RosegardenDocument::saveSegment(QTextStream& outStream, Segment *segment,
                                   long /*totalEvents*/, long &/*count*/,
                                   QString extraAttributes)
{
    QString time;

    outStream << QString("<%1 track=\"%2\" start=\"%3\" ")
    .arg(segment->getXmlElementName())
    .arg(segment->getTrack())
    .arg(segment->getStartTime());

    if (!extraAttributes.isEmpty())
        outStream << extraAttributes << " ";

    outStream << "label=\"" <<
    strtoqstr(XmlExportable::encode(segment->getLabel()));

    if (segment->isRepeating()) {
        outStream << "\" repeat=\"true";
    }

    if (segment->getTranspose() != 0) {
        outStream << "\" transpose=\"" << segment->getTranspose();
    }

    if (segment->getDelay() != 0) {
        outStream << "\" delay=\"" << segment->getDelay();
    }

    if (segment->getRealTimeDelay() != RealTime::zero()) {
        outStream << "\" rtdelaysec=\"" << segment->getRealTimeDelay().sec
        << "\" rtdelaynsec=\"" << segment->getRealTimeDelay().nsec;
    }

    if (segment->getColourIndex() != 0) {
        outStream << "\" colourindex=\"" << segment->getColourIndex();
    }

    if (segment->getSnapGridSize() != -1) {
        outStream << "\" snapgridsize=\"" << segment->getSnapGridSize();
    }

    if (segment->getViewFeatures() != 0) {
        outStream << "\" viewfeatures=\"" << segment->getViewFeatures();
    }

    if (segment->getExcludeFromPrinting()) {
        // For compatibility with older versions of rg.
        outStream << "\" fornotation=\"" << "false";
        // New value to match UI.
        outStream << "\" excludefromprinting=\"" << "true";
    }

    const timeT *endMarker = segment->getRawEndMarkerTime();
    if (endMarker) {
        outStream << "\" endmarker=\"" << *endMarker;
    }

    if (segment->getType() == Segment::Audio) {

        outStream << "\" type=\"audio\" "
                  << "file=\""
                  << segment->getAudioFileId();

        if (segment->getStretchRatio() != 1.f &&
            segment->getStretchRatio() != 0.f) {

            outStream << "\" unstretched=\""
                      << segment->getUnstretchedFileId()
                      << "\" stretch=\""
                      << segment->getStretchRatio();
        }

        outStream << "\">\n";

        // convert out - should do this as XmlExportable really
        // once all this code is centralised
        //

        outStream << "    <begin index=\""
        << segment->getAudioStartTime()
        << "\"/>\n";

        outStream << "    <end index=\""
        << segment->getAudioEndTime()
        << "\"/>\n";

        if (segment->isAutoFading()) {
            outStream << "    <fadein time=\""
            << segment->getFadeInTime()
            << "\"/>\n";

            outStream << "    <fadeout time=\""
            << segment->getFadeOutTime()
            << "\"/>\n";
        }

    } else // Internal type
    {
        outStream << "\">\n";

        bool inChord = false;
        timeT chordStart = 0, chordDuration = 0;
        timeT expectedTime = segment->getStartTime();

        for (Segment::iterator i = segment->begin();
                i != segment->end(); ++i) {

            timeT absTime = (*i)->getAbsoluteTime();

            Segment::iterator nextEl = i;
            ++nextEl;

            if (nextEl != segment->end() &&
                    (*nextEl)->getAbsoluteTime() == absTime &&
                    (*i)->getDuration() != 0 &&
                    !inChord) {
                outStream << "<chord>\n";
                inChord = true;
                chordStart = absTime;
                chordDuration = 0;
            }

            if (inChord && (*i)->getDuration() > 0)
                if (chordDuration == 0 || (*i)->getDuration() < chordDuration)
                    chordDuration = (*i)->getDuration();

            outStream << '\t'
            << strtoqstr((*i)->toXmlString(expectedTime)) << "\n";

            if (nextEl != segment->end() &&
                    (*nextEl)->getAbsoluteTime() != absTime &&
                    inChord) {
                outStream << "</chord>\n";
                inChord = false;
                expectedTime = chordStart + chordDuration;
            } else if (inChord) {
                expectedTime = absTime;
            } else {
                expectedTime = absTime + (*i)->getDuration();
            }

//            if ((++count % 500 == 0) && progress) {
//                progress->setValue(count * 100 / totalEvents);
//            }
        }

        if (inChord) {
            outStream << "</chord>\n";
        }

        // <matrix>

        outStream << "  <matrix>\n";

        // Zoom factors
        outStream << "    <hzoom factor=\"" << segment->matrixHZoomFactor <<
                     "\" />\n";
        outStream << "    <vzoom factor=\"" << segment->matrixVZoomFactor <<
                     "\" />\n";

        // For each matrix ruler...
        for (const Segment::Ruler &ruler : *(segment->matrixRulers))
        {
            outStream << "    <ruler type=\"" << ruler.type << "\"";

            if (ruler.type == Controller::EventType)
                outStream << " ccnumber=\"" << ruler.ccNumber << "\"";

            outStream << " />\n";
        }

        outStream << "  </matrix>\n";

        // <notation>

        outStream << "  <notation>\n";

        // For each notation ruler...
        for (const Segment::Ruler &ruler : *(segment->notationRulers))
        {
            outStream << "    <ruler type=\"" << ruler.type << "\"";

            if (ruler.type == Controller::EventType)
                outStream << " ccnumber=\"" << ruler.ccNumber << "\"";

            outStream << " />\n";
        }

        outStream << "  </notation>\n";

    }


    outStream << QString("</%1>\n").arg(segment->getXmlElementName()); //-------------------------

}

bool RosegardenDocument::saveAs(const QString &newName, QString &errMsg)
{
    QFileInfo newNameInfo(newName);

    // If we're saving under the same name, just do a normal save.
    if (newNameInfo.absoluteFilePath() == m_absFilePath)
        return saveDocument(newName, errMsg);

    QString oldTitle = m_title;
    QString oldAbsFilePath = m_absFilePath;

    m_title = newNameInfo.fileName();
    m_absFilePath = newNameInfo.absoluteFilePath();

    // Lock and lock the new name.  If the lock fails...
    QLockFile *newLock = createLock(m_absFilePath);
    if (!newLock) {
        // Put back the old title/name.
        m_title = oldTitle;
        m_absFilePath = oldAbsFilePath;

        // Fail.
        return false;
    }

    // Save.  If the save fails...
    if (!saveDocument(newName, errMsg)) {
        // Unlock the new name.
        delete newLock;

        // Put back the old title/name.
        m_title = oldTitle;
        m_absFilePath = oldAbsFilePath;

        // Fail.
        return false;
    }

    // Release the old lock
    release();
    m_lockFile = newLock;

    // Success.
    return true;
}

bool RosegardenDocument::isSoundEnabled() const
{
    return m_soundEnabled;
}

bool
RosegardenDocument::xmlParse(QString fileContents, QString &errMsg,
                           bool permanent,
                           bool &cancelled)
{
    //Profiler profiler("RosegardenDocument::xmlParse");

    cancelled = false;

    unsigned int elementCount = 0;
    for (int i = 0; i < fileContents.length() - 1; ++i) {
        if (fileContents[i] == '<' && fileContents[i+1] != '/') {
            ++elementCount;
        }
    }

    if (permanent && m_soundEnabled) RosegardenSequencer::getInstance()->removeAllDevices();

    RoseXmlHandler handler(this, elementCount, m_progressDialog, permanent);

    XMLReader reader;
    reader.setHandler(&handler);

    bool ok = reader.parse(fileContents);

    if (m_progressDialog  &&  m_progressDialog->wasCanceled()) {
        QMessageBox::information(dynamic_cast<QWidget *>(parent()), tr("Rosegarden"), tr("File load cancelled"));
        cancelled = true;
        return true;
    }

    if (!ok) {

#if 0
        if (m_progressDialog  &&  m_progressDialog->wasCanceled()) {
            RG_DEBUG << "File load cancelled";
            StartupLogo::hideIfStillThere();
            QMessageBox::information(dynamic_cast<QWidget *>(parent()), tr("Rosegarden"), tr("File load cancelled"));
            cancelled = true;
            return true;
        } else {
#endif
            errMsg = handler.errorString();
#if 0
        }
#endif

    } else {

        if (getSequenceManager() &&
            !(getSequenceManager()->getSoundDriverStatus() & AUDIO_OK)) {

            StartupLogo::hideIfStillThere();

            if (handler.hasActiveAudio() ||
                (m_pluginManager && !handler.pluginsNotFound().empty())) {

#ifdef HAVE_LIBJACK
                QMessageBox::information
                    (dynamic_cast<QWidget *>(parent()), tr("Rosegarden"), tr("<h3>Audio and plugins not available</h3><p>This composition uses audio files or plugins, but Rosegarden is currently running without audio because the JACK audio server was not available on startup.</p><p>Please exit Rosegarden, start the JACK audio server and re-start Rosegarden if you wish to load this complete composition.</p><p><b>WARNING:</b> If you re-save this composition, all audio and plugin data and settings in it will be lost.</p>"));
#else
                QMessageBox::information
                    (dynamic_cast<QWidget *>(parent()), tr("Rosegarden"), tr("<h3>Audio and plugins not available</h3><p>This composition uses audio files or plugins, but you are running a version of Rosegarden that was compiled without audio support.</p><p><b>WARNING:</b> If you re-save this composition from this version of Rosegarden, all audio and plugin data and settings in it will be lost.</p>"));
#endif
            }

        } else {

            bool shownWarning = false;

            int sr = 0;
            if (getSequenceManager()) {
                sr = getSequenceManager()->getSampleRate();
            }

            int er = m_audioFileManager.getExpectedSampleRate();

            std::set<int> rates = m_audioFileManager.getActualSampleRates();
            bool other = false;
            bool mixed = (rates.size() > 1);
            for (std::set<int>::iterator i = rates.begin();
                 i != rates.end(); ++i) {
                if (*i != sr) {
                    other = true;
                    break;
                }
            }

            if (sr != 0 &&
                handler.hasActiveAudio() &&
                ((er != 0 && er != sr) ||
                 (other && !mixed))) {

                if (er == 0) er = *rates.begin();

                StartupLogo::hideIfStillThere();

                QMessageBox::information(dynamic_cast<QWidget *>(parent()), tr("Rosegarden"), tr("<h3>Incorrect audio sample rate</h3><p>This composition contains audio files that were recorded or imported with the audio server running at a different sample rate (%1 Hz) from the current JACK server sample rate (%2 Hz).</p><p>Rosegarden will play this composition at the correct speed, but any audio files in it will probably sound awful.</p><p>Please consider re-starting the JACK server at the correct rate (%3 Hz) and re-loading this composition before you do any more work with it.</p>").arg(er).arg(sr).arg(er));

                shownWarning = true;

            } else if (sr != 0 && mixed) {

                StartupLogo::hideIfStillThere();

                QMessageBox::information(dynamic_cast<QWidget *>(parent()), tr("Rosegarden"), tr("<h3>Inconsistent audio sample rates</h3><p>This composition contains audio files at more than one sample rate.</p><p>Rosegarden will play them at the correct speed, but any audio files that were recorded or imported at rates different from the current JACK server sample rate (%1 Hz) will probably sound awful.</p><p>Please see the audio file manager dialog for more details, and consider resampling any files that are at the wrong rate.</p>").arg(sr));

                shownWarning = true;
            }

            if (m_pluginManager && !handler.pluginsNotFound().empty()) {

                // We only warn if a plugin manager is present, so as
                // to avoid warnings when importing a studio from
                // another file (which is the normal case in which we
                // have no plugin manager).

                QString msg(tr("<h3>Plugins not found</h3><p>The following audio plugins could not be loaded:</p><ul>"));

                // For each plugin that wasn't found...
                for (const QString &ident : handler.pluginsNotFound()) {
                    msg += QString("<li>%1</li>").arg(ident);
                }
                msg += "</ul>";

                StartupLogo::hideIfStillThere();
                QMessageBox::information(dynamic_cast<QWidget *>(parent()), tr("Rosegarden"), msg);
                shownWarning = true;

            }

            if (handler.isDeprecated() && !shownWarning) {

                QString msg(tr("This file contains one or more old element types that are now deprecated.\nSupport for these elements may disappear in future versions of Rosegarden.\nWe recommend you re-save this file from this version of Rosegarden to ensure that it can still be re-loaded in future versions."));
                slotDocumentModified(); // so file can be re-saved immediately

                StartupLogo::hideIfStillThere();
                QMessageBox::information(dynamic_cast<QWidget *>(parent()), tr("Rosegarden"), msg);
            }

        }

        getComposition().resetLinkedSegmentRefreshStatuses();
    }

    return ok;
}

void
RosegardenDocument::insertRecordedMidi(const MappedEventList &mC)
{
    Profiler profiler("RosegardenDocument::insertRecordedMidi()");

    //RG_DEBUG << "RosegardenDocument::insertRecordedMidi: " << mC.size() << " events";

    // Just create a new record Segment if we don't have one already.
    // Make sure we don't recreate the record segment if it's already
    // freed.
    //

    //Track *midiRecordTrack = 0;

    const Composition::TrackIdSet &recordTracks =
        getComposition().getRecordTracks();

    bool haveMIDIRecordTrack = false;

    // For each recording track
    for (Composition::TrackIdSet::const_iterator i =
            recordTracks.begin(); i != recordTracks.end(); ++i) {
        TrackId tid = (*i);
        Track *track = getComposition().getTrackById(tid);
        if (track) {
            Instrument *instrument =
                m_studio.getInstrumentById(track->getInstrument());
            if (instrument->getType() == Instrument::Midi ||
                    instrument->getType() == Instrument::SoftSynth) {
                haveMIDIRecordTrack = true;
                if (!m_recordMIDISegments[track->getInstrument()]) {
                    addRecordMIDISegment(track->getId());
                }
                // ??? This is a tad perplexing at first glance.  What if
                //     two tracks are armed for record?  Don't we need to
                //     create two segments?  Won't this end the for loop
                //     after creating only one?  Maybe it works because this
                //     routine is called over and over very quickly before
                //     any events come in.  Seems unnecessary to break here.
                break;
            }
        }
    }

    if (!haveMIDIRecordTrack)
        return ;

    // If there are no events, bail.
    // ??? Seems like something we should do at the very top.  But beware the
    //     odd "break" above which may depend on this being here.  And perhaps
    //     we do want to make the record segments even if there is no data
    //     yet.
    if (mC.empty())
        return;

    timeT updateFrom = m_composition.getDuration();
    bool haveNotes = false;

    MappedEventList::const_iterator i;

    // For each incoming event
    for (i = mC.begin(); i != mC.end(); ++i) {

        // Send events from the external controller port to the views
        // ??? Is this how we handle external controller events during record?
        //     Or is this unnecessary?  Need to remove and see if it affects
        //     external controller functionality during record.
        if ((*i)->getRecordedDevice() == Device::EXTERNAL_CONTROLLER) {

            ExternalController::self().processEvent(*i);

            // No further processing is required for this event.
            continue;
        }

        const timeT absTime = m_composition.getElapsedTimeForRealTime((*i)->getEventTime());

        /* This is incorrect, unless the tempo at absTime happens to
           be the same as the tempo at zero and there are no tempo
           changes within the given duration after either zero or
           absTime

           timeT duration = m_composition.getElapsedTimeForRealTime((*i)->getDuration());
        */
        const timeT endTime = m_composition.getElapsedTimeForRealTime(
                (*i)->getEventTime() + (*i)->getDuration());
        timeT duration = endTime - absTime;

        Event *rEvent = nullptr;
        bool isNoteOn = false;
        const int pitch = (*i)->getPitch();
        int channel = (*i)->getRecordedChannel();
        const int device = (*i)->getRecordedDevice();

        switch ((*i)->getType()) {

        case MappedEvent::MidiNote:

            // If this is a note on event.
            // (In AlsaDriver::getMappedEventList() we set the duration to
            // -1 seconds to indicate a note-on event.)
            if ((*i)->getDuration() < RealTime::zero()) {

                //printf("Note On  ch %2d | ptch %3d | vel %3d\n", channel, pitch, (*i)->getVelocity());
                //RG_DEBUG << "RD::iRM Note On cpv:" << channel << "/" << pitch << "/" << (*i)->getVelocity();

                // give it a default duration for insertion into the segment
                duration = Note(Note::Crotchet).getDuration();

                // make a mental note to stick it in the note-on map for when
                // we see the corresponding note-off
                isNoteOn = true;

                rEvent = new Event(Note::EventType,
                                   absTime,
                                   duration);

                rEvent->set<Int>(PITCH, pitch);
                rEvent->set<Int>(VELOCITY, (*i)->getVelocity());

            } else {  // it's a note-off

                //printf("Note Off event on Channel %2d: %5d\n", channel, pitch);
                //RG_DEBUG << "RD::iRM Note Off cp:" << channel << "/" << pitch;

                PitchMap *pitchMap = &m_noteOnEvents[device][channel];
                PitchMap::iterator mi = pitchMap->find(pitch);

                // If we have a matching note-on for this note-off
                if (mi != pitchMap->end()) {

                    // Get the vector of note-ons that match with this
                    // note-off.
                    NoteOnRecSet rec_vec = mi->second;

                    // Adjust updateFrom for quantization.

                    Event *oldEv = *rec_vec[0].m_segmentIterator;
                    timeT eventAbsTime = oldEv->getAbsoluteTime();

                    // Make sure we quantize starting at the beginning of this
                    // note at least.
                    if (updateFrom > eventAbsTime)
                        updateFrom = eventAbsTime;

                    // Modify the previously held note-on Event(s), instead
                    // of assigning to rEvent.
                    NoteOnRecSet *replaced = adjustEndTimes(rec_vec, endTime);
                    delete replaced;

                    // Remove the original note-on(s) from the pitch map.
                    pitchMap->erase(mi);

                    haveNotes = true;

                    // at this point we could quantize the bar if we were
                    // tracking in a notation view

                } else {
                    RG_DEBUG << " WARNING: NOTE OFF received without corresponding NOTE ON  channel:" << channel << "  pitch:" << pitch;
                }
            }

            break;

        case MappedEvent::MidiPitchBend:
            rEvent = PitchBend::makeEvent(
                    absTime,
                    (*i)->getData1(),  // msb
                    (*i)->getData2());  // lsb

            break;

        case MappedEvent::MidiController:
            rEvent = Controller::makeEvent(
                    absTime,
                    (*i)->getData1(),  // number
                    (*i)->getData2());  // value
            break;

        case MappedEvent::MidiProgramChange:
            rEvent = ProgramChange::makeEvent(
                    absTime,
                    (*i)->getData1());  // program
            break;

        case MappedEvent::MidiKeyPressure:
            rEvent = KeyPressure::makeEvent(
                    absTime,
                    (*i)->getData1(),  // pitch
                    (*i)->getData2());  // pressure
            break;

        case MappedEvent::MidiChannelPressure:
            rEvent = ChannelPressure::makeEvent(
                    absTime,
                    (*i)->getData1());  // pressure
            break;

        case MappedEvent::MidiSystemMessage:
            channel = -1;
            if ((*i)->getData1() == MIDI_SYSTEM_EXCLUSIVE) {
                rEvent = SystemExclusive::makeEvent(
                        absTime,
                        DataBlockRepository::getDataBlockForEvent(*i));
            }

            // Ignore other SystemMessage events for the moment
            //

            break;

        case MappedEvent::MidiRPN:
            // ??? Need to implement once the caller is able to generate these.
            break;

        case MappedEvent::MidiNRPN:
            // ??? Need to implement once the caller is able to generate these.
            break;

        case MappedEvent::MidiNoteOneShot:
            RG_DEBUG << "RosegardenDocument::insertRecordedMidi() - "
                     << "GOT UNEXPECTED MappedEvent::MidiNoteOneShot";
            break;

            // Audio control signals - ignore these
        case MappedEvent::Audio:
        case MappedEvent::AudioCancel:
        case MappedEvent::AudioLevel:
        case MappedEvent::AudioStopped:
        case MappedEvent::AudioGeneratePreview:
        case MappedEvent::SystemUpdateInstruments:
            break;


        // list everything in the enum to avoid the annoying compiler
        // warning
        case MappedEvent::InvalidMappedEvent:
        case MappedEvent::Marker:
        case MappedEvent::SystemJackTransport:
        case MappedEvent::SystemMMCTransport:
        case MappedEvent::SystemMIDIClock:
        case MappedEvent::SystemMetronomeDevice:
        case MappedEvent::SystemAudioPortCounts:
        case MappedEvent::SystemAudioPorts:
        case MappedEvent::SystemFailure:
        case MappedEvent::Text:
        case MappedEvent::TimeSignature:
        case MappedEvent::Tempo:
        case MappedEvent::Panic:
        case MappedEvent::SystemMTCTransport:
        case MappedEvent::SystemMIDISyncAuto:
        case MappedEvent::SystemAudioFileFormat:
        case MappedEvent::KeySignature:
        default:
            RG_DEBUG << "RosegardenDocument::insertRecordedMidi() - "
                     << "GOT UNSUPPORTED MAPPED EVENT";
            break;
        }

        // sanity check
        //
        if (rEvent == nullptr)
            continue;

        // Set the recorded input port
        //
        rEvent->set<Int>(RECORDED_PORT, device);

        // Set the recorded channel, if this isn't a sysex event
        if (channel >= 0)
            rEvent->set<Int>(RECORDED_CHANNEL, channel);

        // Set the proper start index (if we haven't before)
        //
        for (RecordingSegmentMap::const_iterator it = m_recordMIDISegments.begin();
             it != m_recordMIDISegments.end(); ++it) {
            Segment *recordMIDISegment = it->second;
            if (recordMIDISegment->size() == 0) {
                recordMIDISegment->setStartTime (m_composition.getBarStartForTime(absTime));
                recordMIDISegment->fillWithRests(absTime);
            }
        }

        // Now insert the new event
        //
        insertRecordedEvent(rEvent, device, channel, isNoteOn);
        delete rEvent;
    }

    // If we have note events, quantize the notation for the recording
    // segments.
    if (haveNotes) {

        QSettings settings;
        settings.beginGroup( GeneralOptionsConfigGroup );

        // This is usually 0.  I don't think there is even a way to change
        // this through the UI.
        int tracking = settings.value("recordtracking", 0).toUInt() ;
        settings.endGroup();
        if (tracking == 1) { // notation
            for (RecordingSegmentMap::const_iterator it = m_recordMIDISegments.begin();
                 it != m_recordMIDISegments.end(); ++it) {

                Segment *recordMIDISegment = it->second;

                EventQuantizeCommand *command = new EventQuantizeCommand
                    (*recordMIDISegment,
                     updateFrom,
                     recordMIDISegment->getEndTime(),
                     NotationOptionsConfigGroup,
                     EventQuantizeCommand::QUANTIZE_NOTATION_ONLY);
                // don't add to history
                command->execute();
            }
        }

        // this signal is currently unused - leaving just in case
        // recording segments are updated through the SegmentObserver::eventAdded() interface
        //         emit recordMIDISegmentUpdated(m_recordMIDISegment, updateFrom);
    }
}

void
RosegardenDocument::updateRecordingMIDISegment()
{
    Profiler profiler("RosegardenDocument::updateRecordingMIDISegment()");

//    RG_DEBUG << "RosegardenDocument::updateRecordingMIDISegment";

    if (m_recordMIDISegments.size() == 0) {
        // make this call once to create one
        insertRecordedMidi(MappedEventList());
        if (m_recordMIDISegments.size() == 0)
            return ; // not recording any MIDI
    }

//    RG_DEBUG << "RosegardenDocument::updateRecordingMIDISegment: have record MIDI segment";

    // The adjusted note on events for copying to m_noteOnEvents.
    NoteOnMap tweakedNoteOnEvents;

    for (NoteOnMap::iterator deviceIter = m_noteOnEvents.begin();
         deviceIter != m_noteOnEvents.end(); ++deviceIter) {
        for (ChanMap::iterator channelIter = deviceIter->second.begin();
             channelIter != deviceIter->second.end(); ++channelIter) {
            for (PitchMap::iterator pitchIter = channelIter->second.begin();
                 pitchIter != channelIter->second.end(); ++pitchIter) {

                // anything in the note-on map should be tweaked so as to end
                // at the recording pointer
                NoteOnRecSet rec_vec = pitchIter->second;
                if (rec_vec.size() > 0) {
                    NoteOnRecSet *newRecordSet =
                            adjustEndTimes(rec_vec, m_composition.getPosition());
                    // Copy to tweakedNoteOnEvents.
                    tweakedNoteOnEvents
                        [deviceIter->first]
                        [channelIter->first]
                        [pitchIter->first] = *newRecordSet;
                    delete newRecordSet;
                }
            }
        }
    }

    m_noteOnEvents = tweakedNoteOnEvents;
}

void
RosegardenDocument::transposeRecordedSegment(Segment *s)
{
    // get a selection of all the events in the segment, since we apparently
    // can't just iterate through a segment's events without one.  (?)
    std::unique_ptr<EventSelection> selectedWholeSegment(new EventSelection(
            *s,
            s->getStartTime(),
            s->getEndMarkerTime()));

    // Say we've got a recorded segment destined for a Bb trumpet track.
    // It will have transpose of -2, and we want to move the notation +2 to
    // compensate, so the user hears the same thing she just recorded
    //
    // (All debate over whether this is the right way to go with this whole
    // issue is now officially settled, and no longer tentative.)
    Composition *c = s->getComposition();
    if (!c)
        return;

    const Track *t = c->getTrackById(s->getTrack());
    if (!t)
        return;

    // pull transpose from the destination track
    const int semitones = t->getTranspose();
    if (!semitones)
        return;

    // For each recorded Event...
    for (Event *event : selectedWholeSegment->getSegmentEvents()) {

        // Not a note?  Try the next.
        if (!event->isa(Note::EventType))
            continue;

        // No pitch?  Warn the user.
        if (!event->has(PITCH)) {
            std::cerr << "WARNING! RosegardenDocument::transposeRecordedSegment(): Note has no pitch!" << std::endl;
            continue;
        }

        const int pitch = event->get<Int>(PITCH) - semitones;
        //std::cerr << "pitch = " << pitch << " after transpose = " << semitones << " (for track " << s->getTrack() << ")" << std::endl;
        event->set<Int>(PITCH, pitch);

    }
}

RosegardenDocument::NoteOnRecSet *
RosegardenDocument::adjustEndTimes(const NoteOnRecSet &rec_vec, timeT endTime)
{
    // Not too keen on profilers, but I'll give it a shot for fun...
    Profiler profiler("RosegardenDocument::adjustEndTimes()");

    // Create a vector to hold the new note-on events for return.
    NoteOnRecSet *new_vector = new NoteOnRecSet();

    // For each note-on event
    for (NoteOnRecSet::const_iterator i = rec_vec.begin();
         i != rec_vec.end();
         ++i) {

        // ??? All this removing and re-inserting of Events from the Segment
        //     seems like a serious waste.  Can't we just modify the Event
        //     in place?  Otherwise we are doing all of this:
        //
        //        1. Segment::erase() notifications.
        //        2. Segment::insert() notifications.
        //        3. Event delete and new.
        //
        //     That causes a lot of churning throughout the UI.  The
        //     reason we cannot modify Event objects in place is because
        //     they live in a sorted list within Segment.  If we modify
        //     their start time, they are now in the wrong place in the
        //     sorted list.

        const Event *oldEvent = *(i->m_segmentIterator);

        timeT newDuration = endTime - oldEvent->getAbsoluteTime();

        // Don't allow zero duration events.
        if (newDuration == 0)
            newDuration = 1;

        // Make a new copy of the event in the segment and modify the
        // duration as needed.
        Event *newEvent = new Event(
                *oldEvent,  // reference Event object
                oldEvent->getAbsoluteTime(),  // absoluteTime (preserved)
                newDuration  // duration (adjusted)
                );

        // Remove the old event from the segment
        Segment *recordMIDISegment = i->m_segment;
        recordMIDISegment->erase(i->m_segmentIterator);

        // Insert the new event into the segment
        NoteOnRec noteRec;
        noteRec.m_segment = recordMIDISegment;
        // ??? Performance: This causes a slew of change notifications to be
        //        sent out by Segment::insert().  That may be causing the
        //        performance issues when recording.  Try removing the
        //        notifications from insert() and see if things improve.
        //        Also take a look at Segment::erase() which is called above.
        noteRec.m_segmentIterator = recordMIDISegment->insert(newEvent);

        // don't need to transpose this event; it was copied from an
        // event that had been transposed already (in storeNoteOnEvent)

        // Collect the new NoteOnRec objects for return.
        new_vector->push_back(noteRec);
    }

    return new_vector;
}

void
RosegardenDocument::storeNoteOnEvent(Segment *s, Segment::iterator it, int device, int channel)
{
    NoteOnRec record;
    record.m_segment = s;
    record.m_segmentIterator = it;

    int pitch = (*it)->get<Int>(PITCH);

    m_noteOnEvents[device][channel][pitch].push_back(record);
}

void
RosegardenDocument::insertRecordedEvent(Event *ev, int device, int channel, bool isNoteOn)
{
    Profiler profiler("RosegardenDocument::insertRecordedEvent()");

    Segment::iterator it;
    for ( RecordingSegmentMap::const_iterator i = m_recordMIDISegments.begin();
            i != m_recordMIDISegments.end(); ++i) {
        Segment *recordMIDISegment = i->second;
        TrackId tid = recordMIDISegment->getTrack();
        Track *track = getComposition().getTrackById(tid);
        if (track) {
            //Instrument *instrument =
            //    m_studio.getInstrumentById(track->getInstrument());
            int chan_filter = track->getMidiInputChannel();
            int dev_filter = track->getMidiInputDevice();

            if (((chan_filter < 0) || (chan_filter == channel)) &&
                ((dev_filter == int(Device::ALL_DEVICES)) || (dev_filter == device))) {

                // Insert the event into the segment.
                it = recordMIDISegment->insert(new Event(*ev));

                if (isNoteOn) {
                    // Add the event to m_noteOnEvents.
                    // To match up with a note-off later.
                    storeNoteOnEvent(recordMIDISegment, it, device, channel);
                }

                //RG_DEBUG << "RosegardenDocument::insertRecordedEvent() - matches filter";

            } else {
                //RG_DEBUG << "RosegardenDocument::insertRecordedEvent() - unmatched event discarded";
            }
        }
    }
}

void
RosegardenDocument::stopPlaying()
{
    emit pointerPositionChanged(m_composition.getPosition());
}

void
RosegardenDocument::stopRecordingMidi()
{
    RG_DEBUG << "RosegardenDocument::stopRecordingMidi";

    Composition &c = getComposition();

    timeT endTime = c.getBarEnd(0);

    bool haveMeaning = false;
    timeT earliestMeaning = 0;

    std::vector<RecordingSegmentMap::iterator> toErase;

    for (RecordingSegmentMap::iterator i = m_recordMIDISegments.begin();
         i != m_recordMIDISegments.end();
         ++i) {

        Segment *s = i->second;

        bool meaningless = true;

        for (Segment::iterator si = s->begin(); si != s->end(); ++si) {

            if ((*si)->isa(Clef::EventType)) continue;

            // no rests in the segment yet, so anything else is meaningful
            meaningless = false;

            if (!haveMeaning || (*si)->getAbsoluteTime() < earliestMeaning) {
                earliestMeaning = (*si)->getAbsoluteTime();
            }

            haveMeaning = true;
            break;
        }

        if (meaningless) {
            if (!c.deleteSegment(s)) delete s;
            toErase.push_back(i);
        } else {
            if (endTime < s->getEndTime()) {
                endTime = s->getEndTime();
            }
        }
    }

    for (size_t i = 0; i < toErase.size(); ++i) {
        m_recordMIDISegments.erase(toErase[i]);
    }

    if (!haveMeaning) return;

    RG_DEBUG << "RosegardenDocument::stopRecordingMidi: have something";

    // adjust the clef timings so as not to leave a clef stranded at
    // the start of an otherwise empty count-in

    timeT meaningfulBarStart = c.getBarStartForTime(earliestMeaning);

    for (RecordingSegmentMap::iterator i = m_recordMIDISegments.begin();
         i != m_recordMIDISegments.end();
         ++i) {

        Segment *s = i->second;
        Segment::iterator j = s->begin();

        if (j == s->end() || !(*j)->isa(Clef::EventType)) continue;

        if ((*j)->getAbsoluteTime() < meaningfulBarStart) {
            Event *e = new Event(**j, meaningfulBarStart);
            s->erase(j);
            s->insert(e);
        }
    }

    for (NoteOnMap::iterator deviceIter = m_noteOnEvents.begin();
         deviceIter != m_noteOnEvents.end(); ++deviceIter) {

        for (ChanMap::iterator channelIter = deviceIter->second.begin();
             channelIter != deviceIter->second.end(); ++channelIter) {

            for (PitchMap::iterator pitchIter = channelIter->second.begin();
                 pitchIter != channelIter->second.end(); ++pitchIter) {

                // anything remaining in the note-on map should be
                // made to end at the end of the segment

                NoteOnRecSet rec_vec = pitchIter->second;

                if (rec_vec.size() > 0) {
                    // Adjust the end times of the note-on events for
                    // this device/channel/pitch.
                    NoteOnRecSet *replaced =
                            adjustEndTimes(rec_vec, endTime);
                    delete replaced;
                }
            }
        }
    }
    m_noteOnEvents.clear();

    while (!m_recordMIDISegments.empty()) {

        Segment *s = m_recordMIDISegments.begin()->second;
        m_recordMIDISegments.erase(m_recordMIDISegments.begin());

        // the record segment will have already been added to the
        // composition if there was anything in it; otherwise we don't
        // need to do so

        if (s->getComposition() == nullptr) {
            delete s;
            continue;
        }

        // Quantize for notation only -- doesn't affect performance timings.
        MacroCommand *command = new MacroCommand(tr("Insert Recorded MIDI"));

        command->addCommand(new EventQuantizeCommand
                            (*s,
                             s->getStartTime(),
                             s->getEndTime(),
                             NotationOptionsConfigGroup,
                             EventQuantizeCommand::QUANTIZE_NOTATION_ONLY));

        command->addCommand(new NormalizeRestsCommand
                            (*s,
                             c.getBarStartForTime(s->getStartTime()),
                             c.getBarEndForTime(s->getEndTime())));

        command->addCommand(new SegmentRecordCommand(s));

        // Transpose the entire recorded segment as a unit, rather than
        // transposing its individual events one time.  This allows the same
        // source recording to be transposed to multiple destination tracks in
        // different transpositions as part of one simultaneous operation from
        // the user's perspective.  This wasn't done as a command, because it
        // will be undone if the segment itself is undone, and I wanted to avoid
        // writing a new command at a time when we've got something completely
        // different over in the new Qt4 branch; to facilitate porting.
        transposeRecordedSegment(s);

        CommandHistory::getInstance()->addCommand(command,
                                                  m_pointerBeforeRecord);
    }

    emit stoppedMIDIRecording();

    slotUpdateAllViews(nullptr);

    emit pointerPositionChanged(m_composition.getPosition());
}

void
RosegardenDocument::prepareAudio()
{
    if (!m_soundEnabled) return;

    // Clear down the sequencer AudioFilePlayer object
    //
    RosegardenSequencer::getInstance()->clearAllAudioFiles();

    for (AudioFileVector::const_iterator it = m_audioFileManager.cbegin();
         it != m_audioFileManager.cend(); ++it) {

        bool result = RosegardenSequencer::getInstance()->
            addAudioFile((*it)->getAbsoluteFilePath(),
                         (*it)->getId());
        if (!result) {
            RG_DEBUG << "prepareAudio() - failed to add file \""
                     << (*it)->getAbsoluteFilePath() << "\"";
        }
    }
}

void
RosegardenDocument::slotSetPointerPosition(timeT t)
{
    m_composition.setPosition(t);
    emit pointerPositionChanged(t);
}

void
RosegardenDocument::addRecordMIDISegment(TrackId tid)
{
    RG_DEBUG << "RosegardenDocument::addRecordMIDISegment(" << tid << ")";
//    std::cerr << kdBacktrace() << std::endl;

    Segment *recordMIDISegment;

    recordMIDISegment = new Segment();
    recordMIDISegment->setTrack(tid);
    recordMIDISegment->setStartTime(m_recordStartTime);

    // Set an appropriate segment label
    //
    std::string label = "";

    Track *track = m_composition.getTrackById(tid);
    if (!track) return;

    if (track->getPresetLabel() != "") {
        label = track->getPresetLabel();
    } else if (track->getLabel() == "") {
        Instrument *instr =
            m_studio.getInstrumentById(track->getInstrument());
        if (instr) {
            label = m_studio.getSegmentName(instr->getId());
        }
    } else {
        label = track->getLabel();
    }

    recordMIDISegment->setLabel(appendLabel(label,
            qstrtostr(tr("(recorded)"))));

    // set segment transpose, color, highest/lowest playable from track parameters
    Clef clef = clefIndexToClef(track->getClef());
    recordMIDISegment->insert(clef.getAsEvent
                            (recordMIDISegment->getStartTime()));

    recordMIDISegment->setTranspose(track->getTranspose());
    recordMIDISegment->setColourIndex(track->getColor());
    recordMIDISegment->setHighestPlayable(track->getHighestPlayable());
    recordMIDISegment->setLowestPlayable(track->getLowestPlayable());

    m_composition.addSegment(recordMIDISegment);

    m_recordMIDISegments[track->getInstrument()] = recordMIDISegment;

    int lenx = m_viewList.count();
    int i = 0;
    //for (w = m_viewList.first(); w != 0; w = m_viewList.next()) {
    for( i=0; i<lenx; i++ ){
        RosegardenMainViewWidget *w = m_viewList.value( i );
        w->getTrackEditor()->getTrackButtons()->slotUpdateTracks();
    }

    emit newMIDIRecordingSegment(recordMIDISegment);
}

void
RosegardenDocument::addRecordAudioSegment(InstrumentId iid,
                                        AudioFileId auid)
{
    Segment *recordSegment = new Segment
                             (Segment::Audio);

    // Find the right track

    Track *recordTrack = nullptr;

    const Composition::TrackIdSet &tr =
        getComposition().getRecordTracks();

    for (Composition::TrackIdSet::const_iterator i =
                tr.begin(); i != tr.end(); ++i) {
        TrackId tid = (*i);
        Track *track = getComposition().getTrackById(tid);
        if (track) {
            if (iid == track->getInstrument()) {
                recordTrack = track;
                break;
            }
        }
    }

    if (!recordTrack) {
        RG_DEBUG << "RosegardenDocument::addRecordAudioSegment(" << iid << ", "
        << auid << "): No record-armed track found for instrument!";
        return;
    }

    recordSegment->setTrack(recordTrack->getId());
    recordSegment->setStartTime(m_recordStartTime);
    recordSegment->setAudioStartTime(RealTime::zero());

    // Set an appropriate segment label
    //
    std::string label = "";

    if (recordTrack->getLabel() == "") {

        Instrument *instr =
            m_studio.getInstrumentById(recordTrack->getInstrument());

        if (instr) {
            label = instr->getName();
        }

    } else {
        label = recordTrack->getLabel();
    }

    recordSegment->setLabel(appendLabel(label, qstrtostr(RosegardenDocument::tr("(recorded)"))));
    recordSegment->setAudioFileId(auid);

    // set color for audio segment to distinguish it from a MIDI segment on an
    // audio track drawn with the pencil (depends on having the current
    // autoload.rg or a file derived from it to deliever predictable results,
    // but the worst case here is segments drawn in the wrong color when
    // adding new segments to old files, which I don't forsee as being enough
    // of a problem to be worth cooking up a more robust implementation of
    // this new color for new audio segments (DMM)
    recordSegment->setColourIndex(GUIPalette::AudioDefaultIndex);

    RG_DEBUG << "RosegardenDocument::addRecordAudioSegment: adding record segment for instrument " << iid << " on track " << recordTrack->getId();
    m_recordAudioSegments[iid] = recordSegment;

    int lenx = m_viewList.count();
    int i = 0;
    //for (w = m_viewList.first(); w != 0; w = m_viewList.next()) {
    for( i=0; i<lenx; i++ ){
        RosegardenMainViewWidget *w = m_viewList.value( i );
        w->getTrackEditor()->getTrackButtons()->slotUpdateTracks();
    }

    emit newAudioRecordingSegment(recordSegment);
}

void
RosegardenDocument::updateRecordingAudioSegments()
{
    const Composition::TrackIdSet &tr =
        getComposition().getRecordTracks();

    for (Composition::TrackIdSet::const_iterator i =
                tr.begin(); i != tr.end(); ++i) {

        TrackId tid = (*i);
        Track *track = getComposition().getTrackById(tid);

        if (track) {

            InstrumentId iid = track->getInstrument();

            if (m_recordAudioSegments[iid]) {

                Segment *recordSegment = m_recordAudioSegments[iid];
                if (!recordSegment->getComposition()) {

                    // always insert straight away for audio
                    m_composition.addSegment(recordSegment);
                }

                recordSegment->setAudioEndTime(
                    m_composition.getRealTimeDifference(recordSegment->getStartTime(),
                                                        m_composition.getPosition()));

            } else {
                //         RG_DEBUG << "RosegardenDocument::updateRecordingAudioSegments: no segment for instr "
                //              << iid;
            }
        }
    }
}

void
RosegardenDocument::stopRecordingAudio()
{
    RG_DEBUG << "RosegardenDocument::stopRecordingAudio";

    for (RecordingSegmentMap::iterator ri = m_recordAudioSegments.begin();
            ri != m_recordAudioSegments.end(); ++ri) {

        Segment *recordSegment = ri->second;

        if (!recordSegment)
            continue;

        // set the audio end time
        //
        recordSegment->setAudioEndTime(
            m_composition.getRealTimeDifference(recordSegment->getStartTime(),
                                                m_composition.getPosition()));

        // now add the Segment
        RG_DEBUG << "RosegardenDocument::stopRecordingAudio - "
        << "got recorded segment";

        // now move the segment back by the record latency
        //
        /*!!!
          No.  I don't like this.

          The record latency doesn't always exist -- for example, if recording
          from a synth plugin there is no record latency, and we have no way
          here to distinguish.

          The record latency is a total latency figure that actually includes
          some play latency, and we compensate for that again on playback (see
          bug #1378766).

          The timeT conversion of record latency is approximate in frames,
          giving potential phase error.

          Cutting this out won't break any existing files, as the latency
          compensation there is already encoded into the file.

            RealTime adjustedStartTime =
                m_composition.getElapsedRealTime(recordSegment->getStartTime()) -
                m_audioRecordLatency;

            timeT shiftedStartTime =
                m_composition.getElapsedTimeForRealTime(adjustedStartTime);

            RG_DEBUG << "RosegardenDocument::stopRecordingAudio - "
                         << "shifted recorded audio segment by "
                         <<  recordSegment->getStartTime() - shiftedStartTime
                 << " clicks (from " << recordSegment->getStartTime()
                 << " to " << shiftedStartTime << ")";

            recordSegment->setStartTime(shiftedStartTime);
        */
    }
    emit stoppedAudioRecording();

    emit pointerPositionChanged(m_composition.getPosition());
}

void
RosegardenDocument::finalizeAudioFile(InstrumentId instrument)
{
    RG_DEBUG << "finalizeAudioFile(" << instrument << ")";

    Segment *recordSegment = m_recordAudioSegments[instrument];

    if (!recordSegment) {
        RG_WARNING << "finalizeAudioFile() WARNING: Failed to find segment";
        return;
    }

    AudioFile *newAudioFile = m_audioFileManager.getAudioFile(
            recordSegment->getAudioFileId());
    if (!newAudioFile) {
        RG_WARNING << "finalizeAudioFile() WARNING: No audio file found for instrument " << instrument << " (audio file id " << recordSegment->getAudioFileId() << ")";
        return;
    }

    // Progress Dialog
    // Note: The label text and range will be set later as needed.
    QProgressDialog progressDialog(
            "...",  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 100,  // min, max
            RosegardenMainWindow::self());  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // Auto-close is ok for this since there is only one step.
    progressDialog.setAutoClose(true);
    // Just force the progress dialog up.
    // Both Qt4 and Qt5 have bugs related to delayed showing of progress
    // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
    // based distros might lock up.  See Bug #1546.
    progressDialog.show();

    m_audioFileManager.setProgressDialog(&progressDialog);

    try {
        m_audioFileManager.generatePreview(newAudioFile->getId());
    } catch (const Exception &e) {
        StartupLogo::hideIfStillThere();
        QMessageBox::critical(dynamic_cast<QWidget *>(parent()), tr("Rosegarden"), strtoqstr(e.getMessage()));
    }

    if (!recordSegment->getComposition())
        getComposition().addSegment(recordSegment);

    CommandHistory::getInstance()->addCommand
        (new SegmentRecordCommand(recordSegment), m_pointerBeforeRecord);

    // update views
    slotUpdateAllViews(nullptr);

    // Add the file to the sequencer
    RosegardenSequencer::getInstance()->addAudioFile(
            newAudioFile->getAbsoluteFilePath(),
            newAudioFile->getId());

    // clear down
    m_recordAudioSegments.erase(instrument);
    emit audioFileFinalized(recordSegment);
}

RealTime
RosegardenDocument::getAudioPlayLatency()
{
    return RosegardenSequencer::getInstance()->getAudioPlayLatency();
}

RealTime
RosegardenDocument::getAudioRecordLatency()
{
    return RosegardenSequencer::getInstance()->getAudioRecordLatency();
}

void
RosegardenDocument::updateAudioRecordLatency()
{
    m_audioRecordLatency = getAudioRecordLatency();
}

void
RosegardenDocument::clearAllPlugins()
{
    RG_DEBUG << "clearAllPlugins";

    InstrumentList list = m_studio.getAllInstruments();
    MappedEventList mC;

    InstrumentList::iterator it = list.begin();
    for (; it != list.end(); ++it) {
        if ((*it)->getType() == Instrument::Audio) {
            AudioPluginVector::iterator pIt = (*it)->beginPlugins();

            for (; pIt != (*it)->endPlugins(); ++pIt) {
                if ((*pIt)->getMappedId() != -1) {
                    if (StudioControl::
                        destroyStudioObject((*pIt)->getMappedId()) == false) {
                        RG_DEBUG << "RosegardenDocument::clearAllPlugins - "
                                 << "couldn't find plugin instance "
                                 << (*pIt)->getMappedId();
                    }
                }
                (*pIt)->clearPorts();
            }
            (*it)->emptyPlugins();

            /*
            RG_DEBUG << "RosegardenDocument::clearAllPlugins - "
                     << "cleared " << (*it)->getName();
            */
        }
    }
}

void RosegardenDocument::slotDocColoursChanged()
{
    RG_DEBUG << "RosegardenDocument::slotDocColoursChanged(): emitting docColoursChanged()";

    emit docColoursChanged();
}

void
RosegardenDocument::addOrphanedRecordedAudioFile(QString fileName)
{
    m_orphanedRecordedAudioFiles.push_back(fileName);
    slotDocumentModified();
}

void
RosegardenDocument::addOrphanedDerivedAudioFile(QString fileName)
{
    m_orphanedDerivedAudioFiles.push_back(fileName);
    slotDocumentModified();
}

void
RosegardenDocument::notifyAudioFileRemoval(AudioFileId id)
{
    AudioFile *file = nullptr;

    if (m_audioFileManager.wasAudioFileRecentlyRecorded(id)) {
        file = m_audioFileManager.getAudioFile(id);
        if (file) addOrphanedRecordedAudioFile( file->getAbsoluteFilePath() );
        return;
    }

    if (m_audioFileManager.wasAudioFileRecentlyDerived(id)) {
        file = m_audioFileManager.getAudioFile(id);
        if (file) addOrphanedDerivedAudioFile( file->getAbsoluteFilePath() );
        return;
    }
}

// Get the instrument that plays the segment.
// @returns a pointer to the instrument object
Instrument *
RosegardenDocument::
getInstrument(Segment *segment)
{
    if (!segment)
        return nullptr;
    if (!(segment->getComposition()))
        return nullptr;

    const Track *track =
            segment->getComposition()->getTrackById(segment->getTrack());
    Instrument *instrument =
            getStudio().getInstrumentById(track->getInstrument());
    return instrument;
}

void
RosegardenDocument::checkAudioPath(Track *track)
{
    // Might consider calling this from a trackChanged() handler.  Although
    // it might not be a good idea to do a dialog from there, and the dialog
    // could keep popping up over and over without some sort of static flag.

    if (!track->isArmed())
        return;

    Instrument *instrument =
            getStudio().getInstrumentById(track->getInstrument());

    bool audio = (instrument  &&
                  instrument->getType() == Instrument::Audio);

    if (!audio)
        return;

    try {
        getAudioFileManager().testAudioPath();
    } catch (AudioFileManager::BadAudioPathException & /*e*/) {
        // ho ho, here was the real culprit: this dialog inherited style
        // from the track button, hence the weird background and black
        // foreground!
        if (QMessageBox::warning(nullptr,
                                 tr("Warning"),
                                 tr("The audio file path does not exist or is not writable.\nPlease set the audio file path to a valid directory in Document Properties before recording audio.\nWould you like to set it now?"),
                                 QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel
                                ) == QMessageBox::Yes) {
            RosegardenMainWindow::self()->slotOpenAudioPathSettings();
        }
    }
}

QString
RosegardenDocument::lockFilename(const QString &absFilePath) // static
{
    QFileInfo fileInfo(absFilePath);
    return fileInfo.absolutePath() + "/.~lock." + fileInfo.fileName() + "#";
}

bool
RosegardenDocument::lock()
{
    // Can't lock something that isn't a file on the filesystem.
    if (!isRegularDotRGFile())
        return true;

    delete m_lockFile;
    m_lockFile = createLock(m_absFilePath);
    return m_lockFile != nullptr;
}

QLockFile *
RosegardenDocument::createLock(const QString &absFilePath) // static
{
    QLockFile *lockFile = new QLockFile(lockFilename(absFilePath));
    lockFile->setStaleLockTime(0);

    // If the lock is successful, return it.
    if (lockFile->tryLock())
        return lockFile;

    // Lock has failed.

    if (lockFile->error() == QLockFile::LockFailedError) {

        // Present a dialog to the user with the info.

        // Read in the existing lock file.
        qint64 pid;
        QString hostname;
        QString appname;
        if (!lockFile->getLockInfo(&pid, &hostname, &appname))
            RG_WARNING << "createLock(): Failed to read lock file information! Permission problem? Deleted meanwhile?";

        QString message;
        message +=
                tr("Could not lock file.\n\n"
                   "Another user or instance of Rosegarden may already be\n"
                   "editing this file.  If you are sure no one else is\n"
                   "editing this file, you may press Ignore to open the file.\n\n");
        message += tr("Lock Filename: ") + lockFilename(absFilePath) + '\n';
        message += tr("Process ID: ") + QString::number(pid) + '\n';
        message += tr("Host: ") + hostname + '\n';
        message += tr("Application: ") + appname + '\n';

        StartupLogo::hideIfStillThere();

        QMessageBox::StandardButton result = QMessageBox::warning(
                RosegardenMainWindow::self(),
                tr("Rosegarden"),
                message,
                QMessageBox::Ok | QMessageBox::Ignore,
                QMessageBox::Ok);

        // Honor the lock and skip the open.
        if (result == QMessageBox::Ok) {
            delete lockFile;
            return nullptr;
        }

        // Ignore the lock and open the file.

        lockFile->removeStaleLockFile();
        // This should succeed now.
        lockFile->tryLock();
        return lockFile;
    }

    // This is why we don't handle the other error codes from tryLock:
    // If we do not have permission, don't worry about it.  Pretend the lock
    // was successful.  After all, we won't be able to save.

    return lockFile;
}

void RosegardenDocument::release()
{
    // Remove the lock file
    delete m_lockFile;
    m_lockFile = nullptr;
}

void
RosegardenDocument::loopButton(bool checked)
{
    const bool loop = (m_composition.getLoopStart() != m_composition.getLoopEnd());

    if (Preferences::getAdvancedLooping()) {
        // Menu item checked?
        if (checked) {
            if (loop)
                m_composition.setLoopMode(Composition::LoopOn);
            else
                m_composition.setLoopMode(Composition::LoopAll);
        } else {  // Button unpressed, turn looping off.
            m_composition.setLoopMode(Composition::LoopOff);
        }
    } else {
        // If a loop range is set, and the menu item is checked...
        if (loop  &&  checked)
            m_composition.setLoopMode(Composition::LoopOn);
        else
            m_composition.setLoopMode(Composition::LoopOff);
    }

    emit loopChanged();
}


}
