/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "MatrixElement.h"
#include "misc/Debug.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/ViewElement.h"
#include "gui/general/GUIPalette.h"
#include "QCanvasMatrixDiamond.h"
#include "QCanvasMatrixRectangle.h"
#include <qbrush.h>
#include <qcanvas.h>
#include <qcolor.h>


namespace Rosegarden
{

MatrixElement::MatrixElement(Event *event, bool drum) :
        ViewElement(event),
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

        m_canvasRect->setPen(GUIPalette::getColour(GUIPalette::MatrixElementBorder));
        m_canvasRect->show();
    }
}

bool MatrixElement::isNote() const
{
    return event()->isa(Note::EventType);
}

}
