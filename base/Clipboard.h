
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#ifndef _CLIPBOARD_H_
#define _CLIPBOARD_H_

#include <set>
#include "Segment.h"

namespace Rosegarden
{

/**
 * Simple container for segments, that can serve as a clipboard for
 * editing operations.  Conceptually it has two "modes",
 * single-segment and multiple-segment, although there's no particular
 * distinction behind the scenes.  The Clipboard owns all the segments
 * it contains -- they should always be deep copies, not aliases.
 */

class Clipboard
{
public:
    typedef std::multiset<Segment *, Segment::SegmentCmp> segmentcontainer;
    typedef segmentcontainer::iterator iterator;
    typedef segmentcontainer::const_iterator const_iterator;

    Clipboard();
    Clipboard(const Clipboard &);
    Clipboard &operator=(const Clipboard &);
    virtual ~Clipboard();

    /**
     * Empty the clipboard.
     */
    void clear();
 
    /**
     * Return true if the clipboard is empty.
     */
    bool isEmpty() const;

    iterator       begin()       { return m_segments.begin(); }
    const_iterator begin() const { return m_segments.begin(); }
    iterator       end()         { return m_segments.end(); }
    const_iterator end() const   { return m_segments.end(); }
    
    /**
     * Return true if the clipboard only contains a single segment.
     * Single-segment and multi-segment are conceptually rather
     * separate -- for example, you can only paste into a segment
     * from a single-segment clipboard.
     */
    bool isSingleSegment() const;

    /**
     * Return the single segment contained by the clipboard.
     * If the clipboard is empty or contains more than one segment,
     * returns null.  (Use the iterator accessors begin()/end() to
     * read from a clipboard for which isSingleSegment is false.)
     */
    Segment *getSingleSegment() const;

    /**
     * Add a new empty segment to the clipboard, and return a
     * pointer to it.  (The clipboard retains ownership.)
     */
    Segment *newSegment();

    /**
     * Add a new segment to the clipboard, containing copies of
     * the events in copyFrom.  (The clipboard retains ownership
     * of the new segment.)
     */
    Segment *newSegment(const Segment *copyFrom);

    /**
     * Add a new segment to the clipboard, containing copies of
     * the events in copyFrom found between from and to.
     * (The clipboard retains ownership of the new segment.)
     */
    Segment *newSegment(const Segment *copyFrom, timeT from, timeT to);

    /**
     * Clear the current clipboard and re-fill it by copying from c.
     */
    void copyFrom(const Clipboard *c);

private:
    segmentcontainer m_segments;
};

}

#endif
