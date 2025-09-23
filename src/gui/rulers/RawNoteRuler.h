
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_RAWNOTERULER_H
#define RG_RAWNOTERULER_H

#include "base/Segment.h"
#include "base/TimeT.h"

#include <QSize>
#include <QWidget>

class QPaintEvent;
class QPainter;

#include <utility>
#include <vector>


namespace Rosegarden
{


class RulerScale;


/**
 * RawNoteRuler is a ruler that shows in a vaguely matrix-like fashion
 * when notes start and end, for use with a notation view that can't
 * otherwise show this relatively precise unquantized information.
 * It has no editing function.
 *
 * The RawNoteRuler shows notes as stacked voices.  This can help with
 * teasing apart voices into separate segments to get notation to look
 * right when printed.
 */
class RawNoteRuler : public QWidget, public SegmentObserver
{
    Q_OBJECT

public:

    RawNoteRuler(RulerScale *rulerScale,
                 Segment *segment,
                 int height);

    ~RawNoteRuler() override;

    void setCurrentSegment(Segment *segment);
    void scrollHoriz(int x);

    // QWidget overrides.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // SegmentObserver overrides
    void eventAdded(const Segment *, Event *) override  { update(); }
    void eventRemoved(const Segment *, Event *) override  { update(); }
    void segmentDeleted(const Segment *) override;

protected:

    // QWidget override
    void paintEvent(QPaintEvent *paintEvent2) override;

private:

    RulerScale *m_rulerScale;
    Segment *m_segment;
    int m_height;

    /// Horizontal scroll offset.  E.g. -100 if we are scrolled right 100 pixels.
    int m_currentXOffset{0};

    struct EventTreeNode;
    typedef std::vector<EventTreeNode *> EventTreeNodeList;

    struct EventTreeNode
    {
        explicit EventTreeNode(Segment::iterator n) : node(n) { }
        ~EventTreeNode() {
            for (const EventTreeNode *node : children) {
                delete node;
            }
        }

        int getDepth() const;
        int getChildrenAboveOrBelow(bool below, int p = -1) const;

        Segment::iterator node;
        EventTreeNodeList children;
    };

    std::pair<timeT, timeT> getExtents(Segment::const_iterator i);
    Segment::iterator addChildren(const Segment *s,
                                  Segment::iterator to,
                                  timeT rightBound,
                                  EventTreeNode *node);

    void drawNode(QPainter &painter,
                  const EventTreeNode *node,
                  double height,
                  double yorigin);

    EventTreeNodeList m_forest;
    void buildForest(const Segment *s, Segment::iterator from, Segment::iterator to);

    // DEBUG
    // ??? Move private statics to .cpp.
    static void dumpSubtree(const EventTreeNode *node, int depth);
    static void dumpForest(const EventTreeNodeList *forest);
};


}

#endif
