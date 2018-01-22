/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#define RG_MODULE_STRING "[StartupTester]"

#include "StartupTester.h"

#include "misc/Strings.h"
#include "misc/Debug.h"
#include "gui/dialogs/LilyPondOptionsDialog.h"
#include "gui/editors/notation/NoteFontFactory.h"

#include "rosegarden-version.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QMutex>
#include <QThread>
#include <QRegExp>


namespace Rosegarden
{

StartupTester::StartupTester() :
//    m_proc(NULL),
    m_ready(false),
    m_haveAudioFileImporter(false)
//    m_versionHttpFailed(false)
{
    QUrl url;
    url.setScheme("http");
    url.setHost("www.rosegardenmusic.com");
    url.setPath("/latest-version.txt");

    network = new QNetworkAccessManager(this);
    network->get(QNetworkRequest(url));
    RG_DEBUG << "StartupTester::StartupTester(): URL: " << url.toString();

    connect(network, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(slotNetworkFinished(QNetworkReply*)));
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

#if 0
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
#endif

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
StartupTester::slotNetworkFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    network->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
//        m_versionHttpFailed = true;
        RG_WARNING << "StartupTester::slotNetworkFinished(): Connection failed: " << reply->errorString();
        return;
    }

    QByteArray responseData = reply->readAll();
    QString str = QString::fromUtf8(responseData.data());
    QStringList lines = str.split('\n', QString::SkipEmptyParts);
    if (lines.empty()) return;

    QString latestVersion = lines[0];
    RG_DEBUG << "StartupTester::slotNetworkFinished(): Comparing current version \"" << VERSION
              << "\" with latest version \"" << latestVersion << "\"";
    if (isVersionNewerThan(latestVersion, VERSION)) {
        emit newerVersionAvailable(latestVersion);
    }
}

}


