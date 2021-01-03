/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    This file originally from Sonic Visualiser, copyright 2007 Queen
    Mary, University of London.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "FileSource.h"

#include "TempDirectory.h"
#include "misc/Strings.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QThreadStorage>

#include <iostream>
#include <cstdlib>

#include <unistd.h>

//#define DEBUG_FILE_SOURCE 1

namespace Rosegarden {

int
FileSource::m_count = 0;

QMutex
FileSource::m_fileCreationMutex;

FileSource::RemoteRefCountMap
FileSource::m_refCountMap;

FileSource::RemoteLocalMap
FileSource::m_remoteLocalMap;

QMutex
FileSource::m_mapMutex;

#ifdef DEBUG_FILE_SOURCE
static int extantCount = 0;
static std::map<QString, int> urlExtantCountMap;
static void incCount(QString url) {
    ++extantCount;
    if (urlExtantCountMap.find(url) == urlExtantCountMap.end()) {
        urlExtantCountMap[url] = 1;
    } else {
        ++urlExtantCountMap[url];
    }
    std::cerr << "FileSource: Now " << urlExtantCountMap[url] << " for this url, " << extantCount << " total" << std::endl;
}
static void decCount(QString url) {
    --extantCount;
    --urlExtantCountMap[url];
    std::cerr << "FileSource: Now " << urlExtantCountMap[url] << " for this url, " << extantCount << " total" << std::endl;
}
#endif

static QThreadStorage<QNetworkAccessManager *> nms;

#if 0
FileSource::FileSource(QString fileOrUrl,
                       QString preferredContentType) :
    m_rawFileOrUrl(fileOrUrl),
    m_url(fileOrUrl, QUrl::StrictMode),
    m_localFile(0),
    m_reply(0),
    m_preferredContentType(preferredContentType),
    m_ok(false),
    m_lastStatus(0),
    m_resource(fileOrUrl.startsWith(':')),
    m_remote(isRemote(fileOrUrl)),
    m_done(false),
    m_leaveLocalFile(false),
    m_refCounted(false)
{
    if (m_resource) { // qrc file
        m_url = QUrl("qrc" + fileOrUrl);
    }

    if (m_url.toString() == "") {
        m_url = QUrl(fileOrUrl, QUrl::TolerantMode);
    }
 
#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::FileSource(" << fileOrUrl << "): url <" << m_url.toString() << ">" << std::endl;
    incCount(m_url.toString());
#endif

    if (!canHandleScheme(m_url)) {
        std::cerr << "FileSource::FileSource: ERROR: Unsupported scheme in URL \"" << m_url.toString() << "\"" << std::endl;
        m_errorString = tr("Unsupported scheme in URL");
        return;
    }

    init();

    if (!isRemote() &&
        !isAvailable()) {
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::FileSource: Failed to open local file with URL \"" << m_url.toString() << "\"; trying again assuming filename was encoded" << std::endl;
#endif
        m_url = QUrl::fromEncoded(fileOrUrl.toLatin1());
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::FileSource: URL is now \"" << m_url.toString() << "\"" << std::endl;
#endif
        init();
    }

    if (isRemote() &&
        (fileOrUrl.contains('%') ||
         fileOrUrl.contains("--"))) { // for IDNA

        waitForStatus();

        if (!isAvailable()) {

            // The URL was created on the assumption that the string
            // was human-readable.  Let's try again, this time
            // assuming it was already encoded.
            std::cerr << "FileSource::FileSource: Failed to retrieve URL \""
                      << fileOrUrl
                      << "\" as human-readable URL; "
                      << "trying again treating it as encoded URL"
                      << std::endl;

            // even though our cache file doesn't exist (because the
            // resource was 404), we still need to ensure we're no
            // longer associating a filename with this url in the
            // refcount map -- or createCacheFile will think we've
            // already done all the work and no request will be sent
            deleteCacheFile();

            m_url = QUrl::fromEncoded(fileOrUrl.toLatin1());

            m_ok = false;
            m_done = false;
            m_lastStatus = 0;
            init();
        }
    }

    if (!isRemote()) {
        emit statusAvailable();
        emit ready();
    }

#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::FileSource(string) exiting" << std::endl;
#endif
}
#endif

FileSource::FileSource(QUrl url) :
    m_url(url),
    m_localFile(nullptr),
    m_reply(nullptr),
    m_ok(false),
    m_lastStatus(0),
    m_resource(false),
    m_remote(isRemote(url.toString())),
    m_done(false),
    m_leaveLocalFile(false),
    m_refCounted(false)
{
#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::FileSource(" << url.toString() << ") [as url]" << std::endl;
    incCount(m_url.toString());
#endif

    if (!canHandleScheme(m_url)) {
        std::cerr << "FileSource::FileSource: ERROR: Unsupported scheme in URL \"" << m_url.toString() << "\"" << std::endl;
        m_errorString = tr("Unsupported scheme in URL");
        return;
    }

    init();

#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::FileSource(url) exiting" << std::endl;
#endif
}

FileSource::FileSource(const FileSource &rf) :
    QObject(),
    m_url(rf.m_url),
    m_localFile(nullptr),
    m_reply(nullptr),
    m_ok(rf.m_ok),
    m_lastStatus(rf.m_lastStatus),
    m_resource(rf.m_resource),
    m_remote(rf.m_remote),
    m_done(false),
    m_leaveLocalFile(false),
    m_refCounted(false)
{
#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::FileSource(" << m_url.toString() << ") [copy ctor]" << std::endl;
    incCount(m_url.toString());
#endif

    if (!canHandleScheme(m_url)) {
        std::cerr << "FileSource::FileSource: ERROR: Unsupported scheme in URL \"" << m_url.toString() << "\"" << std::endl;
        m_errorString = tr("Unsupported scheme in URL");
        return;
    }

    if (!isRemote()) {
        m_localFilename = rf.m_localFilename;
    } else {
        QMutexLocker locker(&m_mapMutex);
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::FileSource(copy ctor): ref count is "
                  << m_refCountMap[m_url] << std::endl;
#endif
        if (m_refCountMap[m_url] > 0) {
            m_refCountMap[m_url]++;
#ifdef DEBUG_FILE_SOURCE
            std::cerr << "raised it to " << m_refCountMap[m_url] << std::endl;
#endif
            m_localFilename = m_remoteLocalMap[m_url];
            m_refCounted = true;
        } else {
            m_ok = false;
            m_lastStatus = 404;
        }
    }

    m_done = true;

#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::FileSource(" << m_url.toString() << ") [copy ctor]: note: local filename is \"" << m_localFilename << "\"" << std::endl;
#endif

#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::FileSource(copy ctor) exiting" << std::endl;
#endif
}

FileSource::~FileSource()
{
#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource(" << m_url.toString() << ")::~FileSource" << std::endl;
    decCount(m_url.toString());
#endif

    cleanup();

    if (isRemote() && !m_leaveLocalFile) deleteCacheFile();
}

void
FileSource::init()
{
    { // check we have a QNetworkAccessManager
        QMutexLocker locker(&m_mapMutex);
        if (!nms.hasLocalData()) {
            nms.setLocalData(new QNetworkAccessManager());
        }
    }

    if (isResource()) {
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::init: Is a resource" << std::endl;
#endif
        QString resourceFile = m_url.toString();
        resourceFile.replace(QRegExp("^qrc:"), ":");
        
        if (!QFileInfo(resourceFile).exists()) {
#ifdef DEBUG_FILE_SOURCE
            std::cerr << "FileSource::init: Resource file of this name does not exist, switching to non-resource URL" << std::endl;
#endif
            m_url = QUrl(resourceFile);
            m_resource = false;
        }
    }

    if (!isRemote() && !isResource()) {
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::init: Not a remote URL \"" << m_url.toString() << "\"" << std::endl;
#endif
        // If the file doesn't otherwise have a scheme set (eg. http://, ftp://), set
        // the scheme to file://
        if (m_url.scheme().isEmpty()) m_url.setScheme("file");

        bool literal = false;
        m_localFilename = m_url.toLocalFile();

        // The next code block doesn't work in Qt5, but when everything works
        // correctly, it is never entered.  I decided to leave the loose end
        // dangling for now.
        if (m_localFilename == "") {
            // QUrl may have mishandled the scheme (e.g. in a DOS path)
            m_localFilename = m_rawFileOrUrl;
#ifdef DEBUG_FILE_SOURCE
            std::cerr << "FileSource::init: Trying literal local filename \""
                      << m_localFilename << "\"" << std::endl;
#endif
            literal = true;
        }
        m_localFilename = QFileInfo(m_localFilename).absoluteFilePath();

#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::init: URL translates to local filename \""
                  << m_localFilename << "\" (with literal=" << literal << ")"
                  << std::endl;
#endif
        m_ok = true;
        m_lastStatus = 200;

        if (!QFileInfo(m_localFilename).exists()) {
            if (literal) {
                m_lastStatus = 404;
            } else {
#ifdef DEBUG_FILE_SOURCE
                std::cerr << "FileSource::init: Local file of this name does not exist, trying URL as a literal filename" << std::endl;
#endif
                // Again, QUrl may have been mistreating us --
                // e.g. dropping a part that looks like query data
                m_localFilename = m_rawFileOrUrl;
                literal = true;
                if (!QFileInfo(m_localFilename).exists()) {
                    m_lastStatus = 404;
                }
            }
        }

        m_done = true;
        return;
    }

    if (createCacheFile()) {
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::init: Already have this one" << std::endl;
#endif
        m_ok = true;
        if (!QFileInfo(m_localFilename).exists()) {
            m_lastStatus = 404;
        } else {
            m_lastStatus = 200;
        }
        m_done = true;
        return;
    }

    if (m_localFilename == "") return;

    m_localFile = new QFile(m_localFilename);
    m_localFile->open(QFile::WriteOnly);

    if (isResource()) {

        // Absent resource file case was dealt with at the top -- this
        // is the successful case

        QString resourceFileName = m_url.toString();
        resourceFileName.replace(QRegExp("^qrc:"), ":");
        QFile resourceFile(resourceFileName);
        resourceFile.open(QFile::ReadOnly);
        QByteArray ba(resourceFile.readAll());
        
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "Copying " << ba.size() << " bytes from resource file to cache file" << std::endl;
#endif

        qint64 written = m_localFile->write(ba);
        m_localFile->close();
        delete m_localFile;
        m_localFile = nullptr;

        if (written != ba.size()) {
#ifdef DEBUG_FILE_SOURCE
            std::cerr << "Copy failed (wrote " << written << " bytes)" << std::endl;
#endif
            m_ok = false;
            return;
        } else {
            m_ok = true;
            m_lastStatus = 200;
            m_done = true;
        }

    } else {

        QString scheme = m_url.scheme().toLower();

#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::init: Don't have local copy of \""
                  << m_url.toString() << "\", retrieving" << std::endl;
#endif

        if (scheme == "http" || scheme == "https" || scheme == "ftp") {
            initRemote();
#ifdef DEBUG_FILE_SOURCE
            std::cerr << "FileSource: initRemote returned" << std::endl;
#endif
        } else {
            m_remote = false;
            m_ok = false;
        }
    }

    if (m_ok) {
        
        QMutexLocker locker(&m_mapMutex);

        if (m_refCountMap[m_url] > 0) {
            // someone else has been doing the same thing at the same time,
            // but has got there first
            cleanup();
            m_refCountMap[m_url]++;
#ifdef DEBUG_FILE_SOURCE
            std::cerr << "FileSource::init: Another FileSource has got there first, abandoning our download and using theirs" << std::endl;
#endif
            m_localFilename = m_remoteLocalMap[m_url];
            m_refCounted = true;
            m_ok = true;
            if (!QFileInfo(m_localFilename).exists()) {
                m_lastStatus = 404;
            }
            m_done = true;
            return;
        }

        m_remoteLocalMap[m_url] = m_localFilename;
        m_refCountMap[m_url]++;
        m_refCounted = true;

//        if (m_progress && !m_done) {
//            m_progress->setLabelText
//                (tr("Downloading %1...").arg(m_url.toString()));
//            connect(m_progress, SIGNAL(canceled()), this, SLOT(cancelled()));
//            connect(this, SIGNAL(progress(int)),
//                    m_progress, SLOT(setValue(int)));
//        }
    }
}

void
FileSource::initRemote()
{
    m_ok = true;

    QNetworkRequest req;
    req.setUrl(m_url);
    
    if (m_preferredContentType != "") {
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource: indicating preferred content type of \""
                  << m_preferredContentType << "\"" << std::endl;
#endif
        req.setRawHeader
            ("Accept",
             QString("%1, */*").arg(m_preferredContentType).toLatin1());
    } else {
        // This is actually the default, however, this call appears to
        // prevent decompression of .rg files by Qt with some servers that
        // notice that an .rg file is really a .gz file and respond with
        // "Content-Encoding: gzip".  See the devel mailing list post by Tim
        // Munro 5/17/2015.
        req.setRawHeader("Accept-Encoding", "gzip, deflate");
    }

    m_reply = nms.localData()->get(req);

    connect(m_reply, &QIODevice::readyRead,
            this, &FileSource::readyRead);
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(replyFailed(QNetworkReply::NetworkError)));
    connect(m_reply, &QNetworkReply::finished,
            this, &FileSource::replyFinished);
    connect(m_reply, &QNetworkReply::metaDataChanged,
            this, &FileSource::metaDataChanged);
    connect(m_reply, &QNetworkReply::downloadProgress,
            this, &FileSource::downloadProgress);
}

