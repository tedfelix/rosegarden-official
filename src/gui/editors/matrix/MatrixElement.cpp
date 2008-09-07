/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include <Q3Canvas>
#include <Q3CanvasItemList>
#include <Q3CanvasRectangle>
#include "MatrixElement.h"
#include "misc/Debug.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/ViewElement.h"
#include "gui/general/GUIPalette.h"
#include "QCanvasMatrixDiamond.h"
#include "QCanvasMatrixRectangle.h"
#include <QBrush>
#include <Q3Canvas>
#include <QColor>


namespace Rosegarden
{

MatrixElement::MatrixElement(Event *event, bool drum) :
        ViewElement(event),
        m_canvasRect(drum ?
                     new QCanvasMatrixDiamond(*this, 0) :
                     new QCanvasMatrixRectangle(*this, 0)),
        m_overlapRectangles(NULL)
{
    //     MATRIX_DEBUG << "new MatrixElement "
    //                          << this << " wrapping " << event << endl;
}

MatrixElement::~MatrixElement()
{
    //     MATRIX_DEBUG << "MatrixElement " << this << "::~MatrixElement() wrapping "
    //                          << event() << endl;

    m_canvasRect->hide();
    delete m_canvasRect;

    removeOverlapRectangles();
}

void MatrixElement::setCanvas(Q3Canvas* c)
{
    if (!m_canvasRect->canvas()) {

        m_canvasRect->setCanvas(c);

        // We set this by velocity now (matrixstaff.cpp)
        //
        //m_canvasRect->setBrush(RosegardenGUIColours::MatrixElementBlock);

        m_canvasRect->setPen(GUIPalette::getColour(GUIPalette::MatrixElementBorder));
        m_canvasRect->show();
    }
}

bool MatrixElement::isNote() const
{
    return event()->isa(Note::EventType);
}

void MatrixElement::drawOverlapRectangles()
{
    if (m_overlapRectangles) removeOverlapRectangles();

    QRect elRect = m_canvasRect->rect();
    Q3CanvasItemList
          itemList = m_canvasRect->canvas()->collisions(elRect);
    Q3CanvasItemList::Iterator it;
    MatrixElement* mel = 0;


    for (it = itemList.begin(); it != itemList.end(); ++it) {

        QCanvasMatrixRectangle *mRect = 0;
        if ((mRect = dynamic_cast<QCanvasMatrixRectangle*>(*it))) {

            // Element does'nt collide with itself
            if (mRect == m_canvasRect) continue;

            QRect rect = mRect->rect() & elRect;
            if (!rect.isEmpty()) {
                if (!m_overlapRectangles) {
                    m_overlapRectangles = new OverlapRectangles();
                }

                Q3CanvasRectangle *
                    overlap = new Q3CanvasRectangle(rect, m_canvasRect->canvas());
                overlap->setBrush(GUIPalette::getColour(GUIPalette::MatrixOverlapBlock));
                overlap->setZ(getCanvasZ() + 1);
                overlap->show();
                m_overlapRectangles->push_back(overlap);
            }
        }
    }
}

void MatrixElement::redrawOverlaps(QRect rect)
{
    Q3CanvasItemList
          itemList = m_canvasRect->canvas()->collisions(rect);
    Q3CanvasItemList::Iterator it;
    MatrixElement* mel = 0;

    for (it = itemList.begin(); it != itemList.end(); ++it) {
        QCanvasMatrixRectangle *mRect = 0;
        if ((mRect = dynamic_cast<QCanvasMatrixRectangle*>(*it))) {
            mRect->getMatrixElement().drawOverlapRectangles();
        }
    }
}

void MatrixElement::removeOverlapRectangles()
{
    if (!m_overlapRectangles) return;

    OverlapRectangles::iterator it;
    for (it = m_overlapRectangles->begin(); it != m_overlapRectangles->end(); ++it) {
        (*it)->hide();
        delete *it;
    }

    delete m_overlapRectangles;
    m_overlapRectangles = NULL;
}

bool MatrixElement::getVisibleRectangle(QRect &rectangle)
{
    if (m_canvasRect && m_canvasRect->isVisible()) {
        rectangle = m_canvasRect->rect();
        return true;
    }
    return false;
}


}
