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


#define RG_MODULE_STRING "[AudioManagerDialog]"

#include "AudioManagerDialog.h"

#include "base/Event.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "AudioPlayingDialog.h"
#include "base/Composition.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/application/RosegardenMainViewWidget.h"
#include "sequencer/RosegardenSequencer.h"
#include "gui/widgets/AudioListItem.h"
#include "gui/widgets/AudioListView.h"
#include "gui/widgets/LineEdit.h"
#include "gui/widgets/InputDialog.h"
#include "gui/widgets/WarningGroupBox.h"
#include "gui/general/IconLoader.h"
#include "gui/dialogs/AboutDialog.h"
#include "sound/AudioFile.h"
#include "sound/AudioFileManager.h"
#include "sound/WAVAudioFile.h"
#include "UnusedAudioSelectionDialog.h"
#include "document/Command.h"
#include "gui/widgets/FileDialog.h"

#include <QApplication>
#include <QMainWindow>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QMessageBox>
#include <QAction>
#include <QByteArray>
#include <QDataStream>
#include <QDialog>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QLabel>
#include <QTreeWidget>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include <QVBoxLayout>
#include <QUrl>
#include <QKeySequence>
#include <QSettings>
#include <QDrag>
#include <QDropEvent>
#include <QMimeData>
#include <QDesktopServices>
#include <QPointer>




