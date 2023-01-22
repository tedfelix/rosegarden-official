/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioFileManager]"

#include <pthread.h>  // pthread_mutex_lock() and friends

#include <string>
#include <sstream>  // std::stringstream
#include <unistd.h>

#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

#include "gui/dialogs/AudioFileLocationDialog.h"
#include "gui/general/FileSource.h"
#include "AudioFile.h"
#include "WAVAudioFile.h"
#include "BWFAudioFile.h"
#include "misc/Debug.h"
#include "misc/Preferences.h"
#include "misc/Strings.h"  // qstrtostr() and friends
#include "sequencer/RosegardenSequencer.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "sound/audiostream/AudioReadStream.h"
#include "sound/audiostream/AudioReadStreamFactory.h"
#include "sound/audiostream/AudioWriteStream.h"
#include "sound/audiostream/AudioWriteStreamFactory.h"
#include "gui/application/SetWaitCursor.h"

#include "AudioFileManager.h"

namespace
{

    QString addTrailingSlash(const QString &path)
    {
        if (path.isEmpty())
            return "/";

        QString path2 = path;

        // Add a trailing "/" if needed.
        if (!path2.endsWith('/'))
            path2 += "/";

        return path2;
    }

}

namespace Rosegarden
{


static pthread_mutex_t audioFileManagerLock;

// ??? Why not use QMutexLocker and QMutex?  Probably need to go through
//     all the code and replace pthread_mutex_lock() with QMutex/QMutexLocker.
class MutexLock
{
public:
    explicit MutexLock(pthread_mutex_t *mutex) : m_mutex(mutex)
    {
        pthread_mutex_lock(m_mutex);
    }
    ~MutexLock()
    {
        pthread_mutex_unlock(m_mutex);
    }
private:
    pthread_mutex_t *m_mutex;
};


AudioFileManager::AudioFileManager(RosegardenDocument *doc) :
    m_document(doc),
    m_lastAudioFileID(0),
    m_audioLocationConfirmed(false),
    m_expectedSampleRate(0)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#ifdef HAVE_PTHREAD_MUTEX_RECURSIVE

    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else
#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__FreeBSD__)
    // cppcheck-suppress ConfigurationNotChecked
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else

    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
#endif

    pthread_mutex_init(&audioFileManagerLock, &attr);

}

AudioFileManager::~AudioFileManager()
{
    clear();
}

AudioFileId
AudioFileManager::addFile(const QString &filePath)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    QString ext;

    if (filePath.length() > 3) {
        // ??? Use QFileInfo::suffix().
        ext = filePath.mid(filePath.length() - 3, 3).toLower();
    }

    // Check for file existing already in manager by path
    //
    int check = fileExists(filePath);
    if (check != -1) {
        return AudioFileId(check);
    }

    // prepare for audio file
    AudioFile *aF = nullptr;
    AudioFileId id = getUniqueAudioFileID();

    if (ext == "wav") {
        // identify file type
        AudioFileType subType = RIFFAudioFile::identifySubType(filePath);

        if (subType == BWF) {
            try {
                aF = new BWFAudioFile(id, qstrtostr(getShortFilename(filePath)), filePath);
            } catch (const SoundFile::BadSoundFileException &e) {
                delete aF;
                throw BadAudioPathException(e);
            }
        } else if (subType == WAV) {
            try {
                aF = new WAVAudioFile(id, qstrtostr(getShortFilename(filePath)), filePath);
            } catch (const SoundFile::BadSoundFileException &e) {
                delete aF;
                throw BadAudioPathException(e);
            }
        }

        // Ensure we have a valid file handle
        //
        if (aF == nullptr) {
            RG_WARNING << "addFile(): Unknown WAV audio file subtype in " << filePath;
            throw BadAudioPathException(filePath, __FILE__, __LINE__);
        }

        // Add file type on extension
        try {
            if (aF->open() == false) {
                delete aF;
                RG_WARNING << "addFile(): Malformed audio file in " << filePath;
                throw BadAudioPathException(filePath, __FILE__, __LINE__);
            }
        } catch (const SoundFile::BadSoundFileException &e) {
            delete aF;
            throw BadAudioPathException(e);
        }
    }
    else {
        RG_WARNING << "addFile(): Unsupported audio file extension in " << filePath;
        throw BadAudioPathException(filePath, __FILE__, __LINE__);
    }

    if (aF) {
        m_audioFiles.push_back(aF);
        return id;
    }

    return 0;
}

