/***************************************************************************
                          staffline.h  -  description
                             -------------------
    begin                : Thu Sep 28 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef STAFFLINE_H
#define STAFFLINE_H

#include <qcanvaslinegroupable.h>

/**
 * A staff line as a canvas item
 *
 *@author Guillaume Laurent, Chris Cannam, Rich Bown
 */
class StaffLine : public QCanvasLineGroupable
{
public:
    StaffLine(QCanvas *c, QCanvasItemGroup *g)
        : QCanvasLineGroupable(c, g), m_pitch(0)
    {}

    /**
     * sets the pitch this staff line corresponds to (e.g. the pitch
     * of a natural note which would be on this line)
     */
    void setAssociatedPitch(unsigned int p) { m_pitch = p; }

    /// returns the pitch this staff line corresponds to
    unsigned int associatedPitch() const { return m_pitch; }

protected:

    unsigned int m_pitch;
};

#endif
