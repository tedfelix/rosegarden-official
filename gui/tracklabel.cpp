/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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


#include "tracklabel.h"

#include <qtimer.h>
#include <klineeditdlg.h>

TrackLabel::TrackLabel(Rosegarden::TrackId id,
                       int position,
                       QWidget *parent,
                       const char *name, WFlags f):
    QLabel(parent, name, f),
    m_id(id),
    m_position(position),
    m_pressPosition(0, 0)
{
    m_pressTimer = new QTimer(this);

    connect(m_pressTimer, SIGNAL(timeout()),
            this, SLOT(slotChangeToInstrumentList()));
}

TrackLabel::~TrackLabel()
{
}

void
TrackLabel::setSelected(bool on)
{
    if (on)
    {
        m_selected = true;
        setBackgroundMode(PaletteBase);
    }
    else
    {
        m_selected = false;
        setBackgroundMode(PaletteBackground);
    }
}

void
TrackLabel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != LeftButton)
        return;

    // store the press coords for positioning the popup
    m_pressPosition = e->globalPos();

    // start a timer on this hold
    m_pressTimer->start(200, true); // 200ms, single shot
}

void
TrackLabel::mouseReleaseEvent(QMouseEvent *e)
{
    // stop the timer if running
    if (m_pressTimer->isActive())
        m_pressTimer->stop();

    if (e->button() != LeftButton)
        return;

    if (m_selected)
        setSelected(false);
    else
        setSelected(true);

    emit released(m_id);
}

void
TrackLabel::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() != LeftButton)
        return;

    // Highlight this label alone and cheat using
    // the released signal
    //
    emit released(m_id);

    // Just in case we've got our timing wrong - reapply
    // this label highlight
    //
    setSelected(true);

    bool ok = false;

    QString newText = KLineEditDlg::getText(QString("Change track name"),
                                            QString("Enter new track name"),
                                            text(),
                                            &ok,
                                            this);

    if ( ok && !newText.isEmpty() )
        emit renameTrack(newText, m_position);
}

void
TrackLabel::slotChangeToInstrumentList()
{
    emit changeToInstrumentList(m_id);
}
