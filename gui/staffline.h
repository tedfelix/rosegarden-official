
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

#ifndef STAFFLINE_H
#define STAFFLINE_H

#include <qcanvaslinegroupable.h>

/**
 * A staff line as a canvas item
 *
 *@author Guillaume Laurent, Chris Cannam, Richard Bown
 */
class StaffLine : public QCanvasLineGroupable
{
public:
    StaffLine(QCanvas *c, QCanvasItemGroup *g, int height);
 
    void setHeight(int h) { m_height = h; }
    int getHeight() const { return m_height; }

    /**
     * "highlight" the line (set its pen to red)
     */
    void setHighlighted(bool);

protected:
    int m_height;
    int m_y;

    QPen m_normalPen;
};

#endif
