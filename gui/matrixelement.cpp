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

#include "NotationTypes.h"

#include "matrixelement.h"
#include "colours.h"

MatrixElement::MatrixElement(Rosegarden::Event *event) :
    Rosegarden::ViewElement(event),
    m_canvasRect(new QCanvasMatrixRectangle(*this, 0))
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
}

void MatrixElement::setCanvas(QCanvas* c)
{
    if (!m_canvasRect->canvas()) {
        
        m_canvasRect->setCanvas(c);

        // We set this by velocity now (matrixstaff.cpp)
        //
        //m_canvasRect->setBrush(RosegardenGUIColours::MatrixElementBlock);

        m_canvasRect->setPen(RosegardenGUIColours::MatrixElementBorder);
        m_canvasRect->show();
    }
}

bool MatrixElement::isNote() const
{
    return event()->isa(Rosegarden::Note::EventType);
}

//////////////////////////////////////////////////////////////////////

QCanvasMatrixRectangle::QCanvasMatrixRectangle(MatrixElement& n,
                                               QCanvas* canvas)
    : QCanvasRectangle(canvas),
      m_matrixElement(n)
{
}

QCanvasMatrixRectangle::~QCanvasMatrixRectangle()
{
}