void
FileSource::cleanup()
{
    if (m_done) {
        delete m_localFile; // does not actually delete the file
        m_localFile = nullptr;
    }
    m_done = true;
    if (m_reply) {
        QNetworkReply *r = m_reply;
        m_reply = nullptr;

        // Can only call abort() when there are no errors.
        if (r->error() == QNetworkReply::NoError) {
            r->abort();
        }

        r->deleteLater();
    }
    if (m_localFile) {
        delete m_localFile; // does not actually delete the file
        m_localFile = nullptr;
    }
}

bool
FileSource::isRemote(QString fileOrUrl)
{
    // Note that a "scheme" with length 1 is probably a DOS drive letter
    QString scheme = QUrl(fileOrUrl).scheme().toLower();
    if (scheme == "" || scheme == "file" || scheme.length() == 1) return false;
    return true;
}

bool
FileSource::canHandleScheme(QUrl url)
{
    // Note that a "scheme" with length 1 is probably a DOS drive letter
    QString scheme = url.scheme().toLower();
    return (scheme == "http" || scheme == "https" ||
            scheme == "ftp" || scheme == "file" || scheme == "qrc" ||
            scheme == "" || scheme.length() == 1);
}

bool
FileSource::isAvailable()
{
    waitForStatus();

    bool available = true;
    if (!m_ok) {
        available = false;
    } else {
        // http 2xx status codes mean success
        available = (m_lastStatus / 100 == 2);
    }

#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::isAvailable: m_ok: " << m_ok << std::endl;
    std::cerr << "FileSource::isAvailable: m_lastStatus: " << m_lastStatus << std::endl;
    std::cerr << "FileSource::isAvailable: " << (available ? "yes" : "no")
              << std::endl;
#endif

    return available;
}


