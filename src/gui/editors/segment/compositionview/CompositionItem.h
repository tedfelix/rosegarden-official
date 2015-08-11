
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

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
#include "base/TimeT.h"  // timeT

#include <QObject>
#include <QRect>
#include <QPointer>

namespace Rosegarden
{


class Segment;
class SnapGrid;


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
 *
 * ??? rename: ChangingSegment
 */
class CompositionItem : public QObject {
public:
    CompositionItem(Segment &s, const CompositionRect &r);

    // Rect Mutators

    void setX(int x)                   { m_rect.setX(x); }
    void setY(int y)                   { m_rect.setY(y); }
    void moveTo(int x, int y)          { m_rect.moveTo(x, y); }
    void setWidth(int w)               { m_rect.setWidth(w); }
    void setZ(unsigned int z)          { m_z = z; }

    // Rect Accessors

    // rename: baseRect()?  Since it has only the baseWidth().
    QRect rect() const;
    int x() const                      { return m_rect.x(); }
    int y() const                      { return m_rect.y(); }
    unsigned int z() const             { return m_z; }
    bool isRepeating() const           { return m_rect.isRepeating(); }
    CompositionRect& getCompRect()     { return m_rect; }

    /// Get the start time of the repeat nearest the point.
    /**
     * Used by CompositionView to determine the time at which to edit a repeat.
     */
    timeT getRepeatTimeAt(const SnapGrid &, const QPoint &);

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

// ??? A QSharedPointer would be a better approach.
// ??? rename: ChangingSegmentPtr
typedef QPointer<CompositionItem> CompositionItemPtr;


}

#endif
