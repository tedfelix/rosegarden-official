// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
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

#include "trackvumeter.h"
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

TrackVUMeter::TrackVUMeter(QWidget *parent,
                           const VUMeterType &type,
                           const int &width,
                           const int &height,
                           const int &position,
                           const char *name):
    VUMeter(parent, type, width, height, name),
    m_position(position), m_textHeight(12)
{
    setAlignment(AlignCenter);
}


void
TrackVUMeter::meterStart()
{
    clear();
    setMinimumHeight(m_originalHeight);
    setMaximumHeight(m_originalHeight);
}


void
TrackVUMeter::meterStop()
{
    setMinimumHeight(m_textHeight);
    setMaximumHeight(m_textHeight);
    setText(QString("%1").arg(m_position + 1));
}

