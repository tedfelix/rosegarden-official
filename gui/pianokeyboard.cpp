// -*- c-basic-offset: 4 -*-

/*
  Rosegarden-4 v0.1
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

#include "pianokeyboard.h"

#include <qpainter.h>
#include <qtooltip.h>

#include <klocale.h>

#include "rosedebug.h"

PianoKeyboard::PianoKeyboard(QSize keySize, QWidget *parent,
                             const char* name, WFlags f)
    : QWidget(parent, name, f),
      m_keySize(48, 18), // keySize),
      m_blackKeySize(24, 8),
      m_nbKeys(88)
{
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
    unsigned const int smallWhiteKeyHeight = 14,
        whiteKeyHeight = 18;
    
    int y = -5;

    unsigned int posInOctave = 0,
        keyHeight = smallWhiteKeyHeight;

    for(unsigned int i = 0; i < m_nbKeys; ++i) {
        posInOctave = (i + 3) % 7;

        if (y >= 0)
            m_whiteKeyPos.push_back(y);

        if (posInOctave == 0)
            m_labelKeyPos.push_back(y + (keyHeight * 3 / 4));

        if (posInOctave == 0 ||
            posInOctave == 2 ||
            posInOctave == 6 ||
            posInOctave == 3) { // draw shorter white key


            keyHeight = smallWhiteKeyHeight;

            if (posInOctave == 2 ||
                posInOctave == 6) --keyHeight;
            
        } else {

            keyHeight = whiteKeyHeight;

        }

        if (posInOctave != 2 && posInOctave != 6) { // draw black key

            unsigned int bY = y + keyHeight - m_blackKeySize.height() / 2;

            m_blackKeyPos.push_back(bY);

        }

        y += keyHeight;
    }
    
}

void PianoKeyboard::mouseMoveEvent(QMouseEvent* e)
{
    emit hoveredOverKeyChanged(e->y());
}


void PianoKeyboard::paintEvent(QPaintEvent*)
{
    static QFont pFont("helvetica", 8);

    QPainter paint(this);

    paint.setFont(pFont);

    for(unsigned int i = 0; i < m_whiteKeyPos.size(); ++i)
        paint.drawLine(0, m_whiteKeyPos[i],
                       m_keySize.width(), m_whiteKeyPos[i]);

    for(unsigned int i = 0; i < m_labelKeyPos.size(); ++i)
        paint.drawText(m_blackKeySize.width(), m_labelKeyPos[i],
                       QString(i18n("C %1")).arg(m_labelKeyPos.size() - i - 2));

    paint.setBrush(colorGroup().foreground());

    for(unsigned int i = 0; i < m_blackKeyPos.size(); ++i)
        paint.drawRect(0, m_blackKeyPos[i],
                       m_blackKeySize.width(), m_blackKeySize.height());
}

