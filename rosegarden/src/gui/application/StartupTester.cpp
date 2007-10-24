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


#include "StartupTester.h"

#include "misc/Debug.h"
#include "gui/dialogs/LilypondOptionsDialog.h"

#include <kprocess.h>
#include <qmutex.h>
#include <qthread.h>
#include <qregexp.h>


namespace Rosegarden
{

StartupTester::StartupTester() :
    m_ready(false),
    m_haveProjectPackager(false),
    m_haveLilypondView(false),
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
StartupTester::run()
{
    m_projectPackagerMutex.lock();
    m_lilypondViewMutex.lock();
    m_audioFileImporterMutex.lock();
    m_ready = true;

    KProcess *proc = new KProcess();
    m_stdoutBuffer = "";
    QObject::connect(proc, SIGNAL(receivedStdout(KProcess *, char *, int)),
                     this, SLOT(stdoutReceived(KProcess *, char *, int)));
    *proc << "rosegarden-audiofile-importer";
    *proc << "--conftest";
    proc->start(KProcess::Block, KProcess::All);
    if (!proc->normalExit() || proc->exitStatus()) {
        RG_DEBUG << "StartupTester - No audio file importer available" << endl;
        m_haveAudioFileImporter = false;
        parseStdoutBuffer(m_audioFileImporterMissing);
    } else {
        RG_DEBUG << "StartupTester - Audio file importer OK" << endl;
        m_haveAudioFileImporter = true;
    }
    delete proc;
    m_audioFileImporterMutex.unlock();

    proc = new KProcess;
    m_stdoutBuffer = "";
    QObject::connect(proc, SIGNAL(receivedStdout(KProcess *, char *, int)),
                     this, SLOT(stdoutReceived(KProcess *, char *, int)));
    *proc << "rosegarden-project-package";
    *proc << "--conftest";
    proc->start(KProcess::Block, KProcess::All);
    if (!proc->normalExit() || proc->exitStatus()) {
        m_haveProjectPackager = false;
        // rosegarden-project-package ran but exited with an error code
        RG_DEBUG << "StartupTester - No project packager available" << endl;
        m_haveProjectPackager = false;
        parseStdoutBuffer(m_projectPackagerMissing);
    } else {
        RG_DEBUG << "StartupTester - Project packager OK" << endl;
        m_haveProjectPackager = true;
    }
    delete proc;
    m_projectPackagerMutex.unlock();

    proc = new KProcess();
    m_stdoutBuffer = "";
    QObject::connect(proc, SIGNAL(receivedStdout(KProcess *, char *, int)),
                     this, SLOT(stdoutReceived(KProcess *, char *, int)));
    *proc << "rosegarden-lilypondview";
    *proc << "--conftest";
    proc->start(KProcess::Block, KProcess::All);
    if (!proc->normalExit() || proc->exitStatus()) {
        RG_DEBUG << "StartupTester - No lilypondview available" << endl;
        m_haveLilypondView = false;
        parseStdoutBuffer(m_lilypondViewMissing);
    } else {
        RG_DEBUG << "StartupTester - Lilypondview OK" << endl;
        m_haveLilypondView = true;
        QRegExp re("Lilypond version: ([^\n]*)");
        if (re.search(m_stdoutBuffer) != -1) {
            LilypondOptionsDialog::setDefaultLilypondVersion(re.cap(1));
        }
    }
    delete proc;
    m_lilypondViewMutex.unlock();
}

bool
StartupTester::isReady()
{
    while (!m_ready)
        usleep(10000);
    if (m_projectPackagerMutex.tryLock()) {
        m_projectPackagerMutex.unlock();
    } else {
        return false;
    }
    if (m_lilypondViewMutex.tryLock()) {
        m_lilypondViewMutex.unlock();
    } else {
        return false;
    }
    return true;
}

void
StartupTester::stdoutReceived(KProcess *, char *buffer, int len)
{
    m_stdoutBuffer += QString::fromLatin1(buffer, len);
}

void
StartupTester::parseStdoutBuffer(QStringList &target)
{
    QRegExp re("Required: ([^\n]*)");
    if (re.search(m_stdoutBuffer) != -1) {
        target = QStringList::split(", ", re.cap(1));
    }
}

bool
StartupTester::haveProjectPackager(QStringList *missing)
{
    while (!m_ready)
        usleep(10000);
    QMutexLocker locker(&m_projectPackagerMutex);
    if (missing) *missing = m_projectPackagerMissing;
    return m_haveProjectPackager;
}

bool
StartupTester::haveLilypondView(QStringList *missing)
{
    while (!m_ready)
        usleep(10000);
    QMutexLocker locker(&m_lilypondViewMutex);
    if (missing) *missing = m_lilypondViewMissing;
    return m_haveLilypondView;
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
    QStringList alist = QStringList::split(re, a);
    QStringList blist = QStringList::split(re, b);
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
    QStringList lines = QStringList::split('\n', str);
    if (lines.empty()) return;

    QString latestVersion = lines[0];
    RG_DEBUG << "Comparing current version \"" << VERSION
             << "\" with latest version \"" << latestVersion << "\""
             << endl;
    if (isVersionNewerThan(latestVersion, VERSION)) {
        emit newerVersionAvailable(latestVersion);
    }
}

}

#include "StartupTester.moc"