QString
AudioFileManager::getShortFilename(const QString &fileName) const
{
    // ??? Use QFileInfo::fileName().

    QString rS = fileName;
    int pos = rS.lastIndexOf("/");

    if (pos > 0 && ( pos + 1 ) < rS.length())
        rS = rS.mid(pos + 1, rS.length());

    return rS;
}

QString
AudioFileManager::getDirectory(const QString &path) const
{
    // ??? Use QFileInfo::dir().

    QString rS = path;
    int pos = rS.lastIndexOf("/");

    if (pos > 0 && ( pos + 1 ) < rS.length())
        rS = rS.mid(0, pos + 1);

    return rS;
}

bool
AudioFileManager::removeFile(AudioFileId id)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    // For each AudioFile
    for (AudioFileVector::iterator audioFileIter = m_audioFiles.begin();
         audioFileIter != m_audioFiles.end();
         ++audioFileIter) {
        AudioFile *audioFile = (*audioFileIter);

        // Not the one we're looking for?  Try the next.
        if (audioFile->getId() != id)
            continue;

        // Found it

        m_peakManager.removeAudioFile(audioFile);
        m_recordedAudioFiles.erase(audioFile);
        m_derivedAudioFiles.erase(audioFile);
        delete(audioFile);
        m_audioFiles.erase(audioFileIter);

        return true;
    }

    return false;
}


AudioFileId
AudioFileManager::getUniqueAudioFileID() {
    m_lastAudioFileID++;
    return m_lastAudioFileID;
}

void
AudioFileManager::updateAudioFileID(AudioFileId id) {

    if (m_lastAudioFileID < id)
        m_lastAudioFileID = id;
}

bool
AudioFileManager::insertFile(const std::string &name,
                             const QString &fileName,
                             AudioFileId id)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    // Normally, the file will be found in the audio path.
    QString absoluteFilePath = getAbsoluteAudioPath() + fileName;

    QFileInfo fileInfo(absoluteFilePath);
    // If it isn't there...
    if (!fileInfo.exists()) {

        // ...see if the name is already an absolute or relative path.

        // Try expanding any beginning tilde or dot.
        absoluteFilePath = toAbsolute(fileName);

        fileInfo.setFile(absoluteFilePath);
        if (!fileInfo.exists())
            return false;

    }

    // Make sure we don't have a file of this ID hanging around already.
    removeFile(id);

    WAVAudioFile *aF = nullptr;

    try {

        aF = new WAVAudioFile(id, name, absoluteFilePath);

        // Test the file
        if (aF->open() == false) {
            delete aF;
            return false;
        }

        m_audioFiles.push_back(aF);
        updateAudioFileID(id);

    } catch (const SoundFile::BadSoundFileException &e) {
        delete aF;
        throw BadAudioPathException(e);
    }

    return true;
}

