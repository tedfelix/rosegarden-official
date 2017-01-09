/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "DummyDriver.h"

#include <QObject>

namespace Rosegarden
{

DummyDriver::DummyDriver(MappedStudio *studio) :
    SoundDriver(studio, std::string("DummyDriver - no sound"))
{
}

DummyDriver::DummyDriver(MappedStudio *studio, QString pastLog) :
    SoundDriver(studio, std::string("DummyDriver - no sound")),
    m_pastLog(pastLog)
{
}

QString
DummyDriver::getStatusLog()
{
    if (m_pastLog != "") {
	return QObject::tr("No sound driver available: Sound driver startup failed, log follows: \n\n%1").arg(m_pastLog);
    } else {
	return QObject::tr("No sound driver available: Application compiled without sound support?");
    }
}

}

