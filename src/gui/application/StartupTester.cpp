/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2014 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "StartupTester.h"

#include "misc/Strings.h"
#include "misc/Debug.h"
#include "gui/dialogs/LilyPondOptionsDialog.h"
#include "gui/editors/notation/NoteFontFactory.h"

#include <QProcess>
#include <QMutex>
#include <QThread>
#include <QRegExp>


namespace Rosegarden
{

StartupTester::StartupTester() :
    m_ready(false),
    m_haveAudioFileImporter(false)
{
    QHttp *http = new QHttp();
    connect(http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
            this, SLOT(slotHttpResponseHeaderReceived(const QHttpResponseHeader &)));
    connect(http, SIGNAL(done(bool)),
            this, SLOT(slotHttpDone(bool)));
    m_versionHttpFailed = false;
    http->setHost("www.rosegardenmusic.com");
    http->get("/latest-version.txt");
}

StartupTester::~StartupTester()
{
}

void
StartupTester::run() {
    m_runningMutex.lock();
    m_ready = true;

    m_haveAudioFileImporter = true;
    NoteFontFactory::getFontNames(true);

    // unlock this as the very last thing we do in this thread,
    // so the parent process knows the thread is completed
    m_runningMutex.unlock();
}

bool
StartupTester::isReady()
{
    while (!m_ready) usleep(10000);
    if (m_runningMutex.tryLock()) {
        m_runningMutex.unlock();
    } else {
        return false;
    }
    return true;
}

void
StartupTester::stdoutReceived()
{
    m_stdoutBuffer.append(m_proc->readAllStandardOutput());
}

void
StartupTester::parseStdoutBuffer(QStringList &target)
{
    QRegExp re("Required: ([^\n]*)");
    if (re.indexIn(m_stdoutBuffer) != -1) {
        target = re.cap(1).split(", ");
    }
}

bool
StartupTester::haveAudioFileImporter(QStringList *missing)
{
    while (!m_ready)
        usleep(10000);
    QMutexLocker locker(&m_audioFileImporterMutex);
    if (missing) *missing = m_audioFileImporterMissing;
    return m_haveAudioFileImporter;
}

bool
StartupTester::isVersionNewerThan(QString a, QString b)
{
    QRegExp re("[._-]");
    QStringList alist = a.split(re, QString::SkipEmptyParts);
    QStringList blist = b.split(re, QString::SkipEmptyParts);
    int ae = alist.size();
    int be = blist.size();
    int e = std::max(ae, be);
    for (int i = 0; i < e; ++i) {
    int an = 0, bn = 0;
    if (i < ae) {
        an = alist[i].toInt();
        if (an == 0) an = -1; // non-numeric field -> "-pre1" etc
    }
    if (i < be) {
        bn = blist[i].toInt();
        if (bn == 0) bn = -1;
    }
    if (an < bn) return false;
    if (an > bn) return true;
    }
    return false;
}

void
StartupTester::slotHttpResponseHeaderReceived(const QHttpResponseHeader &h)
{
    if (h.statusCode() / 100 != 2) m_versionHttpFailed = true;
}

void
StartupTester::slotHttpDone(bool error)
{
    QHttp *http = const_cast<QHttp *>(dynamic_cast<const QHttp *>(sender()));
    if (!http) return;
    http->deleteLater();
    if (error) return;

    QByteArray responseData = http->readAll();
    QString str = QString::fromUtf8(responseData.data());
    QStringList lines = str.split('\n', QString::SkipEmptyParts);
    if (lines.empty()) return;

    QString latestVersion = lines[0];
    std::cerr << "Comparing current version \"" << VERSION
              << "\" with latest version \"" << latestVersion << "\""
              << std::endl;
    if (isVersionNewerThan(latestVersion, VERSION)) {
        emit newerVersionAvailable(latestVersion);
    }
}

}

#include "StartupTester.moc"