namespace Rosegarden
{

const int AudioManagerDialog::m_maxPreviewWidth            = 100;
const int AudioManagerDialog::m_previewHeight              = 30;
const char* const AudioManagerDialog::m_listViewLayoutName = "AudioManagerDialog Layout";

AudioManagerDialog::AudioManagerDialog(QWidget *parent,
                                       RosegardenDocument *doc):
        QMainWindow(parent),
        m_doc(doc),
        m_playingAudioFile(0),
        m_audioPlayingDialog(nullptr),
        m_playTimer(new QTimer(this)),
        m_audiblePreview(true)
{
    setWindowTitle(tr("Audio File Manager"));
    this->setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(IconLoader::loadPixmap("window-audio-manager"));
    setMinimumWidth(800);

    QWidget *centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    QVBoxLayout *boxLayout = new QVBoxLayout;
    centralWidget->setLayout(boxLayout);

    boxLayout->setContentsMargins(10, 10, 10, 10);
    boxLayout->setSpacing(5);

    m_sampleRate = RosegardenSequencer::getInstance()->getSampleRate();

    m_fileList = new AudioListView(centralWidget); // internal class needs parent (?)
    m_fileList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_fileList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileList->setIconSize(QSize(m_maxPreviewWidth, m_previewHeight));

    boxLayout->addWidget(m_fileList);

    m_wrongSampleRates = new WarningGroupBox;
    QVBoxLayout *warningBox = new QVBoxLayout;
    m_wrongSampleRates->setLayout(warningBox);
    QLabel *warning = new QLabel(tr("<qt><p><img src=\":pixmaps/tooltip/warning.png\"></img> <b>Audio files marked with an asterisk (*) are encoded at a sample rate different from that of the JACK audio server.</b></p><p>Rosegarden will play them at the correct speed, but they will sound terrible.  Please consider resampling these files externally, or adjusting the sample rate of the JACK server.</p></qt>"));
    warning->setWordWrap(true);
    warningBox->addWidget(warning);

    boxLayout->addWidget(m_wrongSampleRates);
    m_wrongSampleRates->hide();

    // file menu
    createAction("add_audio", SLOT(slotAdd()));
    createAction("export_audio", SLOT(slotExportAudio()));
    createAction("file_close", SLOT(slotClose()));

    // edit menu
    createAction("remove_audio", SLOT(slotRemove()));
    createAction("remove_all_audio", SLOT(slotRemoveAll()));
    createAction("remove_all_unused_audio", SLOT(slotRemoveAllUnused()));
    createAction("delete_unused_audio", SLOT(slotDeleteUnused()));

    // action menu
    createAction("preview_audio", SLOT(slotPlayPreview()));
    createAction("insert_audio", SLOT(slotInsert()));
    createAction("distribute_audio", SLOT(slotDistributeOnMidiSegment()));

    // help menu
    createAction("audio_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));


    //!!! oh now hang on, does this one work?

    // (No, I've never heard of it until poking around in this code.  Julie
    // hadn't either when she redid the old KXMLGUI stuff for us.  I think we've
    // stumbled across a half eaten bagel here)
    createAction("distribute_audio", SLOT(slotDistributeOnMidiSegment()));

    // Set the column names
    //
    //
    QStringList sl;

    sl << tr("Name");           // 0
    sl << tr("Duration");       // 1
    sl << tr("Envelope");       // 2
    sl << tr("Sample rate");    // 3
    sl << tr("Channels");       // 4
    sl << tr("Resolution");     // 5
    sl << tr("File");           // 6

    m_fileList->setColumnCount(7);
    m_fileList->setHeaderItem(new QTreeWidgetItem(sl));

    m_fileList->setSortingEnabled(true);

    //
    // connect selection mechanism
    connect(m_fileList, &QTreeWidget::itemSelectionChanged,
            this, &AudioManagerDialog::slotSelectionChanged);

    connect(m_fileList, SIGNAL(dropped(QDropEvent*, QTreeWidget*, QList<QUrl>)),
            SLOT(slotDropped(QDropEvent*, QTreeWidget*, QList<QUrl>)));


    slotPopulateFileList();

    // Connect command history for updates
    //
    connect(CommandHistory::getInstance(), &CommandHistory::commandExecuted,
            this, &AudioManagerDialog::slotCommandExecuted);

    connect(m_playTimer, &QTimer::timeout,
            this, &AudioManagerDialog::slotCancelPlayingAudio);


    createMenusAndToolbars("audiomanager.rc"); //@@@ JAS orig. 0

    updateActionState(false);

    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    this->restoreGeometry(settings.value("Audio_File_Manager").toByteArray());
    settings.endGroup();

    setAttribute(Qt::WA_DeleteOnClose);
}

/* unused
void slotFileItemActivated(){

    return;
}
*/

AudioManagerDialog::~AudioManagerDialog()
{
    //RG_DEBUG << "*** dtor";

//    m_fileList->saveLayout(m_listViewLayoutName);    //&&&
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    settings.setValue("Audio_File_Manager", this->saveGeometry());
    settings.endGroup();
}

void
AudioManagerDialog::slotPopulateFileList()
{
    // create pixmap of given size
    QPixmap *audioPixmap = new QPixmap(m_maxPreviewWidth, m_previewHeight);

    // Store last selected item if we have one
    //
    AudioListItem *selectedItem =
            dynamic_cast<AudioListItem *>(m_fileList->currentItem());

    AudioFileId lastId = 0;
    Segment *lastSegment = nullptr;
    bool findSelection = false;
    bool foundSelection = false;

    if (selectedItem) {
        lastId = selectedItem->getId();
        lastSegment = selectedItem->getSegment();
        findSelection = true;
    }

    // We don't want the selection changes to be propagated
    // to the main view
    //
    m_fileList->blockSignals(true);

    // clear file list and disable associated action buttons
    m_fileList->clear();
    // AudioListItem* auItem;
    if (m_doc->getAudioFileManager().cbegin() ==
            m_doc->getAudioFileManager().cend()) {
        // Turn off selection and report empty list
        //
        // auItem = new AudioListItem(m_fileList, QStringList(tr("<no audio files>")));

        m_fileList->setSelectionMode(QAbstractItemView::NoSelection);

        m_fileList->blockSignals(false);
        updateActionState(false);
        return ;
    }

    // show tree hierarchy

    // enable selection
    m_fileList->setSelectionMode(QAbstractItemView::SingleSelection);

    // Create a vector of audio Segments only
    //
    std::vector<Segment*> segments;

    for (Composition::iterator it = m_doc->getComposition().begin();
            it != m_doc->getComposition().end(); ++it) {
        if ((*it)->getType() == Segment::Audio)
            segments.push_back(*it);
    }

    RealTime segmentDuration;
    bool wrongSampleRates = false;

    // For each AudioFile
    for (AudioFileVector::iterator audioFileIter =
            m_doc->getAudioFileManager().begin();
         audioFileIter != m_doc->getAudioFileManager().end();
         ++audioFileIter) {
        AudioFile &audioFile = **audioFileIter;

        try {
            m_doc->getAudioFileManager().
            drawPreview(audioFile.getId(),
                        RealTime::zero(),
                        audioFile.getLength(),
                        audioPixmap);
        } catch (const Exception &e) {
            audioPixmap->fill(); // white
            QPainter p(audioPixmap);
            p.setPen(QColor(Qt::black));
            p.drawText(10, m_previewHeight / 2, QString("<no preview>"));
        }

        // Set the label, duration, envelope pixmap and filename

        AudioListItem *item = new AudioListItem(
                m_fileList,
                QStringList(audioFile.getFileName()),  // ??? Why not getLabel()?
                audioFile.getId());

        // Duration
        const RealTime length = audioFile.getLength();
        const QString msecs = QString::asprintf("%03d", length.nsec / 1000000);
        item->setText(1, QString("%1.%2s").arg(length.sec).arg(msecs));

        // set start time and duration
        item->setStartTime(RealTime::zero());
        item->setDuration(length);

        // Envelope pixmap
        item->setIcon(2, QIcon(*audioPixmap));

        // File location
        item->setText(6, audioFile.getAbsoluteFilePath());

        // Resolution
        item->setText(5, QString("%1 bits").arg(audioFile.getBitsPerSample()));

        // Channels
        item->setText(4, QString("%1").arg(audioFile.getChannels()));

        // Sample rate
        if (m_sampleRate != 0 && int(audioFile.getSampleRate()) != m_sampleRate) {
            const QString sRate =
                QString::asprintf("%.1f KHz *",
                                  float(audioFile.getSampleRate()) / 1000.0);
            wrongSampleRates = true;
            item->setText(3, sRate);
        } else {
            const QString sRate =
                QString::asprintf("%.1f KHz",
                                  float(audioFile.getSampleRate()) / 1000.0);
            item->setText(3, sRate);
        }

        // Test audio file element for selection criteria
        //
        if (findSelection  &&
            lastSegment == nullptr  &&
            lastId == audioFile.getId()) {
            //m_fileList->setSelected(item, true);
            m_fileList->setCurrentItem(item);

            findSelection = false;
        }

        // Add children

        // For each audio segment
        for (Segment *segment : segments) {

            // Not using this audioFile?  Try the next.
            if (segment->getAudioFileId() != audioFile.getId())
                continue;

            AudioListItem *childItem =
                new AudioListItem(item,
                                  QStringList(QString(segment->getLabel().c_str())),
                                  audioFile.getId());
            segmentDuration = segment->getAudioEndTime() -
                              segment->getAudioStartTime();

            // store the start time
            //
            childItem->setStartTime(segment->getAudioStartTime());
            childItem->setDuration(segmentDuration);

            // Write segment duration
            //
            const QString msecs =
                QString::asprintf("%03d", segmentDuration.nsec / 1000000);
            childItem->setText(1, QString("%1.%2s")
                               .arg(segmentDuration.sec)
                               .arg(msecs));

            try {
                m_doc->getAudioFileManager().
                drawHighlightedPreview(audioFile.getId(),
                                       RealTime::zero(),
                                       audioFile.getLength(),
                                       segment->getAudioStartTime(),
                                       segment->getAudioEndTime(),
                                       audioPixmap);
            } catch (const Exception &e) {
                // should already be set to "no file"
            }

            // set pixmap
            //
            //childItem->setPixmap(2, *audioPixmap);
            childItem->setIcon(2, QIcon(*audioPixmap));

            // set segment
            //
            childItem->setSegment(segment);

            if (findSelection  &&  lastSegment == segment) {
                m_fileList->setCurrentItem(childItem);    // select
                findSelection = false;
                foundSelection = true;
            }

        }
    }

    updateActionState(foundSelection);

    if (wrongSampleRates) {
        m_wrongSampleRates->show();
    } else {
        m_wrongSampleRates->hide();
    }

    m_fileList->blockSignals(false);
}

AudioFile*
AudioManagerDialog::getCurrentSelection()
{
    // try and get the selected item
    QList<QTreeWidgetItem *> til= m_fileList->selectedItems();
    if (til.isEmpty()){
        RG_WARNING << "AudioManagerDialog::getCurrentSelection() - til.isEmpty() so we're returning 0 and this game is over. Fail.";
        return nullptr;
    }
    AudioListItem *item = dynamic_cast<AudioListItem*>(til[0]);
    if (item == nullptr) {
        RG_WARNING << "AudioManagerDialog::getCurrentSelection() - item == 0 so we're returning 0 and this game is over. Epic fail.";
        return nullptr;
    }

    for (AudioFileVector::const_iterator it =
             m_doc->getAudioFileManager().cbegin();
         it != m_doc->getAudioFileManager().cend();
         ++it) {
        // If we match then return the valid AudioFile
        //
        if (item->getId() == (*it)->getId()) {
            return (*it);
        } else {
            RG_WARNING << "AudioManagerDialog::getCurrentSelection() - item->getId() of "
                      << item->getId() << " does not match (*it)->getId() of "
                      << (*it)->getId() << " so you are basically screwed.  Sorry about that."
                     ;
        }
    }

    RG_WARNING << "AudioManagerDialog::getCurrentSelection() - we tried so hard, but in the end it was all just bricks in the wall.";
    return nullptr;
}

void
AudioManagerDialog::slotExportAudio()
{
    WAVAudioFile *sourceFile =
            dynamic_cast<WAVAudioFile *>(getCurrentSelection());

    if (!sourceFile)
        return;

    QList<QTreeWidgetItem *> selectedItems = m_fileList->selectedItems();

    if (selectedItems.isEmpty()) {
        RG_WARNING << "slotExportAudio() - nothing selected!";
        return;
    }

    // ??? All we ever look at is the first one.
    AudioListItem *item = dynamic_cast<AudioListItem *>(selectedItems[0]);

    if (!item)
        return;

    Segment *segment = item->getSegment();

    QString destFileName =
            FileDialog::getSaveFileName(
                    this,  // parent
                    tr("Save File As"),  // caption
                    QDir::currentPath(),  // dir
                    sourceFile->getAbsoluteFilePath(),  // defaultName
                    tr("*.wav|WAV files (*.wav)"));  // filter

    if (destFileName.isEmpty())
        return;

    // Check for a dot extension and append ".wav" if not found
    // ??? Should use QFileInfo::suffix() to check for an extension.
    if (destFileName.contains(".") == 0)
        destFileName += ".wav";

    // Progress Dialog
    QProgressDialog progressDialog(
            tr("Exporting audio file..."),  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 0,  // min, max
            this);  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // We will close anyway when this object goes out of scope.
    progressDialog.setAutoClose(false);
    // No cancel button since appendSamples() doesn't support progress.
    progressDialog.setCancelButton(nullptr);
    // Just force the progress dialog up.
    // Both Qt4 and Qt5 have bugs related to delayed showing of progress
    // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
    // based distros might lock up.  See Bug #1546.
    progressDialog.show();

    RealTime clipStartTime;
    RealTime clipDuration = sourceFile->getLength();

    if (segment) {
        clipStartTime = segment->getAudioStartTime();
        clipDuration = segment->getAudioEndTime() - clipStartTime;
    }

    WAVAudioFile destFile(
            destFileName,
            sourceFile->getChannels(),
            sourceFile->getSampleRate(),
            sourceFile->getBytesPerSecond(),
            sourceFile->getBytesPerFrame(),
            sourceFile->getBitsPerSample());

    if (sourceFile->open() == false)
        return;

    destFile.write();

    sourceFile->scanTo(clipStartTime);

    // appendSamples() takes the longest.  Would be nice if it would
    // take a progress dialog.
    destFile.appendSamples(sourceFile->getSampleFrameSlice(clipDuration));

    destFile.close();
    sourceFile->close();
}

void
AudioManagerDialog::slotRemove()
{
    QList<QTreeWidgetItem *> selectedTreeItems = m_fileList->selectedItems();

    // If nothing is selected, bail.
    // ??? Why not disable the action when there is no selection?
    //     Then this will never happen.
    if (selectedTreeItems.isEmpty())
        return;

    AudioListItem *item = dynamic_cast<AudioListItem *>(selectedTreeItems[0]);
    if (!item)
        return;

    // If we're on a Segment then delete it from the Composition
    // and refresh the list.
    if (item->getSegment()) {
        // Get the next item to highlight
        QTreeWidgetItem *newTreeItem = m_fileList->itemBelow(item);

        // Nothing below?  Try above.
        if (!newTreeItem)
            newTreeItem = m_fileList->itemAbove(item);

        const AudioListItem *newAudioItem =
                dynamic_cast<const AudioListItem *>(newTreeItem);

        // If the item was an AudioListItem, and it is a Segment item...
        if (newAudioItem  &&  newAudioItem->getSegment()) {
            // Jump to new selection
            setSelected(newAudioItem->getId(),
                        newAudioItem->getSegment(),
                        true);  // propagate
        }

        // Delete the Segment from the Composition.
        SegmentSelection selection;
        selection.insert(item->getSegment());
        emit deleteSegments(selection);

        return;
    }

    // An audio file item is selected in the tree...

    const AudioFile *audioFile = getCurrentSelection();
    if (!audioFile)
        return;

    // remove segments along with audio file
    //
    AudioFileId id = audioFile->getId();
    SegmentSelection selection;
    Composition &comp = m_doc->getComposition();

    bool haveSegments = false;
    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it) {
        if ((*it)->getType() == Segment::Audio &&
                (*it)->getAudioFileId() == id) {
            haveSegments = true;
            break;
        }
    }

    if (haveSegments) {

        QString question = tr("This will unload audio file \"%1\" and remove all associated segments.  Are you sure?")
                           .arg(audioFile->getAbsoluteFilePath());

        // Ask the question
        int reply = QMessageBox::warning(this, tr("Rosegarden"), question, QMessageBox::Yes | QMessageBox::Cancel , QMessageBox::Cancel);

        if (reply != QMessageBox::Yes)
            return ;
    }

    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it) {
        if ((*it)->getType() == Segment::Audio &&
                (*it)->getAudioFileId() == id)
            selection.insert(*it);
    }
    emit deleteSegments(selection);

