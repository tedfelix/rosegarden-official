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
class RulerScale;
class SimpleRulerScale;


/**
 * The graphical item (rectangle) which represents a Segment
 * on the SegmentCanvas.
 */
class SegmentItem : public QCanvasRectangle
{
public:
    static const int SegmentItemRTTI = 1001;

    /**
     * Create a new segment item without an associated segment (yet)
     */
    SegmentItem(int y, timeT startTime, timeT duration, double unitsPerPixel,
		QCanvas* canvas);

    /**
     * Create a new segment item with an associated segment
     */
    SegmentItem(int y, Rosegarden::Segment *segment, double unitsPerPixel,
		QCanvas* canvas);

    /// Return the item's associated segment 
    Rosegarden::Segment* getSegment() const;

    /// Set the segment this SegmentItem will represent
    void setSegment(Rosegarden::Segment *s);

    timeT getStartTime() const { return m_startTime; }
    void setStartTime(timeT t);

    timeT getDuration() const { return m_duration; }
    void setDuration(timeT d);
    
    /**
     * Modify start time and duration so as to maintain dimensions
     * while ensuring duration is positive.  (Negative duration is
     * fine for a SegmentItem but not for a Segment.)  Should only
     * be called on a SegmentItem that has no Segment yet.
     */
//!!!    void normalize();

    double getUnitsPerPixel() const { return m_unitsPerPixel; }
    void setUnitsPerPixel(double upp);

    /// Return the track for the item's segment
    int getTrack() const;

    /// Set the height of all new SegmentItem objects
    static void setItemHeight(unsigned int);

    bool const isSelected() { return m_selected; }

    // Select this SegmentItem
    void setSelected(const bool &select, const QBrush &highlightBrush);

    virtual int rtti() const { return SegmentItemRTTI; }
    
protected:
    Rosegarden::Segment *m_segment;

    // We need to duplicate these from the segment, because we
    // frequently want to create SegmentItems before their
    // associated Segments
    timeT m_startTime;
    timeT m_duration;
    bool m_selected;
    double m_unitsPerPixel;

    static unsigned int m_itemHeight;
};

class SegmentTool;

/**
 * A class to visualize and edit segment items
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

    // ruler scale must be a SimpleRulerScale, in fact
    SegmentCanvas(RulerScale *, int vStep, QCanvas&,
		  QWidget* parent=0, const char* name=0, WFlags f=0);
    ~SegmentCanvas();

    /// Remove all items
    void clear();

    /**
     * The coordinate grid used to align SegmentItem objects
     */
    class SnapGrid
    {
    public:
        SnapGrid(SimpleRulerScale *rulerScale,
		 int vstep);

	static const timeT NoSnap;
	static const timeT SnapToBar;
	static const timeT SnapToBeat;

	/**
	 * Set the snap size of the grid to the given time.
	 * The snap time must be positive, or else one of the
	 * special constants NoSnap, SnapToBar or SnapToBeat.
	 * The default is SnapToBeat.
	 */
	void setSnapTime(timeT snap);

	/**
	 * Return the snap size of the grid, at the given
	 * x-coordinate.  (The x-coordinate is required in
	 * case the built-in snap size is SnapToBar or
	 * SnapToBeat, in which case we need to know the
	 * current time signature.)
	 * Returns zero for NoSnap.
	 */
	timeT getSnapTime(double x) const;

	/**
	 * Snap a given x-coordinate to the nearest time on
	 * the grid.  Of course this also does x-to-time
	 * conversion, so it's useful even in NoSnap mode.
	 * If the snap time is greater than the bar duration
	 * at this point, the bar duration will be used instead.
	 */
        timeT snapX(double x) const;

	/**
	 * Snap a given y-coordinate to the nearest lower
	 * multiple of the vstep.
	 */
	int snapY(int y) const;

	int getYSnap() const { return m_vstep; }

	double getUnitsPerPixel() const;

    protected:
	SimpleRulerScale *m_rulerScale; // I don't own this
	timeT m_snapTime;
	int m_vstep;
    };

    const SnapGrid& grid() const { return m_grid; }

    /// Return the brush used by all SegmentItem objects (normally, solid blue)
    const QBrush& brush()  const { return m_brush; }

    /// Return the pen used by all SegmentItem objects
    const QPen& pen()      const { return m_pen; }

    /**
     * Add a SegmentItem at the specified height
     */
    SegmentItem* addSegmentItem(int y, timeT startTime, timeT duration);

    /**
     * Find which SegmentItem is under the specified point
     *
     * Note : this doesn't handle overlapping SegmentItems yet
     */
    SegmentItem* findSegmentClickedOn(QPoint);

    /**
     * get the highlight brush from the canvas
     */
    QBrush getHighlightBrush() const { return m_highlightBrush; }

    /*
     * Get the normal segment brush
     */
    QBrush getSegmentBrush() const { return m_brush; }

    /*
     * Show a preview of the Segment we're recording
     */
    void showRecordingSegmentItem(int y, timeT startTime, timeT duration);
    void destroyRecordingSegmentItem();

public slots:
    /// Set the current segment editing tool
    void setTool(SegmentCanvas::ToolType);

    /// Update the SegmentCanvas after a change of content
    virtual void update();

    // This method only operates if we're of the "Selector"
    // tool type - it's called from the View to enable it
    // to automatically set the selection of Segments (say
    // by Track).
    //
    void selectSegments(std::list<Rosegarden::Segment*> segment);

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

    //!!! what if the staff ruler changes (because of a change in time
    //sig or whatever)?

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
    QCanvasRectangle *m_recordingSegment;
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

    std::list<SegmentItem*> m_selectedItems;
    bool m_segmentAddMode;
    bool m_segmentCopyMode;

};

#endif
