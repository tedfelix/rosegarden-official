// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

k_dcop:
    virtual void openFile(const QString &file) = 0;
    virtual void importRG21File(const QString &file) = 0;
    virtual void importMIDIFile(const QString &file) = 0;
    virtual void fileNew()                       = 0;
    virtual void fileSave()                      = 0;
    virtual void fileClose()                     = 0;
    virtual void quit()                          = 0;

    // Sequencer gets slice of MappedEvents wrapped in a
    // MappedComposition.  These are lightweight versions
    // of what we have in Event and Composition.  
    //
    virtual const Rosegarden::MappedComposition&
            getSequencerSlice(long sliceStartSec, long sliceStartUsec,
                              long sliceEndSec, long sliceEndUsec) = 0;

    // The Sequencer sends back MappedCompositions full of
    // newly recorded MappedEvents for storage and presentation
    // by the GUI
    //
    virtual void processRecordedMidi(const Rosegarden::MappedComposition &mC)=0;

    // Used to map unexpected (async) MIDI events to the user interface.
    // We can show these on the Transport or on a MIDI Mixer.
    //
    virtual void processAsynchronousMidi(const Rosegarden::MappedComposition &mC)=0;

    // Sequencer updates GUI pointer position
    //
    virtual void setPointerPosition(const long &realTimeSec,
                                    const long &realTimeUsec) = 0;

    // Sequencer updates GUI with status
    //
    virtual void notifySequencerStatus(const int &status) = 0;

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
