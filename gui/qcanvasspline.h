// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

/**********************************************************************
** Copyright (C) 1999-2001 Trolltech AS.  All rights reserved.
**
** This file is part of the canvas module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition licenses may use this
** file in accordance with the Qt Commercial License Agreement provided
** with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QCANVASSPLINE_H
#define QCANVASSPLINE_H

#include <qcanvas.h>

/**
 * Port of a class from Qt3
 */
class Q3PointArray : public QPointArray
{
public:
    Q3PointArray();
    Q3PointArray( int size ) : QPointArray( size ) {}
    Q3PointArray( const Q3PointArray &a ) : QPointArray( a ) {}
    Q3PointArray( const QRect &r, bool closed=FALSE );
    Q3PointArray( int nPoints, const QCOORD *points );
    
    Q3PointArray cubicBezier() const;
};


/**
 * Port of a class from Qt3
 */
class QCanvasSpline : public QCanvasPolygon
{
public:
    QCanvasSpline(QCanvas* canvas);
    ~QCanvasSpline();

    void setControlPoints(Q3PointArray, bool closed=TRUE);
    Q3PointArray controlPoints() const;
    bool closed() const;

    int rtti() const;
    static int RTTI;
protected:
    virtual void drawShape ( QPainter & p );
    virtual void draw ( QPainter & p );
    
private:
    void recalcPoly();
    Q3PointArray bez;
    bool cl;
};

#endif
