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

#include <vector>

#include "Event.h"
#include "Track.h"
#include "RulerScale.h"
#include "SnapGrid.h"
#include "Selection.h"

#include <qwidget.h>

#include "rosegardencanvasview.h"
#include "segmentcommands.h" // for SegmentRecSet

class RosegardenGUIDoc;
class QFont;
class QFontMetrics;
class QCanvasRepeatRectangle;

class SegmentItemPreview;

/**
 * The graphical item (rectangle) which represents a Segment
 * on the SegmentCanvas.
 */
class SegmentItem : public QCanvasRectangle
{

public:
    /**
     * Create a new segment item without an associated segment (yet)
     */
    SegmentItem(Rosegarden::TrackId track,
		Rosegarden::timeT startTime,
		Rosegarden::timeT endTime,
		bool showPreview,
		Rosegarden::SnapGrid *snapGrid, QCanvas* canvas,
                RosegardenGUIDoc *doc);

    /**
     * Create a new segment item with an associated segment
     */
    SegmentItem(Rosegarden::Segment *segment,
		bool showPreview,
		Rosegarden::SnapGrid *snapGrid, QCanvas* canvas,
                RosegardenGUIDoc *doc);

    virtual ~SegmentItem();

    /// Return the item's associated document
    RosegardenGUIDoc* getDocument() { return m_doc; }

    /// Return the item's associated segment 
    Rosegarden::Segment *getSegment() const;

    /// Set the segment this SegmentItem will represent
    void setSegment(Rosegarden::Segment *s);

    /// Update start time of the rectangle (doesn't modify underlying segment)
    void setStartTime(Rosegarden::timeT t);
    Rosegarden::timeT getStartTime() const { return m_startTime; }

    /// Update end time of the rectangle (doesn't modify underlying segment)
    void setEndTime(Rosegarden::timeT d);
    Rosegarden::timeT getEndTime() const { return m_endTime; }

    /// Update track of the rectangle (doesn't modify underlying segment)
    void setTrack(Rosegarden::TrackId track);
    Rosegarden::TrackId getTrack() const { return m_track; }

    void setShowPreview(bool preview);

    /**
     * Reset the rectangle's location and dimensions following
     * a change in the ruler scale.
     * 
     * If inheritFromSegment is true, will take dimensions from
     * the underlying segment if there is one (overriding m_*)
     */
    void recalculateRectangle(bool inheritFromSegment = true);
    
    /**
     * Modify start and end time so as to maintain dimensions
     * while ensuring duration is positive.  (End time before start
     * is fine for a SegmentItem but not for a Segment.)  Should only
     * be called on a SegmentItem that has no Segment yet.
     */
    void normalize();

    bool isSelected() const { return m_selected; }

    /// Select this SegmentItem
    void setSelected(bool select, const QBrush &highlightBrush);

    /// show or hide the repeat rect
    void showRepeatRect(bool);

    virtual void drawShape(QPainter&);

    SegmentItemPreview* getPreview() { return m_preview; }

protected:

    void setPreview();

    //--------------- Data members ---------------------------------

    Rosegarden::Segment *m_segment;
    RosegardenGUIDoc    *m_doc;

    // We need to duplicate these from the segment, because we
    // frequently want to create SegmentItems before their
    // associated Segments
    Rosegarden::TrackId m_track;
    Rosegarden::timeT m_startTime;
    Rosegarden::timeT m_endTime;

    bool m_selected;
    Rosegarden::SnapGrid *m_snapGrid;

    QCanvasRepeatRectangle*   m_repeatRectangle;
    QString m_label;

    SegmentItemPreview* m_preview;

    bool m_showPreview;

    static QFont *m_font;
    static QFontMetrics *m_fontMetrics;
    static int m_fontHeight;
    void makeFont();

private:
    SegmentItem(const SegmentItem &);
    SegmentItem &operator=(const SegmentItem &);
};


// Marker on Segments to show exactly where a split will
// be made.
//
class SegmentSplitLine : public QCanvasLine
{
public:
    SegmentSplitLine(int x, int y, int height,
                     Rosegarden::RulerScale *rulerScale,
                     QCanvas* canvas);

    void moveLine(int x, int y);
    void hideLine();

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
class SegmentCanvas : public RosegardenCanvasView
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

    SegmentCanvas(RosegardenGUIDoc *doc,
                  Rosegarden::RulerScale *,QScrollBar*,  int vStep,
                  QCanvas*,
		  QWidget* parent=0, const char* name=0, WFlags f=0);

    virtual ~SegmentCanvas();

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
				Rosegarden::timeT startTime,
				Rosegarden::timeT endTime);

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
     * Remove the given segment from the selection, if it's in it
     */
    void removeFromSelection(Rosegarden::Segment *segment);

    /**
     * Add the given Segment to the selection, if we know anything about it
     */
    void addToSelection(Rosegarden::Segment *);

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

    /**
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

    /**
     * Set whether the segment items contain previews or not
     */
    void setShowPreviews(bool previews);

    /**
     * Return whether the segment items contain previews or not
     */
    bool isShowingPreviews() { return m_showPreviews; }

    /**
     * Show a preview of the Segment we're recording
     */
    void showRecordingSegmentItem(Rosegarden::TrackId track,
				  Rosegarden::timeT startTime,
				  Rosegarden::timeT endTime);
    void deleteRecordingSegmentItem();

    /// Return the selected Segments if we're currently using a "Selector"
    Rosegarden::SegmentSelection getSelectedSegments();
    bool haveSelection(); // i.e. would getSelectedSegments return anything
    void clearSelected();

    /**
     * For all segment items, check that the Segment is still linked
     * to a Composition.
     * Also scans the composition for new segments
     */
    void updateAllSegmentItems();

    /*
     * get a SegmentItem for a Segment
     */
    SegmentItem* getSegmentItem(Rosegarden::Segment *segment);

    /*
     * get the selection rectangle
     */
    QCanvasRectangle* getSelectionRectangle();