void
AudioFileManager::setRelativeAudioPath(
        const QString &newPath,
        bool doMoveFiles)
{
    QString newRelativePath = newPath;

    if (newRelativePath.isEmpty())
        newRelativePath = ".";

    // If the path doesn't start with "~", "/", or "."...
    if (!newRelativePath.startsWith('/') &&
        !newRelativePath.startsWith('~') &&
        !newRelativePath.startsWith('.')) {
        // Use the document path.
        newRelativePath = "./" + newRelativePath;
    }

    QString newAbsolutePath = toAbsolute(newRelativePath);
    newAbsolutePath = addTrailingSlash(newAbsolutePath);

    // ??? This is probably only needed if the directory does not
    //     exist in the first place and we have no files to move.
    //     The issue here is that the user should know that if they
    //     keep changing this, they will leave a trail of empty
    //     directories.
    //QMessageBox::information(
    //        RosegardenMainWindow::self(),
    //        tr("Audio File Location"),
    //        tr("Creating the new directory for audio files:<br />%1").
    //            arg(newAbsolutePath));

    QString originalLocation;
    // If the files are moving, provide the user with some additional
    // information in case there is a problem.
    if (doMoveFiles)
        originalLocation = tr("<br />Audio files will remain in their original location.<br />(%1)").
            arg(getAbsoluteAudioPath());

    // Make the path if needed.
    // We need to make the path even if we are not moving files into it to
    // make sure it is there for recording.  Otherwise we will end up back
    // here and the user will be stuck.
    const bool success = QDir().mkpath(newAbsolutePath);
    if (!success) {
        QMessageBox::warning(
                RosegardenMainWindow::self(),
                tr("Audio File Location"),
                tr("Cannot create audio path.<br />%1").
                    arg(newAbsolutePath) + originalLocation);
        return;
    }

    // Is the new path writable?
    // (We could do this only when moving to allow for opening and working
    // with read-only audio repositories.  But then we put off the
    // unpleasant news to recording time.)
    if (access(qstrtostr(newAbsolutePath).c_str(), W_OK) != 0) {
        QMessageBox::warning(
                RosegardenMainWindow::self(),
                tr("Audio File Location"),
                tr("Audio path is not writable.<br />%1").
                    arg(newAbsolutePath) + originalLocation);
        return;
    }

    if (doMoveFiles) {
        // Physically move the files, and adjust their paths to
        // point to the new location.
        moveFiles(newAbsolutePath);
    }

    {
        MutexLock lock (&audioFileManagerLock);

        m_relativeAudioPath = newRelativePath;
    }

    if (doMoveFiles) {
        // Force a save.
        RosegardenMainWindow::self()->slotFileSave();
    }

}

QString
AudioFileManager::getAbsoluteAudioPath() const
{
    QString absoluteAudioPath = toAbsolute(m_relativeAudioPath);
    absoluteAudioPath = addTrailingSlash(absoluteAudioPath);
    return absoluteAudioPath;
}

void
AudioFileManager::testAudioPath()
{
    QFileInfo info(getAbsoluteAudioPath());
    if (!(info.exists() && info.isDir() && !info.isRelative() &&
            info.isWritable() && info.isReadable()))
        throw BadAudioPathException(getAbsoluteAudioPath());
}

int
AudioFileManager::fileExists(const QString &absoluteFilePath)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    // For each AudioFile
    for (const AudioFile *audioFile : m_audioFiles) {
        if (audioFile->getAbsoluteFilePath() == absoluteFilePath)
            return audioFile->getId();
    }

    // Not found.
    return -1;
}

bool
AudioFileManager::fileExists(AudioFileId id)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    // For each AudioFile
    for (const AudioFile *audioFile : m_audioFiles) {
        if (audioFile->getId() == id)
            return true;
    }

    return false;
}

void
AudioFileManager::clear()
{
    MutexLock lock (&audioFileManagerLock)
        ;

    // For each AudioFile
    for (AudioFile *audioFile : m_audioFiles) {
        m_recordedAudioFiles.erase(audioFile);
        m_derivedAudioFiles.erase(audioFile);
        delete audioFile;
    }

    m_audioFiles.erase(m_audioFiles.begin(), m_audioFiles.end());

    m_peakManager.clear();
}

AudioFile *
AudioFileManager::createRecordingAudioFile(QString projectName, QString instrumentAlias)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    // Replace invalid filename characters with "_".
    instrumentAlias.replace(QRegularExpression("[&\\\\\\/%\\*\\?\"'><\\|~: ]"), "_");

    if (instrumentAlias.isEmpty()) instrumentAlias = "not_specified";

    AudioFileId newId = getUniqueAudioFileID();
    QString fileName = "";

    while (fileName == "") {

        fileName = QString("rg-[%1]-[%2]-%3-%4.wav")
                   .arg(projectName)
                   .arg(instrumentAlias)
                   .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss"))
                   .arg(newId + 1);

        if (QFile(getAbsoluteAudioPath() + fileName).exists()) {
            fileName = "";
            ++newId;
        }
    }

    // insert file into vector
    WAVAudioFile *aF = nullptr;

    //QString aup( getAbsoluteAudioPath() );
    //QString fnm(fileName);
    const QString fpath = getAbsoluteAudioPath() + fileName;
    try {
        aF = new WAVAudioFile(newId, qstrtostr(fileName), fpath);
        //aF = new WAVAudioFile(newId, fileName.data(), getAbsoluteAudioPath() + qstrtostr(fileName) );
        m_audioFiles.push_back(aF);
        m_recordedAudioFiles.insert(aF);
    } catch (const SoundFile::BadSoundFileException &e) {
        delete aF;
        throw BadAudioPathException(e);
    }

    return aF;
}

