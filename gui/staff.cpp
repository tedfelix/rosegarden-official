
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

#include "staff.h"
#include "staffline.h"
#include "qcanvasspritegroupable.h"

#include "rosedebug.h"

Staff::Staff(QCanvas *canvas)
    : QCanvasItemGroup(canvas),
      m_barLineHeight(0),
      m_horizLineLength(0)
{
    // horizontal lines

    int w = canvas->width();
    m_horizLineLength = w - (w / 10);

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
    }

    //
    // Add vertical lines
    //
    QCanvasLineGroupable *staffVertLine = new QCanvasLineGroupable(canvas, this);

    m_barLineHeight = nbLines * lineWidth - lineWidth / 2 - 5;

    // First line - thick
    //
    QPen pen(black, 3);
    pen.setCapStyle(Qt::SquareCap);
    staffVertLine->setPen(pen);

    staffVertLine->setPoints(0,linesOffset + 1,
                             0,m_barLineHeight + linesOffset - 1);

    // Second line - thin
    //
    staffVertLine = new QCanvasLineGroupable(canvas, this);

    staffVertLine->setPoints(4,linesOffset,
                             4,m_barLineHeight + linesOffset);


    setActive(false);  // don't react to mousePress events

}

Staff::~Staff()
{
    // TODO : this causes a crash on quit - don't know why
//     for (barlines::iterator i = m_barLines.begin(); i != m_barLines.end(); ++i)
//         delete (*i);
}

int Staff::yCoordOfHeight(int h) const
{
    // 0 is bottom staff-line, 8 is top one
    int y = ((8 - h) * lineWidth) / 2 + linesOffset + ((h % 2 == 1) ? 1 : 0);
    kdDebug(KDEBUG_AREA) << "Staff::yCoordOfHeight: height is " << h
                         << ", lineWidth is " << lineWidth
                         << ", linesOffset is " << linesOffset
                         << ", y is " << y << endl;
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

void
Staff::insertBar(unsigned int barPos, bool correct)
{
    kdDebug(KDEBUG_AREA) << "Staff::insertBar(" << barPos << ")\n";

    QCanvasLineGroupable* barLine = new QCanvasLineGroupable(canvas(), this);

    barLine->setPoints(0, linesOffset,
                       0, barLineHeight() + linesOffset);
    barLine->moveBy(barPos + x(), y());
    if (!correct) barLine->setPen(QPen(red, 1));
    barLine->show();

    barlines::iterator insertPoint = lower_bound(m_barLines.begin(),
                                                 m_barLines.end(),
                                                 barLine, compareBarPos);

    m_barLines.insert(insertPoint, barLine);
}

void
Staff::deleteBars(unsigned int fromPos)
{
    kdDebug(KDEBUG_AREA) << "Staff::deleteBars from " << fromPos << endl;

    barlines::iterator startDeletePoint = lower_bound(m_barLines.begin(),
                                                      m_barLines.end(),
                                                      fromPos, compareBarToPos);

    if (startDeletePoint != m_barLines.end())
        kdDebug(KDEBUG_AREA) << "startDeletePoint pos : "
                             << (*startDeletePoint)->x() << endl;

    // delete the canvas lines
    for (barlines::iterator i = startDeletePoint; i != m_barLines.end(); ++i)
        delete (*i);

    m_barLines.erase(startDeletePoint, m_barLines.end());
}

void
Staff::deleteBars()
{
    kdDebug(KDEBUG_AREA) << "Staff::deleteBars()\n";
    
    for (barlines::iterator i = m_barLines.begin(); i != m_barLines.end(); ++i)
        delete (*i);

    m_barLines.clear();
}


const int Staff::noteHeight = 8;
const int Staff::lineWidth = noteHeight + 1;
const int Staff::noteWidth = 9;
const int Staff::accidentWidth = 6;
const int Staff::stalkLen = noteHeight * 7/2 - 6;
const int Staff::nbLines = 5;
const int Staff::linesOffset = 8;