// #########################################
// The nested event loops below are horribly fragile.
// They can lead to all sorts of bugs due to unexpected reentrancy
// (e.g. the user closes the window while in that while loop)
// There is no easy solution for synchronous networking operations
// in the main thread. Change this API to emit signals and connect to them, instead.
// i.e. make it asynchronous, just like KIO or QNetworkReply.
// #########################################


void
FileSource::waitForStatus()
{
    // ### BAD, see comment above
    while (m_ok && (!m_done && m_lastStatus == 0)) {
//        std::cerr << "waitForStatus: processing (last status " << m_lastStatus << ")" << std::endl;
        QCoreApplication::processEvents();
    }
}

void
FileSource::waitForData()
{
    // ### BAD, see comment above
    while (m_ok && !m_done) {
//        std::cerr << "FileSource::waitForData: calling QApplication::processEvents" << std::endl;
        QCoreApplication::processEvents();
        usleep(10000);
    }
}

void
FileSource::setLeaveLocalFile(bool leave)
{
    m_leaveLocalFile = leave;
}

bool
FileSource::isOK() const
{
    return m_ok;
}

bool
FileSource::isDone() const
{
    return m_done;
}

bool
FileSource::isResource() const
{
    return m_resource;
}

bool
FileSource::isRemote() const
{
    return m_remote;
}

