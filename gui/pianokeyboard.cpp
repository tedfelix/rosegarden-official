// -*- c-basic-offset: 4 -*-

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

#include <qpainter.h>
#include <qtooltip.h>

#include <klocale.h>
#include <iostream>

#include "pianokeyboard.h"
#include "midipitchlabel.h"
#include "colours.h"

const unsigned int _smallWhiteKeyHeight = 14;
const unsigned int _whiteKeyHeight = 18;
    
PianoKeyboard::PianoKeyboard(QWidget *parent)
    : QWidget(parent),
      m_keySize(48, 18),
      m_blackKeySize(24, 8),
      m_nbKeys(88),
      m_mouseDown(false),
      m_hoverHighlight(new QWidget(this))
{
    m_hoverHighlight->hide();
    m_hoverHighlight->setPaletteBackgroundColor(RosegardenGUIColours::MatrixKeyboardFocus);

    computeKeyPos();
    setMouseTracking(true);
}

QSize PianoKeyboard::sizeHint() const
{
    return QSize(m_keySize.width(),
                 m_keySize.height() * m_nbKeys);
}

QSize PianoKeyboard::minimumSizeHint() const
{
    return m_keySize;
}

void PianoKeyboard::computeKeyPos()
{
    int y = -9;

    unsigned int posInOctave = 0,
        keyHeight = _smallWhiteKeyHeight;

    for(unsigned int i = 0; i < m_nbKeys; ++i) {
        posInOctave = (i+5) % 7;

        if (y >= 0)
            m_whiteKeyPos.push_back(y);

        if (posInOctave == 2)
            m_labelKeyPos.push_back(y + (keyHeight * 3/4) - 2);

        if (posInOctave == 0 ||
            posInOctave == 2 ||
            posInOctave == 6 ||
            posInOctave == 3) { // draw shorter white key


            keyHeight = _smallWhiteKeyHeight;

            if (posInOctave == 2 ||
                posInOctave == 6) --keyHeight;
            
        } else {

            keyHeight = _whiteKeyHeight;        }

        if (posInOctave != 2 && posInOctave != 6) { // draw black key

            unsigned int bY = y + keyHeight - m_blackKeySize.height() / 2;

            m_blackKeyPos.push_back(bY);

        }

        y += keyHeight;
    }
    
}

void PianoKeyboard::paintEvent(QPaintEvent*)
{
    static QFont pFont("helvetica", 8);

    QPainter paint(this);

    paint.setFont(pFont);

    for(unsigned int i = 0; i < m_whiteKeyPos.size(); ++i)
        paint.drawLine(0, m_whiteKeyPos[i],
                       m_keySize.width(), m_whiteKeyPos[i]);

    for(unsigned int i = 0; i < m_labelKeyPos.size(); ++i) {

	int pitch = (m_labelKeyPos.size() - i) * 12;

	// for some reason I don't immediately comprehend,
	// m_labelKeyPos contains two more octaves than we need
	pitch -= 24;

	Rosegarden::MidiPitchLabel label(pitch);
        paint.drawText(m_blackKeySize.width(), m_labelKeyPos[i],
		       label.getQString());
    }

    paint.setBrush(colorGroup().foreground());

    for(unsigned int i = 0; i < m_blackKeyPos.size(); ++i)
        paint.drawRect(0, m_blackKeyPos[i],
                       m_blackKeySize.width(), m_blackKeySize.height());
}

void PianoKeyboard::enterEvent(QEvent*)
{
    m_hoverHighlight->show();
}


void PianoKeyboard::leaveEvent(QEvent*)
{
    m_hoverHighlight->hide();
}

void PianoKeyboard::drawHoverNote(unsigned int y)
{
    unsigned int whiteDiff = 0;
    unsigned int whiteYPos = 0;
    bool shortWhiteKey = false;

    for (unsigned int i = 0; i < m_whiteKeyPos.size(); ++i)
    {
        unsigned int diff = (m_whiteKeyPos[i] > y) ?
            m_whiteKeyPos[i] - y : y - m_whiteKeyPos[i];

        if (i == 0) 
            whiteDiff = diff;
        else
        {
            if (diff < whiteDiff)
            {
                whiteDiff = diff;
                whiteYPos = m_whiteKeyPos[i];

                // check to see if it's a short note and adjust position accordingly
                if (i + 1 < m_whiteKeyPos.size())
                    if (m_whiteKeyPos[i + 1] - m_whiteKeyPos[i] < _whiteKeyHeight)
                        shortWhiteKey = true;
                    else
                        shortWhiteKey = false;
            }
        }
    }

    // adjust for large notes
    /*
    if (y > whiteYPos + 14) whiteYPos += 14;
    else if (y < whiteYPos - 14) whiteYPos -= 14;
    */


    unsigned int blackDiff = 0;
    unsigned int blackYPos = 0;

    for (unsigned int i = 0; i < m_blackKeyPos.size(); ++i)
    {
        unsigned int diff = (m_blackKeyPos[i] > y) ?
            m_blackKeyPos[i] - y : y - m_blackKeyPos[i];

        if (i == 0)
            blackDiff = diff;
        else
        {
            if (diff < blackDiff)
            {
                blackDiff = diff;
                blackYPos = m_blackKeyPos[i];
            }
        }
    }

    if (blackDiff < whiteDiff)
    {
        m_hoverHighlight->setFixedSize(
                QSize(m_blackKeySize.width() - 2, 
                      m_blackKeySize.height() - 2));

        m_hoverHighlight->move(pos().x(), blackYPos + 1);
    }
    else
    {
        m_hoverHighlight->setFixedSize(
                QSize(m_keySize.width() - 4,
                      m_keySize.height() - ((shortWhiteKey) ? 12 : 10)));

        m_hoverHighlight->move(pos().x(), whiteYPos + 5);
    }

    // sometimes the enter event gets bypassed
    m_hoverHighlight->show();

}

void PianoKeyboard::mouseMoveEvent(QMouseEvent* e)
{
    drawHoverNote((unsigned int)e->y());

    if (m_mouseDown)
	if (m_selecting)
	    emit keySelected(e->y(), true);
	else 
	    emit keyPressed(e->y(), true); // we're swooshing
    else
        emit hoveredOverKeyChanged(e->y());
}

void PianoKeyboard::mousePressEvent(QMouseEvent *e)
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

void PianoKeyboard::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == LeftButton)
    {
        m_mouseDown = false;
	m_selecting = false;
    }
}

