/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
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

#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include "gui/general/FileSource.h"
#include "AudioFile.h"
#include "WAVAudioFile.h"
#include "BWFAudioFile.h"
#include "misc/Debug.h"
#include "misc/Strings.h"  // qstrtostr() and friends
#include "sequencer/RosegardenSequencer.h"
#include "sound/audiostream/AudioReadStream.h"
#include "sound/audiostream/AudioReadStreamFactory.h"
#include "sound/audiostream/AudioWriteStream.h"
#include "sound/audiostream/AudioWriteStreamFactory.h"

#include "AudioFileManager.h"

// #define DEBUG_AUDIOFILEMANAGER
// #define DEBUG_AUDIOFILEMANAGER_INSERT_FILE

namespace Rosegarden
{

static pthread_mutex_t audioFileManagerLock;

// ??? Why not use QMutexLocker and QMutex?  Probably need to go through
//     all the code and replace pthread_mutex_lock() with QMutex/QMutexLocker.
class MutexLock
{
public:
    MutexLock(pthread_mutex_t *mutex) : m_mutex(mutex)
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


AudioFileManager::AudioFileManager() :
    m_lastAudioFileID(0),
    m_expectedSampleRate(0)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#ifdef HAVE_PTHREAD_MUTEX_RECURSIVE

    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else
#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__FreeBSD__)

    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else

    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
#endif

    pthread_mutex_init(&audioFileManagerLock, &attr);

    // Set this through the set method so that the tilde gets
    // shaken out.
    //
    setAudioPath("~/rosegarden");

    // Retransmit value()
    //
    connect(&m_peakManager, SIGNAL(setValue(int)),
            this, SIGNAL(setValue(int)));
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
    AudioFile *aF = 0;
    AudioFileId id = getUniqueAudioFileID();

    if (ext == "wav") {
        // identify file type
        AudioFileType subType = RIFFAudioFile::identifySubType(filePath);

        if (subType == BWF) {
#ifdef DEBUG_AUDIOFILEMANAGER
            RG_DEBUG << "FOUND BWF";
#endif

            try {
                aF = new BWFAudioFile(id, qstrtostr(getShortFilename(filePath)), filePath);
            } catch (SoundFile::BadSoundFileException e) {
                delete aF;
                throw BadAudioPathException(e);
            }
        } else if (subType == WAV) {
            try {
                aF = new WAVAudioFile(id, qstrtostr(getShortFilename(filePath)), filePath);
            } catch (SoundFile::BadSoundFileException e) {
                delete aF;
                throw BadAudioPathException(e);
            }
        }

        // Ensure we have a valid file handle
        //
        if (aF == 0) {
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
        } catch (SoundFile::BadSoundFileException e) {
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

#if 0
// Create a new AudioFile with unique ID and label - insert from
// our RG4 file
//
AudioFileId
AudioFileManager::insertFile(const std::string &name,
                             const QString &fileName)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    // first try to expand any beginning tilde
    //
    QString foundFileName = tildeToHome(fileName);

    // If we've expanded and we can't find the file
    // then try to find it in audio file directory.
    //
    QFileInfo info(foundFileName);
    if (!info.exists())
        foundFileName = getFileInPath(foundFileName);

#ifdef DEBUG_AUDIOFILEMANAGER_INSERT_FILE
    RG_DEBUG << "insertFile() - expanded fileName = \"" << foundFileName << "\"";
#endif

    // bail if we haven't found any reasonable filename
    if (foundFileName == "")
        return false;

    AudioFileId id = getUniqueAudioFileID();

    WAVAudioFile *aF = 0;

    try {

        aF = new WAVAudioFile(id, name, foundFileName);

        // if we don't recognise the file then don't insert it
        //
        if (aF->open() == false) {
            delete aF;
            RG_WARNING << "insertFile() - don't recognise file type in " << foundFileName;
            throw BadAudioPathException(foundFileName, __FILE__, __LINE__);
        }
        m_audioFiles.push_back(aF);

    } catch (SoundFile::BadSoundFileException e) {
        delete aF;
        throw BadAudioPathException(e);
    }

    return id;
}
#endif

bool
AudioFileManager::removeFile(AudioFileId id)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    // For each AudioFile
    for (std::vector<AudioFile *>::iterator it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it) {
        if ((*it)->getId() == id) {
            m_peakManager.removeAudioFile(*it);
            m_recordedAudioFiles.erase(*it);
            m_derivedAudioFiles.erase(*it);
            delete(*it);
            m_audioFiles.erase(it);
            return true;
        }
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

    // first try to expand any beginning tilde
    QString foundFileName = tildeToHome(fileName);

    // If we've expanded and we can't find the file
    // then try to find it in audio file directory.
    //
    QFileInfo info(foundFileName);
    if (!info.exists())
        foundFileName = getFileInPath(foundFileName);

#ifdef DEBUG_AUDIOFILEMANAGER_INSERT_FILE
    RG_DEBUG << "insertFile() - expanded fileName = \"" << foundFileName << "\"";
#endif

    // If no joy here then we can't find this file
    if (foundFileName == "")
        return false;

    // make sure we don't have a file of this ID hanging around already
    removeFile(id);

    // and insert
    WAVAudioFile *aF = 0;

    try {

        aF = new WAVAudioFile(id, name, foundFileName);

        // Test the file
        if (aF->open() == false) {
            delete aF;
            return false;
        }

        m_audioFiles.push_back(aF);
        updateAudioFileID(id);

    } catch (SoundFile::BadSoundFileException e) {
        delete aF;
        throw BadAudioPathException(e);
    }

    return true;
}

void
AudioFileManager::setAudioPath(const QString &path)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    if (path.size() != 0) {
        QString hPath = path;
        QString homePath = getenv("HOME");
    
        // add a trailing / if we don't have one
        if (hPath[hPath.size() - 1] != '/')
            hPath += "/";

        // get the home directory
        // ??? Use tildeToHome().
        if (hPath[0] == '~') {
            hPath.remove(0, 1);
            hPath = homePath + hPath;
        }

        m_audioPath = hPath;
    } else {
#ifdef DEBUG_AUDIOFILEMANAGER
        RG_DEBUG << "setAudioPath() - zero length path, do nothing";
#endif
    }

}

void
AudioFileManager::testAudioPath()
{
    QFileInfo info(m_audioPath);
    if (!(info.exists() && info.isDir() && !info.isRelative() &&
            info.isWritable() && info.isReadable()))
        throw BadAudioPathException(m_audioPath);
}

QString
AudioFileManager::getFileInPath(const QString &file)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    QFileInfo info(file);

