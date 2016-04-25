/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "FingeringBox.h"
#include "Fingering.h"

#include "misc/Debug.h"

#include <QMouseEvent>

namespace Rosegarden
{

FingeringBox::FingeringBox(
        unsigned int nbFrets,
        unsigned int nbStrings, 
        bool editable, 
        QWidget *parent, 
        bool big
    )
    : QFrame(parent),
    m_nbFretsDisplayed(nbFrets),
    m_startFret(1),
    m_nbStrings(nbStrings),
    m_transientFretNb(0),
    m_transientStringNb(0),
    m_editable(editable),
    m_noteSymbols(m_nbStrings, m_nbFretsDisplayed),
    m_big(big)
{
    init();    
}

FingeringBox::FingeringBox(bool editable, QWidget *parent, bool big)
    : QFrame(parent),
    m_nbFretsDisplayed(DEFAULT_NB_DISPLAYED_FRETS),
    m_startFret(1),
    m_nbStrings(Guitar::Fingering::DEFAULT_NB_STRINGS),
    m_editable(editable),
    m_noteSymbols(m_nbStrings, m_nbFretsDisplayed),
    m_big(big)
{
    init();
}

void
FingeringBox::init()
{
    setFixedSize(IMG_WIDTH, IMG_HEIGHT);
   
    QString localStyle = "background-color: white";
    setStyleSheet(localStyle);

    if (m_editable)
        setMouseTracking(true);    
}

void
FingeringBox::paintEvent(QPaintEvent *)
{
    RG_DEBUG << "FingeringBox::paintEvent()";
    QPainter p;
    drawContents(&p);
}

void
FingeringBox::drawContents(QPainter* p)
{
    RG_DEBUG << "FingeringBox::drawContents()";
    
    // For all strings on guitar
    //   check state of string
    //     If pressed display note
    //     Else display muted or open symbol
    // For all bars
    //   display bar
    // Horizontal separator line
    
    p->begin(this);

    // turn on antialiasing for the X and O symbols,  which makes a spectacular
    // difference here, though it's not guaranteed to work
    p->setRenderHint(QPainter::Antialiasing);

    unsigned int stringNb = 0;
    
    // draw notes
    //
    for (Guitar::Fingering::const_iterator pos = m_fingering.begin();
         pos != m_fingering.end();
         ++pos, ++stringNb) {
                
        switch (*pos) {
        case Guitar::Fingering::OPEN:
//                NOTATION_DEBUG << "Fingering::drawContents - drawing Open symbol on string " << stringNb;
                m_noteSymbols.drawOpenSymbol(m_big, p, stringNb);
                break;

        case Guitar::Fingering::MUTED:
//                NOTATION_DEBUG << "Fingering::drawContents - drawing Mute symbol on string" << stringNb;
                m_noteSymbols.drawMuteSymbol(m_big, p, stringNb);
                break;

        default:
//                NOTATION_DEBUG << "Fingering::drawContents - drawing note symbol at " << *pos << " on string " << stringNb;
                m_noteSymbols.drawNoteSymbol(m_big, p, stringNb, *pos - (m_startFret - 1), false);
                break;
        }
    }

    // draw guitar chord fingering after note symbols, to draw over fuzzy
    // borders caused by antialiasing
    //
    m_noteSymbols.drawFretNumber(p, m_startFret);
    m_noteSymbols.drawFrets(p);
    m_noteSymbols.drawStrings(p);

    // TODO: detect barres and draw them in a special way ?
    
    // draw transient note (visual feedback for mouse move)
    //
    if (testAttribute(Qt::WA_UnderMouse) &&
        m_transientFretNb > 0 && m_transientFretNb <= m_nbFretsDisplayed &&
        m_transientStringNb <= m_nbStrings) {
        p->setBrush(QColor(0, 0x10, 0xFF, 0x10));
        m_noteSymbols.drawNoteSymbol(m_big, p, m_transientStringNb, m_transientFretNb - (m_startFret - 1), true);
    }
    
    // DEBUG
//    p->save();
//    p->setPen(QColor(Qt::red));
//    unsigned int topBorderY = m_noteSymbols.getTopBorder(maximumHeight());
//    p->drawLine(0, topBorderY, 20, topBorderY);
//    p->drawRect(m_r1);
//    p->setPen(QColor(Qt::blue));
//    p->drawRect(m_r2);
//    p->restore();
}

void
FingeringBox::setFingering(const Guitar::Fingering& f) {
    m_fingering = f;
    m_startFret = m_fingering.getStartFret();
    update();
}

unsigned int
FingeringBox::getStringNumber(const QPoint& pos)
{
    PositionPair result = m_noteSymbols.getStringNumber(maximumHeight(),
                                                        pos.x(),
                                                        m_nbStrings);
    unsigned int stringNum = -1;

    if(result.first){
        stringNum = result.second;
//        RG_DEBUG << "FingeringBox::getStringNumber : res = " << stringNum; 
    }

    return stringNum;
}

unsigned int
FingeringBox::getFretNumber(const QPoint& pos)
{
    unsigned int fretNum = 0;

    if(true || pos.y() > int(m_noteSymbols.getTopBorder(maximumHeight()))) {
        // If fret position is below the top line of the guitar chord image.
        PositionPair result = m_noteSymbols.getFretNumber(maximumWidth(),
                                                          pos.y(),
                                                          m_nbFretsDisplayed);

        if(result.first) {
            fretNum = result.second + (m_startFret - 1);
//            RG_DEBUG << "FingeringBox::getFretNumber : res = " << fretNum << " startFret = " << m_startFret; 
        } else {
//            RG_DEBUG << "FingeringBox::getFretNumber : no res\n";
        }
    }

    return fretNum;
}

void
FingeringBox::mousePressEvent(QMouseEvent *event)
{
    if (!m_editable)
        return;
        
    if((event->button() == Qt::LeftButton) && m_editable) {

        // Find string position
        m_press_string_num = getStringNumber(event->pos());

        // Find fret position
        m_press_fret_num = getFretNumber(event->pos());
    }
}

void
FingeringBox::mouseReleaseEvent(QMouseEvent *event)
{
    if(!m_editable)
        return ;

    unsigned int release_string_num = getStringNumber(event->pos());
    unsigned int release_fret_num = getFretNumber(event->pos());

    processMouseRelease(release_string_num, release_fret_num);
}

void
FingeringBox::processMouseRelease(unsigned int release_string_num,
                                   unsigned int release_fret_num)
{
    if(m_press_fret_num == release_fret_num) {
        // If press string & fret pos == release string & fret position, display chord
        if(m_press_string_num == release_string_num) {

            if(m_press_fret_num < (m_startFret + m_nbFretsDisplayed)) {

                unsigned int aVal = m_press_fret_num;
                
                if(m_press_fret_num == 0) {
                    
                    int stringStatus = m_fingering.getStringStatus(m_press_string_num);

                    if (stringStatus == Guitar::Fingering::OPEN)
                        aVal = Guitar::Fingering::MUTED;
                    else if (stringStatus > Guitar::Fingering::OPEN)
                        aVal = Guitar::Fingering::OPEN;

                }
                
                m_fingering.setStringStatus(m_press_string_num, aVal);
                
                update();
            }
        }
        // else if press fret pos == release fret pos & press string pos != release string pos, display bar
        else {
            if(((m_press_string_num > 0)&&(release_string_num > 0)) &&
                   (( m_press_string_num <= m_nbStrings)&&
                     (release_string_num <= m_nbStrings)) &&
                   (( m_press_fret_num <(m_startFret + m_nbFretsDisplayed)) &&
                     (release_fret_num <(m_startFret + m_nbFretsDisplayed)))) {

                // TODO deal with barre later on

            }
        }
    }
}


void
FingeringBox::mouseMoveEvent( QMouseEvent *event )
{
    if (!m_editable)
        return;
        
    unsigned int transientStringNb = getStringNumber(event->pos());
    unsigned int transientFretNb   = getFretNumber(event->pos());
        
    if (transientStringNb != m_transientStringNb ||
        transientFretNb != m_transientFretNb) {

        QRect r1 = m_noteSymbols.getTransientNoteSymbolRect(size(),
                                                            m_transientStringNb,
                                                            m_transientFretNb - (m_startFret - 1));
        m_transientStringNb = transientStringNb;
        m_transientFretNb   = transientFretNb;
        QRect r2 = m_noteSymbols.getTransientNoteSymbolRect(size(),
                                                            m_transientStringNb,
                                                            m_transientFretNb - (m_startFret - 1));
    
        m_r1 = r1;
        m_r2 = r2;
        
//    RG_DEBUG << "Fingering::updateTransientPos r1 = " << r1 << " - r2 = " << r2;
     
//        QRect updateRect = r1 | r2;
//        update(updateRect);

        update();
            
    }    
    
}

void
FingeringBox::leaveEvent(QEvent*)
{
    update();
}

}
