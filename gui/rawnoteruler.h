// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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


#ifndef _RAWNOTERULER_H_
#define _RAWNOTERULER_H_

#include <qwidget.h>
#include <vector>

#include "Segment.h"

namespace Rosegarden {
    class RulerScale;
    class Segment;
}

class QPainter;
class DefaultVelocityColour;


/**
 * RawNoteRuler is a ruler that shows in a vaguely matrix-like fashion
 * when notes start and end, for use with a notation view that can't
 * otherwise show this relatively precise unquantized information.
 * It has no editing function (yet?)
 */

class RawNoteRuler : public QWidget
{
    Q_OBJECT

public:
    RawNoteRuler(Rosegarden::RulerScale *rulerScale,
		 Rosegarden::Segment *segment,
		 double xorigin = 0.0,
		 int height = 0,
		 QWidget* parent = 0,
		 const char *name = 0);

    ~RawNoteRuler();

    void setCurrentSegment(Rosegarden::Segment *segment) {
	m_segment = segment;
    }

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void setMinimumWidth(int width) { m_width = width; }

public slots:
    void slotScrollHoriz(int x);

protected:
    virtual void paintEvent(QPaintEvent*);

private:
    double m_xorigin;
    int  m_height;
    int  m_currentXOffset;
    int  m_width;

    Rosegarden::Segment *m_segment;
    Rosegarden::RulerScale *m_rulerScale;

    struct EventTreeNode
    {
	typedef std::vector<EventTreeNode *> NodeList;
	
	EventTreeNode(Rosegarden::Segment::iterator n) : node(n) { }
	~EventTreeNode() {
	    for (NodeList::iterator i = children.begin();
		 i != children.end(); ++i) {
		delete *i;
	    }
	}

	int getDepth();
	int getChildrenAboveOrBelow(bool below = false, int pitch = -1);
	
	Rosegarden::Segment::iterator node;
	NodeList children;
    };

    std::pair<Rosegarden::timeT, Rosegarden::timeT> getExtents(Rosegarden::Segment::iterator);
    void addChildren(Rosegarden::Segment *, Rosegarden::Segment::iterator, Rosegarden::timeT, EventTreeNode *);
    void dumpSubtree(EventTreeNode *, int);
    void dumpForest(std::vector<EventTreeNode *> *);
    void buildForest(Rosegarden::Segment *, Rosegarden::Segment::iterator, Rosegarden::Segment::iterator);

    void drawNode(QPainter &, DefaultVelocityColour &, EventTreeNode *,
		  double height, double yorigin);

    // needs to be class with dtor &c and containing above methods
    EventTreeNode::NodeList m_forest;
};

#endif // _RAWNOTERULER_H_