public slots:

    /// Set the current segment editing tool
    void slotSetTool(SegmentCanvas::ToolType);

    // This method only operates if we're of the "Selector"
    // tool type - it's called from the View to enable it
    // to automatically set the selection of Segments (say
    // by Track).
    //
    void slotSelectSegments(const Rosegarden::SegmentSelection &segment);

    // These are sent from the top level app when it gets key
    // depresses relating to selection add (usually SHIFT) and
    // selection copy (usually CONTROL)
    //
    void slotSetSelectAdd(const bool &value);
    void slotSetSelectCopy(const bool &value);

    void slotSetFineGrain(bool value);

    // Show and hige the splitting line on a Segment
    //
    void slotShowSplitLine(int x, int y);
    void slotHideSplitLine();

    void slotExternalWheelEvent(QWheelEvent*);

protected:
    virtual void contentsMousePressEvent(QMouseEvent*);
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    virtual void contentsMouseMoveEvent(QMouseEvent*);
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

protected slots:

    /**
     * connected to the 'Edit as Notation' items of the RMB popup menu -
     * re-emits slotEditSegmentNotation(Segment*)
     */
    void slotOnEditNotation();

    /**
     * connected to the 'Edit as Matrix' items of the RMB popup
     * menu - re-emits slotEditSegmentMatrix(Segment*)
     */
    void slotOnEditMatrix();

    /**
     * connected to the 'Edit Audio' item of the RMB popup
     */
    void slotOnEditAudio();

    /**
     * AutoSplit audio segment
     */ 
    void slotOnAutoSplitAudio();

    /*
     * pop-up an event list
     */
    void slotOnEditEventList();

signals:
    void editSegmentNotation(Rosegarden::Segment*);
    void editSegmentMatrix(Rosegarden::Segment*);
    void editSegmentAudio(Rosegarden::Segment*);
    void editSegmentEventList(Rosegarden::Segment*);
    void audioSegmentAutoSplit(Rosegarden::Segment*);

    void selectedSegments(const Rosegarden::SegmentSelection &);

    void scrollTo(int);

private:

    SegmentItem *findSegmentItem(Rosegarden::Segment *segment); // slow

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
    bool m_showPreviews;
    RosegardenGUIDoc *m_doc;
    
    KConfig* m_config;

    // selection bounding box for sweep selections
    //
    QCanvasRectangle* m_selectionRect;
};

//////////////////////////////////////////////////////////////////////
//                 Segment Tools
//////////////////////////////////////////////////////////////////////

class SegmentTool : public QObject
{
public:
    SegmentTool(SegmentCanvas*, RosegardenGUIDoc *doc);
    virtual ~SegmentTool();

    virtual void handleMouseButtonPress(QMouseEvent*)  = 0;
    virtual void handleMouseButtonRelease(QMouseEvent*) = 0;
    virtual bool handleMouseMove(QMouseEvent*)         = 0;

    void addCommandToHistory(KCommand *command);

protected:
    //--------------- Data members ---------------------------------

    SegmentCanvas*  m_canvas;
    SegmentItem* m_currentItem;
    RosegardenGUIDoc* m_doc;
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
    virtual bool handleMouseMove(QMouseEvent*);

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
    virtual bool handleMouseMove(QMouseEvent*);

};

class SegmentMover : public SegmentTool
{
    Q_OBJECT
public:
    SegmentMover(SegmentCanvas*, RosegardenGUIDoc*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual bool handleMouseMove(QMouseEvent*);

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
    SegmentResizer(SegmentCanvas*, RosegardenGUIDoc*, int edgeThreshold = 10);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual bool handleMouseMove(QMouseEvent*);

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
    virtual bool handleMouseMove(QMouseEvent*);

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

public slots:
    void slotSelectSegmentItem(SegmentItem *selectedItem);

signals:
    void selectedSegments(const Rosegarden::SegmentSelection &);

private:
    typedef std::pair<QPoint, SegmentItem *> SegmentItemPair;
    typedef std::vector<SegmentItemPair> SegmentItemList;
    SegmentItemList m_selectedItems;

    bool m_segmentAddMode;
    bool m_segmentCopyMode;
    QPoint m_clickPoint;
    bool m_segmentQuickCopyDone;
    bool m_passedInertiaEdge;

    SegmentTool *m_dispatchTool;
};


class SegmentSplitter : public SegmentTool
{
    Q_OBJECT
public:
    SegmentSplitter(SegmentCanvas*, RosegardenGUIDoc*);
    virtual ~SegmentSplitter();

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual bool handleMouseMove(QMouseEvent*);

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
    virtual bool handleMouseMove(QMouseEvent*);
 
    // don't do double clicks
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

private:

};



#endif
