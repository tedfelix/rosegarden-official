// -*- c-basic-offset: 4 -*-

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

#ifndef NOTATIONCANVASVIEW_H
#define NOTATIONCANVASVIEW_H

#include <qcanvas.h>

// #include "notepixmapfactory.h"

class StaffLine;
class NotationElement;
class QCanvasItemGroup;

/**
 * Central widget for the NotationView window
 *
 * This class only takes care of the event handling
 * (see the various signals).
 *
 * It does not deal with any canvas element. All elements are added by
 * the NotationView.
 *
 *@see NotationView
 */
class NotationCanvasView : public QCanvasView
{
    Q_OBJECT

public:
    NotationCanvasView(QCanvas *viewing=0, QWidget *parent=0,
                       const char *name=0, WFlags f=0);

    ~NotationCanvasView();

    void setPositionTracking(bool t);

public slots:

signals:

    /**
     * Emitted when the user clicks on a staff (e.g. mouse button press)
     * \a height is set to the MIDI pitch on which the click occurred
     * \a staffNo is set to the staff on which the click occurred
     * \a point is set to the coordinates of the click event
     * \a el points to the NotationElement which was clicked on, if any
     */
    void itemPressed(int pitch, int staffNo,
                     QMouseEvent*,
                     NotationElement* el);

    /**
     * Emitted when the user clicks on a QCanvasItem which is active
     *
     * @see QCanvas#setActive
     */
    void activeItemPressed(QMouseEvent*,
                           QCanvasItem* item);

    /**
     * Emitted when the mouse cursor moves to a different height
     * on the staff
     *
     * \a noteName contains the MIDI name of the corresponding note
     */
    void hoveredOverNoteChange(const QString &noteName);

    /**
     * Emitted when the mouse cursor moves to a note which is at a
     * different time
     *
     * \a time is set to the absolute time of the note the cursor is
     * hovering on
     */
    void hoveredOverAbsoluteTimeChange(unsigned int time);

    /**
     * Emitted when the mouse cursor moves (used by the selection tool)
     */
    void mouseMove(QMouseEvent*);

    /**
     * Emitted when the mouse button is released
     */
    void mouseRelease(QMouseEvent*);
    
protected:

    /**
     * Callback for a mouse button press event in the canvas
     */
    virtual void contentsMousePressEvent(QMouseEvent*);

    /**
     * Callback for a mouse button release event in the canvas
     */
    virtual void contentsMouseReleaseEvent(QMouseEvent*);

    /**
     * Callback for a mouse move event in the canvas
     */
    virtual void contentsMouseMoveEvent(QMouseEvent*);

    /**
     * Callback for a mouse double click event in the canvas
     */
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

    void processActiveItems(QMouseEvent*, QCanvasItemList);

    void handleMousePress(const StaffLine*, int staffNo,
                          QMouseEvent*,
                          NotationElement* pressedNotationElement = 0);

    bool posIsTooFarFromStaff(const QPoint &pos);

    /// returns the staff line closest to the mouse event position
    StaffLine* findClosestLineWithinThreshold(QMouseEvent*);

    /** Returns the note name (C4, Bb3) corresponding to the given x-coord
	on the given line (x-coord needed to take clef/key into account) */
    QString getNoteNameForLine(const StaffLine *line, int x);

    NotationElement *getElementAtXCoord(QMouseEvent *e);

    /// the staff line over which the mouse cursor is
    StaffLine* m_currentHighlightedLine;

    int m_lastYPosNearStaff;

    unsigned int m_staffLineThreshold;

    QCanvasItemGroup *m_positionMarker;
    QCanvasItemGroup *m_legerLinePositionMarker;

    bool m_positionTracking;
};


#endif