bool
AudioFileManager::wasAudioFileRecentlyRecorded(AudioFileId id)
{
    AudioFile *file = getAudioFile(id);
    if (file)
        return (m_recordedAudioFiles.find(file) !=
                m_recordedAudioFiles.end());
    return false;
}

bool
AudioFileManager::wasAudioFileRecentlyDerived(AudioFileId id)
{
    AudioFile *file = getAudioFile(id);
    if (file)
        return (m_derivedAudioFiles.find(file) !=
                m_derivedAudioFiles.end());
    return false;
}

void
AudioFileManager::resetRecentlyCreatedFiles()
{
    m_recordedAudioFiles.clear();
    m_derivedAudioFiles.clear();
}

AudioFile *
AudioFileManager::createDerivedAudioFile(AudioFileId source,
                                         const char *prefix)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    AudioFile *sourceFile = getAudioFile(source);
    if (!sourceFile) return nullptr;

    AudioFileId newId = getUniqueAudioFileID();
    QString fileName = "";

    QString sourceBase = sourceFile->getFileName();
    if (sourceBase.length() > 4 && sourceBase.mid(0, 3) == "rg-") {
        sourceBase = sourceBase.mid(3);
    }
    if (sourceBase.length() > 15) sourceBase = sourceBase.mid(0, 15);

    while (fileName == "") {

        fileName = QString("%1-%2-%3-%4.wav")
            .arg(prefix)
            .arg( sourceBase )
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"))
            .arg(newId + 1);

        if (QFile(getAbsoluteAudioPath() + fileName).exists()) {
            fileName = "";
            ++newId;
        }
    }

    // insert file into vector
    WAVAudioFile *aF = nullptr;

    try {
        aF = new WAVAudioFile(newId,
                              qstrtostr(fileName),
                              getAbsoluteAudioPath() + fileName );
        m_audioFiles.push_back(aF);
        m_derivedAudioFiles.insert(aF);
    } catch (const SoundFile::BadSoundFileException &e) {
        delete aF;
        throw BadAudioPathException(e);
    }

    return aF;
}

AudioFileId
AudioFileManager::importURL(const QUrl &url, int targetSampleRate)
{
    if (m_progressDialog) {
        m_progressDialog->setLabelText(tr("Adding audio file..."));
        // Switch to indeterminate mode since we do not provide
        // proper progress.
        m_progressDialog->setRange(0, 0);
    }

    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(nullptr, tr("Rosegarden"),
                tr("Cannot download file %1").arg(url.toString()));
        throw SoundFile::BadSoundFileException(url.toString());
    }

    source.waitForData();

    return importFile(source.getLocalFilename(), targetSampleRate);
}

AudioFileId
AudioFileManager::importFile(const QString &fileName, int targetSampleRate)
{
    if (m_progressDialog)
        m_progressDialog->setLabelText(tr("Importing audio file..."));

    QString targetName = "";
    AudioFileId newId = 0;

    {
        MutexLock lock (&audioFileManagerLock)
            ;

        newId = getUniqueAudioFileID();

        QString sourceBase = QFileInfo(fileName).baseName();
        if (sourceBase.length() > 3 && sourceBase.startsWith("rg-")) {
            sourceBase = sourceBase.right(sourceBase.length() - 3);
        }
        if (sourceBase.length() > 15) sourceBase = sourceBase.left(15);

        while (targetName == "") {

            targetName = QString("conv-%2-%3-%4.wav")
                .arg(sourceBase)
                .arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"))
                .arg(newId + 1);

            if (QFile(getAbsoluteAudioPath() + targetName).exists()) {
                targetName = "";
                ++newId;
            }
        }
    }

    // ??? We should probably switch to indeterminate mode here.  This
    //     is closest to the place where we know that we don't support
    //     actual progress.
    if (m_progressDialog)
        m_progressDialog->setLabelText(tr("Converting audio file..."));

    QString outFileName = getAbsoluteAudioPath() + targetName;
    int ec = convertAudioFile(fileName, outFileName);

    if (ec) {
        throw SoundFile::BadSoundFileException
            (fileName, qstrtostr(tr("Failed to convert or resample audio file on import")) );
    }

    {
        MutexLock lock (&audioFileManagerLock)
            ;

        // insert file into vector
        WAVAudioFile *aF =
            new WAVAudioFile(newId,
                             qstrtostr(targetName),
                             getAbsoluteAudioPath() + targetName);
        m_audioFiles.push_back(aF);
        m_derivedAudioFiles.insert(aF);
        // Don't catch SoundFile::BadSoundFileException

        m_expectedSampleRate = targetSampleRate;

        return aF->getId();
    }
}

