
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2013 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COMPOSITIONITEM_H
#define RG_COMPOSITIONITEM_H

#include "CompositionRect.h"

#include <QObject>
#include <QRect>
#include <QPointer>

namespace Rosegarden
{


class Segment;

/// Representation of segments that are changing.
/**
 * When segments are being selected, moved, or resized, CompositionModelImpl
 * creates CompositionItem objects to represent those changing segments
 * as they change.
 *
 * All these accessors and mutators strike me as being rather unsavory.
 * Might want to just turn this into a wide-open struct.  That's essentially
 * what it is.  rect() could be a helper function, and m_savedRect could
 * be kept private since its mutator is interesting.
 * It would be a lot easier to understand.
 */
class CompositionItem : public QObject {
public:
    CompositionItem(Segment &s, const CompositionRect &r);

    // Rect Mutators

    void setX(int x)                   { m_rect.setX(x); }
    void setY(int y)                   { m_rect.setY(y); }
    void setWidth(int w)               { m_rect.setWidth(w); }
    void setZ(unsigned int z)          { m_z = z; }

    // ??? Can this be implemented as:
    //     m_rect.translate(x, y);
    void moveTo(int x, int y)          { m_rect.setRect(x, y, m_rect.width(), m_rect.height()); }

    // Rect Accessors

    // rename: baseRect()?  Since it has only the baseWidth().
    QRect rect() const;
    int x() const                      { return m_rect.x(); }
    int y() const                      { return m_rect.y(); }
    unsigned int z() const             { return m_z; }
    bool isRepeating() const           { return m_rect.isRepeating(); }
    CompositionRect& getCompRect()     { return m_rect; }

    // Access to the contained segment
    Segment *getSegment()              { return &m_segment; }
    const Segment *getSegment() const  { return &m_segment; }

    // Saved rect.  Used to store the original rect before changing it.
    void saveRect()                    { m_savedRect = rect(); }
    QRect savedRect() const            { return m_savedRect; }

private:

    Segment &m_segment;
    CompositionRect m_rect;
    unsigned int m_z;

    QRect m_savedRect;
};

// ??? Need to clean up users of this.  They are doing things like passing
//     references to this, which is pretty hard to follow.
typedef QPointer<CompositionItem> CompositionItemPtr;


}

#endif
