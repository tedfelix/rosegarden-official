/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CompositionItemImpl.h"

#include "misc/Debug.h"
#include "base/Segment.h"
#include "CompositionRect.h"
#include <QBrush>
#include <QColor>
#include <QPen>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>


namespace Rosegarden
{

CompositionItemImpl::CompositionItemImpl(Segment& s, const CompositionRect& rect)
        : m_segment(s),
        m_rect(rect),
        m_z(0)
{}

QRect CompositionItemImpl::rect() const
{
    QRect res = m_rect;
    if (m_rect.isRepeating()) {
        CompositionRect::repeatmarks repeatMarks = m_rect.getRepeatMarks();
        int neww = m_rect.getBaseWidth();

        //         RG_DEBUG << "CompositionItemImpl::rect() -  width = "
        //                  << m_rect.width() << " - base w = " << neww << endl;
        res.setWidth(neww);
    } else {
        //         RG_DEBUG << "CompositionItemImpl::rect() m_rect not repeating\n";
    }


    return res;
}

}