int AudioFileManager::convertAudioFile(const QString &inFile, const QString &outFile)
{
    AudioReadStream *rs = AudioReadStreamFactory::createReadStream( inFile);
    if (!rs || !rs->isOK()) {
        RG_WARNING << "convertAudioFile(): ERROR: Failed to read audio file";
        if (rs) RG_WARNING << "convertAudioFile(): Error: " << rs->getError();

        // ??? LEAK!  rs.  Use a QScopedPointer.
        return -1;
    }

    int channels = rs->getChannelCount();
    int rate = RosegardenSequencer::getInstance()->getSampleRate();
    // Block size in number of sample frames.  A sample frame consists of
    // all the channels for a particular sample.
    int blockSize = 20480; // or anything

    rs->setRetrievalSampleRate(rate);

    AudioWriteStream *ws = AudioWriteStreamFactory::createWriteStream
            (outFile, channels, rate);

    if (!ws || !ws->isOK()) {
        RG_WARNING << "convertAudioFile(): ERROR: Failed to write audio file";
        if (ws) RG_WARNING << "convertAudioFile(): Error: " << ws->getError();

        // ??? LEAK!  rs and ws.  Use a QScopedPointer.
        return -1;
    }

    // ??? Use a QScopedArrayPointer.
    float *block = new float[blockSize * channels];

    int i = 0;
    while (1) {
        int got = rs->getInterleavedFrames(blockSize, block);
        qApp->processEvents();
        ws->putInterleavedFrames(got, block);
        qApp->processEvents();
        if (got < blockSize) break;

        // ??? It would be nice if we could provide progress to the
        //     progress dialog like this:
        //
        //       if (m_progressDialog)
        //           m_progressDialog->setValue(
        //               static_cast<double>(currentBytes)/totalBytes * 100);
        //
        //     However, we don't have any way to get the total bytes from
        //     AudioReadStream, let alone the current bytes.

        if (m_progressDialog  &&  m_progressDialog->wasCanceled()) {
            // Clean up the file that we were writing.
            ws->remove();

            // ??? Cleanup not needed with smart pointers mentioned above.
            delete[] block;
            delete ws;
            delete rs;

            // Failure.
            return -1;
        }

        ++i;
    }

    // ??? Cleanup not needed with smart pointers mentioned above.
    delete[] block;
    delete ws;
    delete rs;

    // Success.
    return 0;
}

std::string
AudioFileManager::toXmlString() const
{
    MutexLock lock (&audioFileManagerLock)
        ;

    std::stringstream audioFiles;

    audioFiles << "<audiofiles";
    if (m_expectedSampleRate != 0) {
        audioFiles << " expectedRate=\"" << m_expectedSampleRate << "\"";
    }
    audioFiles << ">" << std::endl;
    audioFiles << "    <audioPath value=\"" << m_relativeAudioPath << "\"/>" << std::endl;

    QString fileName;

    // For each AudioFile
    for (const AudioFile *audioFile : m_audioFiles) {
        fileName = audioFile->getAbsoluteFilePath();

        // If the absolute audio path is here, remove it.
        // ??? insertFile() is the other side of this.
        if (getDirectory(fileName) == getAbsoluteAudioPath())
            fileName = getShortFilename(fileName);

        audioFiles << "    <audio id=\"" << audioFile->getId()
                   << "\" file=\"" << fileName
                   << "\" label=\"" << encode(audioFile->getLabel())
                   << "\"/>" << std::endl;
    }

    audioFiles << "</audiofiles>" << std::endl;

    audioFiles << std::endl;

    return audioFiles.str();
}

