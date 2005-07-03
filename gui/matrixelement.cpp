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

#include "NotationTypes.h"

#include "rosedebug.h"

#include "qpainter.h"
#include "matrixelement.h"
#include "colours.h"

MatrixElement::MatrixElement(Rosegarden::Event *event, bool drum) :
    Rosegarden::ViewElement(event),
    m_canvasRect(drum ?
		 new QCanvasMatrixDiamond(*this, 0) :
		 new QCanvasMatrixRectangle(*this, 0))
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

        m_canvasRect->setPen(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::MatrixElementBorder));
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


QCanvasMatrixDiamond::QCanvasMatrixDiamond(MatrixElement &n,
					   QCanvas* canvas) :
    QCanvasMatrixRectangle(n, canvas)
{
}

QCanvasMatrixDiamond::~QCanvasMatrixDiamond()
{
    hide();
}

QPointArray QCanvasMatrixDiamond::areaPoints() const
{
    QPointArray pa(4);
    int pw = (pen().width()+1)/2;
    if ( pw < 1 ) pw = 1;
    if ( pen() == NoPen ) pw = 0;
    pa[0] = QPoint((int)x()-height()/2-pw,(int)y()-pw);
    pa[1] = pa[0] + QPoint(height()+pw*2,0);
    pa[2] = pa[1] + QPoint(0,height()+pw*2);
    pa[3] = pa[0] + QPoint(0,height()+pw*2);
    return pa;
}

void QCanvasMatrixDiamond::drawShape(QPainter & p)
{
    p.save();
    p.setWorldXForm(false);

    QPointArray pa(4);
    int q = height() / 2 + 2;
    QPoint mapPos = p.worldMatrix().map(QPoint(int(x()), int(y())));
    
    pa[0] = QPoint(mapPos.x(), mapPos.y() - 3);
    pa[1] = QPoint(mapPos.x() + q, mapPos.y() - 3 + q);
    pa[2] = pa[0] + QPoint(0, q * 2);
    pa[3] = pa[1] - QPoint(q * 2, 0);
    p.drawConvexPolygon(pa);

    p.restore();
}