    m_doc->notifyAudioFileRemoval(id);

    m_doc->getAudioFileManager().removeFile(id);

    // tell the sequencer
    emit deleteAudioFile(id);

    // repopulate
    slotPopulateFileList();
}

void
AudioManagerDialog::slotPlayPreview()
{
    AudioFile *audioFile = getCurrentSelection();

    QList<QTreeWidgetItem*> til = m_fileList->selectedItems();
    if (til.isEmpty()) {
        RG_WARNING << "AudioManagerDialog::slotPlayPreview() - nothing selected!";
        return;
    }
    AudioListItem *item = dynamic_cast<AudioListItem*>(til[0]);

    if (item == nullptr || audioFile == nullptr)
        return ;

    // store the audio file we're playing
    m_playingAudioFile = audioFile->getId();

    // tell the sequencer
    emit playAudioFile(audioFile->getId(),
                       item->getStartTime(),
                       item->getDuration());

    // now open up the playing dialog
    //
    m_audioPlayingDialog =
        new AudioPlayingDialog(this, audioFile->getAbsoluteFilePath());

    // Setup timer to pop down dialog after file has completed
    //
    int msecs = item->getDuration().sec * 1000 +
                item->getDuration().nsec / 1000000;
    m_playTimer->setSingleShot(true);
    m_playTimer->start(msecs);

    // just execute
    //
    if (m_audioPlayingDialog->exec() == QDialog::Rejected)
        emit cancelPlayingAudioFile(m_playingAudioFile);

    delete m_audioPlayingDialog;
    m_audioPlayingDialog = nullptr;

    m_playTimer->stop();

}

