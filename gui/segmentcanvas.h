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
#include "Track.h"
#include "RulerScale.h"

#include <qwidget.h>
#include <qcanvas.h>
#include <list>

namespace Rosegarden {
    class Segment;
}


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
    SegmentItem(Rosegarden::TrackId track,
		Rosegarden::timeT startTime, Rosegarden::timeT duration,
		Rosegarden::SnapGrid *snapGrid, QCanvas* canvas);

    /**
     * Create a new segment item with an associated segment
     */
    SegmentItem(Rosegarden::Segment *segment,
		Rosegarden::SnapGrid *snapGrid, QCanvas* canvas);

    ~SegmentItem();

    /// Return the item's associated segment 
    Rosegarden::Segment *getSegment() const;

    /// Set the segment this SegmentItem will represent
    void setSegment(Rosegarden::Segment *s);

    /// Update start time of the rectangle (doesn't modify underlying segment)
    void setStartTime(Rosegarden::timeT t);
    Rosegarden::timeT getStartTime() const { return m_startTime; }

    /// Update duration of the rectangle (doesn't modify underlying segment)
    void setDuration(Rosegarden::timeT d);
    Rosegarden::timeT getDuration() const { return m_duration; }

    /// Update track of the rectangle (doesn't modify underlying segment)
    void setTrack(Rosegarden::TrackId track);
    Rosegarden::TrackId getTrack() const { return m_track; }

    /**
     * Reset the rectangle's location and dimensions following
     * a change in the ruler scale.
     * 
     * If inheritFromSegment is true, will take dimensions from
     * the underlying segment if there is one (overriding m_*)
     */
    void recalculateRectangle(bool inheritFromSegment = true);
    
    /**
     * Modify start time and duration so as to maintain dimensions
     * while ensuring duration is positive.  (Negative duration is
     * fine for a SegmentItem but not for a Segment.)  Should only
     * be called on a SegmentItem that has no Segment yet.
     */
    void normalize();

    bool const isSelected() { return m_selected; }

    // Select this SegmentItem
    void setSelected(const bool &select, const QBrush &highlightBrush);

    virtual int rtti() const { return SegmentItemRTTI; }

protected:
    Rosegarden::Segment *m_segment;

    // We need to duplicate these from the segment, because we
    // frequently want to create SegmentItems before their
    // associated Segments
    Rosegarden::TrackId m_track;
    Rosegarden::timeT m_startTime;
    Rosegarden::timeT m_duration;

    bool m_selected;
    Rosegarden::SnapGrid *m_snapGrid;
};


// Marker on Segments to show exactly where a split will
// be made.
//
class SegmentSplitLine : public QCanvasLine
{
public:
    static const int SegmentSplitLineRTTI = 1002;
    SegmentSplitLine(int x, int y, int height,
                     Rosegarden::RulerScale *rulerScale,
                     QCanvas* canvas);

    void moveLine(int x, int y);
    void hideLine();

    virtual int rtti() const { return SegmentSplitLineRTTI; }

private:
    Rosegarden::RulerScale *m_rulerScale;
    int m_height;

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
    enum ToolType { NoTool,
                    Pencil,
                    Eraser,
                    Mover,
                    Resizer,
                    Selector,
                    Joiner,
                    Splitter };

    SegmentCanvas(Rosegarden::RulerScale *, int vStep,
                  unsigned int leftMargin,
                  unsigned int topMargin,
                  QCanvas*,
		  QWidget* parent=0, const char* name=0, WFlags f=0);
    ~SegmentCanvas();

    /// Remove all items
    void clear();

    Rosegarden::SnapGrid &grid() { return m_grid; }

    /// Return the brush used by all SegmentItem objects (normally, solid blue)
    const QBrush& brush()  const { return m_brush; }

    /// Return the pen used by all SegmentItem objects
    const QPen& pen()      const { return m_pen; }

    /**
     * Add a SegmentItem of the specified geometry, with no
     * underlying Segment
     */
    SegmentItem* addSegmentItem(Rosegarden::TrackId track,
				Rosegarden::timeT startTime, Rosegarden::timeT duration);

    /**
     * Add a SegmentItem for the given underlying Segment
     */
    SegmentItem *addSegmentItem(Rosegarden::Segment *segment);

    /**
     * Find the item corresponding to this segment and update it
     * from the segment's attributes -- or create a new one if none
     * is found.
     */
    void updateSegmentItem(Rosegarden::Segment *segment);

    /**
     * Find the item corresponding to this segment and remove it
     */
    void removeSegmentItem(Rosegarden::Segment *segment);

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

    /**
     * Set the snap resolution of the grid to something suitable.
     * 
     * fineTool indicates whether the current tool is a fine-grain sort
     * (such as the resize or move tools) or a coarse one (such as the
     * segment creation pencil).  If the user is requesting extra-fine
     * resolution (through the setFineGrain method) that will also be
     * taken into account.
     */
    void setSnapGrain(bool fine);

