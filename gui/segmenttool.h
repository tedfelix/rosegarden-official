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

#include "edittool.h"

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

class SegmentTool : public BaseTool
{
public:
    friend class SegmentToolBox;

    virtual ~SegmentTool();

    /**
     * Is called by the parent View (EditView or SegmentCanvas) when
     * the tool is set as current.
     * Add any setup here
     */
    virtual void ready();

    virtual void handleRightButtonPress(QMouseEvent*);
    virtual void handleMouseButtonPress(QMouseEvent*)     = 0;
    virtual void handleMouseButtonRelease(QMouseEvent*)   = 0;
    virtual int  handleMouseMove(QMouseEvent*)            = 0;

    void addCommandToHistory(KCommand *command);

protected:
    SegmentTool(SegmentCanvas*, RosegardenGUIDoc *doc);

    virtual void createMenu();
    SegmentToolBox* getToolBox();

    //--------------- Data members ---------------------------------

    SegmentCanvas*  m_canvas;
    SegmentItem* m_currentItem;
    RosegardenGUIDoc* m_doc;
};

class SegmentToolBox : public BaseToolBox
{
public:
    SegmentToolBox(SegmentCanvas* parent, RosegardenGUIDoc*);

    virtual SegmentTool* getTool(const QString& toolName);
    
protected:
    virtual SegmentTool* createTool(const QString& toolName);

    //--------------- Data members ---------------------------------

    SegmentCanvas* m_canvas;
    RosegardenGUIDoc* m_doc;
};

//////////////////////////////
// SegmentPencil
//////////////////////////////

class SegmentPencil : public SegmentTool
{
    Q_OBJECT

    friend class SegmentToolBox;
    friend class SegmentSelector;

public:

    virtual void ready();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

    static const QString ToolName;

protected:
    SegmentPencil(SegmentCanvas*, RosegardenGUIDoc*);

    //--------------- Data members ---------------------------------

    bool m_newRect;
    Rosegarden::TrackId m_track;
    Rosegarden::timeT m_startTime;
    Rosegarden::timeT m_endTime;
};

class SegmentEraser : public SegmentTool
{
    Q_OBJECT

    friend class SegmentToolBox;

public:

    virtual void ready();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

    static const QString ToolName;

protected:
    SegmentEraser(SegmentCanvas*, RosegardenGUIDoc*);
};

class SegmentMover : public SegmentTool
{
    Q_OBJECT

    friend class SegmentToolBox;

public:

    virtual void ready();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

    static const QString ToolName;

protected:
    SegmentMover(SegmentCanvas*, RosegardenGUIDoc*);

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

    friend class SegmentToolBox;
    friend class SegmentSelector;

public:

    virtual void ready();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

    static bool cursorIsCloseEnoughToEdge(SegmentItem*, QMouseEvent*, int);

    void setEdgeThreshold(int e) { m_edgeThreshold = e; }
    int getEdgeThreshold() { return m_edgeThreshold; }

    static const QString ToolName;

protected:
    SegmentResizer(SegmentCanvas*, RosegardenGUIDoc*, int edgeThreshold = 10);

    //--------------- Data members ---------------------------------

    int m_edgeThreshold;
};

class SegmentSelector : public SegmentTool
{
    Q_OBJECT

    friend class SegmentToolBox;
    friend class SegmentTool;

public:

    virtual ~SegmentSelector();

    virtual void stow();

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

    static const QString ToolName;

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
    SegmentSelector(SegmentCanvas*, RosegardenGUIDoc*);

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

    friend class SegmentToolBox;

public:

    virtual ~SegmentSplitter();

    virtual void ready();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

    // don't do double clicks
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

    static const QString ToolName;

protected:
    SegmentSplitter(SegmentCanvas*, RosegardenGUIDoc*);
    
    void drawSplitLine(QMouseEvent*);
    void splitSegment(Rosegarden::Segment *segment,
                      Rosegarden::timeT &splitTime);
};

class SegmentJoiner : public SegmentTool
{
    Q_OBJECT

    friend class SegmentToolBox;

public:

    virtual ~SegmentJoiner();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);
 
    // don't do double clicks
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

    static const QString ToolName;

protected:
    SegmentJoiner(SegmentCanvas*, RosegardenGUIDoc*);
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