    if (info.exists())
        return file;

    // Build the search filename from the audio path and
    // the file basename.
    //
    QString searchFile = m_audioPath + info.fileName();
    QFileInfo searchInfo(searchFile);

    if (searchInfo.exists())
        return searchFile.toLatin1().data();

    RG_DEBUG << "getFileInPath() - searchInfo = " << searchFile;

    return "";
}

int
AudioFileManager::fileExists(const QString &path)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    // For each AudioFile
    for (std::vector<AudioFile *>::const_iterator it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it) {
        if ((*it)->getFilename() == path)
            return (*it)->getId();
    }

    return -1;

}

bool
AudioFileManager::fileExists(AudioFileId id)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    // For each AudioFile
    for (std::vector<AudioFile *>::const_iterator it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it) {
        if ((*it)->getId() == id)
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
    for (std::vector<AudioFile *>::const_iterator it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it) {
        m_recordedAudioFiles.erase(*it);
        m_derivedAudioFiles.erase(*it);
        delete(*it);
    }

    m_audioFiles.erase(m_audioFiles.begin(), m_audioFiles.end());

    m_peakManager.clear();
}

AudioFile *
AudioFileManager::createRecordingAudioFile(QString projectName, QString instrumentAlias)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    RG_DEBUG << "createRecordingAudioFile(): " << projectName;

    // Replace invalid filename characters with "_".
    instrumentAlias.replace(QRegExp("[&\\\\\\/%\\*\\?\"'><\\|~: ]"), "_");

    if (instrumentAlias.isEmpty()) instrumentAlias = "not_specified";

    AudioFileId newId = getUniqueAudioFileID();
    QString fileName = "";

    while (fileName == "") {

        fileName = QString("rg-[%1]-[%2]-%3-%4.wav")
                   .arg(projectName)
                   .arg(instrumentAlias)
                   .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss"))
                   .arg(newId + 1);

        if (QFile(m_audioPath + fileName).exists()) {
            fileName = "";
            ++newId;
        }
    }

    // insert file into vector
    WAVAudioFile *aF = 0;

    QString aup( m_audioPath );
    QString fnm(fileName);
    const QString fpath = m_audioPath + fileName;
    try {
        aF = new WAVAudioFile( static_cast<const unsigned int>(newId), qstrtostr(fileName), fpath );
        //aF = new WAVAudioFile(newId, fileName.data(), m_audioPath + qstrtostr(fileName) );
        m_audioFiles.push_back(aF);
        m_recordedAudioFiles.insert(aF);
    } catch (SoundFile::BadSoundFileException e) {
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
    if (!sourceFile) return 0;

    AudioFileId newId = getUniqueAudioFileID();
    QString fileName = "";

    QString sourceBase = sourceFile->getShortFilename();
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

        if (QFile(m_audioPath + fileName).exists()) {
            fileName = "";
            ++newId;
        }
    }

    // insert file into vector
    WAVAudioFile *aF = 0;

    try {
        aF = new WAVAudioFile(newId,
                              qstrtostr(fileName),
                              m_audioPath + fileName );
        m_audioFiles.push_back(aF);
        m_derivedAudioFiles.insert(aF);
    } catch (SoundFile::BadSoundFileException e) {
        delete aF;
        throw BadAudioPathException(e);
    }

    return aF;
}

