
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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
#include <QSize>
#include <QWidget>
#include <utility>
#include <vector>
#include "base/Event.h"


class QPaintEvent;
class QPainter;


namespace Rosegarden
{

class RulerScale;
class DefaultVelocityColour;


/**
 * RawNoteRuler is a ruler that shows in a vaguely matrix-like fashion
 * when notes start and end, for use with a notation view that can't
 * otherwise show this relatively precise unquantized information.
 * It has no editing function (yet?)
 */

class RawNoteRuler : public QWidget, public SegmentObserver
{
    Q_OBJECT

public:
    RawNoteRuler(RulerScale *rulerScale,
                 Segment *segment,
                 int height = 0,
                 QWidget* parent = nullptr);

    ~RawNoteRuler() override;

    void setCurrentSegment(Segment *segment);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void setMinimumWidth(int width) { m_width = width; }


/** SegmentObserver methods : **/

// Used to update the ruler when notes are moved around or deleted
    void eventAdded(const Segment *, Event *) override { update(); }
    void eventRemoved(const Segment *, Event *) override { update(); }

    void segmentDeleted(const Segment *) override;


public slots:
    void slotScrollHoriz(int x);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    int  m_height;
    int  m_currentXOffset;
    int  m_width;

    Segment *m_segment;
    RulerScale *m_rulerScale;

    struct EventTreeNode
    {
        typedef std::vector<EventTreeNode *> NodeList;

        explicit EventTreeNode(Segment::iterator n) : node(n) { }
        ~EventTreeNode() {
            for (NodeList::iterator i = children.begin();
                 i != children.end(); ++i) {
                delete *i;
            }
        }

        int getDepth();
        int getChildrenAboveOrBelow(bool below = false, int p = -1);

        Segment::iterator node;
        NodeList children;
    };

    std::pair<timeT, timeT> getExtents(Segment::iterator);
    Segment::iterator addChildren(Segment *, Segment::iterator, timeT, EventTreeNode *);
    void dumpSubtree(EventTreeNode *, int);
    void dumpForest(std::vector<EventTreeNode *> *);
    void buildForest(Segment *, Segment::iterator, Segment::iterator);

    void drawNode(QPainter &, DefaultVelocityColour &, EventTreeNode *,
                  double height, double yorigin);

    // needs to be class with dtor &c and containing above methods
    EventTreeNode::NodeList m_forest;
};


}

#endif
