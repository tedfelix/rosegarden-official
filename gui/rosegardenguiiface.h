// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

    // Sequener gets slice of Events in a MappedComposition
    virtual const Rosegarden::MappedComposition&
            getSequencerSlice(const Rosegarden::timeT &sliceStart,
                              const Rosegarden::timeT &sliceEnd) = 0;

    // Sequencer updates GUI pointer position
    virtual void setPointerPosition(const int &position) = 0;

    // Sequencer updates GUI with status
    virtual void notifySequencerStatus(const int &status) = 0;

    // Actions proxy
    DCOPRef action( const QCString &name );
    QCStringList actions();
    QMap<QCString,DCOPRef> actionMap();

protected:
    KDCOPActionProxy *m_dcopActionProxy;

};

#endif // ROSEGARDENGUIIFACE_H