void
AudioFileManager::generatePreviews()
{
    MutexLock lock (&audioFileManagerLock)
        ;

    if (m_progressDialog) {
        // Or should we push this all the way down into PeakFile?
        m_progressDialog->setLabelText(tr("Generating audio previews..."));
        m_progressDialog->setRange(0, 100);
    }

    m_peakManager.setProgressDialog(m_progressDialog);

    // Generate peaks if we need to

    // For each AudioFile
    for (AudioFile *audioFile : m_audioFiles) {
        if (!m_peakManager.hasValidPeaks(audioFile))
            m_peakManager.generatePeaks(audioFile);

        if (m_progressDialog  &&  m_progressDialog->wasCanceled())
            break;
    }

    // Even if we didn't do anything, reset the progress dialog.
    if (m_progressDialog)
        m_progressDialog->setValue(100);
}

bool
AudioFileManager::generatePreview(AudioFileId id)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    if (m_progressDialog) {
        // Or should we push this all the way down into PeakFile?
        m_progressDialog->setLabelText(tr("Generating audio preview..."));
        m_progressDialog->setRange(0, 100);
    }

    // Pass on any progress dialog we might have.
    m_peakManager.setProgressDialog(m_progressDialog);

    AudioFile *audioFile = getAudioFile(id);

    if (audioFile == nullptr)
        return false;

    if (!m_peakManager.hasValidPeaks(audioFile))
        m_peakManager.generatePeaks(audioFile);

    return true;
}

AudioFile *
AudioFileManager::getAudioFile(AudioFileId id)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    // ??? A std::map<AudioFileId, AudioFile *> would be faster.

    // For each AudioFile
    for (AudioFile *audioFile : m_audioFiles) {
        if (audioFile->getId() == id)
            return audioFile;
    }

    // Not found
    return nullptr;
}

std::vector<float>
AudioFileManager::getPreview(AudioFileId id,
                             const RealTime &startTime,
                             const RealTime &endTime,
                             int width,
                             bool withMinima)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    AudioFile *audioFile = getAudioFile(id);
    if (!audioFile)
        return std::vector<float>();

    if (!m_peakManager.hasValidPeaks(audioFile)) {
        // ??? This happens a lot when recording audio.  Need to detect
        //     that we are recording and suppress this.  Or just don't
        //     call this when recording.  The caller has comments in
        //     its catch() to this effect.
        //RG_WARNING << "getPreview(): No peaks for audio file" << audioFile->getAbsoluteFilePath() << "(this is probably OK when recording)";
        throw PeakFileManager::BadPeakFileException(
                audioFile->getAbsoluteFilePath(), __FILE__, __LINE__);
    }

    return m_peakManager.getPreview(audioFile,
                                    startTime,
                                    endTime,
                                    width,
                                    withMinima);
}

void
AudioFileManager::drawPreview(AudioFileId id,
                              const RealTime &startTime,
                              const RealTime &endTime,
                              QPixmap *pixmap)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    AudioFile *audioFile = getAudioFile(id);
    if (!audioFile)
        return;

    if (!m_peakManager.hasValidPeaks(audioFile)) {
        RG_WARNING << "drawPreview(): No peaks for audio file " << audioFile->getAbsoluteFilePath();
        throw PeakFileManager::BadPeakFileException(
                audioFile->getAbsoluteFilePath(), __FILE__, __LINE__);
    }

    std::vector<float> values = m_peakManager.getPreview
                                (audioFile,
                                 startTime,
                                 endTime,
                                 pixmap->width(),
                                 false);

    QPainter painter(pixmap);
    pixmap->fill(Qt::white);
    painter.setPen(Qt::gray);
