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

#ifndef STARTUP_TESTER_H
#define STARTUP_TESTER_H

#include <qthread.h>
#include <qmutex.h>

// Class for miscellaneous and potentially slow tests that can be
// backgrounded on startup.  This is very specific to the RG startup
// requirements.

class StartupTester : public QThread
{
public:
    StartupTester();
    
    virtual void run();

    bool isReady();
    
    // If you call one of these methods before the startup test has
    // completed in the background, then it will block.
    
    bool haveProjectPackager();
    bool haveLilypondView();
    
protected:
    bool m_ready;
    QMutex m_projectPackagerMutex;
    QMutex m_lilypondViewMutex;
    bool m_haveProjectPackager;
    bool m_haveLilypondView;
};

#endif
