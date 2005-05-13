// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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

#include <qpainter.h>
#include <qtooltip.h>

#include <klocale.h>
#include <iostream>

#include "percussionpitchruler.h"
#include "midipitchlabel.h"
#include "colours.h"
#include "rosedebug.h"
#include "matrixview.h"
#include "rosestrings.h"

using Rosegarden::MidiKeyMapping;

PercussionPitchRuler::PercussionPitchRuler(QWidget *parent, 
					   const MidiKeyMapping *mapping,
					   int lineSpacing) :
    PitchRuler(parent),
    m_mapping(mapping),
    m_width(50),
    m_lineSpacing(lineSpacing),
    m_mouseDown(false),
    m_hoverHighlight(new QWidget(this)),
    m_lastHoverHighlight(0)
{
    m_hoverHighlight->hide();
    m_hoverHighlight->setPaletteBackgroundColor
	(Rosegarden::GUIPalette::getColour
	 (Rosegarden::GUIPalette::MatrixKeyboardFocus));

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
    static QFont pFont("helvetica", 8);

    QPainter paint(this);

    paint.setFont(pFont);

    int minPitch = m_mapping->getPitchForOffset(0);
    int extent = m_mapping->getPitchExtent();

    for (int i = 0; i < extent; ++i) {
	paint.drawLine(0,       i * (m_lineSpacing + 1),
		       m_width, i * (m_lineSpacing + 1));
    }

    for (int i = 0; i < extent; ++i) {

	Rosegarden::MidiPitchLabel label(minPitch + i);
	std::string key = m_mapping->getMapForKeyName(minPitch + i);

        paint.drawText(0, (extent - i - 1) * (m_lineSpacing + 1),
		       QString("%1: %2")
		       .arg(label.getQString())
		       .arg(strtoqstr(key)));
    }
}

void PercussionPitchRuler::enterEvent(QEvent *)
{
}

void PercussionPitchRuler::leaveEvent(QEvent*)
{
    m_hoverHighlight->hide();
}

void PercussionPitchRuler::drawHoverNote(int evPitch)
{
    if (m_lastHoverHighlight != evPitch)
    {
        m_lastHoverHighlight = evPitch;
/*!!!
        int count = 0;
        std::vector<unsigned int>::iterator it;
        for (it = m_allKeyPos.begin(); it != m_allKeyPos.end(); ++it, ++count)
        {
            if (126 - evPitch == count)
            {
                int width = m_keySize.width() - 8;
                int yPos = *it + 5;

                // check if this is a black key
                //
                std::vector<unsigned int>::iterator bIt;
                bool isBlack = false;
                for (bIt = m_blackKeyPos.begin(); bIt != m_blackKeyPos.end(); ++bIt)
                {
                    if (*bIt == *it)
                    {
                        isBlack = true;
                        break;
                    }
                }

                // Adjust for black note
                //
                if (isBlack)
                {
                    width = m_blackKeySize.width() - 8;
                    yPos -= 3;
                }
                else
                {
                    // If a white note then ensure that we allow for short/tall ones
                    //
                    std::vector<unsigned int>::iterator wIt = m_whiteKeyPos.begin(), tIt;
                    
                    while (wIt != m_whiteKeyPos.end())
                    {
                        if (*wIt == *it)
                        {
                            tIt = wIt;

                            if (++tIt != m_whiteKeyPos.end())
                            {
                                //MATRIX_DEBUG << "WHITE KEY HEIGHT = " << *tIt - *wIt << endl;
                                if (*tIt - *wIt == _whiteKeyHeight)
                                {
                                    yPos += 2;
                                }

                            }
                        }

                        ++wIt;
                    }


                }

                m_hoverHighlight->setFixedSize(width, 4);
                m_hoverHighlight->move(3, yPos);
                m_hoverHighlight->show();

                return;
            }
        }
    }
*/
    }
}

void PercussionPitchRuler::mouseMoveEvent(QMouseEvent* e)
{
    // The routine to work out where this should appear doesn't coincide with the note
    // that we send to the sequencer - hence this is a bit pointless and crap at the moment.
    // My own fault it's so crap but there you go.
    //
    // RWB (20040220)
    //
/*!!!
    MatrixView *matrixView = dynamic_cast<MatrixView*>(topLevelWidget());
    if (matrixView)
    {
        MatrixStaff *staff = matrixView->getStaff(0);

        if (staff)
        {
            drawHoverNote(staff->getHeightAtCanvasCoords(e->x(), e->y()));
        }
    }

    if (m_mouseDown)
	if (m_selecting)
	    emit keySelected(e->y(), true);
	else 
	    emit keyPressed(e->y(), true); // we're swooshing
    else
        emit hoveredOverKeyChanged(e->y());
*/
}

void PercussionPitchRuler::mousePressEvent(QMouseEvent *e)
{
    Qt::ButtonState bs = e->state();

    if (e->button() == LeftButton)
    {
        m_mouseDown = true;
	m_selecting = (bs & Qt::ShiftButton);
	
	if (m_selecting)
	    emit keySelected(e->y(), false);
	else
	    emit keyPressed(e->y(), false);
    }
}

void PercussionPitchRuler::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == LeftButton)
    {
        m_mouseDown = false;
	m_selecting = false;
    }
}

#include "percussionpitchruler.moc"

