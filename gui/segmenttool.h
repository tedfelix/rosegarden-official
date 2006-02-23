// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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
#include "compositionitem.h"

namespace Rosegarden { class SegmentSelection; class Segment; }
class RosegardenGUIDoc;
class RosegardenGUIApp;
class KCommand;
class QPopupMenu;  // LDB
class CompositionView;

namespace Rosegarden { class RulerScale; }

class SegmentToolBox;

//////////////////////////////////////////////////////////////////////
//                 Segment Tools
//////////////////////////////////////////////////////////////////////

class SegmentToolBox;
class SegmentSelector;

// Allow the tools to share the Selector tool's selection
// through these.
//
typedef std::pair<QPoint, CompositionItem> SegmentItemPair;
typedef std::vector<SegmentItemPair> SegmentItemList;

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
    SegmentTool(CompositionView*, RosegardenGUIDoc *doc);

    virtual void createMenu();
    virtual bool hasMenu() { return true; }
    
    void setCurrentItem(CompositionItem item) { if (item != m_currentItem) { delete m_currentItem; m_currentItem = item; } }

    SegmentToolBox* getToolBox();

    bool changeMade() { return m_changeMade; }
    void setChangeMade(bool c) { m_changeMade = c; }

    //--------------- Data members ---------------------------------

    CompositionView*  m_canvas;
    CompositionItem   m_currentItem;
    RosegardenGUIDoc* m_doc;
    QPoint            m_origPos;
    bool              m_changeMade;
};

class SegmentToolBox : public BaseToolBox
{
public:
    SegmentToolBox(CompositionView* parent, RosegardenGUIDoc*);

    virtual SegmentTool* getTool(const QString& toolName);
    
protected:
    virtual SegmentTool* createTool(const QString& toolName);

    //--------------- Data members ---------------------------------

    CompositionView* m_canvas;
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
    virtual void stow();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

    static const QString ToolName;

protected slots:
    void slotCanvasScrolled(int newX, int newY);
    
protected:
    SegmentPencil(CompositionView*, RosegardenGUIDoc*);

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
    SegmentEraser(CompositionView*, RosegardenGUIDoc*);
};

class SegmentMover : public SegmentTool
{
    Q_OBJECT

    friend class SegmentToolBox;

public:

    virtual void ready();
    virtual void stow();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

    static const QString ToolName;

protected slots:
    void slotCanvasScrolled(int newX, int newY);

protected:
    SegmentMover(CompositionView*, RosegardenGUIDoc*);

    //--------------- Data members ---------------------------------

    QPoint            m_clickPoint;
    bool              m_passedInertiaEdge;
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
    virtual void stow();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

    static bool cursorIsCloseEnoughToEdge(const CompositionItem&, const QPoint&, int, bool &);

    void setEdgeThreshold(int e) { m_edgeThreshold = e; }
    int getEdgeThreshold() { return m_edgeThreshold; }

    static const QString ToolName;

protected slots:
    void slotCanvasScrolled(int newX, int newY);

protected:
    SegmentResizer(CompositionView*, RosegardenGUIDoc*, int edgeThreshold = 10);

    //--------------- Data members ---------------------------------

    int m_edgeThreshold;
    bool m_resizeStart;
};

class SegmentSelector : public SegmentTool
{
    Q_OBJECT

    friend class SegmentToolBox;
    friend class SegmentTool;

public:

    virtual ~SegmentSelector();

    virtual void ready();
    virtual void stow();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

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

    bool isSegmentAdding() const { return m_segmentAddMode; }
    bool isSegmentCopying() const { return m_segmentCopyMode; }

    // Return the SegmentItem list for other tools to use
    //
    SegmentItemList* getSegmentItemList() { return &m_selectedItems; }

    static const QString ToolName;

protected slots:
    void slotCanvasScrolled(int newX, int newY);

protected:
    SegmentSelector(CompositionView*, RosegardenGUIDoc*);

    //--------------- Data members ---------------------------------

    SegmentItemList m_selectedItems;

    bool m_segmentAddMode;
    bool m_segmentCopyMode;
    QPoint m_clickPoint;
    bool m_segmentQuickCopyDone;
    bool m_passedInertiaEdge;
    bool m_buttonPressed;
    bool m_selectionMoveStarted;

    SegmentTool *m_dispatchTool;
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
    SegmentSplitter(CompositionView*, RosegardenGUIDoc*);
    
    void drawSplitLine(QMouseEvent*);
    void splitSegment(Rosegarden::Segment *segment,
                      Rosegarden::timeT &splitTime);

    //--------------- Data members ---------------------------------
    int m_prevX;
    int m_prevY;
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
    SegmentJoiner(CompositionView*, RosegardenGUIDoc*);
};


//////////////////////////////////////////////////////////////////////
//                SegmentItemPreview
//////////////////////////////////////////////////////////////////////
class SegmentItemPreview 
{
public:
    SegmentItemPreview(Rosegarden::Segment& parent,
                       Rosegarden::RulerScale* scale);
    virtual ~SegmentItemPreview();

    enum PreviewState {
	PreviewChanged,
	PreviewCalculating,
	PreviewCurrent
    };

    virtual void drawShape(QPainter&) = 0;

    PreviewState getPreviewState() const { return m_previewState; }

    /**
     * Sets whether the preview shape shown in the segment needs
     * to be refreshed
     */
    void setPreviewCurrent(bool c)
    { m_previewState = (c ? PreviewCurrent : PreviewChanged); }

    /**
     * Clears out the preview entirely so that it will be regenerated
     * next time
     */
    virtual void clearPreview() = 0;

    QRect rect();
    
protected:
    virtual void updatePreview(const QWMatrix &matrix) = 0;

    //--------------- Data members ---------------------------------

    Rosegarden::Segment *m_segment;
    Rosegarden::RulerScale *m_rulerScale;

    PreviewState m_previewState;
};


#endif
