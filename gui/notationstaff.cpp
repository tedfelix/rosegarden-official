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

#include <algorithm>

#include "notationstaff.h"
#include "staffline.h"
#include "qcanvasspritegroupable.h"

#include "rosedebug.h"

using Rosegarden::Track;

NotationStaff::NotationStaff(QCanvas *canvas, Track *track, int resolution) :
    Rosegarden::Staff<NotationElement>(*track),
    QCanvasItemGroup(canvas),
    m_barLineHeight(0),
    m_horizLineLength(0),
    m_resolution(resolution),
    m_npf(resolution)
{
    // horizontal lines

    int w = canvas->width();
    m_horizLineLength = w - 20;

    // Pitch is represented with the MIDI pitch scale; NotationTypes.h
    // contains methods to convert this to and from staff-height
    // according to the current clef and key.  Staff-height is
    // represented with signed integers such that the bottom staff
    // line is 0, the space immediately above it is 1, and so on up to
    // the top staff line which has a height of 8.  We shouldn't be
    // concerned with pitch in this class, only with staff-height.

    // Now, the y-coord of a staff m whole-lines below the top
    // staff-line (where 0 <= m <= 4) is m * lineWidth + linesOffset.
    // For a staff at height h, m = (8-h)/2.  Therefore the y-coord of
    // a staff at height h is (8-h)/2 * lineWidth + linesOffset

    // Let's just make the regular staff lines for now, not the leger
    // lines...

    for (int h = 0; h <= 8; ++h) {

        StaffLine *line = new StaffLine(canvas, this, h);
        int y = yCoordOfHeight(h);
        line->setPoints(0, y, m_horizLineLength, y);

        if (h % 2 == 1) {
            // make the line invisible
            line->setPen(QPen(white, 1)); // invisibleLineWidth
            line->setZ(-1);
        }

        m_staffLines.push_back(line);
    }

    m_barLineHeight = (nbLines - 1) * m_npf.getLineSpacing();

    // First line - thick
    //
    QPen pen(black, 3);
    pen.setCapStyle(Qt::SquareCap);
    m_initialBarA = new QCanvasLineGroupable(canvas, this);
    m_initialBarA->setPen(pen);
    m_initialBarA->setPoints(0, linesOffset + 1,
                             0, m_barLineHeight + linesOffset - 1);
    
    // Second line - thin
    //
    m_initialBarB = new QCanvasLineGroupable(canvas, this);
    m_initialBarB->setPoints(4, linesOffset,
                             4, m_barLineHeight + linesOffset);

    setActive(false);  // don't react to mousePress events
}

NotationStaff::~NotationStaff()
{
    // TODO : this causes a crash on quit - don't know why
//     for (barlines::iterator i = m_barLines.begin(); i != m_barLines.end(); ++i)
//         delete (*i);
}

int NotationStaff::yCoordOfHeight(int h) const
{
    // 0 is bottom staff-line, 8 is top one
    int y = ((8 - h) * m_npf.getLineSpacing()) / 2 +
        linesOffset + ((h % 2 == 1) ? 1 : 0);
    return y;
}

static bool
compareBarPos(QCanvasLineGroupable *barLine1, QCanvasLineGroupable *barLine2)
{
    return barLine1->x() < barLine2->x();
}

static bool
compareBarToPos(QCanvasLineGroupable *barLine1, unsigned int pos)
{
    return barLine1->x() < pos;
}

void NotationStaff::insertBar(unsigned int barPos, bool correct)
{
//    kdDebug(KDEBUG_AREA) << "Staff::insertBar(" << barPos << ")\n";

    QCanvasLineGroupable* barLine = new QCanvasLineGroupable(canvas(), this);

    barLine->setPoints(0, linesOffset,
                       0, getBarLineHeight() + linesOffset);
    barLine->moveBy(barPos + x(), y());
    if (!correct) barLine->setPen(QPen(red, 1));
    barLine->show();

    barlines::iterator insertPoint = lower_bound(m_barLines.begin(),
                                                 m_barLines.end(),
                                                 barLine, compareBarPos);

    m_barLines.insert(insertPoint, barLine);
}

void NotationStaff::deleteBars(unsigned int fromPos)
{
    kdDebug(KDEBUG_AREA) << "NotationStaff::deleteBars from " << fromPos << endl;

    barlines::iterator startDeletePoint =
        lower_bound(m_barLines.begin(), m_barLines.end(),
                    fromPos, compareBarToPos);

    if (startDeletePoint != m_barLines.end())
        kdDebug(KDEBUG_AREA) << "startDeletePoint pos : "
                             << (*startDeletePoint)->x() << endl;

    // delete the canvas lines
    for (barlines::iterator i = startDeletePoint; i != m_barLines.end(); ++i)
        delete (*i);

    m_barLines.erase(startDeletePoint, m_barLines.end());
}

void NotationStaff::deleteBars()
{
    kdDebug(KDEBUG_AREA) << "NotationStaff::deleteBars()\n";
    
    for (barlines::iterator i = m_barLines.begin(); i != m_barLines.end(); ++i)
        delete (*i);

    m_barLines.clear();
}

void NotationStaff::setLines(double xfrom, double xto)
{
    for (barlines::iterator i = m_staffLines.begin();
         i != m_staffLines.end(); ++i) {

        QPoint p = (*i)->startPoint();
        (*i)->setPoints((int)xfrom - 4, p.y(), (int)xto, p.y());
    }

    QPoint sp = m_initialBarA->startPoint();
    QPoint ep = m_initialBarA->endPoint();

    m_initialBarA->setPoints((int)xfrom - 4, sp.y(), (int)xfrom - 4, ep.y());
    m_initialBarB->setPoints((int)xfrom, sp.y(), (int)xfrom, ep.y());
}

const int NotationStaff::nbLines = 5;
const int NotationStaff::linesOffset = 40;