QString
FileSource::getLocation() const
{
    return m_url.toString();
}

QString
FileSource::getLocalFilename() const
{
    return m_localFilename;
}

QString
FileSource::getBasename() const
{
    return QFileInfo(m_localFilename).fileName();
}

QString
FileSource::getContentType() const
{
    return m_contentType;
}

QString
FileSource::getExtension() const
{
    if (m_localFilename != "") {
        return QFileInfo(m_localFilename).suffix().toLower();
    } else {
        return QFileInfo(m_url.toLocalFile()).suffix().toLower();
    }
}

QString
FileSource::getErrorString() const
{
    return m_errorString;
}

void
FileSource::readyRead()
{
    m_localFile->write(m_reply->readAll());
}

void
FileSource::metaDataChanged()
{
#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::metaDataChanged" << std::endl;
#endif

    if (!m_reply) {
        std::cerr << "WARNING: FileSource::metaDataChanged() called without a reply object being known to us" << std::endl;
        return;
    }

    // Handle http transfer status codes.

    int status =
        m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // If this is a redirection (3xx) code, do the redirect
    if (status / 100 == 3) {
        QString location = m_reply->header
            (QNetworkRequest::LocationHeader).toString();
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::metaDataChanged: redirect to \""
                  << location << "\" received" << std::endl;
#endif
        if (location != "") {
            QUrl newUrl(location);
            if (newUrl != m_url) {
                cleanup();
                deleteCacheFile();
#ifdef DEBUG_FILE_SOURCE
                decCount(m_url.toString());
                incCount(newUrl.toString());
#endif
                m_url = newUrl;
                m_localFile = nullptr;
                m_lastStatus = 0;
                m_done = false;
                m_refCounted = false;
                init();
                return;
            }
        }
    }

    m_lastStatus = status;

    // 400 and up are failures, get the error string
    if (m_lastStatus / 100 >= 4) {
        m_errorString = QString("%1 %2")
            .arg(status)
            .arg(m_reply->attribute
                 (QNetworkRequest::HttpReasonPhraseAttribute).toString());
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::metaDataChanged: "
                  << m_errorString << std::endl;
#endif
    } else {
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::metaDataChanged: status: "
                  << m_lastStatus << std::endl;
#endif
        m_contentType =
            m_reply->header(QNetworkRequest::ContentTypeHeader).toString();
    }

    emit statusAvailable();
}

