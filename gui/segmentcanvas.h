// -*- c-basic-offset: 4 -*-

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

#ifndef SEGMENTCANVAS_H
#define SEGMENTCANVAS_H

#include "Event.h"

#include <qwidget.h>
#include <qcanvas.h>
#include <list>

using Rosegarden::timeT;
namespace Rosegarden { class Segment; }


/**
 * The graphical item (rectangle) which represents a Segment
 * on the SegmentCanvas.
 */
class SegmentItem : public QCanvasRectangle
{
public:
    /**
     * Create a new segment item
     *
     * The item will be at coordinates \a x, \a y, representing a time
     * segment of \a nbSteps time steps.
     */

    static const int SegmentItemRTTI = 1001;

    SegmentItem(int x, int y, int nbSteps, QCanvas* canvas);

    /// Return the nb of bars the item represents
    int getItemNbBars() const;

    /// Return the number of the bar at which the item's segment starts
    int getStartBar() const;

    /// Return the track for the item's segment
    int getTrack() const;

    /// Set the track for the item's segment
    void setTrack(int t);

    /// Set the segment this SegmentItem will represent
    void setSegment(Rosegarden::Segment *p)  { m_segment = p; }

    /// Return the item's associated segment 
    Rosegarden::Segment* getSegment() const  { return m_segment; }

    /// Set the width to duration ratio for all SegmentItem objects
    static void setWidthToDurationRatio(unsigned int);

    /// Set the resolution in bars for all new SegmentItem objects
    static void setBarResolution(unsigned int);

    /// Return the bar resolution used by all SegmentItem objects
    static unsigned int getBarResolution();

    /// Set the height of all new SegmentItem objects
    static void setItemHeight(unsigned int);

    /**
     * Helper function to convert a number of bars to a width in
     * pixels
     */
    static unsigned int nbBarsToWidth(unsigned int);
    /**
     * Helper function to convert a width in pixels to a number of
     * bars
     */
    static unsigned int widthToNbBars(unsigned int);

    // Select this SegmentItem
    //
    bool const isSelected() { return m_selected; }
    void setSelected(const bool &select, const QBrush &highlightBrush);

    virtual int rtti() { return SegmentItemRTTI; }
    
protected:

    //--------------- Data members ---------------------------------

    int m_instrument;

    Rosegarden::Segment* m_segment;

    static unsigned int m_widthToDurationRatio;
    static unsigned int m_barResolution;
    static unsigned int m_itemHeight;

    bool m_selected;

};

class SegmentTool;

/**
 * A class to visualize and edit segment parts
 *
 * A coordinate grid is used to align SegmentItem objects, which can be
 * manipulated with a set of tools : pencil, eraser, mover, resizer.
 *
 * There are no restrictions as to when a segment part starts and how
 * long it lasts. Several parts can overlap partially or completely.
 *
 * @see TrackEditor
 */
class SegmentCanvas : public QCanvasView
{
    Q_OBJECT

public:
    /// Available tools
    enum ToolType { NoTool, Pencil, Eraser, Mover, Resizer, Selector };
    
    SegmentCanvas(int gridH, int gridV,
                 QCanvas&,
                 QWidget* parent=0, const char* name=0, WFlags f=0);
    ~SegmentCanvas();

    /// Remove all items
    void clear();

    /// Return the horizontal step of the coordinate grid
    unsigned int gridHStep() const { return m_grid.hstep(); }

    /**
     * The coordinate grid used to align SegmentItem objects
     */
    class SnapGrid
    {
    public:
        SnapGrid(unsigned int hstep, unsigned int vstep)
            : m_hstep(hstep), m_vstep(vstep), m_hdiv(hstep / 1) // disabled
        {}

        // We provide two methods for getting X snap - one for
        // actual new Segment size (as provided by m_hstep) -
        // and one to allow us to change the X snap division
        // (to allow for smaller than m_hstep movement)
        //
        int snappedSegmentSizeX(int x) const;

        int snapX(int x) const;
        int snapY(int y) const;

        unsigned int hstep() const { return m_hstep; }
        unsigned int vstep() const { return m_vstep; }

    protected:
        unsigned int m_hstep;
        unsigned int m_vstep;
        unsigned int m_hdiv;
    };


    const SnapGrid& grid() const { return m_grid; }

    /// Return the brush used by all SegmentItem objects (normally, solid blue)
    const QBrush& brush()  const { return m_brush; }

    /// Return the pen used by all SegmentItem objects
    const QPen& pen()      const { return m_pen; }

    /**
     * Add a part item at the specified coordinates, lasting \a nbSteps
     * Called when reading a music file
     */
    SegmentItem* addPartItem(int x, int y, unsigned int nbSteps);

    /**
     * Find which SegmentItem is under the specified point
     *
     * Note : this doesn't handle overlapping SegmentItems yet
     */
    SegmentItem* findPartClickedOn(QPoint);

    /**
     * get the highlight brush from the canvas
     */
    QBrush getHighlightBrush() const { return m_highlightBrush; }

