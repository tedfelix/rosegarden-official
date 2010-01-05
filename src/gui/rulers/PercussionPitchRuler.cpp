/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include <QMouseEvent>
#include "PercussionPitchRuler.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/MidiProgram.h"
#include "gui/editors/matrix/MatrixView.h"
#include "gui/general/MidiPitchLabel.h"
#include "PitchRuler.h"
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QSize>
#include <QWidget>


namespace Rosegarden
{

PercussionPitchRuler::PercussionPitchRuler(QWidget *parent,
        const MidiKeyMapping *mapping,
        int lineSpacing) :
        PitchRuler(parent),
        m_mapping(mapping),
        m_lineSpacing(lineSpacing),
        m_mouseDown(false),
        m_hoverNotePitch(-1),
        m_lastHoverHighlight( -1)
{
    m_font = new QFont();
    m_font->setPixelSize(9);
    m_fontMetrics = new QFontMetrics(*m_font);
    m_width = m_fontMetrics->width("  A#2   Acoustic Bass Drum  ");

    setPaletteBackgroundColor(QColor(238, 238, 224));

    setMouseTracking(true);
}

QSize PercussionPitchRuler::sizeHint() const
{
    return QSize(m_width,
                 (m_lineSpacing + 1) * m_mapping->getPitchExtent());
}

QSize PercussionPitchRuler::minimumSizeHint() const
{
    return QSize(m_width, m_lineSpacing + 1);
}

void PercussionPitchRuler::paintEvent(QPaintEvent*)
{
    QPainter paint(this);

    paint.setFont(*m_font);

    int minPitch = m_mapping->getPitchForOffset(0);
    int extent = m_mapping->getPitchExtent();

    // Draw the ruler

    for (int i = 0; i < extent; ++i) {
        paint.drawLine(0, i * (m_lineSpacing + 1),
                       m_width, i * (m_lineSpacing + 1));
    }

    int lw = m_fontMetrics->width("A#2");

    for (int i = 0; i < extent; ++i) {

        MidiPitchLabel label(minPitch + i);
        std::string key = m_mapping->getMapForKeyName(minPitch + i);

        RG_DEBUG << i << ": " << label.getQString() << ": " << key << endl;

        paint.drawText
        (2, (extent - i - 1) * (m_lineSpacing + 1) +
         m_fontMetrics->ascent() + 1,
         label.getQString());

        paint.drawText
        (9 + lw, (extent - i - 1) * (m_lineSpacing + 1) +
         m_fontMetrics->ascent() + 1,
         strtoqstr(key));
    }

    // Draw hover note
    if (m_lastHoverHighlight != m_hoverNotePitch) {

        // Unhighlight the last hover note
        if (m_lastHoverHighlight >= 0) {

            int y = (extent - (m_lastHoverHighlight - minPitch) - 1) * (m_lineSpacing + 1);
            paint.setBrush(QColor(238, 238, 224));
            paint.setPen(QColor(238, 238, 224));
            paint.drawRect(lw + 7, y + 1, m_width - lw, m_lineSpacing);
            std::string key = m_mapping->getMapForKeyName(m_lastHoverHighlight);
            paint.setPen(QColor(Qt::black));
            paint.drawText
            (9 + lw, y + m_fontMetrics->ascent() + 1,
             strtoqstr(key));
        }

        // Highlight the new hover note
        int y = (extent - (m_hoverNotePitch - minPitch) - 1) * (m_lineSpacing + 1);
        m_lastHoverHighlight = m_hoverNotePitch;
        paint.setBrush(paint.pen().color());
        paint.drawRect(lw + 7, y, m_width - lw, m_lineSpacing + 1);
        paint.setPen(QColor(238, 238, 224));

        std::string key = m_mapping->getMapForKeyName(m_hoverNotePitch);
        paint.drawText
        (9 + lw, y + m_fontMetrics->ascent() + 1,
         strtoqstr(key));
    }

}

void PercussionPitchRuler::enterEvent(QEvent *)
{}

void PercussionPitchRuler::leaveEvent(QEvent*)
{
    //    m_hoverHighlight->hide();
}

void PercussionPitchRuler::drawHoverNote(int evPitch)
{
    m_hoverNotePitch = evPitch;
    update();
}

void PercussionPitchRuler::mouseMoveEvent(QMouseEvent* e)
{
    if (m_mouseDown)
        if (m_selecting)
            emit keySelected(e->y(), true);
        else
            emit keyPressed(e->y(), true); // we're swooshing
    else
        emit hoveredOverKeyChanged(e->y());
}

void PercussionPitchRuler::mousePressEvent(QMouseEvent *e)
{
    Qt::ButtonState bs = e->state();

	if (e->button() == Qt::LeftButton) {

        m_mouseDown = true;
        m_selecting = (bs & Qt::ShiftModifier);

        if (m_selecting)
            emit keySelected(e->y(), false);
        else
            emit keyPressed(e->y(), false);
    }
}

void PercussionPitchRuler::mouseReleaseEvent(QMouseEvent *e)
{
	if (e->button() == Qt::LeftButton) {
        m_mouseDown = false;
        m_selecting = false;
    }
}

}
#include "PercussionPitchRuler.moc"
