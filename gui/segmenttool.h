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

#ifndef SEGMENTTOOL_H
#define SEGMENTTOOL_H

#include <qobject.h>

#include "Track.h"
#include "Event.h"
#include "Segment.h"
#include "Selection.h"

class SegmentCanvas;
class RosegardenGUIDoc;
class RosegardenGUIApp;
class SegmentItem;
class KCommand;
class QCanvasRectangle;
class QPopupMenu;  // LDB

namespace Rosegarden { class RulerScale; }

//////////////////////////////////////////////////////////////////////
//                 Segment Tools
//////////////////////////////////////////////////////////////////////

class SegmentTool : public QObject
{
public:
    SegmentTool(SegmentCanvas*, RosegardenGUIDoc *doc);
    virtual ~SegmentTool();

    /**
     * handleMouseMove() will return a OR-ed combination of these
     */
    enum {
        NoFollow = 0x0,
        FollowHorizontal = 0x1,
        FollowVertical = 0x2
    };


    virtual void handleRightButtonPress(QMouseEvent*);
    virtual void handleMouseButtonPress(QMouseEvent*)     = 0;
    virtual void handleMouseButtonRelease(QMouseEvent*)   = 0;
    virtual int  handleMouseMove(QMouseEvent*)            = 0;


    /**
     * Show the menu if there is one
     */
    virtual void showMenu();

    void addCommandToHistory(KCommand *command);

protected:

    void createMenu();

    //--------------- Data members ---------------------------------

    SegmentCanvas*  m_canvas;
    SegmentItem* m_currentItem;
    RosegardenGUIDoc* m_doc;
    QPopupMenu* m_menu;
};

//////////////////////////////
// SegmentPencil
//////////////////////////////

class SegmentPencil : public SegmentTool
{
    Q_OBJECT
public:
    SegmentPencil(SegmentCanvas*, RosegardenGUIDoc*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

protected:
    //--------------- Data members ---------------------------------

    bool m_newRect;
    Rosegarden::TrackId m_track;
    Rosegarden::timeT m_startTime;
    Rosegarden::timeT m_endTime;
};

class SegmentEraser : public SegmentTool
{
    Q_OBJECT
public:
    SegmentEraser(SegmentCanvas*, RosegardenGUIDoc*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

};

class SegmentMover : public SegmentTool
{
    Q_OBJECT
public:
    SegmentMover(SegmentCanvas*, RosegardenGUIDoc*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

private:

    //--------------- Data members ---------------------------------

    QPoint            m_clickPoint;
    double            m_currentItemStartX;

    QCanvasRectangle *m_foreGuide;
    QCanvasRectangle *m_topGuide;

};

/**
 * Segment Resizer tool. Allows resizing only at the end of the segment part
 */
class SegmentResizer : public SegmentTool
{
    Q_OBJECT
public:
    SegmentResizer(SegmentCanvas*, RosegardenGUIDoc*, int edgeThreshold = 10);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

    static bool cursorIsCloseEnoughToEdge(SegmentItem*, QMouseEvent*, int);

protected:
    //--------------- Data members ---------------------------------

    int m_edgeThreshold;
};

class SegmentSelector : public SegmentTool
{
    Q_OBJECT
public:
    SegmentSelector(SegmentCanvas*, RosegardenGUIDoc*);
    virtual ~SegmentSelector();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

    // Clear all Segments in our vector and on the view
    //
    void clearSelected();

    // Remove the given Segment from the selection, if it's in it
    // 
    void removeFromSelection(Rosegarden::Segment *);

    // Add the given Segment to the selection, if we have a SegmentItem for it
    // 
    void addToSelection(Rosegarden::Segment *);

    // These two alter the behaviour of the selection mode
    //
    // - SegmentAdd (usually when SHIFT is held down) allows
    //   multiple selections of Segments.
    //
    // - SegmentCopy (usually CONTROL) allows draw and drop
    //   copying of Segments - it's a quick shortcut
    //
    void setSegmentAdd(const bool &value)  { m_segmentAddMode = value; }
    void setSegmentCopy(const bool &value) { m_segmentCopyMode = value; }

    // Return a set of selected Segments
    //
    Rosegarden::SegmentSelection getSelectedSegments();

    /**
     * Sets the selector to "greedy" mode
     *
     * In greedy mode, elements which merely touch the edges of the
     * selection rectangle are selected. Otherwise, elements have to
     * actually be fully included in the rectangle to be selected.
     *
     * @see #isGreedy
     */
    static void setGreedyMode(bool s) { m_greedy = s; }

    /**
     * Returns whether the selector is in "greedy" mode
     *
     * @see #setGreedy
     */
    static bool isGreedy() { return m_greedy; }

public slots:
    void slotSelectSegmentItem(SegmentItem *selectedItem);

    /**
     * Connected to the destroyed() signal of the selected segment items
     *
     * This is for maintaining the list of selected items
     */
    void slotDestroyedSegmentItem(QObject*);

signals:
    void selectedSegments(const Rosegarden::SegmentSelection &);

protected:
    void addToSelection(SegmentItem*);

    typedef std::pair<QPoint, SegmentItem *> SegmentItemPair;
    typedef std::vector<SegmentItemPair> SegmentItemList;

    //--------------- Data members ---------------------------------

    SegmentItemList m_selectedItems;

    bool m_segmentAddMode;
    bool m_segmentCopyMode;
    QPoint m_clickPoint;
    bool m_segmentQuickCopyDone;
    bool m_passedInertiaEdge;

    SegmentTool *m_dispatchTool;

    static bool m_greedy;

    QCanvasRectangle *m_foreGuide;
    QCanvasRectangle *m_topGuide;

};


class SegmentSplitter : public SegmentTool
{
    Q_OBJECT
public:
    SegmentSplitter(SegmentCanvas*, RosegardenGUIDoc*);
    virtual ~SegmentSplitter();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

    // don't do double clicks
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

private:
    void drawSplitLine(QMouseEvent*);
    void splitSegment(Rosegarden::Segment *segment,
                      Rosegarden::timeT &splitTime);
};

class SegmentJoiner : public SegmentTool
{
    Q_OBJECT
public:
    SegmentJoiner(SegmentCanvas*, RosegardenGUIDoc*);
    virtual ~SegmentJoiner();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);
 
    // don't do double clicks
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

private:

};


//////////////////////////////////////////////////////////////////////
//                SegmentItemPreview
//////////////////////////////////////////////////////////////////////
class SegmentItemPreview
{
public:
    SegmentItemPreview(SegmentItem& parent,
                       Rosegarden::RulerScale* scale);
    virtual ~SegmentItemPreview();

    virtual void drawShape(QPainter&) = 0;

    /**
     * Returns whether the preview shape shown in the segment needs
     * to be refreshed
     */
    bool isPreviewCurrent()        { return m_previewIsCurrent; }

    /**
     * Sets whether the preview shape shown in the segment needs
     * to be refreshed
     */
    void setPreviewCurrent(bool c) { m_previewIsCurrent = c; }

    QRect rect();
    
protected:
    virtual void updatePreview() = 0;

    //--------------- Data members ---------------------------------

    SegmentItem& m_parent;
    Rosegarden::Segment *m_segment;
    Rosegarden::RulerScale *m_rulerScale;

    bool m_previewIsCurrent;
};


#endif