    /*
     * Get the normal segment brush
     */
    QBrush getSegmentBrush() const { return m_brush; }

public slots:
    /// Set the current segment edition tool
    void setTool(SegmentCanvas::ToolType);

    /// Update the SegmentCanvas after a change of content
    virtual void update();

    // This method only operates if we're of the "Selector"
    // tool type - it's called from the View to enable it
    // to automatically set the selection of Segments (say
    // by Track).
    //
    void selectSegments(list<Rosegarden::Segment*> segment);

    // These are sent from the top level app when it gets key
    // depresses relating to selection add (usually SHIFT) and
    // selection copy (usually CONTROL)
    //
    void setSelectAdd(const bool &value);
    void setSelectCopy(const bool &value);


protected:
    virtual void contentsMousePressEvent(QMouseEvent*);
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    virtual void contentsMouseMoveEvent(QMouseEvent*);
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);
    virtual void wheelEvent(QWheelEvent*);

protected slots:

    /**
     * connected to the 'Edit as Notation' items of the RMB popup menu -
     * re-emits editSegmentNotation(Segment*)
     */
    void onEditNotation();

    /**
     * connected to the 'Edit as Matrix' items of the RMB popup
     * menu - re-emits editSegmentMatrix(Segment*)
     */
    void onEditMatrix();

    /**
     * connected to the 'Edit Audio' item of the RMB popup
     */
    void onEditAudio();

signals:
    /**
     * Emitted when a new Segment is created, the argument is the
     * corresponding SegmentItem
     */
    void addSegment(SegmentItem*);

    /**
     * Emitted when a Segment is deleted, the argument is a pointer to
     * the Segment being deleted
     */
    void deleteSegment(Rosegarden::Segment*);

    /**
     * Emitted when a Segment's duration is changed
     */
    void updateSegmentDuration(SegmentItem*);

    /**
     * Emitted when a Segment is moved to a different start time
     * (horizontally) or instrument (vertically)
     */
    void updateSegmentTrackAndStartIndex(SegmentItem*);

    void editSegmentNotation(Rosegarden::Segment*);
    void editSegmentMatrix(Rosegarden::Segment*);
    void editSegmentAudio(Rosegarden::Segment*);

private:
    //--------------- Data members ---------------------------------

    SegmentTool *m_tool;

    SnapGrid m_grid;

    SegmentItem* m_currentItem;

    QCanvasItem* m_moving;

    QBrush m_brush;
    QBrush m_highlightBrush;
    QPen m_pen;

    QPopupMenu *m_editMenu;
    
};

//////////////////////////////////////////////////////////////////////
//                 Segment Tools
//////////////////////////////////////////////////////////////////////

class SegmentTool : public QObject
{
public:
    SegmentTool(SegmentCanvas*);
    virtual ~SegmentTool();

    virtual void handleMouseButtonPress(QMouseEvent*)  = 0;
    virtual void handleMouseButtonRelease(QMouseEvent*) = 0;
    virtual void handleMouseMove(QMouseEvent*)         = 0;

protected:
    //--------------- Data members ---------------------------------

    SegmentCanvas*  m_canvas;
    SegmentItem* m_currentItem;
};

//////////////////////////////
// SegmentPencil
//////////////////////////////

class SegmentPencil : public SegmentTool
{
    Q_OBJECT
public:
    SegmentPencil(SegmentCanvas*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);

signals:
    void addSegment(SegmentItem*);
    void deleteSegment(Rosegarden::Segment*);
    void setSegmentDuration(SegmentItem*);

protected:
    //--------------- Data members ---------------------------------

    bool m_newRect;
};

class SegmentEraser : public SegmentTool
{
    Q_OBJECT
public:
    SegmentEraser(SegmentCanvas*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);

signals:
    void deleteSegment(Rosegarden::Segment*);
};

class SegmentMover : public SegmentTool
{
    Q_OBJECT
public:
    SegmentMover(SegmentCanvas*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);

signals:
    void updateSegmentTrackAndStartIndex(SegmentItem*);
};

/**
 * Segment Resizer tool. Allows resizing only at the end of the segment part
 */
class SegmentResizer : public SegmentTool
{
    Q_OBJECT
public:
    SegmentResizer(SegmentCanvas*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);

signals:
    void deleteSegment(Rosegarden::Segment*);
    void setSegmentDuration(SegmentItem*);

protected:
    bool cursorIsCloseEnoughToEdge(SegmentItem*, QMouseEvent*);

    //--------------- Data members ---------------------------------

    unsigned int m_edgeThreshold;
};

class SegmentSelector : public SegmentTool
{
    Q_OBJECT
public:
    SegmentSelector(SegmentCanvas*);
    virtual ~SegmentSelector();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);

    // Clear all Segments in our list and on the view
    //
    void clearSelected();

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


public slots:
    void selectSegmentItem(SegmentItem *selectedItem);

signals:
    void updateSegmentTrackAndStartIndex(SegmentItem*);


private:

    list<SegmentItem*> m_selectedItems;
    bool               m_segmentAddMode;
    bool               m_segmentCopyMode;

};

#endif