void
AudioManagerDialog::slotCancelPlayingAudio()
{
    //std::cout << "AudioManagerDialog::slotCancelPlayingAudio";
    if (m_audioPlayingDialog) {
        m_playTimer->stop();
        delete m_audioPlayingDialog;
        m_audioPlayingDialog = nullptr;
    }
}

void
AudioManagerDialog::slotAdd()
{
    QString extensionList = tr("WAV files") + " (*.wav *.WAV);;" +
                            tr("All files") + " (*)";

    if (RosegardenMainWindow::self()->haveAudioImporter()) {
        //!!! This list really needs to come from the importer helper program
        // (which has an option to supply it -- we just haven't recorded it)
        //
        extensionList = tr("Audio files") + " (*.wav *.flac *.ogg *.mp3 *.WAV *.FLAC *.OGG *.MP3)" + ";;" +
                        tr("WAV files") + " (*.wav *.WAV)" + ";;" +
                        tr("FLAC files") + " (*.flac *.FLAC)" + ";;" +
                        tr("Ogg files") + " (*.ogg *.OGG)" + ";;" +
                        tr("MP3 files") + " (*.mp3 *.MP3)" + ";;" +
                        tr("All files") + " (*)";
    }

    // default to ~ for files if nothing previously stored in settings
    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    QString directory = settings.value("add_audio_file", QDir::homePath()).toString();

    //RG_DEBUG << "slotAdd(): using stored/default path: " << qstrtostr(directory);

    const QStringList fileList = FileDialog::getOpenFileNames(this, tr("Select one or more audio files"), directory, extensionList);

    QDir d;
    for (int i = 0 ; i < fileList.size(); i++) {
        addFile(QUrl::fromLocalFile(fileList.at(i)));
        d = QFileInfo(fileList.at(i)).dir();
    }

    // pick the directory from the last URL encountered to save for future
    // reference, but don't store anything if no URLs were encountered (ie. the
    // user hit cancel on the file dialog without choosing anything)
    directory = d.canonicalPath();

    if (!fileList.isEmpty()) {
        settings.setValue("add_audio_file", directory);

        //RG_DEBUG << "slotAdd(): storing path: " << qstrtostr(directory);
    } else {
        //RG_DEBUG << "slotAdd(): URL list was empty.  No files added.  Not storing path.";
    }

    settings.endGroup();
}

