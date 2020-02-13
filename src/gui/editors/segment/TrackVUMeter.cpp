/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
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

#include <QFont>
#include <QString>
#include <QWidget>

#include <iostream>

namespace Rosegarden
{

TrackVUMeter::TrackVUMeter(QWidget *parent,
                           VUMeterType type,
                           int width,
                           int height,
                           int position) :
        VUMeter(parent, type, false, false, width, height, VUMeter::Horizontal),
        m_position(position), m_textHeight(height)
{
    setAlignment(Qt::AlignCenter);

    QFont font;
    // If you are considering adjusting this size, it might be better to
    // adjust the height that is coming in.  This 13/10 ratio makes the
    // font fit perfectly in the bar.
    font.setPixelSize(height * 13 / 10);
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