AudioFileId
AudioFileManager::importURL(const QUrl &url, int sampleRate,
                            QPointer<QProgressDialog> progressDialog)
{
    if (progressDialog)
        m_progressDialog = progressDialog;

    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(0, tr("Rosegarden"),
                tr("Cannot download file %1").arg(url.toString()));
        throw SoundFile::BadSoundFileException(url.toString());
    }

    source.waitForData();

    return importFile(source.getLocalFilename(), sampleRate);
}

AudioFileId
AudioFileManager::importFile(const QString &fileName, int sampleRate)
{
    RG_DEBUG << "importFile("<< fileName << ", " << sampleRate << ")";

    emit setOperationName(tr("Importing audio file..."));
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

            if (QFile(m_audioPath + targetName).exists()) {
                targetName = "";
                ++newId;
            }
        }
    }

    emit setOperationName(tr("Converting audio file..."));
    // ??? We should probably switch to indeterminate mode here.  This
    //     is closest to the place where we know that we don't support
    //     actual progress.
    if (m_progressDialog)
        m_progressDialog->setLabelText(tr("Converting audio file..."));

    QString outFileName = m_audioPath + targetName;
    int ec = convertAudioFile(fileName, outFileName);

    if (ec) {
        throw SoundFile::BadSoundFileException
            (fileName, qstrtostr(tr("Failed to convert or resample audio file on import")) );
    }

    {
        MutexLock lock (&audioFileManagerLock)
            ;

        // insert file into vector
        WAVAudioFile *aF = 0;

        aF = new WAVAudioFile(newId,
                              qstrtostr(targetName),
                              m_audioPath + targetName);
        m_audioFiles.push_back(aF);
        m_derivedAudioFiles.insert(aF);
        // Don't catch SoundFile::BadSoundFileException

        m_expectedSampleRate = sampleRate;

        return aF->getId();
    }
}

int AudioFileManager::convertAudioFile(const QString &inFile, const QString &outFile)
{
    RG_DEBUG << "convertAudioFile(): inFile = " << inFile << ", outFile = " << outFile;

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
            // ??? We should delete the output file being generated by ws.
            //ws->deleteFile();

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

void
AudioFileManager::slotStopImport()
{
    //!!!
}

#if 0
AudioFile *
AudioFileManager::getLastAudioFile()
{
    MutexLock lock (&audioFileManagerLock)
        ;

    std::vector<AudioFile *>::iterator it = m_audioFiles.begin();
    AudioFile * audioFile = 0;

    while (it != m_audioFiles.end()) {
        audioFile = (*it);
        ++it;
    }

    return audioFile;
}
#endif

QString
AudioFileManager::homeToTilde(const QString &path) const
{
    QString rS = path;
    QString homePath = getenv("HOME");

    // if path length is less than homePath then just return unchanged
    if (rS.length() < homePath.length())
        return rS;

    // if the first section matches the path then substitute
    if (rS.mid(0, homePath.length()) == homePath) {
        rS.remove(0, homePath.length());
        rS = "~" + rS;
    }

    return rS;
}

QString
AudioFileManager::tildeToHome(const QString &path) const
{
    QString rS = path;
    const QString homePath = QDir::homePath();

    if (rS.mid(0, 2) == "~/") {
        rS.remove(0, 1); // erase tilde and prepend HOME env
        rS = homePath + rS;
    }

    return rS;
}

std::string
AudioFileManager::toXmlString() const
{
    MutexLock lock (&audioFileManagerLock)
        ;

    std::stringstream audioFiles;
    QString audioPath = homeToTilde(m_audioPath);

    audioFiles << "<audiofiles";
    if (m_expectedSampleRate != 0) {
        audioFiles << " expectedRate=\"" << m_expectedSampleRate << "\"";
    }
    audioFiles << ">" << std::endl;
    audioFiles << "    <audioPath value=\"" << audioPath << "\"/>" << std::endl;

    QString fileName;

    // For each AudioFile
    for (std::vector<AudioFile *>::const_iterator it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it) {
        fileName = (*it)->getFilename();

        // attempt two substitutions - If the prefix to the filename
        // is the same as the audio path then we can dock the prefix
        // as it'll be added again next time.  If the path doesn't
        // have the audio path in it but has our home directory in it
        // then swap this out for a tilde '~'
        //
#ifdef DEBUG_AUDIOFILEMANAGER
        RG_DEBUG << "toXmlString(): DIR = " << getDirectory(fileName) << " :  PATH = " << m_audioPath;
#endif

        if (getDirectory(fileName) == m_audioPath)
            fileName = getShortFilename(fileName);
        else
            fileName = homeToTilde(fileName);

        audioFiles << "    <audio id=\"" << (*it)->getId()
                   << "\" file=\"" << fileName
                   << "\" label=\"" << encode((*it)->getLabel())
                   << "\"/>" << std::endl;
    }

    audioFiles << "</audiofiles>" << std::endl;

    audioFiles << std::endl;

    return audioFiles.str();
}

void
AudioFileManager::generatePreviews(QPointer<QProgressDialog> progressDialog)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    if (progressDialog)
        m_progressDialog = progressDialog;

    m_peakManager.setProgressDialog(m_progressDialog);

#ifdef DEBUG_AUDIOFILEMANAGER
    RG_DEBUG << "generatePreviews() - for " << m_audioFiles.size() << " files";
#endif


    // Generate peaks if we need to

    // For each AudioFile
    for (std::vector<AudioFile *>::const_iterator it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it) {
        if (!m_peakManager.hasValidPeaks(*it))
            m_peakManager.generatePeaks(*it);

        if (m_progressDialog  &&  m_progressDialog->wasCanceled())
            break;
    }

    // Even if we didn't do anything, reset the progress dialog.
    emit setValue(100);
    if (m_progressDialog)
        m_progressDialog->setValue(100);
}