void
AudioManagerDialog::updateActionState(bool haveSelection)
{
    //RG_DEBUG << "updateActionState(" << (haveSelection ? "true" : "false") << ")";

    if (m_doc->getAudioFileManager().cbegin() ==
            m_doc->getAudioFileManager().cend()) {
        leaveActionState("have_audio_files"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    } else {
        enterActionState("have_audio_files"); //@@@ JAS orig. KXMLGUIClient::StateNoReverse
    }

    if (haveSelection) {

        enterActionState("have_audio_selected"); //@@@ JAS orig. KXMLGUIClient::StateNoReverse

        if (m_audiblePreview) {
            enterActionState("have_audible_preview"); //@@@ JAS orig. KXMLGUIClient::StateNoReverse
        } else {
            leaveActionState("have_audible_preview"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        }

        if (isSelectedTrackAudio()) {
            enterActionState("have_audio_insertable"); //@@@ JAS orig. KXMLGUIClient::StateNoReverse
        } else {
            leaveActionState("have_audio_insertable"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        }

    } else {
        leaveActionState("have_audio_selected"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        leaveActionState("have_audio_insertable"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        leaveActionState("have_audible_preview"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    }
}

void
AudioManagerDialog::slotInsert()
{
    //RG_DEBUG << "slotInsert(): begin...";

    AudioFile *audioFile = getCurrentSelection();
    if (audioFile == nullptr)
        return ;

    //RG_DEBUG << "slotInsert(): emitting insertAudioSegment()";

    emit insertAudioSegment(audioFile->getId(),
                            RealTime::zero(),
                            audioFile->getLength());
}

void
AudioManagerDialog::slotRemoveAll()
{
    QString question =
        tr("This will unload all audio files and remove their associated segments.\nThis action cannot be undone, and associations with these files will be lost.\nFiles will not be removed from your disk.\nAre you sure?");

    int reply = QMessageBox::warning(this, tr("Rosegarden"), question, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

    if (reply != QMessageBox::Yes)
        return ;

    SegmentSelection selection;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it) {
        if ((*it)->getType() == Segment::Audio)
            selection.insert(*it);
    }
    // delete segments
    emit deleteSegments(selection);

    for (AudioFileVector::const_iterator
            aIt = m_doc->getAudioFileManager().cbegin();
            aIt != m_doc->getAudioFileManager().cend(); ++aIt) {
        m_doc->notifyAudioFileRemoval((*aIt)->getId());
    }

    m_doc->getAudioFileManager().clear();

    // and now the audio files
    emit deleteAllAudioFiles();

    // clear the file list
    m_fileList->clear();
    slotPopulateFileList();
}

void
AudioManagerDialog::slotRemoveAllUnused()
{
    QString question =
        tr("This will unload all audio files that are not associated with any segments in this composition.\nThis action cannot be undone, and associations with these files will be lost.\nFiles will not be removed from your disk.\nAre you sure?");

    int reply = QMessageBox::warning(this, tr("Rosegarden"), question,QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

    if (reply != QMessageBox::Yes)
        return ;

    std::set
        <AudioFileId> audioFiles;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it) {
        if ((*it)->getType() == Segment::Audio)
            audioFiles.insert((*it)->getAudioFileId());
    }

    std::vector<AudioFileId> toDelete;
    for (AudioFileVector::const_iterator
            aIt = m_doc->getAudioFileManager().cbegin();
            aIt != m_doc->getAudioFileManager().cend(); ++aIt) {
        if (audioFiles.find((*aIt)->getId()) == audioFiles.end())
            toDelete.push_back((*aIt)->getId());
    }

    // Delete the audio files from the AFM
    //
    for (std::vector<AudioFileId>::iterator dIt = toDelete.begin();
            dIt != toDelete.end(); ++dIt) {

        m_doc->notifyAudioFileRemoval(*dIt);
        m_doc->getAudioFileManager().removeFile(*dIt);
        emit deleteAudioFile(*dIt);
    }

    // clear the file list
    m_fileList->clear();
    slotPopulateFileList();
}

void
AudioManagerDialog::slotDeleteUnused()
{
    std::set
        <AudioFileId> audioFiles;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it) {
        if ((*it)->getType() == Segment::Audio)
            audioFiles.insert((*it)->getAudioFileId());
    }

    std::vector<QString> toDelete;
    std::map<QString, AudioFileId> nameMap;

    for (AudioFileVector::const_iterator
            aIt = m_doc->getAudioFileManager().cbegin();
            aIt != m_doc->getAudioFileManager().cend(); ++aIt) {
        if (audioFiles.find((*aIt)->getId()) == audioFiles.end()) {
            toDelete.push_back((*aIt)->getAbsoluteFilePath());
            nameMap[(*aIt)->getAbsoluteFilePath()] = (*aIt)->getId();
        }
    }

    UnusedAudioSelectionDialog *dialog = new UnusedAudioSelectionDialog
                                         (this,
                                          tr("The following audio files are not used in the current composition.\n\nPlease select the ones you wish to delete permanently from the hard disk.\n"),
                                          toDelete);

    if (dialog->exec() == QDialog::Accepted) {

        std::vector<QString> names = dialog->getSelectedAudioFileNames();

        if (names.size() > 0) {

            QString question =
                tr("<qt>About to delete %n audio file(s) permanently from the hard disk.<br>This action cannot be undone, and there will be no way to recover the files.<br>Are you sure?</qt>", "", names.size());

            int reply = QMessageBox::warning(this, tr("Rosegarden"), question, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

            if (reply != QMessageBox::Yes) {
                delete dialog;
                return ;
            }

            for (unsigned int i = 0; i < names.size(); ++i) {
                //RG_DEBUG << i << ": " << names[i];
                QFile file(names[i]);
                if (!file.remove()) {
                    QMessageBox::critical(this, tr("Rosegarden"), tr("File %1 could not be deleted.").arg(names[i]));
                } else {
                    if (nameMap.find(names[i]) != nameMap.end()) {
                        m_doc->getAudioFileManager().removeFile(nameMap[names[i]]);
                        emit deleteAudioFile(nameMap[names[i]]);
                    } else {
                        RG_WARNING << "slotDeleteUnused(): WARNING: Audio file name " << names[i] << " not in name map";
                    }

                    QFile peakFile(QString("%1.pk").arg(names[i]));
                    peakFile.remove();
                }
            }
        }
    }

    m_fileList->clear();
    slotPopulateFileList();

    delete dialog;
}

/* unused
void
AudioManagerDialog::slotRename()
{
    AudioFile *audioFile = getCurrentSelection();

    if (audioFile == nullptr)
        return ;

    bool ok = false;

    QString newText = InputDialog::getText(this,
                          tr("Change Audio File label"),
                          tr("Enter new label"),
                          LineEdit::Normal,
                          QString(audioFile->getLabel().c_str()),
                          &ok);

    if (ok && !newText.isEmpty())
        audioFile->setLabel(qstrtostr(newText));

    slotPopulateFileList();
}
*/

/*
void AudioManagerDialog::slotItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    //RG_DEBUG << "Warning: AudioManagerDialog::slotItemChanged not implemented";
    return;
}
*/

//void AudioManagerDialog::slotSelectionChanged(QTreeWidgetItem *item)
void AudioManagerDialog::slotSelectionChanged()
{
    AudioListItem *aItem = nullptr;
    //AudioListItem *aItem = dynamic_cast<AudioListItem*>(item);

    QList<QTreeWidgetItem *> itemsx = m_fileList->selectedItems();
    if (itemsx.count() > 0){
        aItem = dynamic_cast<AudioListItem*>(itemsx.at(0));
    }

    // en/disable Actions
    //QAction *ea = findAction("export_audio");
    //if (ea) ea->setEnabled("false");

    // If we're on a segment then send a "select" signal
    // and enable appropriate buttons.
    //
    if (aItem && aItem->getSegment()) {

        //### required to enable it?
        //if (ea) ea->setEnabled("true");

        SegmentSelection selection;
        selection.insert(aItem->getSegment());
        emit segmentsSelected(selection);
    }

    updateActionState(aItem != nullptr);
}

void
AudioManagerDialog::setSelected(AudioFileId id,
                                const Segment *segment,
                                bool propagate)
{
    // note: this iterates over topLevelItems and childItems too.
    // I hope that's what we want to do (?)
    // otherwise re-code to iterate over topLevelItems only.
    QTreeWidgetItemIterator treeItemIter(m_fileList,
                                         QTreeWidgetItemIterator::All);

    QTreeWidgetItem *treeItem = *treeItemIter;

    while (treeItem) {

        AudioListItem *audioItem = dynamic_cast<AudioListItem *>(treeItem);

        if (audioItem) {
            if ((audioItem->getId() == id)  &&
                (audioItem->getSegment() == segment)) {

                selectFileListItemNoSignal(treeItem);

                // Only propagate to compositionview if asked to
                if (propagate) {
                    SegmentSelection selection;
                    selection.insert(audioItem->getSegment());
                    emit segmentsSelected(selection);
                }

                return;

            }

        }

        ++treeItemIter;
        treeItem = *treeItemIter;

    }

}


void
AudioManagerDialog::selectFileListItemNoSignal(QTreeWidgetItem* it)
{
    m_fileList->blockSignals(true);

    if (it) {
//        m_fileList->ensureItemVisible(it);
        m_fileList->scrollToItem(it, QAbstractItemView::PositionAtTop);
//        m_fileList->setSelected(it, true);
        m_fileList->setCurrentItem(it);
        updateActionState(true);
    } else {
        m_fileList->clearSelection();
    }

    m_fileList->blockSignals(false);
}

void
AudioManagerDialog::slotCommandExecuted()
{
    slotPopulateFileList();
}

void
AudioManagerDialog::slotSegmentSelection(
    const SegmentSelection &segments)
{
    const Segment *segment = nullptr;

    for (SegmentSelection::const_iterator it = segments.begin();
            it != segments.end(); ++it) {
        if ((*it)->getType() == Segment::Audio) {
            // Only get one audio segment
            if (segment == nullptr)
                segment = *it;
            else
                segment = nullptr;
        }

    }

    if (segment) {
        // We don't propagate this segment setting to the canvas
        // as we probably got called from there.
        //
        setSelected(segment->getAudioFileId(), segment, false);
    } else {
        selectFileListItemNoSignal(nullptr);
    }

}

/* unused
void
AudioManagerDialog::slotCancelPlayingAudioFile()
{
    emit cancelPlayingAudioFile(m_playingAudioFile);
}
*/

void
AudioManagerDialog::closePlayingDialog(AudioFileId id)
{
    //std::cout << "AudioManagerDialog::closePlayingDialog";
    if (m_audioPlayingDialog && id == m_playingAudioFile) {
        m_playTimer->stop();
        delete m_audioPlayingDialog;
        m_audioPlayingDialog = nullptr;
    }

}

bool
AudioManagerDialog::addFile(const QUrl& kurl)
{
    AudioFileId id = 0;

    AudioFileManager &aFM = m_doc->getAudioFileManager();

    if (!RosegardenMainWindow::self()->testAudioPath(tr("importing an audio file that needs to be converted or resampled"))) {
        return false;
    }

    // If multiple audio files are added concurrently, this implementation
    // looks funny to the user, but it is functional for now.  NO time for
    // a more robust solution.
    // ??? By "looks funny" I assume this means that it pops up a new
    //     progress dialog for each file being added.  Moving the
    //     QProgressDialog up the call stack shouldn't be too hard.
    //     That might cover one of the cases anyway.

    // Progress Dialog
    // Note: The label text and range will be set later as needed.
    QProgressDialog progressDialog(
            tr("Adding audio file..."),  // labelText
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

    aFM.setProgressDialog(&progressDialog);

    // Flush the event queue.
    qApp->processEvents(QEventLoop::AllEvents);

    try {
        id = aFM.importURL(kurl, m_sampleRate);
    } catch (const AudioFileManager::BadAudioPathException &e) {
        QString errorString = tr("Failed to add audio file. ") + strtoqstr(e.getMessage());
        QMessageBox::warning(this, tr("Rosegarden"), errorString);
        return false;
    } catch (const SoundFile::BadSoundFileException &e) {
        QString errorString = tr("Failed to add audio file. ") + strtoqstr(e.getMessage());
        QMessageBox::warning(this, tr("Rosegarden"), errorString);
        return false;
    }

    try {
        aFM.generatePreview(id);
    } catch (const Exception &e) {
        QString message = strtoqstr(e.getMessage()) + "\n\n" +
                          tr("Try copying this file to a directory where you have write permission and re-add it");
        QMessageBox::information(this, tr("Rosegarden"), message);
    }

    slotPopulateFileList();

    // tell the sequencer
    emit addAudioFile(id);

    return true;
}


void
AudioManagerDialog::slotDropped(QDropEvent* /* event */, QTreeWidget*, const QList<QUrl> &sl){
    /// signaled from AudioListView on dropEvent, sl = list of items (URLs)
    if( sl.empty() ) return;

    // iterate over dropped URIs
    for( int i=0; i<sl.count(); i++ ) {
        //RG_DEBUG << "slotDropped() - Adding DroppedFile " << sl.at(i);
        addFile( sl.at(i) );
    }
}

//void
//AudioManagerDialog::slotDropped(QDropEvent *event, QTreeWidgetItem*)
//{

/*
    //QStrList uri;
    QList<QString> uri;

    // see if we can decode a URI.. if not, just ignore it
//    if (QUriDrag::decode(event, uri)) {            //&&& QUriDrag, implement drag/drop

        // okay, we have a URI.. process it
//        for (QString url = uri.first(); !url.isEmpty(); url = uri.next()) { //!!! this one is really weird and uncertain
        for (int i=0; i < uri.size(); i++){
            QString url = uri.at(i);

            RG_DEBUG << "AudioManagerDialog::dropEvent() : got " << url;

            addFile(QUrl(url));
        }
//    }// end if QUriDrag
*/
//}

void
AudioManagerDialog::closeEvent(QCloseEvent *e)
{
    //RG_DEBUG << "closeEvent()\n";
    emit closing();
    QMainWindow::closeEvent(e);
}

void
AudioManagerDialog::slotClose()
{
    //RG_DEBUG << "slotClose()";
    emit closing();
    close();
}

void
AudioManagerDialog::setAudioSubsystemStatus(bool ok)
{
    // We can do something more fancy in the future but for the moment
    // this will suffice.
    //
    m_audiblePreview = ok;
}

bool
AudioManagerDialog::addAudioFile(const QString &filePath)
{
    QString fp = QFileInfo(filePath).absoluteFilePath();
    //RG_DEBUG << "addAudioFile(): fp =" << fp;
    return addFile(QUrl::fromLocalFile(fp));
}

bool
AudioManagerDialog::isSelectedTrackAudio()
{
    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();

    TrackId currentTrackId = comp.getSelectedTrack();
    Track *track = comp.getTrackById(currentTrackId);

    if (track) {
        InstrumentId ii = track->getInstrument();
        Instrument *instrument = studio.getInstrumentById(ii);

        if (instrument &&
                instrument->getType() == Instrument::Audio)
            return true;
    }

    return false;

}

void
AudioManagerDialog::slotDistributeOnMidiSegment()
{
    //RG_DEBUG << "slotDistributeOnMidiSegment()";

    //Composition &comp = m_doc->getComposition();

    QList<RosegardenMainViewWidget*> viewList_ = m_doc->getViewList();
    QListIterator<RosegardenMainViewWidget*> viewList(viewList_);

    RosegardenMainViewWidget *w = nullptr;
    SegmentSelection selection;

    viewList.toFront();
    while (viewList.hasNext()){
        w = viewList.next();
        selection = w->getSelection();
    }

    // Store the insert times in a local vector
    //
    std::vector<timeT> insertTimes;

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        // For MIDI (Internal) Segments only of course
        //
        if ((*i)->getType() == Segment::Internal) {
            for (Segment::iterator it = (*i)->begin(); it != (*i)->end(); ++it) {
                if ((*it)->isa(Note::EventType))
                    insertTimes.push_back((*it)->getAbsoluteTime());
            }
        }
    }

#if 0
    for (unsigned int i = 0; i < insertTimes.size(); ++i) {
        RG_DEBUG << "slotDistributeOnMidiSegment(): Insert audio segment at " << insertTimes[i];
    }
#endif
}

void
AudioManagerDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:audioManager-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:audioManager-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}


void
AudioManagerDialog::slotHelpAbout()
{
    new AboutDialog(this);
}
}
