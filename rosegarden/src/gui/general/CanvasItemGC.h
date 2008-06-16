
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_CANVASITEMGC_H_
#define _RG_CANVASITEMGC_H_

#include <vector>


class QCanvasItem;


namespace Rosegarden
{



/**
 * A pseudo GC in which CanvasItems whose ownership isn't clear cut
 * can be put for periodical removal.
 *
 * This is especially for SegmentItems which can put their repeat
 * rectangles when they're being deleted.
 *
 * The problem this solves is a classic ownership/double deletion
 * case. The SegmentCanvas deletes all its items on destruction. But
 * the SegmentItems have an auxiliary "repeat rectangle" which is a
 * QCanvasRectangle, that needs to be deleted when the SegmentItem is
 * itself deleted.
 *
 * However, if the SegmentItem deletes its repeat rectangle, then when
 * the SegmentCanvas destruction occurs, the SegmentCanvas dtor will
 * get a list of all its children (QCanvas::allItems()), containing
 * both SegmentItems and their repeat rectangles. Deleting a
 * SegmentItem will delete its repeat rectangle, which will still be
 * present in the all children list which the SegmentCanvas dtor is
 * iterating over.
 * 
 * So a solution is simply to push to-be-deleted repeat rectangles on
 * this GC, which should be processed on canvas updates, for instance.
 *
 */
class CanvasItemGC
{
public:
    /// mark the given item for GC
    static void mark(QCanvasItem*);

    /// GC all marked items
    static void gc();

    /// Forget all marked items - don't delete them
    static void flush();

protected:
    static std::vector<QCanvasItem*> m_garbage;
};



}

#endif