    /*
     * Show a preview of the Segment we're recording
     */
    void showRecordingSegmentItem(Rosegarden::TrackId track,
				  Rosegarden::timeT startTime,
				  Rosegarden::timeT duration);
    void deleteRecordingSegmentItem();

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

    void setFineGrain(bool value);

    // Show and hige the splitting line on a Segment
    //
    void showSplitLine(int x, int y);
    void hideSplitLine();

protected:
    virtual void contentsMousePressEvent(QMouseEvent*);
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    virtual void contentsMouseMoveEvent(QMouseEvent*);
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

    virtual void setHBarGeometry(QScrollBar &hbar,
                                 int x, int y, int w, int h);
    virtual void setVBarGeometry(QScrollBar &vbar,
                                 int x, int y, int w, int h);
    
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
     * Emitted when a new Segment is created, the arguments are the
     * dimensions for a new SegmentItem but we don't create it at
     * this stage
     */
    void addSegment(Rosegarden::TrackId, Rosegarden::timeT, Rosegarden::timeT);

    /**
     * Emitted when a Segment is deleted, the argument is a pointer to
     * the Segment being deleted
     */
    void deleteSegment(Rosegarden::Segment *);

    /**
     * Emitted when a Segment's duration is changed
     */
    void changeSegmentDuration(Rosegarden::Segment *,
			       Rosegarden::timeT duration);

    /**
     * Emitted when a Segment's start time and duration have changed
     */
    void changeSegmentTimes(Rosegarden::Segment *,
			    Rosegarden::timeT startTime,
			    Rosegarden::timeT duration);

    /**
     * Emitted when a Segment is moved to a different start time
     * (horizontally) or instrument (vertically)
     */
    void changeSegmentTrackAndStartTime(Rosegarden::Segment *,
					Rosegarden::TrackId track,
					Rosegarden::timeT startTime);

    /*
     * Split a Segment - send it to the doc
     */
    void splitSegment(Rosegarden::Segment *, Rosegarden::timeT);

    void editSegmentNotation(Rosegarden::Segment*);
    void editSegmentMatrix(Rosegarden::Segment*);
    void editSegmentAudio(Rosegarden::Segment*);

    //!!! what if the staff ruler changes (because of a change in time
    //sig or whatever)?

private:

    SegmentItem *findSegmentItem(Rosegarden::Segment *segment);

    //--------------- Data members ---------------------------------

    SegmentTool *m_tool;

    Rosegarden::SnapGrid m_grid;

    SegmentItem *m_currentItem;
    SegmentItem *m_recordingSegment;

    SegmentSplitLine *m_splitLine;

    QBrush m_brush;
    QBrush m_highlightBrush;
    QPen m_pen;
    QPopupMenu *m_editMenu;

    bool m_fineGrain;
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
    void addSegment(Rosegarden::TrackId, Rosegarden::timeT, Rosegarden::timeT);

protected:
    //--------------- Data members ---------------------------------

    bool m_newRect;
    Rosegarden::TrackId m_track;
    Rosegarden::timeT m_startTime;
    Rosegarden::timeT m_duration;
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
    void changeSegmentTrackAndStartTime(Rosegarden::Segment*,
					Rosegarden::TrackId,
					Rosegarden::timeT);

private:
    QPoint m_clickPoint;
    double m_currentItemStartX;
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
    void changeSegmentTimes(Rosegarden::Segment*, Rosegarden::timeT, Rosegarden::timeT);

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
    void changeSegmentTrackAndStartTime(Rosegarden::Segment *,
					Rosegarden::TrackId,
					Rosegarden::timeT);


private:
    typedef std::pair<QPoint, SegmentItem *> SegmentItemPair;
    typedef std::list<SegmentItemPair> SegmentItemList;
    SegmentItemList m_selectedItems;

    bool m_segmentAddMode;
    bool m_segmentCopyMode;
    QPoint m_clickPoint;
};


class SegmentSplitter : public SegmentTool
{
    Q_OBJECT
public:
    SegmentSplitter(SegmentCanvas*);
    virtual ~SegmentSplitter();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);

    // don't do double clicks
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

signals:
    // Emit this when we want to split the Segment
    //
    void splitSegment(Rosegarden::Segment *, Rosegarden::timeT);

private:
    void drawSplitLine(QMouseEvent*);
    void splitSegment(Rosegarden::Segment *segment,
                      Rosegarden::timeT &splitTime);
};

class SegmentJoiner : public SegmentTool
{
    Q_OBJECT
public:
    SegmentJoiner(SegmentCanvas*);
    virtual ~SegmentJoiner();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);
 
    // don't do double clicks
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

private:

};



#endif
