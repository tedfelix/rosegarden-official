/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_DUMMYDRIVER_H
#define RG_DUMMYDRIVER_H

#include "SoundDriver.h"

#include <QObject>
#include <QString>

namespace Rosegarden
{


/// Allow Rosegarden to run without sound support.
class DummyDriver : public SoundDriver
{
public:
    DummyDriver(MappedStudio *studio, const QString &pastLog = "") :
        SoundDriver(studio, "DummyDriver - no sound"),
        m_pastLog(pastLog)
    {
    }

    QString getStatusLog() override
    {
        if (m_pastLog.isEmpty())
            return QObject::tr("No sound driver available: Application compiled without sound support?");

        return QObject::tr("No sound driver available: Sound driver startup failed, log follows: \n\n%1").arg(m_pastLog);
    }

protected:
    QString m_pastLog;
};


}

#endif // RG_DUMMYDRIVER_H

