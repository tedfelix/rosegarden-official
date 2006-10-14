
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _RG_ROSEGARDENAPPLICATION_H_
#define _RG_ROSEGARDENAPPLICATION_H_

#include <kuniqueapplication.h>
#include <qcstring.h>
#include <qstring.h>
#include <qobject.h>


class QSessionManager;
class KProcess;


namespace Rosegarden
{



/**
 * RosegardenApplication
 *
 * Handles RosegardenGUIApps perceived uniqueness for us.
 *
 */
class RosegardenApplication : public KUniqueApplication
{
    Q_OBJECT
public:
    RosegardenApplication(): KUniqueApplication(), m_noSequencerMode(false) {}

    /**
     * Handle the attempt at creation of a new instance - 
     * only accept new file names which we attempt to load
     * into the existing instance (if it exists)
     */
    virtual int newInstance();

    void refreshGUI(int maxTime);

    bool isSequencerRegistered();
    bool sequencerSend(QCString dcopCall, QByteArray params = Empty);
    bool sequencerCall(QCString dcopCall, QCString& replyType,
                       QByteArray& replyData, QByteArray params = Empty, bool useEventLoop = false);

    static RosegardenApplication* rgApp();

    static QByteArray Empty;

    void setNoSequencerMode(bool m=true) { m_noSequencerMode = m; }
    bool noSequencerMode() { return m_noSequencerMode; }

    virtual void saveState(QSessionManager&);
    
signals:
    // connect this to RosegardenGUIApp
    void aboutToSaveState();
    
public slots:
    void sfxLoadExited(KProcess *proc);
    void slotSetStatusMessage(QString txt);

protected:
    //--------------- Data members ---------------------------------
    
    bool m_noSequencerMode;
};

#define rgapp RosegardenApplication::rgApp()


}

#endif
