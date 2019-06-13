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

#define RG_MODULE_STRING "[PianoKeyboard]"

#include "PianoKeyboard.h"
#include "misc/Debug.h"

#include "gui/general/GUIPalette.h"
#include "gui/general/MidiPitchLabel.h"
#include "gui/rulers/PitchRuler.h"

#include <QColor>
#include <QCursor>
#include <QEvent>
#include <QFont>
#include <QPainter>
#include <QSize>
#include <QWidget>
#include <QMouseEvent>


namespace Rosegarden
{

const unsigned int smallWhiteKeyHeight = 14;
const unsigned int whiteKeyHeight = 18;
    
PianoKeyboard::PianoKeyboard(QWidget *parent, int keys)
        : PitchRuler(parent),
        m_keySize(48, 18),
        m_blackKeySize(24, 8),
        m_nbKeys(keys),
        m_mouseDown(false),
        m_highlight(new QWidget(this)),
        m_lastHighlightPitch(-1),
        m_lastKeyPressed(0)
{
    m_highlight->hide();
    QPalette highlightPalette = m_highlight->palette();
    highlightPalette.setColor(QPalette::Window, GUIPalette::getColour(GUIPalette::MatrixKeyboardFocus));
    m_highlight->setPalette(highlightPalette);
    m_highlight->setAutoFillBackground(true);

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
    return QSize(m_keySize.width(),
                 m_keySize.height() * m_nbKeys);
}

void PianoKeyboard::computeKeyPos()
{
    //    int y = -9;
    int y = -4;

    unsigned int posInOctave = 0,
                               keyHeight = smallWhiteKeyHeight;

    for (unsigned int i = 0; i < m_nbKeys; ++i) {
        posInOctave = (i + 5) % 7;

        if (y >= 0) {
            m_whiteKeyPos.push_back(y);
            m_allKeyPos.push_back(y);
        }

        if (posInOctave == 2)
            m_labelKeyPos.push_back(y + (keyHeight * 3 / 4) - 2);

        if (posInOctave == 0 ||
                posInOctave == 2 ||
                posInOctave == 6 ||
                posInOctave == 3) { // draw shorter white key


            keyHeight = smallWhiteKeyHeight;

            if (posInOctave == 2 ||
                    posInOctave == 6)
                --keyHeight;

        } else {

            keyHeight = whiteKeyHeight;
        }

        if (posInOctave != 2 && posInOctave != 6) { // draw black key

            unsigned int bY = y + keyHeight - m_blackKeySize.height() / 2;

            m_blackKeyPos.push_back(bY);
            m_allKeyPos.push_back(bY);

        }

        y += keyHeight;
    }
}

void PianoKeyboard::paintEvent(QPaintEvent*)
{
    static QFont *pFont = nullptr;
    if (!pFont) {
        pFont = new QFont();
        pFont->setPixelSize(9);
    }

    QPainter paint(this);

    // Fill the keyboard with ivory.
    paint.fillRect(rect(), QColor(255, 255, 240));

    // White Keys

    paint.setPen(Qt::black);

    for (unsigned int i = 0; i < m_whiteKeyPos.size(); ++i)
        paint.drawLine(0, m_whiteKeyPos[i],
                       m_keySize.width(), m_whiteKeyPos[i]);

    // White Key Labels

    paint.setFont(*pFont);

    for (unsigned int i = 0; i < m_labelKeyPos.size(); ++i) {

        int pitch = (m_labelKeyPos.size() - i) * 12;

        // for some reason I don't immediately comprehend,
        // m_labelKeyPos contains two more octaves than we need
        pitch -= 24;

        MidiPitchLabel label(pitch);
        paint.drawText(m_blackKeySize.width(), m_labelKeyPos[i],
                       label.getQString());
    }

    // Black Keys

    paint.setBrush(Qt::black);

    for (unsigned int i = 0; i < m_blackKeyPos.size(); ++i)
        paint.drawRect(0, m_blackKeyPos[i],
                       m_blackKeySize.width(), m_blackKeySize.height());
}

void PianoKeyboard::enterEvent(QEvent *)
{
    //showHighlight(e->y());
}

void PianoKeyboard::leaveEvent(QEvent*)
{
    hideHighlight();

    int pos = mapFromGlobal( cursor().pos() ).x();
    if ( pos > m_keySize.width() - 5 || pos < 0 ) { // bit of a hack
        emit keyReleased(m_lastKeyPressed, false);
    }
}

void PianoKeyboard::showHighlight(int evPitch)
{
    if (m_lastHighlightPitch != evPitch) {
        //RG_DEBUG << "showHighlight() : note = " << evPitch;

        m_lastHighlightPitch = evPitch;

        int count = 0;
        std::vector<unsigned int>::iterator it;
        for (it = m_allKeyPos.begin(); it != m_allKeyPos.end(); ++it, ++count) {
            if (126 - evPitch == count) {
                int width = m_keySize.width() - 8;
                int yPos = *it + 5;

                // check if this is a black key
                //
                std::vector<unsigned int>::iterator bIt;
                bool isBlack = false;
                for (bIt = m_blackKeyPos.begin(); bIt != m_blackKeyPos.end(); ++bIt) {
                    if (*bIt == *it) {
                        isBlack = true;
                        break;
                    }
                }

                // Adjust for black note
                //
                if (isBlack) {
                    width = m_blackKeySize.width() - 8;
                    yPos -= 3;
                } else {
                    // If a white note then ensure that we allow for short/tall ones
                    //
                    std::vector<unsigned int>::iterator wIt = m_whiteKeyPos.begin(), tIt;

                    while (wIt != m_whiteKeyPos.end()) {
                        if (*wIt == *it) {
                            tIt = wIt;

                            if (++tIt != m_whiteKeyPos.end()) {
                                //MATRIX_DEBUG << "WHITE KEY HEIGHT = " << *tIt - *wIt;
                                if (*tIt - *wIt == whiteKeyHeight) {
                                    yPos += 2;
                                }

                            }
                        }

                        ++wIt;
                    }


                }

                m_highlight->setFixedSize(width, 4);
                m_highlight->move(3, yPos);
                m_highlight->show();

                return ;
            }
        }
    }
}

void PianoKeyboard::hideHighlight()
{
    m_highlight->hide();
    m_lastHighlightPitch = -1;
}

void PianoKeyboard::mouseMoveEvent(QMouseEvent* e)
{
    // The routine to work out where this should appear doesn't coincide with the note
    // that we send to the sequencer - hence this is a bit pointless and crap at the moment.
    // My own fault it's so crap but there you go.
    //
    // RWB (20040220)
    //
    if (e->buttons() & Qt::LeftButton) {
        if (m_selecting)
            emit keySelected(e->y(), true);
        else
            emit keyPressed(e->y(), true); // we're swooshing

        emit keyReleased(m_lastKeyPressed, true);
        m_lastKeyPressed = e->y();
    } else
        emit hoveredOverKeyChanged(e->y());
}

void PianoKeyboard::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_mouseDown = true;
        m_selecting = (e->modifiers() & Qt::ShiftModifier);
        m_lastKeyPressed = e->y();

        if (m_selecting)
            emit keySelected(e->y(), false);
        else
            emit keyPressed(e->y(), false);
    }
}

void PianoKeyboard::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_mouseDown = false;
        m_selecting = false;
        emit keyReleased(e->y(), false);
    }
}

}
