
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PROGRESSREPORTER_H
#define RG_PROGRESSREPORTER_H

#include <QObject>

#include <rosegardenprivate_export.h>


namespace Rosegarden
{



class ROSEGARDENPRIVATE_EXPORT ProgressReporter : public QObject
{
    Q_OBJECT
public:
    ProgressReporter(QObject* parent);

    /*
      We have to use these accessors rather than throwing directly
      from slotCancel() because Qt is generally compiled without
      exception support, so we can't throw from a slot.
    */
    bool isOperationCancelled() const { return m_isCancelled; }

    // exception class for cancellations
    class Cancelled { };

protected:
    /**
     * Call this at appropriate times if you know Qt isn't in the stack
     */
    void throwIfCancelled();

//     void resetOperationCancelledState() { m_isCancelled = false; }

protected slots:
     virtual void slotCancel();

signals:
    /// Report value()
    void setValue(int);
//    void incrementProgress(int);
    void setOperationName(QString);

protected:
    //--------------- Data members ---------------------------------
    bool m_isCancelled;
};



}

#endif
