
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef TRACKSCANVAS_H
#define TRACKSCANVAS_H

#include "Event.h"

#include <qwidget.h>
#include <qcanvas.h>

using Rosegarden::timeT;
namespace Rosegarden { class Track; }


/**
 * The graphical item (rectangle) which represents a Track
 * on the TracksCanvas.
 */
class TrackItem : public QCanvasRectangle
{
public:
    /**
     * Create a new track item
     *
     * The item will be at coordinates \a x, \a y, representing a time
     * segment of \a nbSteps time steps.
     */
    TrackItem(int x, int y, int nbSteps, QCanvas* canvas);

    /// Return the nb of time steps the item represents
    unsigned int getItemNbTimeSteps() const;

    /// Return the time index at which the item's track starts
    timeT getStartIndex() const;

    /// Return the instrument for the item's track
    int  getInstrument() const;

    /// Set the instrument for the item's track
    void setInstrument(int i);

    /// Set the track this TrackItem will represent
    void setTrack(Rosegarden::Track *p)  { m_track = p; }

    /// Return the item's associated track 
    Rosegarden::Track* getTrack() const  { return m_track; }

    /// Set the width to duration ratio for all TrackItem objects
    static void setWidthToDurationRatio(unsigned int);

    /// Set the resolution in timesteps for all new TrackItem objects
    static void setTimeStepsResolution(unsigned int);

    /// Return the timestep resolution used by all TrackItem objects
    static unsigned int getTimeStepsResolution();

    /// Set the height of all new TrackItem objects
    static void setItemHeight(unsigned int);

    /**
     * Helper function to convert a number of time steps to a width in
     * pixels
     */
    static unsigned int nbStepsToWidth(unsigned int);
    /**
     * Helper function to convert a width in pixels to a number of
     * time steps
     */
    static unsigned int widthToNbSteps(unsigned int);
    
protected:
    int m_instrument;

    Rosegarden::Track* m_track;

    static unsigned int m_widthToDurationRatio;
    static unsigned int m_timeStepsResolution;
    static unsigned int m_itemHeight;

};

class TrackTool;

/**
 * A class to visualize and edit track parts
 *
 * A coordinate grid is used to align TrackItem objects, which can be
 * manipulated with a set of tools : pencil, eraser, mover, resizer.
 *
 * There are no restrictions as to when a track part starts and how
 * long it lasts. Several parts can overlap partially or completely.
 *
 * @see TracksEditor
 */
class TracksCanvas : public QCanvasView
{
    Q_OBJECT

public:
    /// Available tools
    enum ToolType { Pencil, Eraser, Mover, Resizer };
    
    TracksCanvas(int gridH, int gridV,
                 QCanvas&,
                 QWidget* parent=0, const char* name=0, WFlags f=0);
    ~TracksCanvas();

    /// Remove all items
    void clear();

    /// Return the horizontal step of the coordinate grid
    unsigned int gridHStep() const { return m_grid.hstep(); }

    /**
     * The coordinate grid used to align TrackItem objects
     */
    class SnapGrid
    {
    public:
        SnapGrid(unsigned int hstep, unsigned int vstep)
            : m_hstep(hstep), m_vstep(vstep)
        {}

        int snapX(int x) const { return x / m_hstep * m_hstep; }
        int snapY(int y) const { return y / m_vstep * m_vstep; }

        unsigned int hstep() const { return m_hstep; }
        unsigned int vstep() const { return m_vstep; }

    protected:
        unsigned int m_hstep;
        unsigned int m_vstep;
    };

    const SnapGrid& grid() const { return m_grid; }

    /// Return the brush used by all TrackItem objects (normally, solid blue)
    const QBrush& brush()  const { return m_brush; }

    /// Return the pen used by all TrackItem objects
    const QPen& pen()      const { return m_pen; }

    /**
     * Add a part item at the specified coordinates, lasting \a nbSteps
     * Called when reading a music file
     */
    TrackItem* addPartItem(int x, int y, unsigned int nbSteps);

    /**
     * Find which TrackItem is under the specified point
     *
     * Note : this doesn't handle overlapping TrackItems yet
     */
    TrackItem* findPartClickedOn(QPoint);

public slots:
    /// Set the current track edition tool
    void setTool(TracksCanvas::ToolType);

    /// Update the TracksCanvas after a change of content
    virtual void update();

protected:
    virtual void contentsMousePressEvent(QMouseEvent*);
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    virtual void contentsMouseMoveEvent(QMouseEvent*);
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);
    virtual void wheelEvent(QWheelEvent*);

protected slots:
    /**
     * connected to the 'Edit' item of the RMB popup menu - re-emits
     * editTrack(Track*)
     */
    void onEdit();

    /**
     * connected to the 'Edit Small' item of the RMB popup menu - re-emits
     * editTrackSmall(Track*)
     */
    void onEditSmall();

signals:
    /**
     * Emitted when a new Track is created, the argument is the
     * corresponding TrackItem
     */
    void addTrack(TrackItem*);

    /**
     * Emitted when a Track is deleted, the argument is a pointer to
     * the Track being deleted
     */
    void deleteTrack(Rosegarden::Track*);

    /**
     * Emitted when a Track is moved to a different start time
     * (horizontally) or instrument (vertically)
     */
    void updateTrackInstrumentAndStartIndex(TrackItem*);

    /**
     * Emitted when the user requests the edition of a Track in notation form
     */
    void editTrack(Rosegarden::Track*);

    /**
     * Emitted when the user requests the edition of a Track in
     * notation form, using small pixmaps
     */
    void editTrackSmall(Rosegarden::Track*);

private:
    ToolType m_toolType;
    TrackTool *m_tool;

    SnapGrid m_grid;

    TrackItem* m_currentItem;

    QCanvasItem* m_moving;

    QBrush m_brush;
    QPen m_pen;

    QPopupMenu *m_editMenu;
    
};

//////////////////////////////////////////////////////////////////////
//                 Track Tools
//////////////////////////////////////////////////////////////////////

class TrackTool : public QObject
{
public:
    TrackTool(TracksCanvas*);
    virtual ~TrackTool();

    virtual void handleMouseButtonPress(QMouseEvent*)  = 0;
    virtual void handleMouseButtonRelase(QMouseEvent*) = 0;
    virtual void handleMouseMove(QMouseEvent*)         = 0;

protected:
    TracksCanvas*  m_canvas;
    TrackItem* m_currentItem;
};

//////////////////////////////
// TrackPencil
//////////////////////////////

class TrackPencil : public TrackTool
{
    Q_OBJECT
public:
    TrackPencil(TracksCanvas*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelase(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);

signals:
    void addTrack(TrackItem*);
    void deleteTrack(Rosegarden::Track*);

protected:
    bool m_newRect;
};

class TrackEraser : public TrackTool
{
    Q_OBJECT
public:
    TrackEraser(TracksCanvas*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelase(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);

signals:
    void deleteTrack(Rosegarden::Track*);
};

class TrackMover : public TrackTool
{
    Q_OBJECT
public:
    TrackMover(TracksCanvas*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelase(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);

signals:
    void updateTrackInstrumentAndStartIndex(TrackItem*);
};

/**
 * Track Resizer tool. Allows resizing only at the end of the track part
 */
class TrackResizer : public TrackTool
{
    Q_OBJECT
public:
    TrackResizer(TracksCanvas*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelase(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);

signals:
    void deleteTrack(Rosegarden::Track*);

protected:
    bool cursorIsCloseEnoughToEdge(TrackItem*, QMouseEvent*);

    unsigned int m_edgeThreshold;
};

#endif
