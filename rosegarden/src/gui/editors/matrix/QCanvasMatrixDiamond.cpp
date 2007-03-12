/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "QCanvasMatrixDiamond.h"

#include "MatrixElement.h"
#include "QCanvasMatrixRectangle.h"
#include <qcanvas.h>
#include <qpainter.h>
#include <qpointarray.h>
#include <qpoint.h>


namespace Rosegarden
{

QCanvasMatrixDiamond::QCanvasMatrixDiamond(MatrixElement &n,
        QCanvas* canvas) :
        QCanvasMatrixRectangle(n, canvas)
{}

QCanvasMatrixDiamond::~QCanvasMatrixDiamond()
{
    hide();
}

QPointArray QCanvasMatrixDiamond::areaPoints() const
{
    QPointArray pa(4);
    int pw = (pen().width() + 1) / 2;
    if ( pw < 1 )
        pw = 1;
    if ( pen() == NoPen )
        pw = 0;
    pa[0] = QPoint((int)x() - height() / 2 - pw, (int)y() - pw);
    pa[1] = pa[0] + QPoint(height() + pw * 2, 0);
    pa[2] = pa[1] + QPoint(0, height() + pw * 2);
    pa[3] = pa[0] + QPoint(0, height() + pw * 2);
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

}
