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

#include "instrumentlabel.h"


InstrumentLabel::InstrumentLabel(const QString & text,
                                 int position,
                                 QWidget *parent, const char *name):
    QLabel(text, parent, name),
    m_position(position), m_pressPosition(0, 0),
    m_alternativeLabel(""),
    m_selected(false)
{
    m_pressTimer = new QTimer();

    connect(m_pressTimer, SIGNAL(timeout()),
            this, SLOT(slotChangeToInstrumentList()));
}

InstrumentLabel::InstrumentLabel(int position,
                                 QWidget *parent, const char *name):
    QLabel(parent, name),
    m_position(position), m_pressPosition(0, 0)
{
    m_pressTimer = new QTimer();

    connect(m_pressTimer, SIGNAL(timeout()),
            this, SLOT(slotChangeToInstrumentList()));
}

InstrumentLabel::~InstrumentLabel()
{
}

void
InstrumentLabel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != LeftButton)
                return;

    // store the press coords for positioning the popup
    m_pressPosition = e->globalPos();

    // start a timer on this hold
    m_pressTimer->start(200, true); // 200ms, single shot

}

void
InstrumentLabel::mouseReleaseEvent(QMouseEvent *e)
{
    // stop the timer if running
    if (m_pressTimer->isActive())
        m_pressTimer->stop();

    // now send released signal to update selected track
    emit released(m_position);
}

void
InstrumentLabel::slotChangeToInstrumentList()
{
    emit changeToInstrumentList(m_position);
}

void
InstrumentLabel::setLabelHighlight(bool value)
{
    m_selected = value;

    if (value)
        setBackgroundMode(PaletteBase);
    else
        setBackgroundMode(PaletteBackground);
}


void
InstrumentLabel::slotSetAlternativeLabel(const QString &label)
{
    // recover saved original
    if (label == "" && m_alternativeLabel != "")
    {
        setText(m_alternativeLabel);
        return;
    }

    // do nothing if we've got nothing to swap
    if (label == "" && m_alternativeLabel == "")
        return;

    // Store the current (first) label 
    //
    if(m_alternativeLabel == "")
        m_alternativeLabel = text();

    // set new label
    setText(label);
}


