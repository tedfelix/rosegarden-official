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
#include <kprocess.h>
#include <qmutex.h>
#include <qthread.h>


namespace Rosegarden
{

StartupTester::StartupTester() :
    m_ready(false),
    m_haveProjectPackager(false),
    m_haveLilypondView(false),
    m_haveAudioFileImporter(false)
{}

void
StartupTester::run()
{
    m_projectPackagerMutex.lock();
    m_lilypondViewMutex.lock();
    m_audioFileImporterMutex.lock();
    m_ready = true;

    KProcess *proc = new KProcess();
    *proc << "rosegarden-audiofile-importer";
    *proc << "-t";
    proc->start(KProcess::Block, KProcess::All);
    if (!proc->normalExit() || proc->exitStatus()) {
        RG_DEBUG << "StartupTester - No audio file importer available" << endl;
        m_haveAudioFileImporter = false;
    } else {
        RG_DEBUG << "StartupTester - Audio file importer OK" << endl;
        m_haveAudioFileImporter = true;
    }
    delete proc;
    m_audioFileImporterMutex.unlock();

    proc = new KProcess;
    *proc << "rosegarden-project-package";
    *proc << "--conftest";
    proc->start(KProcess::Block, KProcess::All);
    if (!proc->normalExit() || proc->exitStatus()) {
        RG_DEBUG << "StartupTester - No project packager available" << endl;
        m_haveProjectPackager = false;
    } else {
        RG_DEBUG << "StartupTester - Project packager OK" << endl;
        m_haveProjectPackager = true;
    }
    delete proc;
    m_projectPackagerMutex.unlock();

    proc = new KProcess();
    *proc << "rosegarden-lilypondview";
    *proc << "--conftest";
    proc->start(KProcess::Block, KProcess::All);
    if (!proc->normalExit() || proc->exitStatus()) {
        RG_DEBUG << "StartupTester - No lilypondview available" << endl;
        m_haveLilypondView = false;
    } else {
        RG_DEBUG << "StartupTester - Lilypondview OK" << endl;
        m_haveLilypondView = true;
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

bool
StartupTester::haveProjectPackager()
{
    while (!m_ready)
        usleep(10000);
    QMutexLocker locker(&m_projectPackagerMutex);
    return m_haveProjectPackager;
}

bool
StartupTester::haveLilypondView()
{
    while (!m_ready)
        usleep(10000);
    QMutexLocker locker(&m_lilypondViewMutex);
    return m_haveLilypondView;
}

bool
StartupTester::haveAudioFileImporter()
{
    while (!m_ready)
        usleep(10000);
    QMutexLocker locker(&m_audioFileImporterMutex);
    return m_haveAudioFileImporter;
}

}
