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

#ifndef MATRIXCANVASVIEW_H
#define MATRIXCANVASVIEW_H

#include "Event.h"

#include "rosegardencanvasview.h"

class MatrixStaff;
class MatrixElement;

class MatrixCanvasView : public RosegardenCanvasView
{
    Q_OBJECT

public:
    MatrixCanvasView(MatrixStaff&, QScrollBar* hsb,
                     QCanvas *viewing,
                     QWidget *parent=0, const char *name=0, WFlags f=0);

    ~MatrixCanvasView();

signals:

    /**
     * Emitted when the user clicks on a QCanvasItem which is active
     *
     * @see QCanvasItem#setActive
     */
    void activeItemPressed(QMouseEvent*,
                           QCanvasItem* item);

    /**
     * Emitted when the mouse cursor moves to a different height
     * on the staff
     *
     * \a noteName contains the MIDI name of the corresponding note
     */
    void hoveredOverNoteChanged(const QString &noteName);

    /**
     * Emitted when the mouse cursor moves to a note which is at a
     * different time
     *
     * \a time is set to the absolute time of the note the cursor is
     * hovering on
     */
    void hoveredOverAbsoluteTimeChanged(unsigned int time);

    void mousePressed(Rosegarden::timeT time, int pitch,
                      QMouseEvent*, MatrixElement*);

    void mouseMoved(Rosegarden::timeT time, int pitch, QMouseEvent*);

    void mouseReleased(Rosegarden::timeT time, int pitch, QMouseEvent*);

public slots:
    void slotExternalWheelEvent(QWheelEvent*);

protected:
    /**
     * Callback for a mouse button press event in the canvas
     */
    virtual void contentsMousePressEvent(QMouseEvent*);

    /**
     * Callback for a mouse move event in the canvas
     */
    virtual void contentsMouseMoveEvent(QMouseEvent*);

    /**
     * Callback for a mouse button release event in the canvas
     */
    virtual void contentsMouseReleaseEvent(QMouseEvent*);

    /**
     * Callback for a mouse double-click event in the canvas
     *
     * NOTE: a double click event is always preceded by a mouse press
     * event
     */
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

    //--------------- Data members ---------------------------------

    MatrixStaff& m_staff;

    Rosegarden::timeT m_previousEvTime;
    int m_previousEvPitch;

    bool m_mouseWasPressed;
    bool m_ignoreClick;
};


#endif
