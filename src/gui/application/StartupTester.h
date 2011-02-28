
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_STARTUPTESTER_H_
#define _RG_STARTUPTESTER_H_

#include <QMutex>
#include <QThread>
#include <QStringList>
#include <QObject>
#include <QHttp>

class QProcess;

namespace Rosegarden
{

class StartupTester : public QThread
{
    Q_OBJECT

public:
    StartupTester();
    virtual ~StartupTester();
    
    virtual void run();

    bool isReady();
    
    // If you call one of these methods before the startup test has
    // completed in the background, then it will block.
    bool haveAudioFileImporter(QStringList *missingApplications);

signals:
    void newerVersionAvailable(QString);

protected slots:
    void stdoutReceived();

    void slotHttpResponseHeaderReceived(const QHttpResponseHeader &);
    void slotHttpDone(bool);

protected:
    QProcess* m_proc;
    bool m_ready;
    QMutex m_audioFileImporterMutex;
    bool m_haveAudioFileImporter;
    QStringList m_audioFileImporterMissing;
    QMutex m_runningMutex;
    QByteArray m_stdoutBuffer;
    bool m_versionHttpFailed;
    void parseStdoutBuffer(QStringList &target);
    bool isVersionNewerThan(QString, QString);
};


}

#endif
