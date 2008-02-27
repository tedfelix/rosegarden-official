/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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

#include "base/NotationRules.h"
#include "base/NotationTypes.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include <qcanvas.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qwidget.h>


namespace Rosegarden
{

PitchDragLabel::PitchDragLabel(QWidget *parent,
                               int defaultPitch,
                               bool defaultSharps) :
        QWidget(parent),
        m_pitch(defaultPitch),
        m_usingSharps(defaultSharps),
        m_clickedY(0),
        m_clicked(false),
        m_npf(new NotePixmapFactory())
{
    calculatePixmap();
}

PitchDragLabel::~PitchDragLabel()
{
    delete m_npf;
}

void
PitchDragLabel::slotSetPitch(int p)
{
    bool up = (p > m_pitch);
    m_usingSharps = up;
    if (m_pitch == p)
        return ;
    m_pitch = p;
    calculatePixmap();
    emitPitchChange();
    paintEvent(0);
}

void
PitchDragLabel::slotSetPitch(int pitch, int octave, int step)
{
    if (m_pitch == pitch)
        return ;
    m_pitch = pitch;
    calculatePixmap(pitch, octave, step);
    emit pitchChanged(pitch);
    emit pitchChanged(pitch, octave, step);
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
            m_usingSharps = up;
            calculatePixmap();
            emit pitchDragged(m_pitch);
	    if (up)
	    {
		// use sharps
		emit pitchDragged(m_pitch, (int)(((long)m_pitch) / 12),
                                  steps_Cmajor_with_sharps[m_pitch % 12]);
	    }
	    else
	    {
		// use flats
		emit pitchDragged(m_pitch, (int)(((long)m_pitch) / 12),
                                  steps_Cmajor_with_flats[m_pitch % 12]);
	    }
            emit preview(m_pitch);
            paintEvent(0);
        }
    }
}

void
PitchDragLabel::mouseReleaseEvent(QMouseEvent *e)
{
    mouseMoveEvent(e);
    emitPitchChange();
    m_clicked = false;
}

void
PitchDragLabel::emitPitchChange()
{
    emit pitchChanged(m_pitch);
    
	Pitch newPitch(m_pitch);
	
	if (m_usingSharps)
	{
		Rosegarden::Key key = Rosegarden::Key("C major");
    	emit pitchDragged(m_pitch, newPitch.getOctave(0), newPitch.getNoteInScale(key));
	}
	else
	{
		Rosegarden::Key key = Rosegarden::Key("A minor");
		emit pitchDragged(m_pitch, newPitch.getOctave(0), (newPitch.getNoteInScale(key) + 5) % 7);
	}
}

void
PitchDragLabel::wheelEvent(QWheelEvent *e)
{
    if (e->delta() > 0) {
        if (m_pitch < 127) {
            ++m_pitch;
            m_usingSharps = true;
            calculatePixmap();
			emitPitchChange();
            emit preview(m_pitch);
            paintEvent(0);
        }
    } else {
        if (m_pitch > 0) {
            --m_pitch;
            m_usingSharps = false;
            calculatePixmap();
            emitPitchChange();
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
PitchDragLabel::calculatePixmap(int pitch, int octave, int step) const
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
                           octave, step);

    m_pixmap = *pmap;

    delete pmap;
}

void
PitchDragLabel::calculatePixmap() const
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
                           m_usingSharps);

    m_pixmap = *pmap;

    delete pmap;
}

}
#include "PitchDragLabel.moc"
