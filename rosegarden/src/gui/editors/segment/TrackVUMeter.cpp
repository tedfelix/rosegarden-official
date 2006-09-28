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


#include "TrackVUMeter.h"

#include "gui/widgets/VUMeter.h"
#include <qfont.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

TrackVUMeter::TrackVUMeter(QWidget *parent,
                           VUMeterType type,
                           int width,
                           int height,
                           int position,
                           const char *name):
        VUMeter(parent, type, false, false, width, height, VUMeter::Horizontal, name),
        m_position(position), m_textHeight(12)
{
    setAlignment(AlignCenter);

    QFont font;
    font.setPointSize(font.pointSize() * 95 / 100);
    if (font.pointSize() > 14)
        font.setPointSize(14);
    font.setBold(false);
    setFont(font);
}

void
TrackVUMeter::meterStart()
{
    clear();
    setMinimumHeight(m_originalHeight);
    setMaximumHeight(m_originalHeight);
    m_active = true;
}

void
TrackVUMeter::meterStop()
{
    setMinimumHeight(m_textHeight);
    setMaximumHeight(m_textHeight);
    setText(QString("%1").arg(m_position + 1));
    if (m_active) {
        m_active = false;
        update();
    }
}

}
