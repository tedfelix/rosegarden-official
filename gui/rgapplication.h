// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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


#ifndef _RGAPPLICATION_H_
#define _RGAPPLICATION_H_

#include <kuniqueapplication.h>

/**
 * RosegardenApplication
 *
 * Handles RosegardenGUIApps perceived uniqueness for us.
 *
 */
class RosegardenApplication : public KUniqueApplication
{
public:
    RosegardenApplication(): KUniqueApplication(), m_noSequencerMode(false) {}

    /**
     * Handle the attempt at creation of a new instance - 
     * only accept new file names which we attempt to load
     * into the existing instance (if it exists)
     */
    virtual int newInstance();

    void commitData(QSessionManager& sm)
    {
        KApplication::commitData( sm );
    }

    bool isSequencerRegistered();
    void sequencerSend(QCString dcopCall, QByteArray params = Empty);
    void sequencerCall(QCString dcopCall, QCString& replyType, QByteArray& replyData, QByteArray params = Empty, bool useEventLoop = false);

    // same as above, but just return false instead of throwing
    bool trySequencerSend(QCString dcopCall, QByteArray params = Empty);
    bool trySequencerCall(QCString dcopCall, QCString& replyType, QByteArray& replyData, QByteArray params = Empty, bool useEventLoop = false);

    static RosegardenApplication* rgApp();

    static QByteArray Empty;

    void setNoSequencerMode(bool m=true) { m_noSequencerMode = m; }
    bool noSequencerMode() { return m_noSequencerMode; }

protected:
    //--------------- Data members ---------------------------------
    
    bool m_noSequencerMode;
};

#define rgapp RosegardenApplication::rgApp()

#endif
