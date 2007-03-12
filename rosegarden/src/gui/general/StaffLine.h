
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

#ifndef _RG_STAFFLINE_H_
#define _RG_STAFFLINE_H_

#include "gui/kdeext/QCanvasGroupableItem.h"
#include <qpen.h>


class QCanvasItemGroup;
class QCanvas;


namespace Rosegarden
{



/**
 * A staff line
 *
 * A groupable line which can be "highlighted"
 * (drawn with a different color)
 */
class StaffLine : public QCanvasLineGroupable
{
public:
    StaffLine(QCanvas *c, QCanvasItemGroup *g, int height);

    enum { NoHeight = -150 };
 
    void setHeight(int h) { m_height = h; }
    int  getHeight() const { return m_height; }

    void setSignificant(bool s) { m_significant = s; }
    bool isSignificant() const { return m_significant; }

    /**
     * "highlight" the line (set its pen to red)
     */
    void setHighlighted(bool);

protected:
    //--------------- Data members ---------------------------------

    int m_height;
    bool m_significant;

    QPen m_normalPen;
};


}

#endif
