// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#ifndef PROGRESSREPORTER_H
#define PROGRESSREPORTER_H

#include <qobject.h>
#include "Exception.h"

class ProgressReporter : public QObject
{
    Q_OBJECT
public:
    ProgressReporter(QObject* parent, const char* name = 0)
        : QObject(parent, name), m_isCancelled(false) {}

    // exception class for cancellations
    class Cancelled { };

protected:
    /*
      We have to use these accessors rather than throwing directly
      from slotCancel() because Qt is generally compiled without
      exception support, so we can't throw from a slot.
    */
    bool isCancelled() const { return m_isCancelled; }
    void resetCancelledState() { m_isCancelled = false; }

protected slots:
    virtual void slotCancel() { m_isCancelled = true; };

signals:
    /// Report progress
    void setProgress(int);
    void incrementProgress(int);

protected:
    //--------------- Data members ---------------------------------
    bool m_isCancelled;
};


#endif
