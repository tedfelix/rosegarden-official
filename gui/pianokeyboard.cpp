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

PianoKeyboard::PianoKeyboard(QSize keySize, QWidget *parent,
                             const char* name, WFlags f)
    : QWidget(parent, name, f),
      m_keySize(48, 19), // keySize),
      m_blackKeySize(24, 9),
      m_nbKeys(88)
{
    addTips();
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

void PianoKeyboard::paintEvent(QPaintEvent*)
{
    QPainter paint(this);

    QPoint pos(0, 4);

    // Draw 1st white key
    QRect firstWhiteKeyRect(pos, m_keySize);

    firstWhiteKeyRect.setHeight(firstWhiteKeyRect.height() -
                                m_blackKeySize.height() / 2);

    paint.drawRect(firstWhiteKeyRect);

    // Draw 1st black key
    paint.save();

    paint.setBrush(colorGroup().foreground());

    QPoint bPos = pos;

    bPos.setY(bPos.y() + firstWhiteKeyRect.height() - m_blackKeySize.height() / 2 - 1);
            
    paint.drawRect(QRect(bPos, m_blackKeySize));
    
    paint.restore();


    // Draw the rest
    pos.setY(pos.y() + firstWhiteKeyRect.height() - 1);

    unsigned int posInOctave = 1;

    for(unsigned int i = 1; i < m_nbKeys; ++i) {
        posInOctave = i % 7;

        QRect whiteKeyRect(pos, m_keySize);
        
        if (posInOctave == 2 ||
            posInOctave == 6 ||
            posInOctave == 3 ||
            posInOctave == 7) { // draw shorter white key

            whiteKeyRect.setHeight(firstWhiteKeyRect.height());
        }

        paint.drawRect(whiteKeyRect);

        if (posInOctave != 2 && posInOctave != 6) { // draw black key

            paint.save();

            paint.setBrush(colorGroup().foreground());

            QPoint bPos = pos;

            bPos.setY(bPos.y() + whiteKeyRect.height() -
                      m_blackKeySize.height() / 2 - 1);
            
            paint.drawRect(QRect(bPos, m_blackKeySize));

            // Debug
            //             paint.setPen(red);
            //             paint.drawLine(bPos, bPos);
            // Debug

            paint.restore();

        }

        pos.setY(pos.y() + whiteKeyRect.height() - 1);
        
    }
    
}

void PianoKeyboard::addTips()
{
    QPoint pos;
    unsigned int posInOctave = 0;
    
    for(unsigned int i = 0; i < m_nbKeys; ++i) {
        posInOctave = i % 7;

        QRect rect(pos, m_keySize);
        rect.setWidth(rect.width() - m_blackKeySize.width());

        QToolTip::add(this, rect, QString("%1").arg(i));

        if (posInOctave != 2 && posInOctave != 6) { // draw black key

            QPoint bPos = pos;
            bPos.setX(bPos.x() + m_keySize.width() / 2);
            bPos.setY(bPos.y() + m_keySize.height() / 2
                      + m_blackKeySize.height() / 2 + 1);
            
            QRect bRect(bPos, m_blackKeySize);
            QToolTip::add(this, bRect, QString("black %1").arg(i));

        }

        pos.setY(pos.y() + m_keySize.height());

    }
}
