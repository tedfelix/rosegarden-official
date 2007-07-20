/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RosegardenGUIDoc.h"
#include <kapplication.h>

#include <qxml.h>
#include "sound/Midi.h"
#include "gui/editors/segment/TrackEditor.h"
#include "gui/editors/segment/TrackButtons.h"
#include <klocale.h>
#include <kstddirs.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "gui/general/ClefIndex.h"
#include "document/ConfigGroups.h"
#include "base/AudioDevice.h"
#include "base/AudioPluginInstance.h"
#include "base/BaseProperties.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/Configuration.h"
#include "base/Device.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/RealTime.h"
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
#include "gui/application/RosegardenApplication.h"
#include "gui/application/RosegardenGUIApp.h"
#include "gui/application/RosegardenGUIView.h"
#include "gui/dialogs/UnusedAudioSelectionDialog.h"
#include "gui/editors/segment/AudioPreviewThread.h"
#include "gui/editors/segment/TrackLabel.h"
#include "gui/general/EditViewBase.h"
#include "gui/general/GUIPalette.h"
#include "gui/kdeext/KStartupLogo.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/studio/AudioPluginManager.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/CurrentProgressDialog.h"
#include "gui/widgets/ProgressDialog.h"
#include "MultiViewCommandHistory.h"
#include "RoseXmlHandler.h"
#include "sound/AudioFile.h"
#include "sound/AudioFileManager.h"
#include "sound/MappedCommon.h"
#include "sound/MappedComposition.h"
#include "sound/MappedDevice.h"
#include "sound/MappedInstrument.h"
#include "sound/MappedEvent.h"
#include "sound/MappedRealTime.h"
#include "sound/MappedStudio.h"
#include "sound/PluginIdentifier.h"
#include "sound/SoundDriver.h"
#include <kcommand.h>
#include <kconfig.h>
#include <kfilterdev.h>
#include <kglobal.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kprogress.h>
#include <ktempfile.h>
#include <qcstring.h>
#include <qdatastream.h>
#include <qdialog.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include <qwidget.h>
#include "gui/widgets/ProgressBar.h"


