// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "startuptester.h"
#include "rosedebug.h"

#include <kprocess.h>

StartupTester::StartupTester() :
    m_ready(false),
    m_haveProjectPackager(false),
    m_haveLilypondView(false)
{
}

void
StartupTester::run()
{
    m_projectPackagerMutex.lock();
    m_lilypondViewMutex.lock();
    m_ready = true;
    
    KProcess *proc = new KProcess;
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
    while (!m_ready) usleep(10000);
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
    while (!m_ready) usleep(10000);
    QMutexLocker locker(&m_projectPackagerMutex);
    return m_haveProjectPackager;
}

bool
StartupTester::haveLilypondView()
{
    while (!m_ready) usleep(10000);
    QMutexLocker locker(&m_lilypondViewMutex);
    return m_haveLilypondView;
}

