// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

/*
 * Code borrowed from KDE Konqueror : KonqMainWindowIface.h
 * Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2000 David Faure <faure@kde.org>
 */

#ifndef ROSEGARDENGUIIFACE_H
#define ROSEGARDENGUIIFACE_H

#include <dcopobject.h>
#include <qvaluelist.h>
#include <dcopref.h>

#include "rosegardendcop.h"
#include "MappedComposition.h"

class KDCOPActionProxy;
class KMainWindow;

/**
 * RosegardenGUI DCOP Interface
 */
class RosegardenIface : virtual public DCOPObject
{
    K_DCOP

public:
    RosegardenIface(KMainWindow*);
    void iFaceDelayedInit(KMainWindow*);

k_dcop:
    virtual void openFile(QString file)  = 0;
    virtual void openURL(QString url)    = 0;
    virtual void mergeFile(QString file) = 0;
    virtual void fileNew()               = 0;
    virtual void fileSave()              = 0;
    virtual void fileClose()             = 0;
    virtual void quit()                  = 0;

    virtual void play() = 0;
    virtual void stop() = 0;
    virtual void rewind() = 0;
    virtual void fastForward() = 0;
    virtual void record() = 0;
    virtual void rewindToBeginning() = 0;
    virtual void fastForwardToEnd() = 0;
    virtual void jumpToTime(int sec, int usec) = 0;
    virtual void startAtTime(int sec, int usec) = 0;

    // Sequencer updates GUI with status
    //
    virtual void notifySequencerStatus(int status) = 0;

    // Used to map unexpected (async) MIDI events to the user interface.
    // We can show these on the Transport or on a MIDI Mixer.
    //
    virtual void processAsynchronousMidi(const Rosegarden::MappedComposition &mC) = 0;

    // The sequencer tries to call this action until it can - then
    // we can go on and retrive device information
    //
    virtual void alive() = 0;

    // The sequencer requests that a new audio file is created - the
    // gui does so and returns the path of the new file so that the
    // sequencer can use it.
    //
    virtual QString createNewAudioFile() = 0;

    virtual void showError(QString error) = 0;

    // Actions proxy
    //
    DCOPRef action( const QCString &name );
    QCStringList actions();
    QMap<QCString,DCOPRef> actionMap();

protected:
    //--------------- Data members ---------------------------------

    KDCOPActionProxy *m_dcopActionProxy;

};

#endif // ROSEGARDENGUIIFACE_H