namespace Rosegarden
{

using namespace BaseProperties;

RosegardenGUIDoc::RosegardenGUIDoc(QWidget *parent,
                                   AudioPluginManager *pluginManager,
                                   bool skipAutoload,
                                   const char *name)
        : QObject(parent, name),
        m_modified(false),
        m_autoSaved(false),
        m_audioPreviewThread(&m_audioFileManager),
        m_commandHistory(new MultiViewCommandHistory()),
        m_pluginManager(pluginManager),
        m_audioRecordLatency(0, 0),
        m_autoSavePeriod(0),
        m_beingDestroyed(false)
{
    syncDevices();

    m_viewList.setAutoDelete(false);
    m_editViewList.setAutoDelete(false);

    connect(m_commandHistory, SIGNAL(commandExecuted(KCommand *)),
            this, SLOT(slotDocumentModified()));

    connect(m_commandHistory, SIGNAL(documentRestored()),
            this, SLOT(slotDocumentRestored()));

    // autoload a new document
    if (!skipAutoload)
        performAutoload();

    // now set it up as a "new document"
    newDocument();
}

RosegardenGUIDoc::~RosegardenGUIDoc()
{
    RG_DEBUG << "~RosegardenGUIDoc()\n";
    m_beingDestroyed = true;

    m_audioPreviewThread.finish();
    m_audioPreviewThread.wait();

    deleteEditViews();

    //     ControlRulerCanvasRepository::clear();

    delete m_commandHistory; // must be deleted before the Composition is
}

unsigned int
RosegardenGUIDoc::getAutoSavePeriod() const
{
    KConfig* config = kapp->config();
    config->setGroup(GeneralOptionsConfigGroup);
    return config->readUnsignedNumEntry("autosaveinterval", 60);
}

void RosegardenGUIDoc::attachView(RosegardenGUIView *view)
{
    m_viewList.append(view);
}

void RosegardenGUIDoc::detachView(RosegardenGUIView *view)
{
    m_viewList.remove(view);
}

void RosegardenGUIDoc::attachEditView(EditViewBase *view)
{
    m_editViewList.append(view);
}

void RosegardenGUIDoc::detachEditView(EditViewBase *view)
{
    // auto-deletion is disabled, as
    // the editview detaches itself when being deleted
    m_editViewList.remove(view);
}

void RosegardenGUIDoc::deleteEditViews()
{
    // enabled auto-deletion : edit views will be deleted
    m_editViewList.setAutoDelete(true);
    m_editViewList.clear();
}

void RosegardenGUIDoc::setAbsFilePath(const QString &filename)
{
    m_absFilePath = filename;
}

void RosegardenGUIDoc::setTitle(const QString &_t)
{
    m_title = _t;
}

const QString &RosegardenGUIDoc::getAbsFilePath() const
{
    return m_absFilePath;
}

const QString& RosegardenGUIDoc::getTitle() const
{
    return m_title;
}

void RosegardenGUIDoc::slotUpdateAllViews(RosegardenGUIView *sender)
{
    RosegardenGUIView *w;

    for (w = m_viewList.first(); w != 0; w = m_viewList.next()) {
        if (w != sender)
            w->repaint();
    }
}

void RosegardenGUIDoc::setModified(bool m)
{
    m_modified = m;
    RG_DEBUG << "RosegardenGUIDoc[" << this << "]::setModified(" << m << ")\n";
}

void RosegardenGUIDoc::clearModifiedStatus()
{
    setModified(false);
    setAutoSaved(true);
    emit documentModified(false);
}

void RosegardenGUIDoc::slotDocumentModified()
{
    RG_DEBUG << "RosegardenGUIDoc::slotDocumentModified()" << endl;
    setModified(true);
    setAutoSaved(false);
    emit documentModified(true);
}

void RosegardenGUIDoc::slotDocumentRestored()
{
    RG_DEBUG << "RosegardenGUIDoc::slotDocumentRestored()\n";
    setModified(false);
}

QString RosegardenGUIDoc::getAutoSaveFileName()
{
    QString filename = getAbsFilePath();
    if (filename.isEmpty())
        filename = QDir::currentDirPath() + "/" + getTitle();

    QString autoSaveFileName = kapp->tempSaveName(filename);

    return autoSaveFileName;
}

void RosegardenGUIDoc::slotAutoSave()
{
    //     RG_DEBUG << "RosegardenGUIDoc::slotAutoSave()\n" << endl;

    if (isAutoSaved() || !isModified())
        return ;

    QString autoSaveFileName = getAutoSaveFileName();

    RG_DEBUG << "RosegardenGUIDoc::slotAutoSave() - doc modified - saving '"
    << getAbsFilePath() << "' as "
    << autoSaveFileName << endl;

    QString errMsg;

    saveDocument(autoSaveFileName, errMsg, true);

}

bool RosegardenGUIDoc::isRegularDotRGFile()
{
    return getAbsFilePath().right(3).lower() == ".rg";
}

bool RosegardenGUIDoc::saveIfModified()
{
    RG_DEBUG << "RosegardenGUIDoc::saveIfModified()" << endl;
    bool completed = true;

    if (!isModified())
        return completed;


    RosegardenGUIApp *win = (RosegardenGUIApp *)parent();

    int wantSave = KMessageBox::warningYesNoCancel
                   (win,
                    i18n("The current file has been modified.\n"
                         "Do you want to save it?"),
                    i18n("Warning"));

    RG_DEBUG << "wantSave = " << wantSave << endl;

    switch (wantSave) {

    case KMessageBox::Yes:

        if (!isRegularDotRGFile()) {

            RG_DEBUG << "RosegardenGUIDoc::saveIfModified() : new or imported file\n";
            completed = win->slotFileSaveAs();

        } else {

            RG_DEBUG << "RosegardenGUIDoc::saveIfModified() : regular file\n";
            QString errMsg;
            completed = saveDocument(getAbsFilePath(), errMsg);

            if (!completed) {
                if (errMsg) {
                    KMessageBox::error(0, i18n(QString("Could not save document at %1\n(%2)")
                                               .arg(getAbsFilePath()).arg(errMsg)));
                } else {
                    KMessageBox::error(0, i18n(QString("Could not save document at %1")
                                               .arg(getAbsFilePath())));
                }
            }
        }

        break;

    case KMessageBox::No:
        // delete the autosave file so it won't annoy
        // the user when reloading the file.
        QFile::remove
            (getAutoSaveFileName());
        completed = true;
        break;

    case KMessageBox::Cancel:
        completed = false;
        break;

    default:
        completed = false;
        break;
    }

    if (completed) {
        completed = deleteOrphanedAudioFiles(wantSave == KMessageBox::No);
        if (completed) {
            m_audioFileManager.resetRecentlyCreatedFiles();
        }
    }

    if (completed)
        setModified(false);
    return completed;
}

bool
RosegardenGUIDoc::deleteOrphanedAudioFiles(bool documentWillNotBeSaved)
{
    std::vector<QString> recordedOrphans;
    std::vector<QString> derivedOrphans;

    if (documentWillNotBeSaved) {

        // All audio files recorded or derived in this session are
        // about to become orphans

        for (std::vector<AudioFile *>::const_iterator i =
                    m_audioFileManager.begin();
                i != m_audioFileManager.end(); ++i) {

            if (m_audioFileManager.wasAudioFileRecentlyRecorded((*i)->getId())) {
                recordedOrphans.push_back(strtoqstr((*i)->getFilename()));
            }

            if (m_audioFileManager.wasAudioFileRecentlyDerived((*i)->getId())) {
                derivedOrphans.push_back(strtoqstr((*i)->getFilename()));
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

        for (std::vector<AudioFile *>::const_iterator j =
                 m_audioFileManager.begin();
                j != m_audioFileManager.end(); ++j) {
            if (strtoqstr((*j)->getFilename()) == *i) {
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

        for (std::vector<AudioFile *>::const_iterator j =
                 m_audioFileManager.begin();
                j != m_audioFileManager.end(); ++j) {
            if (strtoqstr((*j)->getFilename()) == *i) {
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

        int reply = KMessageBox::warningYesNoCancel
                    (0,
                     i18n("Delete the 1 audio file recorded during the unsaved session?",
                          "Delete the %n audio files recorded during the unsaved session?",
                          recordedOrphans.size()));

        switch (reply) {

        case KMessageBox::Yes:
            break;

        case KMessageBox::No:
            return true;

        default:
        case KMessageBox::Cancel:
            return false;
        }

    } else {

        UnusedAudioSelectionDialog *dialog =
            new UnusedAudioSelectionDialog
            (0,
             i18n("The following audio files were recorded during this session but have been unloaded\nfrom the audio file manager, and so are no longer in use in the document you are saving.\n\nYou may want to clean up these files to save disk space.\n\nPlease select any you wish to delete permanently from the hard disk.\n"),
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
        i18n("About to delete 1 audio file permanently from the hard disk.\nThere will be no way to recover this file.\nAre you sure?", "About to delete %n audio files permanently from the hard disk.\nThere will be no way to recover these files.\nAre you sure?", recordedOrphans.size());

    int reply = KMessageBox::warningContinueCancel(0, question);

    if (reply == KMessageBox::Continue) {
        for (size_t i = 0; i < recordedOrphans.size(); ++i) {
            QFile file(recordedOrphans[i]);
            if (!file.remove()) {
                KMessageBox::error(0, i18n("File %1 could not be deleted.")
                                   .arg(recordedOrphans[i]));
            }

            QFile peakFile(QString("%1.pk").arg(recordedOrphans[i]));
            peakFile.remove();
        }
    }

    return true;
}

void RosegardenGUIDoc::newDocument()
{
    setModified(false);
    setAbsFilePath(QString::null);
    setTitle(i18n("Untitled"));
    m_commandHistory->clear();
}

void RosegardenGUIDoc::performAutoload()
{
    QString autoloadFile =
        KGlobal::dirs()->findResource("appdata", "autoload.rg");

    QFileInfo autoloadFileInfo(autoloadFile);

    if (!autoloadFileInfo.isReadable()) {
        RG_DEBUG << "RosegardenGUIDoc::performAutoload - "
        << "can't find autoload file - defaulting" << endl;
        return ;
    }

    openDocument(autoloadFile);

}

bool RosegardenGUIDoc::openDocument(const QString& filename,
                                    bool permanent,
                                    const char* /*format*/ /*=0*/)
{
    RG_DEBUG << "RosegardenGUIDoc::openDocument("
    << filename << ")" << endl;

    if (!filename || filename.isEmpty())
        return false;

    newDocument();

    QFileInfo fileInfo(filename);
    setTitle(fileInfo.fileName());

    // Check if file readable with fileInfo ?
    if (!fileInfo.isReadable() || fileInfo.isDir()) {
        KStartupLogo::hideIfStillThere();
        QString msg(i18n("Can't open file '%1'").arg(filename));
        KMessageBox::sorry(0, msg);
        return false;
    }

    ProgressDialog progressDlg(i18n("Reading file..."),
                               100,
                               (QWidget*)parent());

    connect(&progressDlg, SIGNAL(cancelClicked()),
            &m_audioFileManager, SLOT(slotStopPreview()));

    progressDlg.setMinimumDuration(500);
    progressDlg.setAutoReset(true); // we're re-using it for the preview generation
    setAbsFilePath(fileInfo.absFilePath());

    QString errMsg;
    QString fileContents;
    bool cancelled = false, okay = true;

    KFilterDev* fileCompressedDevice = static_cast<KFilterDev*>(KFilterDev::deviceForFile(filename, "application/x-gzip"));
    if (fileCompressedDevice == 0) {

        errMsg = i18n("Could not open Rosegarden file");

    } else {
        fileCompressedDevice->open(IO_ReadOnly);

        unsigned int elementCount = fileInfo.size() / 4; // approx. guess
        //         RG_DEBUG << "RosegardenGUIDoc::xmlParse() : elementCount = " << elementCount
        //                  << " - file size : " << file->size()
        //                  << endl;


        // Fugly work-around in case of broken rg files
        //
        int c = 0;
        std::vector<char> baseBuffer;

        while (c != -1) {
            c = fileCompressedDevice->getch();
            if (c != -1)
                baseBuffer.push_back(c);
        }

        fileCompressedDevice->close();

        QString fileContents = QString::fromUtf8(&baseBuffer[0],
                               baseBuffer.size());

        // parse xml file
        okay = xmlParse(fileContents, errMsg, &progressDlg,
                        elementCount, permanent, cancelled);
        // 	okay = xmlParse(fileCompressedDevice, errMsg, &progressDlg,
        //                         elementCount, permanent, cancelled);
        delete fileCompressedDevice;

    }

    if (!okay) {
        KStartupLogo::hideIfStillThere();
        QString msg(i18n("Error when parsing file '%1': \"%2\"")
                    .arg(filename)
                    .arg(errMsg));

        CurrentProgressDialog::freeze();
        KMessageBox::sorry(0, msg);
        CurrentProgressDialog::thaw();

        return false;

    } else if (cancelled) {
        newDocument();
        return false;
    }

    RG_DEBUG << "RosegardenGUIDoc::openDocument() end - "
    << "m_composition : " << &m_composition
    << " - m_composition->getNbSegments() : "
    << m_composition.getNbSegments()
    << " - m_composition->getDuration() : "
    << m_composition.getDuration() << endl;

    if (m_composition.begin() != m_composition.end()) {
        RG_DEBUG << "First segment starts at " << (*m_composition.begin())->getStartTime() << endl;
    }

    // Ensure a minimum of 64 tracks
    //
    //     unsigned int nbTracks = m_composition.getNbTracks();
    //     TrackId maxTrackId = m_composition.getMaxTrackId();
    //     InstrumentId instBase = MidiInstrumentBase;

    //     for(unsigned int i = nbTracks; i < MinNbOfTracks; ++i) {

    //         Track *track;

    //         track = new Track(maxTrackId + 1,          // id
    //                                       (i + instBase) % 16,     // instrument
    //                                       i,                       // position
    //                                       "untitled",
    //                                       false);                  // mute

    //         m_composition.addTrack(track);
    //         ++maxTrackId;
    //     }

    // We might need a progress dialog when we generate previews,
    // reuse the previous one
    progressDlg.setLabel(i18n("Generating audio previews..."));

    connect(&m_audioFileManager, SIGNAL(setProgress(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));
    try {
        // generate any audio previews after loading the files
        m_audioFileManager.generatePreviews();
    } catch (Exception e) {
        KStartupLogo::hideIfStillThere();
        CurrentProgressDialog::freeze();
        KMessageBox::error(0, strtoqstr(e.getMessage()));
        CurrentProgressDialog::thaw();
    }

    if (isSequencerRunning()) {
        // Initialise the whole studio - faders, plugins etc.
        //
        initialiseStudio();

        // Initialise the MIDI controllers (reaches through to MIDI devices
        // to set them up)
        //
        initialiseControllers();
    }

    return true;
}

void
RosegardenGUIDoc::mergeDocument(RosegardenGUIDoc *doc,
                                int options)
{
    KMacroCommand *command = new KMacroCommand(i18n("Merge"));

    timeT time0 = 0;
    if (options & MERGE_AT_END) {
        time0 = getComposition().getBarEndForTime(getComposition().getDuration());
    }

    int myMaxTrack = getComposition().getMaxTrackId();
    int yrMinTrack = doc->getComposition().getMinTrackId();
    int yrMaxTrack = doc->getComposition().getMaxTrackId();
    int yrNrTracks = yrMaxTrack - yrMinTrack + 1;

    if (options & MERGE_IN_NEW_TRACKS) {

        //!!! worry about instruments and other studio stuff later... if at all
        command->addCommand(new AddTracksCommand
                            (&getComposition(),
                             yrNrTracks,
                             MidiInstrumentBase));

    } else if (yrMaxTrack > myMaxTrack) {

        command->addCommand(new AddTracksCommand
                            (&getComposition(),
                             yrMaxTrack - myMaxTrack,
                             MidiInstrumentBase));
    }

    for (Composition::iterator i = doc->getComposition().begin(), j = i;
            i != doc->getComposition().end(); i = j) {

        ++j;
        Segment *s = *i;

        int yrTrack = s->getTrack();
        int myTrack = yrTrack;

        if (options & MERGE_IN_NEW_TRACKS) {
            myTrack = yrTrack - yrMinTrack + myMaxTrack + 1;
        }

        doc->getComposition().detachSegment(s);

        if (options & MERGE_AT_END) {
            s->setStartTime(s->getStartTime() + time0);
        }

        command->addCommand(new SegmentInsertCommand(&getComposition(), s, myTrack));
    }

    if (!(options & MERGE_KEEP_OLD_TIMINGS)) {
        for (int i = getComposition().getTimeSignatureCount() - 1; i >= 0; --i) {
            getComposition().removeTimeSignature(i);
        }
        for (int i = getComposition().getTempoChangeCount() - 1; i >= 0; --i) {
            getComposition().removeTempoChange(i);
        }
    }

    if (options & MERGE_KEEP_NEW_TIMINGS) {
        for (int i = 0; i < doc->getComposition().getTimeSignatureCount(); ++i) {
            std::pair<timeT, TimeSignature> ts =
                doc->getComposition().getTimeSignatureChange(i);
            getComposition().addTimeSignature(ts.first + time0, ts.second);
        }
        for (int i = 0; i < doc->getComposition().getTempoChangeCount(); ++i) {
            std::pair<timeT, tempoT> t =
                doc->getComposition().getTempoChange(i);
            getComposition().addTempoAtTime(t.first + time0, t.second);
        }
    }

    m_commandHistory->addCommand(command);
}

void RosegardenGUIDoc::clearStudio()
{
    QCString replyType;
    QByteArray replyData;
    rgapp->sequencerCall("clearStudio()", replyType, replyData);
    RG_DEBUG << "cleared studio\n";
}

void RosegardenGUIDoc::initialiseStudio()
{
    Profiler profiler("initialiseStudio", true);

    RG_DEBUG << "RosegardenGUIDoc::initialiseStudio - "
    << "clearing down and initialising" << endl;

    clearStudio();

    InstrumentList list = m_studio.getAllInstruments();
    InstrumentList::iterator it = list.begin();
    int audioCount = 0;

    BussList busses = m_studio.getBusses();
    RecordInList recordIns = m_studio.getRecordIns();

    // To reduce the number of DCOP calls at this stage, we put some
    // of the float property values in a big list and commit in one
    // single call at the end.  We can only do this with properties
    // that aren't depended on by other port, connection, or non-float
    // properties during the initialisation process.
    MappedObjectIdList ids;
    MappedObjectPropertyList properties;
    MappedObjectValueList values;

    std::vector<PluginContainer *> pluginContainers;

    for (unsigned int i = 0; i < busses.size(); ++i) {

        // first one is master
        MappedObjectId mappedId =
            StudioControl::createStudioObject(
                MappedObject::AudioBuss);

        StudioControl::setStudioObjectProperty
        (mappedId,
         MappedAudioBuss::BussId,
         MappedObjectValue(i));

        ids.push_back(mappedId);
        properties.push_back(MappedAudioBuss::Level);
        values.push_back(MappedObjectValue(busses[i]->getLevel()));

        ids.push_back(mappedId);
        properties.push_back(MappedAudioBuss::Pan);
        values.push_back(MappedObjectValue(busses[i]->getPan()) - 100.0);

        busses[i]->setMappedId(mappedId);

        pluginContainers.push_back(busses[i]);
    }

    for (unsigned int i = 0; i < recordIns.size(); ++i) {

        MappedObjectId mappedId =
            StudioControl::createStudioObject(
                MappedObject::AudioInput);

        StudioControl::setStudioObjectProperty
        (mappedId,
         MappedAudioInput::InputNumber,
         MappedObjectValue(i));

        recordIns[i]->setMappedId(mappedId);
    }

    for (; it != list.end(); it++) {
        if ((*it)->getType() == Instrument::Audio ||
                (*it)->getType() == Instrument::SoftSynth) {
            MappedObjectId mappedId =
                StudioControl::createStudioObject(
                    MappedObject::AudioFader);

            // Set the object id against the instrument
            //
            (*it)->setMappedId(mappedId);

            /*
            cout << "SETTING MAPPED OBJECT ID = " << mappedId
                 << " - on Instrument " << (*it)->getId() << endl;
                 */


            // Set the instrument id against this object
            //
            StudioControl::setStudioObjectProperty
            (mappedId,
             MappedObject::Instrument,
             MappedObjectValue((*it)->getId()));

            // Set the level
            //
            ids.push_back(mappedId);
            properties.push_back(MappedAudioFader::FaderLevel);
            values.push_back(MappedObjectValue((*it)->getLevel()));

            // Set the record level
            //
            ids.push_back(mappedId);
            properties.push_back(MappedAudioFader::FaderRecordLevel);
            values.push_back(MappedObjectValue((*it)->getRecordLevel()));

            // Set the number of channels
            //
            ids.push_back(mappedId);
            properties.push_back(MappedAudioFader::Channels);
            values.push_back(MappedObjectValue((*it)->getAudioChannels()));

            // Set the pan - 0 based
            //
            ids.push_back(mappedId);
            properties.push_back(MappedAudioFader::Pan);
            values.push_back(MappedObjectValue(float((*it)->getPan())) - 100.0);

            // Set up connections: first clear any existing ones (shouldn't
            // be necessary, but)
            //
            StudioControl::disconnectStudioObject(mappedId);

            // then handle the output connection
            //
            BussId outputBuss = (*it)->getAudioOutput();
            if (outputBuss < busses.size()) {
                MappedObjectId bmi = busses[outputBuss]->getMappedId();

                if (bmi > 0) {
                    StudioControl::connectStudioObjects(mappedId, bmi);
                }
            }

            // then the input
            //
            bool isBuss;
            int channel;
            int input = (*it)->getAudioInput(isBuss, channel);
            MappedObjectId rmi = 0;

            if (isBuss) {
                if (input < int(busses.size())) {
                    rmi = busses[input]->getMappedId();
                }
            } else {
                if (input < int(recordIns.size())) {
                    rmi = recordIns[input]->getMappedId();
                }
            }

            ids.push_back(mappedId);
            properties.push_back(MappedAudioFader::InputChannel);
            values.push_back(MappedObjectValue(channel));

            if (rmi > 0) {
                StudioControl::connectStudioObjects(rmi, mappedId);
            }

            pluginContainers.push_back(*it);

            audioCount++;
        }
    }

    for (std::vector<PluginContainer *>::iterator pci =
                pluginContainers.begin(); pci != pluginContainers.end(); ++pci) {

        // Initialise all the plugins for this Instrument or Buss

        for (PluginInstanceIterator pli = (*pci)->beginPlugins();
                pli != (*pci)->endPlugins(); ++pli) {

            AudioPluginInstance *plugin = *pli;

            if (plugin->isAssigned()) {
                // Create the plugin slot at the sequencer Studio
                //
                MappedObjectId pluginMappedId =
                    StudioControl::createStudioObject(
                        MappedObject::PluginSlot);

                // Create the back linkage from the instance to the
                // studio id
                //
                plugin->setMappedId(pluginMappedId);

                //RG_DEBUG << "CREATING PLUGIN ID = "
                //<< pluginMappedId << endl;

                // Set the position
                StudioControl::setStudioObjectProperty
                (pluginMappedId,
                 MappedObject::Position,
                 MappedObjectValue(plugin->getPosition()));

                // Set the id of this instrument or buss on the plugin
                //
                StudioControl::setStudioObjectProperty
                (pluginMappedId,
                 MappedObject::Instrument,
                 (*pci)->getId());

                // Set the plugin type id - this will set it up ready
                // for the rest of the settings.  String value, so can't
                // go in the main property list.
                //
                StudioControl::setStudioObjectProperty
                (pluginMappedId,
                 MappedPluginSlot::Identifier,
                 plugin->getIdentifier().c_str());

                plugin->setConfigurationValue
                (qstrtostr(PluginIdentifier::RESERVED_PROJECT_DIRECTORY_KEY),
                 getAudioFileManager().getAudioPath());

                // Set opaque string configuration data (e.g. for DSSI plugin)
                //
                MappedObjectPropertyList config;
                for (AudioPluginInstance::ConfigMap::const_iterator
                        i = plugin->getConfiguration().begin();
                        i != plugin->getConfiguration().end(); ++i) {
                    config.push_back(strtoqstr(i->first));
                    config.push_back(strtoqstr(i->second));
                }

                StudioControl::setStudioObjectPropertyList
                (pluginMappedId,
                 MappedPluginSlot::Configuration,
                 config);

                // Set the bypass
                //
                ids.push_back(pluginMappedId);
                properties.push_back(MappedPluginSlot::Bypassed);
                values.push_back(MappedObjectValue(plugin->isBypassed()));

                // Set all the port values
                //
                PortInstanceIterator portIt;

                for (portIt = plugin->begin();
                        portIt != plugin->end(); ++portIt) {
                    StudioControl::setStudioPluginPort
                    (pluginMappedId,
                     (*portIt)->number,
                     (*portIt)->value);
                }

                // Set the program
                //
                if (plugin->getProgram() != "") {
                    StudioControl::setStudioObjectProperty
                    (pluginMappedId,
                     MappedPluginSlot::Program,
                     strtoqstr(plugin->getProgram()));
                }

                // Set the post-program port values
                //
                for (portIt = plugin->begin();
                        portIt != plugin->end(); ++portIt) {
                    if ((*portIt)->changedSinceProgramChange) {
                        StudioControl::setStudioPluginPort
                        (pluginMappedId,
                         (*portIt)->number,
                         (*portIt)->value);
                    }
                }
            }
        }
    }

    // Now commit all the remaining changes
    StudioControl::setStudioObjectProperties(ids, properties, values);

    KConfig* config = kapp->config();
    config->setGroup(SequencerOptionsConfigGroup);

    bool faderOuts = config->readBoolEntry("audiofaderouts", false);
    bool submasterOuts = config->readBoolEntry("audiosubmasterouts", false);
    unsigned int audioFileFormat = config->readUnsignedNumEntry("audiorecordfileformat", 1);

    MidiByte ports = 0;
    if (faderOuts) {
        ports |= MappedEvent::FaderOuts;
    }
    if (submasterOuts) {
        ports |= MappedEvent::SubmasterOuts;
    }
    MappedEvent mEports
    (MidiInstrumentBase,
     MappedEvent::SystemAudioPorts,
     ports);

    StudioControl::sendMappedEvent(mEports);

    MappedEvent mEff
    (MidiInstrumentBase,
     MappedEvent::SystemAudioFileFormat,
     audioFileFormat);
    StudioControl::sendMappedEvent(mEff);
}

int RosegardenGUIDoc::FILE_FORMAT_VERSION_MAJOR = 1;

int RosegardenGUIDoc::FILE_FORMAT_VERSION_MINOR = 4;

int RosegardenGUIDoc::FILE_FORMAT_VERSION_POINT = 0;



bool RosegardenGUIDoc::saveDocument(const QString& filename,
                                    QString& errMsg,
                                    bool autosave)
{
    if (!QFileInfo(filename).exists()) { // safe to write directly
        return saveDocumentActual(filename, errMsg, autosave);
    }

    KTempFile temp(filename + ".", "", 0644); // will be umask'd

    int status = temp.status();
    if (status != 0) {
        errMsg = i18n(QString("Could not create temporary file in directory of '%1': %2").arg(filename).arg(strerror(status)));
        return false;
    }

    QString tempFileName = temp.name();

    std::cerr << "Temporary file name is: \"" << tempFileName << "\"" << std::endl;

    // KTempFile creates a temporary file that is already open: close it
    if (!temp.close()) {
        status = temp.status();
        errMsg = i18n(QString("Failure in temporary file handling for file '%1': %2")
                      .arg(tempFileName).arg(strerror(status)));
        return false;
    }

    bool success = saveDocumentActual(tempFileName, errMsg, autosave);

    if (!success) {
        // errMsg should be already set
        return false;
    }

    QDir dir(QFileInfo(tempFileName).dir());
    if (!dir.rename(tempFileName, filename)) {
        errMsg = i18n(QString("Failed to rename temporary output file '%1' to desired output file '%2'").arg(tempFileName).arg(filename));
        return false;
    }

    return true;
}


bool RosegardenGUIDoc::saveDocumentActual(const QString& filename,
                                          QString& errMsg,
                                          bool autosave)
{
    Profiler profiler("RosegardenGUIDoc::saveDocumentActual");
    RG_DEBUG << "RosegardenGUIDoc::saveDocumentActual(" << filename << ")\n";

    KFilterDev* fileCompressedDevice = static_cast<KFilterDev*>(KFilterDev::deviceForFile(filename, "application/x-gzip"));
    fileCompressedDevice->setOrigFileName("audio/x-rosegarden");
    bool rc = fileCompressedDevice->open(IO_WriteOnly);

    if (!rc) {
        // do some error report
        errMsg = i18n(QString("Could not open file '%1' for writing").arg(filename));
        delete fileCompressedDevice;
        return false; // couldn't open file
    }


    QTextStream outStream(fileCompressedDevice);
    outStream.setEncoding(QTextStream::UnicodeUTF8);

    // output XML header
    //
    outStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    << "<!DOCTYPE rosegarden-data>\n"
    << "<rosegarden-data version=\"" << VERSION
    << "\" format-version-major=\"" << FILE_FORMAT_VERSION_MAJOR
    << "\" format-version-minor=\"" << FILE_FORMAT_VERSION_MINOR
    << "\" format-version-point=\"" << FILE_FORMAT_VERSION_POINT
    << "\">\n";

    ProgressDialog *progressDlg = 0;
    KProgress *progress = 0;

    if (!autosave) {

        progressDlg = new ProgressDialog(i18n("Saving file..."),
                                         100,
                                         (QWidget*)parent());
        progress = progressDlg->progressBar();

        progressDlg->setMinimumDuration(500);
        progressDlg->setAutoReset(true);

    } else {

        progress = ((RosegardenGUIApp *)parent())->getProgressBar();
    }

    // Send out Composition (this includes Tracks, Instruments, Tempo
    // and Time Signature changes and any other sub-objects)
    //
    outStream << strtoqstr(getComposition().toXmlString())
    << endl << endl;

    outStream << strtoqstr(getAudioFileManager().toXmlString())
    << endl << endl;

    outStream << strtoqstr(getConfiguration().toXmlString())
    << endl << endl;

    long totalEvents = 0;
    for (Composition::iterator segitr = m_composition.begin();
            segitr != m_composition.end(); ++segitr) {
        totalEvents += (*segitr)->size();
    }

    for (Composition::triggersegmentcontaineriterator ci =
                m_composition.getTriggerSegments().begin();
            ci != m_composition.getTriggerSegments().end(); ++ci) {
        totalEvents += (*ci)->getSegment()->size();
    }

    // output all elements
    //
    // Iterate on segments
    long eventCount = 0;

    for (Composition::iterator segitr = m_composition.begin();
            segitr != m_composition.end(); ++segitr) {

        Segment *segment = *segitr;

        saveSegment(outStream, segment, progress, totalEvents, eventCount);

    }

    // Put a break in the file
    //
    outStream << endl << endl;

    for (Composition::triggersegmentcontaineriterator ci =
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
        saveSegment(outStream, segment, progress, totalEvents, eventCount, triggerAtts);
    }

    // Put a break in the file
    //
    outStream << endl << endl;

    // Send out the studio - a self contained command
    //
    outStream << strtoqstr(m_studio.toXmlString()) << endl << endl;


    // Send out the appearance data
    outStream << "<appearance>" << endl;
    outStream << strtoqstr(getComposition().getSegmentColourMap().toXmlString("segmentmap"));
    outStream << strtoqstr(getComposition().getGeneralColourMap().toXmlString("generalmap"));
    outStream << "</appearance>" << endl << endl << endl;

    // close the top-level XML tag
    //
    outStream << "</rosegarden-data>\n";

    // check that all went ok
    //
    if (fileCompressedDevice->status() != IO_Ok) {
        errMsg = i18n(QString("Error while writing on '%1'").arg(filename));
        delete fileCompressedDevice;
        return false;
    }

    fileCompressedDevice->close();

    delete fileCompressedDevice; // DO NOT USE outStream AFTER THIS POINT

    RG_DEBUG << endl << "RosegardenGUIDoc::saveDocument() finished\n";

    if (!autosave) {
        emit documentModified(false);
        setModified(false);
        m_commandHistory->documentSaved();
        delete progressDlg;
    } else {
        progress->setProgress(0);
    }

    setAutoSaved(true);

    return true;
}

bool RosegardenGUIDoc::exportStudio(const QString& filename,
                                    std::vector<DeviceId> devices)
{
    Profiler profiler("RosegardenGUIDoc::exportStudio");
    RG_DEBUG << "RosegardenGUIDoc::exportStudio("
    << filename << ")\n";

    KFilterDev* fileCompressedDevice = static_cast<KFilterDev*>(KFilterDev::deviceForFile(filename, "application/x-gzip"));
    fileCompressedDevice->setOrigFileName("audio/x-rosegarden-device");
    fileCompressedDevice->open(IO_WriteOnly);
    QTextStream outStream(fileCompressedDevice);
    outStream.setEncoding(QTextStream::UnicodeUTF8);

    // output XML header
    //
    outStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    << "<!DOCTYPE rosegarden-data>\n"
    << "<rosegarden-data version=\"" << VERSION << "\">\n";

    // Send out the studio - a self contained command
    //
    outStream << strtoqstr(m_studio.toXmlString(devices)) << endl << endl;

    // close the top-level XML tag
    //
    outStream << "</rosegarden-data>\n";

    delete fileCompressedDevice;

    RG_DEBUG << endl << "RosegardenGUIDoc::exportStudio() finished\n";
    return true;
}

void RosegardenGUIDoc::saveSegment(QTextStream& outStream, Segment *segment,
                                   KProgress* progress, long totalEvents, long &count,
                                   QString extraAttributes)
{
    QString time;

    outStream << QString("<segment track=\"%1\" start=\"%2\" ")
    .arg(segment->getTrack())
    .arg(segment->getStartTime());

    if (extraAttributes)
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

    if (segment->getRealTimeDelay() != RealTime::zeroTime) {
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
        time.sprintf("%d.%06d", segment->getAudioStartTime().sec,
                     segment->getAudioStartTime().usec());

        outStream << "    <begin index=\""
        << time
        << "\"/>\n";

        time.sprintf("%d.%06d", segment->getAudioEndTime().sec,
                     segment->getAudioEndTime().usec());

        outStream << "    <end index=\""
        << time
        << "\"/>\n";

        if (segment->isAutoFading()) {
            time.sprintf("%d.%06d", segment->getFadeInTime().sec,
                         segment->getFadeInTime().usec());

            outStream << "    <fadein time=\""
            << time
            << "\"/>\n";

            time.sprintf("%d.%06d", segment->getFadeOutTime().sec,
                         segment->getFadeOutTime().usec());

            outStream << "    <fadeout time=\""
            << time
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
                outStream << "<chord>" << endl;
                inChord = true;
                chordStart = absTime;
                chordDuration = 0;
            }

            if (inChord && (*i)->getDuration() > 0)
                if (chordDuration == 0 || (*i)->getDuration() < chordDuration)
                    chordDuration = (*i)->getDuration();

            outStream << '\t'
            << strtoqstr((*i)->toXmlString(expectedTime)) << endl;

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

            if ((++count % 500 == 0) && progress) {
                progress->setValue(count * 100 / totalEvents);
            }
        }

        if (inChord) {
            outStream << "</chord>\n";
        }

        // Add EventRulers to segment - we call them controllers because of
        // a historical mistake in naming them.  My bad.  RWB.
        //
        Segment::EventRulerList list = segment->getEventRulerList();

        if (list.size()) {
            outStream << "<gui>\n"; // gui elements
            Segment::EventRulerListConstIterator it;
            for (it = list.begin(); it != list.end(); ++it) {
                outStream << "  <controller type=\"" << strtoqstr((*it)->m_type);

                if ((*it)->m_type == Controller::EventType) {
                    outStream << "\" value =\"" << (*it)->m_controllerValue;
                }

                outStream << "\"/>\n";
            }
            outStream << "</gui>\n";
        }

    }


    outStream << "</segment>\n"; //-------------------------

}

bool RosegardenGUIDoc::isSequencerRunning()
{
    RosegardenGUIApp* parentApp = dynamic_cast<RosegardenGUIApp*>(parent());
    if (!parentApp) {
        RG_DEBUG << "RosegardenGUIDoc::isSequencerRunning() : parentApp == 0\n";
        return false;
    }

    return parentApp->isSequencerRunning();
}

bool
RosegardenGUIDoc::xmlParse(QString fileContents, QString &errMsg,
                           ProgressDialog *progress,
                           unsigned int elementCount,
                           bool permanent,
                           bool &cancelled)
{
    cancelled = false;

    RoseXmlHandler handler(this, elementCount, permanent);

    if (progress) {
        connect(&handler, SIGNAL(setProgress(int)),
                progress->progressBar(), SLOT(setValue(int)));
        connect(&handler, SIGNAL(setOperationName(QString)),
                progress, SLOT(slotSetOperationName(QString)));
        connect(&handler, SIGNAL(incrementProgress(int)),
                progress->progressBar(), SLOT(advance(int)));
        connect(progress, SIGNAL(cancelClicked()),
                &handler, SLOT(slotCancel()));
    }

    QXmlInputSource source;
    source.setData(fileContents);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);

    START_TIMING;
    bool ok = reader.parse(source);
    PRINT_ELAPSED("RosegardenGUIDoc::xmlParse (reader.parse())");

    if (!ok) {

        if (handler.isCancelled()) {
            RG_DEBUG << "File load cancelled\n";
            KStartupLogo::hideIfStillThere();
            KMessageBox::information(0, i18n("File load cancelled"));
            cancelled = true;
            return true;
        } else {
            errMsg = handler.errorString();
        }

    } else {

        if (getSequenceManager() &&
            !(getSequenceManager()->getSoundDriverStatus() & AUDIO_OK)) {

            KStartupLogo::hideIfStillThere();
            CurrentProgressDialog::freeze();

            if (handler.hasActiveAudio() ||
                (m_pluginManager && !handler.pluginsNotFound().empty())) {

#ifdef HAVE_LIBJACK
                KMessageBox::information
                    (0, i18n("<h3>Audio and plugins not available</h3><p>This composition uses audio files or plugins, but Rosegarden is currently running without audio because the JACK audio server was not available on startup.</p><p>Please exit Rosegarden, start the JACK audio server and re-start Rosegarden if you wish to load this complete composition.</p><p><b>WARNING:</b> If you re-save this composition, all audio and plugin data and settings in it will be lost.</p>"));
#else
                KMessageBox::information
                    (0, i18n("<h3>Audio and plugins not available</h3><p>This composition uses audio files or plugins, but you are running a version of Rosegarden that was compiled without audio support.</p><p><b>WARNING:</b> If you re-save this composition from this version of Rosegarden, all audio and plugin data and settings in it will be lost.</p>"));
#endif
            }
            CurrentProgressDialog::thaw();

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

                KStartupLogo::hideIfStillThere();
                CurrentProgressDialog::freeze();

                KMessageBox::information(0, i18n("<h3>Incorrect audio sample rate</h3><p>This composition contains audio files that were recorded or imported with the audio server running at a different sample rate (%1 Hz) from the current JACK server sample rate (%2 Hz).</p><p>Rosegarden will play this composition at the correct speed, but any audio files in it will probably sound awful.</p><p>Please consider re-starting the JACK server at the correct rate (%3 Hz) and re-loading this composition before you do any more work with it.</p>").arg(er).arg(sr).arg(er));

                CurrentProgressDialog::thaw();
                shownWarning = true;
 
            } else if (sr != 0 && mixed) {
                    
                KStartupLogo::hideIfStillThere();
                CurrentProgressDialog::freeze();
                
                KMessageBox::information(0, i18n("<h3>Inconsistent audio sample rates</h3><p>This composition contains audio files at more than one sample rate.</p><p>Rosegarden will play them at the correct speed, but any audio files that were recorded or imported at rates different from the current JACK server sample rate (%1 Hz) will probably sound awful.</p><p>Please see the audio file manager dialog for more details, and consider resampling any files that are at the wrong rate.</p>").arg(sr),
                                         i18n("Inconsistent sample rates"),
                                         "file-load-inconsistent-samplerates");
                    
                CurrentProgressDialog::thaw();
                shownWarning = true;
            }
 
            if (m_pluginManager && !handler.pluginsNotFound().empty()) {

                // We only warn if a plugin manager is present, so as
                // to avoid warnings when importing a studio from
                // another file (which is the normal case in which we
                // have no plugin manager).

                QString msg(i18n("<h3>Plugins not found</h3><p>The following audio plugins could not be loaded:</p><ul>"));

                for (std::set<QString>::iterator i = handler.pluginsNotFound().begin();
                     i != handler.pluginsNotFound().end(); ++i) {
                    QString ident = *i;
                    QString type, soName, label;
                    PluginIdentifier::parseIdentifier(ident, type, soName, label);
                    QString pluginFileName = QFileInfo(soName).fileName();
                    msg += i18n("<li>%1 (from %2)</li>").arg(label).arg(pluginFileName);
                }
                msg += "</ul>";
                
                KStartupLogo::hideIfStillThere();
                CurrentProgressDialog::freeze();
                KMessageBox::information(0, msg);
                CurrentProgressDialog::thaw();
                shownWarning = true;
                
            }

            if (handler.isDeprecated() && !shownWarning) {
                
                QString msg(i18n("This file contains one or more old element types that are now deprecated.\nSupport for these elements may disappear in future versions of Rosegarden.\nWe recommend you re-save this file from this version of Rosegarden to ensure that it can still be re-loaded in future versions."));
                slotDocumentModified(); // so file can be re-saved immediately
                
                KStartupLogo::hideIfStillThere();
                CurrentProgressDialog::freeze();
                KMessageBox::information(0, msg);
                CurrentProgressDialog::thaw();
            }
        }
    }

    return ok;
}

void
RosegardenGUIDoc::insertRecordedMidi(const MappedComposition &mC)
{
    //     RG_DEBUG << "RosegardenGUIDoc::insertRecordedMidi" << endl;

    // Just create a new record Segment if we don't have one already.
    // Make sure we don't recreate the record segment if it's already
    // freed.
    //

    //Track *midiRecordTrack = 0;

    const Composition::recordtrackcontainer &tr =
        getComposition().getRecordTracks();

    bool haveMIDIRecordTrack = false;

    for (Composition::recordtrackcontainer::const_iterator i =
                tr.begin(); i != tr.end(); ++i) {
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
                break;
            }
        }
    }

    if (!haveMIDIRecordTrack)
        return ;

    if (mC.size() > 0) {
        MappedComposition::const_iterator i;
        Event *rEvent = 0;
        timeT duration, absTime;
        timeT updateFrom = m_composition.getDuration();
        bool haveNotes = false;

        // process all the incoming MappedEvents
        //
        for (i = mC.begin(); i != mC.end(); ++i) {
            if ((*i)->getRecordedDevice() == Device::CONTROL_DEVICE) {
                // send to GUI
                RosegardenGUIView *v;
                for (v = m_viewList.first(); v != 0; v = m_viewList.next()) {
                    v->slotControllerDeviceEventReceived(*i);
                }
                continue;
            }

            absTime = m_composition.getElapsedTimeForRealTime((*i)->getEventTime());

            /* This is incorrect, unless the tempo at absTime happens to
               be the same as the tempo at zero and there are no tempo
               changes within the given duration after either zero or
               absTime

               duration = m_composition.getElapsedTimeForRealTime((*i)->getDuration());
            */
            duration = m_composition.
                       getElapsedTimeForRealTime((*i)->getEventTime() +
                                                 (*i)->getDuration()) - absTime;

            rEvent = 0;
            bool isNoteOn = false;
            int pitch = 0;
            int channel = (*i)->getRecordedChannel();
            int device = (*i)->getRecordedDevice();

            switch ((*i)->getType()) {
            case MappedEvent::MidiNote:

                pitch = (*i)->getPitch();

                if ((*i)->getDuration() < RealTime::zeroTime) {

                    // it's a note-on; give it a default duration
                    // for insertion into the segment, and make a
                    // mental note to stick it in the note-on map
                    // for when we see the corresponding note-off

                    duration =
                        Note(Note::Crotchet).getDuration();
                    isNoteOn = true;

                    rEvent = new Event(Note::EventType,
                                       absTime,
                                       duration);

                    rEvent->set
                    <Int>(PITCH, pitch);
                    rEvent->set
                    <Int>(VELOCITY, (*i)->getVelocity());

                } else {

                    // it's a note-off

                    //NoteOnMap::iterator mi = m_noteOnEvents.find((*i)->getPitch());
                    PitchMap *pm = &m_noteOnEvents[device][channel];
                    PitchMap::iterator mi = pm->find(pitch);

                    if (mi != pm->end()) {
                        // modify the previously held note-on event,
                        // instead of assigning to rEvent
                        NoteOnRecSet rec_vec = mi->second;
                        Event *oldEv = *rec_vec[0].m_segmentIterator;
                        Event *newEv = new Event
                                       (*oldEv, oldEv->getAbsoluteTime(), duration);

                        newEv->set
                        <Int>(RECORDED_CHANNEL, channel);
                        NoteOnRecSet *replaced =
                            replaceRecordedEvent(rec_vec, newEv);
                        delete replaced;
                        pm->erase(mi);
                        if (updateFrom > newEv->getAbsoluteTime()) {
                            updateFrom = newEv->getAbsoluteTime();
                        }
                        haveNotes = true;
                        delete newEv;
                        // at this point we could quantize the bar if we were
                        // tracking in a notation view
                    } else {
                        std::cerr << " WARNING: NOTE OFF received without corresponding NOTE ON" << std::endl;
                    }
                }

                break;

            case MappedEvent::MidiPitchBend:
                rEvent = PitchBend
                         ((*i)->getData1(), (*i)->getData2()).getAsEvent(absTime);
                rEvent->set
                <Int>(RECORDED_CHANNEL, channel);
                break;

            case MappedEvent::MidiController:
                rEvent = Controller
                         ((*i)->getData1(), (*i)->getData2()).getAsEvent(absTime);
                rEvent->set
                <Int>(RECORDED_CHANNEL, channel);
                break;

            case MappedEvent::MidiProgramChange:
                RG_DEBUG << "RosegardenGUIDoc::insertRecordedMidi()"
                << " - got Program Change (unsupported)"
                << endl;
                break;

            case MappedEvent::MidiKeyPressure:
                rEvent = KeyPressure
                         ((*i)->getData1(), (*i)->getData2()).getAsEvent(absTime);
                rEvent->set
                <Int>(RECORDED_CHANNEL, channel);
                break;

            case MappedEvent::MidiChannelPressure:
                rEvent = ChannelPressure
                         ((*i)->getData1()).getAsEvent(absTime);
                rEvent->set
                <Int>(RECORDED_CHANNEL, channel);
                break;

            case MappedEvent::MidiSystemMessage:
                channel = -1;
                if ((*i)->getData1() == MIDI_SYSTEM_EXCLUSIVE) {
                    rEvent = SystemExclusive
                             (DataBlockRepository::getDataBlockForEvent((*i))).getAsEvent(absTime);
                }

                // Ignore other SystemMessage events for the moment
                //

                break;

            case MappedEvent::MidiNoteOneShot:
                RG_DEBUG << "RosegardenGUIDoc::insertRecordedMidi() - "
                << "GOT UNEXPECTED MappedEvent::MidiNoteOneShot"
                << endl;
                break;

                // Audio control signals - ignore these
            case MappedEvent::Audio:
            case MappedEvent::AudioCancel:
            case MappedEvent::AudioLevel:
            case MappedEvent::AudioStopped:
            case MappedEvent::AudioGeneratePreview:
            case MappedEvent::SystemUpdateInstruments:
                break;

            default:
                RG_DEBUG << "RosegardenGUIDoc::insertRecordedMidi() - "
                << "GOT UNSUPPORTED MAPPED EVENT"
                << endl;
                break;
            }

            // sanity check
            //
            if (rEvent == 0)
                continue;

            // Set the recorded input port
            //
            rEvent->set
            <Int>(RECORDED_PORT, device);

            // Set the proper start index (if we haven't before)
            //
            for ( RecordingSegmentMap::const_iterator it = m_recordMIDISegments.begin();
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

        if (haveNotes) {

            KConfig* config = kapp->config();
            config->setGroup(GeneralOptionsConfigGroup);

            int tracking = config->readUnsignedNumEntry("recordtracking", 0);
            if (tracking == 1) { // notation
                for ( RecordingSegmentMap::const_iterator it = m_recordMIDISegments.begin();
                        it != m_recordMIDISegments.end(); ++it) {
                    Segment *recordMIDISegment = it->second;

                    EventQuantizeCommand *command = new EventQuantizeCommand
                                                    (*recordMIDISegment,
                                                     updateFrom,
                                                     recordMIDISegment->getEndTime(),
                                                     "Notation Options",
                                                     true);
                    // don't add to history
                    command->execute();
                }
            }

            // this signal is currently unused - leaving just in case
            // recording segments are updated through the SegmentObserver::eventAdded() interface
            // 	    emit recordMIDISegmentUpdated(m_recordMIDISegment, updateFrom);
        }
    }
}

void
RosegardenGUIDoc::updateRecordingMIDISegment()
{
    //RG_DEBUG << "RosegardenGUIDoc::updateRecordingMIDISegment" << endl;

    if (m_recordMIDISegments.size() == 0) {
        // make this call once to create one
        insertRecordedMidi(MappedComposition());
        if (m_recordMIDISegments.size() == 0)
            return ; // not recording any MIDI
    }

    //RG_DEBUG << "RosegardenGUIDoc::updateRecordingMIDISegment: have record MIDI segment" << endl;

    NoteOnMap tweakedNoteOnEvents;
    for (NoteOnMap::iterator mi = m_noteOnEvents.begin();
            mi != m_noteOnEvents.end(); ++mi)
        for (ChanMap::iterator cm = mi->second.begin();
                cm != mi->second.end(); ++cm)
            for (PitchMap::iterator pm = cm->second.begin();
                    pm != cm->second.end(); ++pm) {

                // anything in the note-on map should be tweaked so as to end
                // at the recording pointer
                NoteOnRecSet rec_vec = pm->second;
                if (rec_vec.size() > 0) {
                    Event *oldEv = *rec_vec[0].m_segmentIterator;
                    Event *newEv = new Event(
                                       *oldEv, oldEv->getAbsoluteTime(),
                                       m_composition.getPosition() - oldEv->getAbsoluteTime() );

                    tweakedNoteOnEvents[mi->first][cm->first][pm->first] =
                        *replaceRecordedEvent(rec_vec, newEv);
                    delete newEv;
                }
            }
    m_noteOnEvents = tweakedNoteOnEvents;
}

RosegardenGUIDoc::NoteOnRecSet *

RosegardenGUIDoc::replaceRecordedEvent(NoteOnRecSet& rec_vec, Event *fresh)
{
    NoteOnRecSet *new_vector = new NoteOnRecSet();
    for ( NoteOnRecSet::const_iterator i = rec_vec.begin(); i != rec_vec.end(); ++i) {
        Segment *recordMIDISegment = i->m_segment;
        recordMIDISegment->erase(i->m_segmentIterator);
        NoteOnRec noteRec;
        noteRec.m_segment = recordMIDISegment;
        noteRec.m_segmentIterator = recordMIDISegment->insert(new Event(*fresh));
        new_vector->push_back(noteRec);
    }
    return new_vector;
}

void
RosegardenGUIDoc::storeNoteOnEvent(Segment *s, Segment::iterator it, int device, int channel)
{
    NoteOnRec record;
    record.m_segment = s;
    record.m_segmentIterator = it;
    int pitch = (*it)->get
                <Int>(PITCH);
    m_noteOnEvents[device][channel][pitch].push_back(record);
}

void
RosegardenGUIDoc::insertRecordedEvent(Event *ev, int device, int channel, bool isNoteOn)
{
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
                it = recordMIDISegment->insert(new Event(*ev));
                if (isNoteOn) {
                    storeNoteOnEvent(recordMIDISegment, it, device, channel);
                }
                RG_DEBUG << "RosegardenGUIDoc::insertRecordedEvent() - matches filter" << endl;
            } else {
                RG_DEBUG << "RosegardenGUIDoc::insertRecordedEvent() - unmatched event discarded" << endl;
            }
        }
    }
}

void
RosegardenGUIDoc::stopRecordingMidi()
{
    RG_DEBUG << "RosegardenGUIDoc::stopRecordingMidi" << endl;

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

        for (Segment::iterator i = s->begin(); i != s->end(); ++i) {

            if ((*i)->isa(Clef::EventType)) continue;

            // no rests in the segment yet, so anything else is meaningful
            meaningless = false;

            if (!haveMeaning || (*i)->getAbsoluteTime() < earliestMeaning) {
                earliestMeaning = (*i)->getAbsoluteTime();
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

    for (int i = 0; i < toErase.size(); ++i) {
        m_recordMIDISegments.erase(toErase[i]);
    }

    if (!haveMeaning) return;

    RG_DEBUG << "RosegardenGUIDoc::stopRecordingMidi: have something" << endl;

    // adjust the clef timings so as not to leave a clef stranded at
    // the start of an otherwise empty count-in

    timeT meaningfulBarStart = c.getBarStartForTime(earliestMeaning);
    
    for (RecordingSegmentMap::iterator i = m_recordMIDISegments.begin();
         i != m_recordMIDISegments.end();
         ++i) {

        Segment *s = i->second;
        Segment::iterator i = s->begin();

        if (i == s->end() || !(*i)->isa(Clef::EventType)) continue;

        if ((*i)->getAbsoluteTime() < meaningfulBarStart) {
            Event *e = new Event(**i, meaningfulBarStart);
            s->erase(i);
            s->insert(e);
        }
    }

    for (NoteOnMap::iterator mi = m_noteOnEvents.begin();
         mi != m_noteOnEvents.end(); ++mi) {

        for (ChanMap::iterator cm = mi->second.begin();
             cm != mi->second.end(); ++cm) {

            for (PitchMap::iterator pm = cm->second.begin();
                 pm != cm->second.end(); ++pm) {

                // anything remaining in the note-on map should be
                // made to end at the end of the segment

                NoteOnRecSet rec_vec = pm->second;

                if (rec_vec.size() > 0) {
                    Event *oldEv = *rec_vec[0].m_segmentIterator;
                    Event *newEv = new Event
                        (*oldEv, oldEv->getAbsoluteTime(),
                         endTime - oldEv->getAbsoluteTime());
                    NoteOnRecSet *replaced =
                        replaceRecordedEvent(rec_vec, newEv);
                    delete newEv;
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

        if (s->getComposition() == 0) {
            delete s;
            continue;
        }

        // Quantize for notation only -- doesn't affect performance timings.
        KMacroCommand *command = new KMacroCommand(i18n("Insert Recorded MIDI"));

        command->addCommand(new EventQuantizeCommand
                            (*s,
                             s->getStartTime(),
                             s->getEndTime(),
                             "Notation Options",
                             true));

        command->addCommand(new NormalizeRestsCommand
                            (*s,
                             c.getBarStartForTime(s->getStartTime()),
                             c.getBarEndForTime(s->getEndTime())));

        command->addCommand(new SegmentRecordCommand(s));

        m_commandHistory->addCommand(command);
    }

    emit stoppedMIDIRecording();

    slotUpdateAllViews(0);
}

void
RosegardenGUIDoc::prepareAudio()
{
    if (!isSequencerRunning())
        return ;

    QCString replyType;
    QByteArray replyData;

    // Clear down the sequencer AudioFilePlayer object
    //
    rgapp->sequencerSend("clearAllAudioFiles()");

    for (AudioFileManagerIterator it = m_audioFileManager.begin();
            it != m_audioFileManager.end(); it++) {

        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        // We have to pass the filename as a QString
        //
        streamOut << QString(strtoqstr((*it)->getFilename()));
        streamOut << (int)(*it)->getId();

        rgapp->sequencerCall("addAudioFile(QString, int)", replyType, replyData, data);
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
        if (!result) {
            RG_DEBUG << "prepareAudio() - failed to add file \""
            << (*it)->getFilename() << "\"" << endl;
        }
    }
}

void
RosegardenGUIDoc::slotSetPointerPosition(timeT t)
{
    m_composition.setPosition(t);
    emit pointerPositionChanged(t);
}

void
RosegardenGUIDoc::setPlayPosition(timeT t)
{
    emit playPositionChanged(t);
}

void
RosegardenGUIDoc::setLoop(timeT t0, timeT t1)
{
    m_composition.setLoopStart(t0);
    m_composition.setLoopEnd(t1);
    emit loopChanged(t0, t1);
}

void
RosegardenGUIDoc::syncDevices()
{
    Profiler profiler("RosegardenGUIDoc::syncDevices", true);

    // Start up the sequencer
    //
    int timeout = 60;

    while (isSequencerRunning() && !rgapp->isSequencerRegistered() && timeout > 0) {
        RG_DEBUG << "RosegardenGUIDoc::syncDevices - "
        << "waiting for Sequencer to come up" << endl;

        ProgressDialog::processEvents();
        sleep(1); // 1s
        --timeout;
    }

    if (isSequencerRunning() && !rgapp->isSequencerRegistered() && timeout == 0) {

        // Give up, kill sequencer if possible, and report
        KProcess *proc = new KProcess;
        *proc << "/usr/bin/killall";
        *proc << "rosegardensequencer";
        *proc << "lt-rosegardensequencer";

        proc->start(KProcess::Block, KProcess::All);

        if (proc->exitStatus()) {
            RG_DEBUG << "couldn't kill any sequencer processes" << endl;
        }

        delete proc;
        RosegardenGUIApp *app = (RosegardenGUIApp*)parent();
        app->slotSequencerExited(0);
        return ;
    }

    if (!isSequencerRunning())
        return ;

    // Set the default timer first.  We only do this first time and
    // when changed in the configuration dialog.
    static bool setTimer = false;
    if (!setTimer) {
        kapp->config()->setGroup(SequencerOptionsConfigGroup);
        QString currentTimer = getCurrentTimer();
        currentTimer = kapp->config()->readEntry("timer", currentTimer);
        setCurrentTimer(currentTimer);
        setTimer = true;
    }

    QByteArray replyData;
    QCString replyType;

    // Get number of devices the sequencer has found
    //
    rgapp->sequencerCall("getDevices()", replyType, replyData, RosegardenApplication::Empty, true);

    unsigned int devices = 0;

    if (replyType == "unsigned int") {
        QDataStream reply(replyData, IO_ReadOnly);
        reply >> devices;
    } else {
        RG_DEBUG << "RosegardenGUIDoc::syncDevices - "
        << "got unknown returntype from getDevices()" << endl;
        return ;
    }

    RG_DEBUG << "RosegardenGUIDoc::syncDevices - devices = "
    << devices << endl;

    for (unsigned int i = 0; i < devices; i++) {

        RG_DEBUG << "RosegardenGUIDoc::syncDevices - i = "
        << i << endl;

        getMappedDevice(i);
    }

    RG_DEBUG << "RosegardenGUIDoc::syncDevices - "
    << "Sequencer alive - Instruments synced" << endl;


    // Force update of view on current track selection
    //
    kapp->config()->setGroup(GeneralOptionsConfigGroup);
    bool opt = kapp->config()->readBoolEntry("Show Track labels", true);
    TrackLabel::InstrumentTrackLabels labels = TrackLabel::ShowInstrument;
    if (opt)
        labels = TrackLabel::ShowTrack;

    RosegardenGUIView *w;
    for (w = m_viewList.first(); w != 0; w = m_viewList.next()) {
        w->slotSelectTrackSegments(m_composition.getSelectedTrack());
        w->getTrackEditor()->getTrackButtons()->changeTrackInstrumentLabels(labels);
    }

    emit devicesResyncd();
}

void
RosegardenGUIDoc::getMappedDevice(DeviceId id)
{
    QByteArray data;
    QByteArray replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    arg << (unsigned int)id;

    rgapp->sequencerCall("getMappedDevice(unsigned int)",
                         replyType, replyData, data);

    MappedDevice *mD = new MappedDevice();
    QDataStream reply(replyData, IO_ReadOnly);

    if (replyType == "MappedDevice")
        // unfurl
        reply >> mD;
    else
        return ;

    // See if we've got this device already
    //
    Device *device = m_studio.getDevice(id);

    if (mD->getId() == Device::NO_DEVICE) {
        if (device)
            m_studio.removeDevice(id);
        delete mD;
        return ;
    }

    if (mD->size() == 0) {
        // no instruments is OK for a record device
        if (mD->getType() != Device::Midi ||
                mD->getDirection() != MidiDevice::Record) {

            RG_DEBUG << "RosegardenGUIDoc::getMappedDevice() - "
            << "no instruments found" << endl;
            if (device)
                m_studio.removeDevice(id);
            delete mD;
            return ;
        }
    }

    bool hadDeviceAlready = (device != 0);

    if (!hadDeviceAlready) {
        if (mD->getType() == Device::Midi) {
            device =
                new MidiDevice
                (id,
                 mD->getName(),
                 mD->getDirection());

            dynamic_cast<MidiDevice *>(device)
            ->setRecording(mD->isRecording());

            m_studio.addDevice(device);

            RG_DEBUG << "RosegardenGUIDoc::getMappedDevice - "
            << "adding MIDI Device \""
            << device->getName() << "\" id = " << id
            << " direction = " << mD->getDirection()
            << " recording = " << mD->isRecording()
            << endl;
        } else if (mD->getType() == Device::SoftSynth) {
            device = new SoftSynthDevice(id, mD->getName());
            m_studio.addDevice(device);

            RG_DEBUG << "RosegardenGUIDoc::getMappedDevice - "
            << "adding soft synth Device \""
            << device->getName() << "\" id = " << id << endl;
        } else if (mD->getType() == Device::Audio) {
            device = new AudioDevice(id, mD->getName());
            m_studio.addDevice(device);

            RG_DEBUG << "RosegardenGUIDoc::getMappedDevice - "
            << "adding audio Device \""
            << device->getName() << "\" id = " << id << endl;
        } else {
            RG_DEBUG << "RosegardenGUIDoc::getMappedDevice - "
            << "unknown device - \"" << mD->getName()
            << "\" (type = "
            << mD->getType() << ")\n";
            return ;
        }
    }

    if (hadDeviceAlready) {
        // direction might have changed
        if (mD->getType() == Device::Midi) {
            MidiDevice *midid =
                dynamic_cast<MidiDevice *>(device);
            if (midid) {
                midid->setDirection(mD->getDirection());
                midid->setRecording(mD->isRecording());
            }
        }
    }

    std::string connection(mD->getConnection());
    RG_DEBUG << "RosegardenGUIDoc::getMappedDevice - got \"" << connection
    << "\", direction " << mD->getDirection()
    << " recording " << mD->isRecording()
    << endl;
    device->setConnection(connection);

    Instrument *instrument;
    MappedDeviceIterator it;

    InstrumentList existingInstrs(device->getAllInstruments());

    for (it = mD->begin(); it != mD->end(); it++) {
        InstrumentId instrumentId = (*it)->getId();

        bool haveInstrument = false;
        for (InstrumentList::iterator iit = existingInstrs.begin();
                iit != existingInstrs.end(); ++iit) {

            if ((*iit)->getId() == instrumentId) {
                haveInstrument = true;
                break;
            }
        }

        if (!haveInstrument) {
            RG_DEBUG << "RosegardenGUIDoc::getMappedDevice: new instr " << (*it)->getId() << endl;
            instrument = new Instrument((*it)->getId(),
                                        (*it)->getType(),
                                        (*it)->getName(),
                                        (*it)->getChannel(),
                                        device);
            device->addInstrument(instrument);
        }
    }

    delete mD;
}

void
RosegardenGUIDoc::addRecordMIDISegment(TrackId tid)
{
    RG_DEBUG << "RosegardenGUIDoc::addRecordMIDISegment(" << tid << ")" << endl;

    Segment *recordMIDISegment;

    recordMIDISegment = new Segment();
    recordMIDISegment->setTrack(tid);
    recordMIDISegment->setStartTime(m_recordStartTime);

    // Set an appropriate segment label
    //
    std::string label = "";

    Track *track = m_composition.getTrackById(tid);
    if (track) {
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
        label = qstrtostr(i18n("%1 (recorded)").arg(strtoqstr(label)));
    }

    recordMIDISegment->setLabel(label);

    Clef clef;

    // insert an initial clef from track parameters
    switch (track->getClef()) {
    case TrebleClef:       clef = Clef(Clef::Treble);       break;
    case BassClef:         clef = Clef(Clef::Bass);         break;
    case CrotalesClef:     clef = Clef(Clef::Treble, 2);    break;
    case XylophoneClef:    clef = Clef(Clef::Treble, 1);    break;
    case GuitarClef:       clef = Clef(Clef::Treble, -1);   break;
    case ContrabassClef:   clef = Clef(Clef::Bass, -1);     break;
    case CelestaClef:      clef = Clef(Clef::Bass, 2);      break;
    case OldCelestaClef:   clef = Clef(Clef::Bass, 1);      break;
    case FrenchClef:       clef = Clef(Clef::French);       break;
    case SopranoClef:      clef = Clef(Clef::Soprano);      break;
    case MezzosopranoClef: clef = Clef(Clef::Mezzosoprano); break;
    case AltoClef:         clef = Clef(Clef::Alto);         break;
    case TenorClef:        clef = Clef(Clef::Tenor);        break;
    case BaritoneClef:     clef = Clef(Clef::Baritone);     break;
    case VarbaritoneClef:  clef = Clef(Clef::Varbaritone);  break;
    case SubbassClef:      clef = Clef(Clef::Subbass);      break;
    default:               clef = Clef(Clef::Treble);       break;
    }

    recordMIDISegment->insert(clef.getAsEvent
                              (recordMIDISegment->getStartTime()));

    // set segment transpose, color, highest/lowest playable from track parameters
    recordMIDISegment->setTranspose(track->getTranspose());
    recordMIDISegment->setColourIndex(track->getColor());
    recordMIDISegment->setHighestPlayable(track->getHighestPlayable());
    recordMIDISegment->setLowestPlayable(track->getLowestPlayable());

    m_composition.addSegment(recordMIDISegment);

    m_recordMIDISegments[track->getInstrument()] = recordMIDISegment;

    emit newMIDIRecordingSegment(recordMIDISegment);
}

void
RosegardenGUIDoc::addRecordAudioSegment(InstrumentId iid,
                                        AudioFileId auid)
{
    Segment *recordSegment = new Segment
                             (Segment::Audio);

    // Find the right track

    Track *recordTrack = 0;

    const Composition::recordtrackcontainer &tr =
        getComposition().getRecordTracks();

    for (Composition::recordtrackcontainer::const_iterator i =
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
        RG_DEBUG << "RosegardenGUIDoc::addRecordAudioSegment(" << iid << ", "
        << auid << "): No record-armed track found for instrument!"
        << endl;
        return ;
    }

    recordSegment->setTrack(recordTrack->getId());
    recordSegment->setStartTime(m_recordStartTime);
    recordSegment->setAudioStartTime(RealTime::zeroTime);

    // Set an appropriate segment label
    //
    std::string label = "";

    if (recordTrack) {
        if (recordTrack->getLabel() == "") {

            Instrument *instr =
                m_studio.getInstrumentById(recordTrack->getInstrument());

            if (instr) {
                label = instr->getName() + std::string(" ");
            }

        } else {
            label = recordTrack->getLabel() + std::string(" ");
        }

        label += std::string("(recorded audio)");
    }

    recordSegment->setLabel(label);
    recordSegment->setAudioFileId(auid);

    // set color for audio segment to distinguish it from a MIDI segment on an
    // audio track drawn with the pencil (depends on having the current
    // autoload.rg or a file derived from it to deliever predictable results,
    // but the worst case here is segments drawn in the wrong color when
    // adding new segments to old files, which I don't forsee as being enough
    // of a problem to be worth cooking up a more robust implementation of
    // this new color for new audio segments (DMM)
    recordSegment->setColourIndex(GUIPalette::AudioDefaultIndex);

    RG_DEBUG << "RosegardenGUIDoc::addRecordAudioSegment: adding record segment for instrument " << iid << " on track " << recordTrack->getId() << endl;
    m_recordAudioSegments[iid] = recordSegment;

    emit newAudioRecordingSegment(recordSegment);
}

void
RosegardenGUIDoc::updateRecordingAudioSegments()
{
    const Composition::recordtrackcontainer &tr =
        getComposition().getRecordTracks();

    for (Composition::recordtrackcontainer::const_iterator i =
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
                // 		RG_DEBUG << "RosegardenGUIDoc::updateRecordingAudioSegments: no segment for instr "
                // 			 << iid << endl;
            }
        }
    }
}

void
RosegardenGUIDoc::stopRecordingAudio()
{
    RG_DEBUG << "RosegardenGUIDoc::stopRecordingAudio" << endl;

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
        RG_DEBUG << "RosegardenGUIDoc::stopRecordingAudio - "
        << "got recorded segment" << endl;

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
         
        	RG_DEBUG << "RosegardenGUIDoc::stopRecordingAudio - "
                         << "shifted recorded audio segment by "
                         <<  recordSegment->getStartTime() - shiftedStartTime
        		 << " clicks (from " << recordSegment->getStartTime()
        		 << " to " << shiftedStartTime << ")" << endl;
         
        	recordSegment->setStartTime(shiftedStartTime);
        */
    }
    emit stoppedAudioRecording();
}

void
RosegardenGUIDoc::finalizeAudioFile(InstrumentId iid)
{
    RG_DEBUG << "RosegardenGUIDoc::finalizeAudioFile(" << iid << ")" << endl;

    Segment *recordSegment = 0;
    recordSegment = m_recordAudioSegments[iid];

    if (!recordSegment) {
        RG_DEBUG << "RosegardenGUIDoc::finalizeAudioFile: Failed to find segment" << endl;
        return ;
    }

    AudioFile *newAudioFile = m_audioFileManager.getAudioFile
                              (recordSegment->getAudioFileId());
    if (!newAudioFile) {
        std::cerr << "WARNING: RosegardenGUIDoc::finalizeAudioFile: No audio file found for instrument " << iid << " (audio file id " << recordSegment->getAudioFileId() << ")" << std::endl;
        return ;
    }

    // Create a progress dialog
    //
    ProgressDialog *progressDlg = new ProgressDialog
                                  (i18n("Generating audio preview..."), 100, (QWidget*)parent());
    progressDlg->setAutoClose(false);
    progressDlg->setAutoReset(false);
    progressDlg->show();

    connect(progressDlg, SIGNAL(cancelClicked()),
            &m_audioFileManager, SLOT(slotStopPreview()));

    connect(&m_audioFileManager, SIGNAL(setProgress(int)),
            progressDlg->progressBar(), SLOT(setValue(int)));

    try {
        m_audioFileManager.generatePreview(newAudioFile->getId());
        //!!! mtr just for now?: or better to do this once after the fact?
        //!!!	m_audioFileManager.generatePreviews();
    } catch (Exception e) {
        KStartupLogo::hideIfStillThere();
        CurrentProgressDialog::freeze();
        KMessageBox::error(0, strtoqstr(e.getMessage()));
        CurrentProgressDialog::thaw();
    }

    delete progressDlg;

    if (!recordSegment->getComposition()) {
        getComposition().addSegment(recordSegment);
    }

    m_commandHistory->addCommand
    (new SegmentRecordCommand(recordSegment));

    // update views
    slotUpdateAllViews(0);

    // Now install the file in the sequencer
    //
    // We're playing fast and loose with DCOP here - we just send
    // this request and carry on regardless otherwise the sequencer
    // can just hang our request.  We don't risk a call() and we
    // don't get a return type.  Ugly and hacky but it appears to
    // work for me - so hey.
    //
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);
    streamOut << QString(strtoqstr(newAudioFile->getFilename()));
    streamOut << (int)newAudioFile->getId();
    rgapp->sequencerSend("addAudioFile(QString, int)", data);

    // clear down
    m_recordAudioSegments.erase(iid);
    emit audioFileFinalized(recordSegment);
}

RealTime
RosegardenGUIDoc::getAudioPlayLatency()
{
    QCString replyType;
    QByteArray replyData;

    if (!rgapp->sequencerCall("getAudioPlayLatency()", replyType, replyData)) {
        RG_DEBUG << "RosegardenGUIDoc::getAudioPlayLatency - "
        << "Playback failed to contact Rosegarden sequencer"
        << endl;
        return RealTime::zeroTime;
    }

    // ensure the return type is ok
    QDataStream streamIn(replyData, IO_ReadOnly);
    MappedRealTime result;
    streamIn >> result;

    return (result.getRealTime());
}

RealTime
RosegardenGUIDoc::getAudioRecordLatency()
{
    QCString replyType;
    QByteArray replyData;

    if (!rgapp->sequencerCall("getAudioRecordLatency()", replyType, replyData)) {
        RG_DEBUG << "RosegardenGUIDoc::getAudioRecordLatency - "
        << "Playback failed to contact Rosegarden sequencer"
        << endl;
        return RealTime::zeroTime;
    }

    // ensure the return type is ok
    QDataStream streamIn(replyData, IO_ReadOnly);
    MappedRealTime result;
    streamIn >> result;

    return (result.getRealTime());
}

void
RosegardenGUIDoc::updateAudioRecordLatency()
{
    m_audioRecordLatency = getAudioRecordLatency();
}

QStringList
RosegardenGUIDoc::getTimers()
{
    QStringList list;

    QCString replyType;
    QByteArray replyData;

    if (!rgapp->sequencerCall("getTimers()", replyType, replyData)) {
        RG_DEBUG << "RosegardenGUIDoc::getTimers - "
        << "failed to contact Rosegarden sequencer" << endl;
        return list;
    }

    if (replyType != "unsigned int") {
        RG_DEBUG << "RosegardenGUIDoc::getTimers - "
        << "wrong reply type (" << replyType << ") from sequencer" << endl;
        return list;
    }

    QDataStream streamIn(replyData, IO_ReadOnly);
    unsigned int count = 0;
    streamIn >> count;

    for (unsigned int i = 0; i < count; ++i) {

        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        streamOut << i;

        if (!rgapp->sequencerCall("getTimer(unsigned int)",
                                  replyType, replyData, data)) {
            RG_DEBUG << "RosegardenGUIDoc::getTimers - "
            << "failed to contact Rosegarden sequencer" << endl;
            return list;
        }

        if (replyType != "QString") {
            RG_DEBUG << "RosegardenGUIDoc::getTimers - "
            << "wrong reply type (" << replyType << ") from sequencer" << endl;
            return list;
        }

        QDataStream streamIn(replyData, IO_ReadOnly);
        QString name;
        streamIn >> name;

        list.push_back(name);
    }

    return list;
}

QString
RosegardenGUIDoc::getCurrentTimer()
{
    QCString replyType;
    QByteArray replyData;

    if (!rgapp->sequencerCall("getCurrentTimer()", replyType, replyData)) {
        RG_DEBUG << "RosegardenGUIDoc::getCurrentTimer - "
        << "failed to contact Rosegarden sequencer" << endl;
        return "";
    }

    if (replyType != "QString") {
        RG_DEBUG << "RosegardenGUIDoc::getCurrentTimer - "
        << "wrong reply type (" << replyType << ") from sequencer" << endl;
        return "";
    }

    QDataStream streamIn(replyData, IO_ReadOnly);
    QString name;
    streamIn >> name;
    return name;
}

void
RosegardenGUIDoc::setCurrentTimer(QString name)
{
    QCString replyType;
    QByteArray replyData;

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << name;

    if (!rgapp->sequencerCall("setCurrentTimer(QString)",
                              replyType, replyData, data)) {
        RG_DEBUG << "RosegardenGUIDoc::setCurrentTimer - "
        << "failed to contact Rosegarden sequencer" << endl;
    }
}

void
RosegardenGUIDoc::initialiseControllers()
{
    InstrumentList list = m_studio.getAllInstruments();
    MappedComposition mC;
    MappedEvent *mE;

    InstrumentList::iterator it = list.begin();
    for (; it != list.end(); it++) {
        if ((*it)->getType() == Instrument::Midi) {
            std::vector<MidiControlPair> advancedControls;

            // push all the advanced static controls
            //
            StaticControllers &list = (*it)->getStaticControllers();
            for (StaticControllerConstIterator cIt = list.begin(); cIt != list.end(); ++cIt) {
                advancedControls.push_back(MidiControlPair(cIt->first, cIt->second));
            }

            advancedControls.
            push_back(
                MidiControlPair(MIDI_CONTROLLER_PAN,
                                (*it)->getPan()));
            advancedControls.
            push_back(
                MidiControlPair(MIDI_CONTROLLER_VOLUME,
                                (*it)->getVolume()));


            std::vector<MidiControlPair>::iterator
            iit = advancedControls.begin();
            for (; iit != advancedControls.end(); iit++) {
                try {
                    mE =
                        new MappedEvent((*it)->getId(),
                                        MappedEvent::MidiController,
                                        iit->first,
                                        iit->second);
                } catch (...) {
                    continue;
                }

                mC.insert(mE);
            }
        }
    }

    StudioControl::sendMappedComposition(mC);
}

void
RosegardenGUIDoc::clearAllPlugins()
{
    //RG_DEBUG << "clearAllPlugins" << endl;

    InstrumentList list = m_studio.getAllInstruments();
    MappedComposition mC;

    InstrumentList::iterator it = list.begin();
    for (; it != list.end(); it++) {
        if ((*it)->getType() == Instrument::Audio) {
            PluginInstanceIterator pIt = (*it)->beginPlugins();

            for (; pIt != (*it)->endPlugins(); pIt++) {
                if ((*pIt)->getMappedId() != -1) {
                    if (StudioControl::
                            destroyStudioObject((*pIt)->getMappedId()) == false) {
                        RG_DEBUG << "RosegardenGUIDoc::clearAllPlugins - "
                        << "couldn't find plugin instance "
                        << (*pIt)->getMappedId() << endl;
                    }
                }
                (*pIt)->clearPorts();
            }
            (*it)->emptyPlugins();

            /*
            RG_DEBUG << "RosegardenGUIDoc::clearAllPlugins - "
                     << "cleared " << (*it)->getName() << endl;
            */
        }
    }
}

Clipboard*
RosegardenGUIDoc::getClipboard()
{
    RosegardenGUIApp *app = (RosegardenGUIApp*)parent();
    return app->getClipboard();
}

void RosegardenGUIDoc::slotDocColoursChanged()
{
    RG_DEBUG << "RosegardenGUIDoc::slotDocColoursChanged(): emitting docColoursChanged()" << endl;

    emit docColoursChanged();
}

void
RosegardenGUIDoc::addOrphanedRecordedAudioFile(QString fileName)
{
    m_orphanedRecordedAudioFiles.push_back(fileName);
    slotDocumentModified();
}

void
RosegardenGUIDoc::addOrphanedDerivedAudioFile(QString fileName)
{
    m_orphanedDerivedAudioFiles.push_back(fileName);
    slotDocumentModified();
}

void
RosegardenGUIDoc::notifyAudioFileRemoval(AudioFileId id)
{
    AudioFile *file = 0;

    if (m_audioFileManager.wasAudioFileRecentlyRecorded(id)) {
        file = m_audioFileManager.getAudioFile(id);
        if (file) addOrphanedRecordedAudioFile(file->getFilename());
        return;
    }

    if (m_audioFileManager.wasAudioFileRecentlyDerived(id)) {
        file = m_audioFileManager.getAudioFile(id);
        if (file) addOrphanedDerivedAudioFile(file->getFilename());
        return;
    }
}

}
#include "RosegardenGUIDoc.moc"
