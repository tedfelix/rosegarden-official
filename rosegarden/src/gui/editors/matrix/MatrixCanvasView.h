
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_MATRIXCANVASVIEW_H_
#define _RG_MATRIXCANVASVIEW_H_

#include "gui/general/RosegardenCanvasView.h"
#include "base/Event.h"


class QWidget;
class QWheelEvent;
class QMouseEvent;
class QCanvasItem;
class QCanvas;


namespace Rosegarden
{

class SnapGrid;
class MatrixStaff;
class MatrixElement;


class MatrixCanvasView : public RosegardenCanvasView
{
    Q_OBJECT

public:
    MatrixCanvasView(MatrixStaff&,
                     SnapGrid *,
                     bool drumMode,
                     QCanvas *viewing,
                     QWidget *parent=0, const char *name=0, WFlags f=0);

    ~MatrixCanvasView();

    void setSmoothModifier(Qt::ButtonState s) { m_smoothModifier = s; }
    Qt::ButtonState getSmoothModifier()       { return m_smoothModifier; }

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
     * on the staff.  Returns the new pitch.
     */
    void hoveredOverNoteChanged(int evPitch, bool haveEvent,
                                timeT evTime);

    /**
     * Emitted when the mouse cursor moves to a note which is at a
     * different time
     *
     * \a time is set to the absolute time of the note the cursor is
     * hovering on
     */
    void hoveredOverAbsoluteTimeChanged(unsigned int time);

    void mousePressed(timeT time, int pitch,
                      QMouseEvent*, MatrixElement*);

    void mouseMoved(timeT time, int pitch, QMouseEvent*);

    void mouseReleased(timeT time, int pitch, QMouseEvent*);

    void mouseEntered();
    void mouseLeft();

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

    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);

    /**
     * Update the value of snap grid according to the button's state
     *
     * If the button was pressed with the 'smooth' modifier, set the
     * grid so it won't snap time.
     *
     * @see #setSmoothModifier
     * @see #getSmoothModifier
     */
    void updateGridSnap(QMouseEvent *e);

    //--------------- Data members ---------------------------------

    MatrixStaff          &m_staff;
    SnapGrid *m_snapGrid;
    bool                  m_drumMode;

    timeT     m_previousEvTime;
    int                   m_previousEvPitch;

    bool                  m_mouseWasPressed;
    bool                  m_ignoreClick;

    Qt::ButtonState       m_smoothModifier;
    timeT     m_lastSnap;
    bool                  m_isSnapTemporary;
};



}

#endif
