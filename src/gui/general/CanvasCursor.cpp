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
#include <Q3CanvasRectangle>
#include "CanvasCursor.h"

#include "GUIPalette.h"
#include <qcanvas.h>
#include <QPen>


namespace Rosegarden
{

CanvasCursor::CanvasCursor(Q3Canvas* c, int width)
        : Q3CanvasRectangle(c),
        m_width(width)
{
    QPen pen(GUIPalette::getColour(GUIPalette::Pointer));
    //     pen.setWidth(width);
    setPen(pen);
    setBrush(GUIPalette::getColour(GUIPalette::Pointer));
}

void CanvasCursor::updateHeight()
{
    setSize(m_width, canvas()->height());
    //     setPoints(0, 0, 0, canvas()->height());
}

}
