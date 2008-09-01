
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

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
#include <QByteArray>
#include <QString>


class QSessionManager;
class QProcess;


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
/*!!!
    bool isSequencerRegistered();
    bool sequencerSend(QByteArray dcopCall, QByteArray params = Empty);
    bool sequencerCall(QByteArray dcopCall, QByteArray& replyType,
                       QByteArray& replyData, QByteArray params = Empty, bool useEventLoop = false);
*/

    static RosegardenApplication* rgApp();

    static QByteArray Empty;

    void setNoSequencerMode(bool m=true) { m_noSequencerMode = m; }
    bool noSequencerMode() { return m_noSequencerMode; }

    virtual void saveState(QSessionManager&);
    
signals:
    // connect this to RosegardenGUIApp
    void aboutToSaveState();
    
public slots:
    void sfxLoadExited(QProcess *proc);
    void slotSetStatusMessage(QString txt);

protected:
    //--------------- Data members ---------------------------------
    
    bool m_noSequencerMode;
};

#define rgapp RosegardenApplication::rgApp()


}

#endif