void
FileSource::downloadProgress(qint64 done, qint64 total)
{
    int percent = int((double(done) / double(total)) * 100.0 - 0.1);
    emit progress(percent);
}

void
FileSource::cancelled()
{
    m_done = true;
    cleanup();

    m_ok = false;
    m_errorString = tr("Download cancelled");
}

void
FileSource::replyFinished()
{
    emit progress(100);

#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::replyFinished()" << std::endl;
#endif

    if (m_done) return;

    QString scheme = m_url.scheme().toLower();
    // For ftp transfers, replyFinished() will be called on success.
    // metaDataChanged() is never called for ftp transfers.
    if (scheme == "ftp")
        m_lastStatus = 200;  // http ok

    bool error = (m_lastStatus / 100 >= 4);

    cleanup();

    if (!error) {
        QFileInfo fi(m_localFilename);
        if (!fi.exists()) {
            m_errorString = tr("Failed to create local file %1").arg(m_localFilename);
            error = true;
        } else if (fi.size() == 0) {
            m_errorString = tr("File contains no data!");
            error = true;
        }
    }

    if (error) {
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::done: error is " << error << ", deleting cache file" << std::endl;
#endif
        deleteCacheFile();
    }

    m_ok = !error;
    if (m_localFile) m_localFile->flush();
    m_done = true;
    emit ready();
}

void
FileSource::replyFailed(QNetworkReply::NetworkError networkError)
{
#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::replyFailed(" << networkError << ")" << std::endl;
#else
    (void)networkError;  // Suppress compiler warning
#endif

    emit progress(100);
    if (!m_reply) {
        std::cerr << "WARNING: FileSource::replyFailed() called without a reply object being known to us" << std::endl;
    } else {
        m_errorString = m_reply->errorString();
    }
    m_ok = false;
    m_done = true;
    cleanup();
    emit ready();
}

