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


#include "PitchDragLabel.h"

#include "base/NotationTypes.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include <qpainter.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qwidget.h>


namespace Rosegarden
{

PitchDragLabel::PitchDragLabel(QWidget *parent,
                               int defaultPitch) :
        QWidget(parent),
        m_pitch(defaultPitch),
        m_clickedY(0),
        m_clicked(false),
        m_npf(new NotePixmapFactory())
{
    calculatePixmap(true);
}

PitchDragLabel::~PitchDragLabel()
{
    delete m_npf;
}

void
PitchDragLabel::slotSetPitch(int p)
{
    bool up = (p > m_pitch);
    if (m_pitch == p)
        return ;
    m_pitch = p;
    calculatePixmap(up);
    emit pitchChanged(m_pitch);
    paintEvent(0);
}

void
PitchDragLabel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == LeftButton) {
        m_clickedY = e->y();
        m_clickedPitch = m_pitch;
        m_clicked = true;
        emit preview(m_pitch);
    }
}

void
PitchDragLabel::mouseMoveEvent(QMouseEvent *e)
{
    if (m_clicked) {

        int y = e->y();
        int diff = y - m_clickedY;
        int pitchDiff = diff * 4 / m_npf->getLineSpacing();

        int newPitch = m_clickedPitch - pitchDiff;
        if (newPitch < 0)
            newPitch = 0;
        if (newPitch > 127)
            newPitch = 127;

        if (m_pitch != newPitch) {
            bool up = (newPitch > m_pitch);
            m_pitch = newPitch;
            calculatePixmap(up);
            emit pitchDragged(m_pitch);
            emit preview(m_pitch);
            paintEvent(0);
        }
    }
}

void
PitchDragLabel::mouseReleaseEvent(QMouseEvent *e)
{
    mouseMoveEvent(e);
    emit pitchChanged(m_pitch);
    m_clicked = false;
}

void
PitchDragLabel::wheelEvent(QWheelEvent *e)
{
    if (e->delta() > 0) {
        if (m_pitch < 127) {
            ++m_pitch;
            calculatePixmap(true);
            emit pitchChanged(m_pitch);
            emit preview(m_pitch);
            paintEvent(0);
        }
    } else {
        if (m_pitch > 0) {
            --m_pitch;
            calculatePixmap(false);
            emit pitchChanged(m_pitch);
            emit preview(m_pitch);
            paintEvent(0);
        }
    }
}

void
PitchDragLabel::paintEvent(QPaintEvent *)
{
    QPainter paint(this);
    paint.fillRect(0, 0, width(), height(), paint.backgroundColor());

    int x = width() / 2 - m_pixmap.width() / 2;
    if (x < 0)
        x = 0;

    int y = height() / 2 - m_pixmap.height() / 2;
    if (y < 0)
        y = 0;

    paint.drawPixmap(x, y, m_pixmap);


}

QSize
PitchDragLabel::sizeHint() const
{
    return QSize(150, 135);
}

void
PitchDragLabel::calculatePixmap(bool useSharps) const
{
    std::string clefType = Clef::Treble;
    int octaveOffset = 0;

    if (m_pitch > 94) {
        octaveOffset = 2;
    } else if (m_pitch > 82) {
        octaveOffset = 1;
    } else if (m_pitch < 60) {
        clefType = Clef::Bass;
        if (m_pitch < 24) {
            octaveOffset = -2;
        } else if (m_pitch < 36) {
            octaveOffset = -1;
        }
    }

    QCanvasPixmap *pmap = m_npf->makePitchDisplayPixmap
                          (m_pitch,
                           Clef(clefType, octaveOffset),
                           useSharps);

    m_pixmap = *pmap;

    delete pmap;
}

}