void
AudioFileManager::slotStopPreview()
{
    MutexLock lock (&audioFileManagerLock)
        ;

    m_peakManager.stopPreview();
}


bool
AudioFileManager::generatePreview(AudioFileId id)
{
    MutexLock lock (&audioFileManagerLock)
        ;

    // Pass on any progress dialog we might have.
    m_peakManager.setProgressDialog(m_progressDialog);

    AudioFile *audioFile = getAudioFile(id);

    if (audioFile == 0)
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

    // For each AudioFile
    for (std::vector<AudioFile *>::const_iterator it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it) {
        if ((*it)->getId() == id)
            return (*it);
    }
    return 0;
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

    if (audioFile == 0) {
        return std::vector<float>();
    }

    if (!m_peakManager.hasValidPeaks(audioFile)) {
        RG_WARNING << "getPreview(): No peaks for audio file " << audioFile->getFilename();
        throw PeakFileManager::BadPeakFileException(
                audioFile->getFilename(), __FILE__, __LINE__);
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
        RG_WARNING << "drawPreview(): No peaks for audio file " << audioFile->getFilename();
        throw PeakFileManager::BadPeakFileException(
                audioFile->getFilename(), __FILE__, __LINE__);
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

    if (values.size() == 0) {
#ifdef DEBUG_AUDIOFILEMANAGER
        RG_WARNING << "drawPreview() - no preview values returned!";
#endif

        return;
    }

    float yStep = pixmap->height() / 2;
    int channels = audioFile->getChannels();
    float ch1Value, ch2Value;

    if (channels == 0) {
#ifdef DEBUG_AUDIOFILEMANAGER
        RG_WARNING << "drawPreview() - no channels in audio file!";
#endif

        return;
    }


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
        RG_WARNING << "getPreview(): No peaks for audio file " << audioFile->getFilename();
        throw PeakFileManager::BadPeakFileException
        (audioFile->getFilename(), __FILE__, __LINE__);
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

        if (i < startWidth || i > endWidth) painter.setPen(Qt::gray);
//            painter.setPen(qApp->palette().color(QPalette::Active,
//                                                 QColorGroup::Mid));
        else
            painter.setPen(Qt::black);
//            painter.setPen(qApp->palette().color(QPalette::Active,
//                                                 QColorGroup::Dark));

        painter.drawLine(i, static_cast<int>(yStep - ch1Value * yStep),
                         i, static_cast<int>(yStep + ch2Value * yStep));
    }
}


void
AudioFileManager::print()
{
    MutexLock lock (&audioFileManagerLock)
        ;

#ifdef DEBUG_AUDIOFILEMANAGER
    RG_DEBUG << "print():" << m_audioFiles.size() << "entries";

    // For each AudioFile
    for (std::vector<AudioFile *>::const_iterator it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it) {
        RG_DEBUG << "  " << (*it)->getId() << " : " << (*it)->getLabel() << " : \"" << (*it)->getFilename() << "\"";
    }
#endif
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

    if (audioFile == 0)
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
    for (std::vector<AudioFile *>::const_iterator i = m_audioFiles.begin();
         i != m_audioFiles.end();
         ++i) {

        unsigned int sr = (*i)->getSampleRate();
        if (sr != 0) rates.insert(int(sr));
    }

    return rates;
}

}