//    painter.setPen(qApp->palette().color(QPalette::Active,
//                                         QColorGroup::Mid));

    if (values.empty())
        return;

    float yStep = pixmap->height() / 2;
    int channels = audioFile->getChannels();
    float ch1Value, ch2Value;

    if (channels == 0)
        return;


    // Render pixmap
    //
    for (int i = 0; i < pixmap->width(); i++) {
        // Always get two values for our pixmap no matter how many
        // channels in AudioFile as that's all we can display.
        //
        if (channels == 1) {
            ch1Value = values[i];
            ch2Value = values[i];
        } else {
            ch1Value = values[i * channels];
            ch2Value = values[i * channels + 1];
        }

        painter.drawLine(i, static_cast<int>(yStep - ch1Value * yStep),
                         i, static_cast<int>(yStep + ch2Value * yStep));
    }
}

void
AudioFileManager::drawHighlightedPreview(AudioFileId id,
        const RealTime &startTime,
        const RealTime &endTime,
        const RealTime &highlightStart,
        const RealTime &highlightEnd,
        QPixmap *pixmap)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    AudioFile *audioFile = getAudioFile(id);
    if (!audioFile)
        return;

    if (!m_peakManager.hasValidPeaks(audioFile)) {
        RG_WARNING << "drawHighlightedPreview(): No peaks for audio file " << audioFile->getAbsoluteFilePath();
        throw PeakFileManager::BadPeakFileException
        (audioFile->getAbsoluteFilePath(), __FILE__, __LINE__);
    }

    std::vector<float> values = m_peakManager.getPreview
                                (audioFile,
                                 startTime,
                                 endTime,
                                 pixmap->width(),
                                 false);

    int startWidth = (int)(double(pixmap->width()) * (highlightStart /
                           (endTime - startTime)));
    int endWidth = (int)(double(pixmap->width()) * (highlightEnd /
                         (endTime - startTime)));

    QPainter painter(pixmap);
    pixmap->fill(Qt::white);
//    pixmap->fill(qApp->palette().color(QPalette::Active,
//                                       QColorGroup::Base));

    float yStep = pixmap->height() / 2;
    int channels = audioFile->getChannels();
    float ch1Value, ch2Value;

    // Render pixmap
    //
    for (int i = 0; i < pixmap->width(); ++i) {
        if ((i * channels + (channels - 1)) >= int(values.size()))
            break;

        // Always get two values for our pixmap no matter how many
        // channels in AudioFile as that's all we can display.
        //
        if (channels == 1) {
            ch1Value = values[i];
            ch2Value = values[i];
        } else {
            ch1Value = values[i * channels];
            ch2Value = values[i * channels + 1];
        }

        if (i < startWidth || i > endWidth)
            painter.setPen(Qt::gray);
            //painter.setPen(qApp->palette().color(QPalette::Active,
            //                                     QColorGroup::Mid));
        else
            painter.setPen(Qt::black);
            //painter.setPen(qApp->palette().color(QPalette::Active,
            //                                     QColorGroup::Dark));

        painter.drawLine(i, static_cast<int>(yStep - ch1Value * yStep),
                         i, static_cast<int>(yStep + ch2Value * yStep));
    }
}


void
AudioFileManager::print()
{
    MutexLock lock (&audioFileManagerLock)
        ;

    RG_DEBUG << "print():" << m_audioFiles.size() << "entries";

    // For each AudioFile
    for (AudioFile *audioFile : m_audioFiles) {
        RG_DEBUG << "  " << audioFile->getId() << " : " <<
                            audioFile->getLabel() << " : \"" <<
                            audioFile->getAbsoluteFilePath() << "\"";
    }
}

std::vector<SplitPointPair>
AudioFileManager::getSplitPoints(AudioFileId id,
                                 const RealTime &startTime,
                                 const RealTime &endTime,
                                 int threshold,
                                 const RealTime &minTime)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    AudioFile *audioFile = getAudioFile(id);

    if (audioFile == nullptr)
        return std::vector<SplitPointPair>();

    return m_peakManager.getSplitPoints(audioFile,
                                        startTime,
                                        endTime,
                                        threshold,
                                        minTime);
}

std::set<int>
AudioFileManager::getActualSampleRates() const
{
    std::set<int> rates;

    // For each AudioFile
    for (const AudioFile *audioFile : m_audioFiles) {
        int sampleRate = audioFile->getSampleRate();
        if (sampleRate != 0)
            rates.insert(sampleRate);
    }

    return rates;
}