void
FileSource::deleteCacheFile()
{
#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::deleteCacheFile(\"" << m_localFilename << "\")" << std::endl;
#endif

    cleanup();

    if (m_localFilename == "") {
        return;
    }

    if (!isRemote()) {
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "not a cache file" << std::endl;
#endif
        return;
    }

    if (m_refCounted) {

        QMutexLocker locker(&m_mapMutex);
        m_refCounted = false;

        if (m_refCountMap[m_url] > 0) {
            m_refCountMap[m_url]--;
#ifdef DEBUG_FILE_SOURCE
            std::cerr << "reduced ref count to " << m_refCountMap[m_url] << std::endl;
#endif
            if (m_refCountMap[m_url] > 0) {
                m_done = true;
                return;
            }
        }
    }

    m_fileCreationMutex.lock();

    if (!QFile(m_localFilename).remove()) {
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::deleteCacheFile: ERROR: Failed to delete file \"" << m_localFilename << "\"" << std::endl;
#endif
    } else {
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::deleteCacheFile: Deleted cache file \"" << m_localFilename << "\"" << std::endl;
#endif
        m_localFilename = "";
    }

    m_fileCreationMutex.unlock();

    m_done = true;
}

bool
FileSource::createCacheFile()
{
    {
        QMutexLocker locker(&m_mapMutex);

#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::createCacheFile: refcount is " << m_refCountMap[m_url] << std::endl;
#endif

        if (m_refCountMap[m_url] > 0) {
            m_refCountMap[m_url]++;
            m_localFilename = m_remoteLocalMap[m_url];
#ifdef DEBUG_FILE_SOURCE
            std::cerr << "raised it to " << m_refCountMap[m_url] << std::endl;
#endif
            m_refCounted = true;
            return true;
        }
    }

    QDir dir;
    try {
        dir = TempDirectory::getInstance()->getSubDirectoryPath("download");
    } catch (const DirectoryCreationFailed &f) {
#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::createCacheFile: ERROR: Failed to create temporary directory: " << f.what() << std::endl;
#endif
        m_localFilename = "";
        return false;
    }

    QString filepart = m_url.path().section('/', -1, -1,
                                            QString::SectionSkipEmpty);

    QString extension = "";
    if (filepart.contains('.')) extension = filepart.section('.', -1);

    QString base = filepart;
    if (extension != "") {
        base = base.left(base.length() - extension.length() - 1);
    }
    if (base == "") base = "remote";

    QString filename;

    if (extension == "") {
        filename = base;
    } else {
        filename = QString("%1.%2").arg(base).arg(extension);
    }

    QString filepath(dir.filePath(filename));

#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::createCacheFile: URL is \"" << m_url.toString() << "\", dir is \"" << dir.path() << "\", base \"" << base << "\", extension \"" << extension << "\", filebase \"" << filename << "\", filename \"" << filepath << "\"" << std::endl;
#endif

    QMutexLocker fcLocker(&m_fileCreationMutex);

    ++m_count;

    if (QFileInfo(filepath).exists() ||
        !QFile(filepath).open(QFile::WriteOnly)) {

#ifdef DEBUG_FILE_SOURCE
        std::cerr << "FileSource::createCacheFile: Failed to create local file \""
                  << filepath << "\" for URL \""
                  << m_url.toString() << "\" (or file already exists): appending suffix instead" << std::endl;
#endif

        if (extension == "") {
            filename = QString("%1_%2").arg(base).arg(m_count);
        } else {
            filename = QString("%1_%2.%3").arg(base).arg(m_count).arg(extension);
        }
        filepath = dir.filePath(filename);

        if (QFileInfo(filepath).exists() ||
            !QFile(filepath).open(QFile::WriteOnly)) {

#ifdef DEBUG_FILE_SOURCE
            std::cerr << "FileSource::createCacheFile: ERROR: Failed to create local file \""
                      << filepath << "\" for URL \""
                      << m_url.toString() << "\" (or file already exists)" << std::endl;
#endif

            m_localFilename = "";
            return false;
        }
    }

#ifdef DEBUG_FILE_SOURCE
    std::cerr << "FileSource::createCacheFile: url "
              << m_url.toString() << " -> local filename "
              << filepath << std::endl;
#endif
    
    m_localFilename = filepath;
    return false;
}

}


