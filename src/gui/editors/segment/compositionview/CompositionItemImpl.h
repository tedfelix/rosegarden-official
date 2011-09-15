
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

#ifndef _RG_COMPOSITIONITEMIMPL_H_
#define _RG_COMPOSITIONITEMIMPL_H_

#include "CompositionRect.h"
#include "CompositionItem.h"
#include <QRect>




namespace Rosegarden
{

class Segment;

/// Representation of segments that are changing.
/**
 * When segments are being selected, moved, or resized, CompositionModelImpl
 * creates CompositionItemImpl objects to represent those changing segments
 * as they change.
 */
class CompositionItemImpl : public _CompositionItem {
public:
    CompositionItemImpl(Segment& s, const CompositionRect&);
    virtual bool isRepeating() const              { return m_rect.isRepeating(); }
    virtual QRect rect() const;
    virtual void translate(int x, int y)          { m_rect.translate(x, y); }
    virtual void moveTo(int x, int y)             { m_rect.setRect(x, y, m_rect.width(), m_rect.height()); }
    virtual void setX(int x)                      { m_rect.setX(x); }
    virtual void setY(int y)                      { m_rect.setY(y); }
    virtual void setZ(unsigned int z)             { m_z = z; }
    virtual int x()                               { return m_rect.x(); }
    virtual int y()                               { return m_rect.y(); }
    virtual unsigned int z()                      { return m_z; }
    virtual void setWidth(int w)                  { m_rect.setWidth(w); }
    // use segment address as hash key
    virtual long hashKey()                        { return (long)getSegment(); }

    Segment* getSegment()             { return &m_segment; }
    const Segment* getSegment() const { return &m_segment; }
    CompositionRect& getCompRect()                { return m_rect; }

protected:

    //--------------- Data members ---------------------------------
    Segment& m_segment;
    CompositionRect m_rect;
    unsigned int m_z;
};


}

#endif