QString
AudioFileManager::toAbsolute(const QString &relativePath) const
{
    if (relativePath.isEmpty())
        return relativePath;

    QString absolutePath = relativePath;

    // Convert tilde to home dir.
    if (absolutePath.left(1) == "~") {
        absolutePath.remove(0, 1);
        absolutePath = QDir::homePath() + absolutePath;
    }

    // Handle double-dot.  A bit messy, but should work.
    if (absolutePath.left(2) == "..")
        absolutePath = "./" + absolutePath;

    // Convert dot to .rg file location.
    if (absolutePath.left(1) == "."  &&  m_document) {
        absolutePath.remove(0, 1);
        QString absFilePath = m_document->getAbsFilePath();
        QFileInfo fileInfo(absFilePath);
        absolutePath = fileInfo.canonicalPath() + absolutePath;
    }

    return absolutePath;
}

void
AudioFileManager::moveFiles(const QString &newPath)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    SetWaitCursor setWaitCursor;

    QString newPath2 = toAbsolute(newPath);
    newPath2 = addTrailingSlash(newPath2);

    // for each audio file
    for (AudioFile *audioFile : m_audioFiles)
    {
        const QString oldName = audioFile->getAbsoluteFilePath();

        QFileInfo fileInfo(oldName);
        const QString newName = newPath2 + fileInfo.fileName();

        // If there is no actual change, try the next.
        if (newName == oldName)
            continue;

        // Delete the old peak file.
        // ??? It would be more clever to just move the .pk file.
        //     Might want to look into doing that instead.
        m_peakManager.deletePeakFile(audioFile);

        // Close the old file.
        audioFile->close();

        // Move it to the new path.
        const bool success = QFile::rename(oldName, newName);

        if (!success) {
            RG_WARNING << "moveFiles(): rename failed for:";
            RG_WARNING << "  oldName:" << oldName;
            RG_WARNING << "  newName:" << newName;
        }

        // Adjust the stored path for this file.
        audioFile->setAbsoluteFilePath(newName);

        // Open the file in its new location.
        audioFile->open();

        // Create the peak file in the new location.
        m_peakManager.generatePeaks(audioFile);
    }

    // Reset sequencer audio so that we can hear the audio files again.
    m_document->prepareAudio();
}

void
AudioFileManager::save()
{
    // We've saved, so we don't want to delete the audio files that
    // have been created up to now.
    resetRecentlyCreatedFiles();

    // If the user has already confirmed the audio location, bail.
    if (m_audioLocationConfirmed)
        return;

    // No audio files?  No need to prompt for save location.  Bail.
    if (m_audioFiles.empty())
        return;

    QFileInfo fileInfo(m_document->getAbsFilePath());
    const QString documentNameDir = "./" + fileInfo.completeBaseName();

    // If the preferences indicate prompting
    if (!Preferences::getAudioFileLocationDlgDontShow())
    {

        // Ask the user to pick an audio file path.
        // Results go to the Preferences.
        AudioFileLocationDialog audioFileLocationDialog(
                RosegardenMainWindow::self(),
                documentNameDir);
        audioFileLocationDialog.exec();

    }

    const AudioFileLocationDialog::Location location =
            static_cast<AudioFileLocationDialog::Location>(
                    Preferences::getDefaultAudioLocation());

    // Indicate audio location was confirmed by the user.
    // Also avoid recursion loop as the second save will end up here again.
    m_audioLocationConfirmed = true;

    QString audioPath = "./audio";

    switch (location) {
    case AudioFileLocationDialog::AudioDir:
        audioPath = "./audio";
        break;
    case AudioFileLocationDialog::DocumentNameDir:
        audioPath = documentNameDir;
        break;
    case AudioFileLocationDialog::DocumentDir:
        audioPath = ".";
        break;
    case AudioFileLocationDialog::CentralDir:
        audioPath = "~/rosegarden-audio";
        break;
    case AudioFileLocationDialog::CustomDir:
        audioPath = Preferences::getCustomAudioLocation();
        break;
    }

    // Set the new location, move the files, and save.
    setRelativeAudioPath(audioPath, true);
}


}
