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

#ifndef EDITVIEW_H
#define EDITVIEW_H

#include <vector>
#include <set>

#include <qaccel.h>

#include "editviewbase.h"

#include "Event.h"
#include "Selection.h"

namespace Rosegarden { class Segment; }

class QCanvasItem;
class QScrollView;
class QCanvasView;
class QVBox;
class QGridLayout;
class QVBoxLayout;
class QScrollBar;

class KCommand;
class KToggleAction;

class RosegardenGUIDoc;
class MultiViewCommandHistory;
class BasicCommand;
class ActiveItem;
class BarButtons;
class RosegardenCanvasView;

/**
 * An interface for canvas items which are capable of handling
 * mouse events
 */
class ActiveItem
{
public:
    virtual void handleMousePress(QMouseEvent*) = 0;
    virtual void handleMouseMove(QMouseEvent*) = 0;
    virtual void handleMouseRelease(QMouseEvent*) = 0;
};


class EditView : public EditViewBase
{
    Q_OBJECT

public:

    /**
     * Create an EditView for the segments \a segments from document \a doc.
     *
     * \arg cols : number of columns, main column is always rightmost
     *
     */
    EditView(RosegardenGUIDoc *doc,
             std::vector<Rosegarden::Segment *> segments,
             unsigned int cols,
             QWidget *parent,
             const char *name = 0);

    virtual ~EditView();

    /**
     * "Clever" readjustment of the view size
     * If the new size is larger, enlarge to that size plus a margin
     * If it is smaller, only shrink if the reduction is significant
     * (e.g. new size is less than 75% of the old one)
     *
     * @arg exact if true, then set to newSize exactly
     */
    virtual void readjustViewSize(QSize newSize, bool exact = false);

    /**
     * Return the active item
     */
    ActiveItem* activeItem() { return m_activeItem; }

    /**
     * Set the active item
     */
    void setActiveItem(ActiveItem* i) { m_activeItem = i; }

    virtual void setCurrentSelection(Rosegarden::EventSelection* s,
				     bool preview = false) = 0;
    Rosegarden::EventSelection* getCurrentSelection()
        { return m_currentEventSelection; }


public slots:
    /**
     * Called when a mouse press occurred on an active canvas item
     *
     * @see ActiveItem
     * @see QCanvasItem#setActive
     */
    virtual void slotActiveItemPressed(QMouseEvent*, QCanvasItem*);

    virtual void slotSetInsertCursorPosition(Rosegarden::timeT position) = 0;
    
    void slotExtendSelectionBackward();
    void slotExtendSelectionForward();
    void slotExtendSelectionBackwardBar();
    void slotExtendSelectionForwardBar();
    void slotExtendSelectionBackward(bool bar);
    void slotExtendSelectionForward(bool bar);

    void slotStepBackward();
    void slotStepForward();
    void slotJumpBackward();
    void slotJumpForward();
    void slotJumpToStart();
    void slotJumpToEnd();

protected:

    virtual void paintEvent(QPaintEvent* e);
    
    /**
     * Locate the given widget in the top bar-buttons position and
     * connect up its scrolling signals.
     */
    void setTopBarButtons(QWidget*);

    /**
     * Locate the given widget in the bottom bar-buttons position and
     * connect up its scrolling signals.
     */
    void setBottomBarButtons(QWidget*);

    /**
     * Locate the given widget right above the top bar-buttons and
     * connect up its scrolling signals.
     * The widget has to have a slotScrollHoriz(int) slot
     */
    void addRuler(QWidget*);

    /**
     * Add a ruler control box
     */
    void addControl(QWidget*);

    /**
     * Create an action menu for inserting notes from the PC keyboard,
     * and add it to the action collection
     */
    void createInsertPitchActionMenu();

    /**
     * Get a note pitch from an action name (where the action is one of
     * those created by createInsertPitchActionMenu).  Can throw an
     * Exception to mean that the action is not an insert one
     */
    int getPitchFromNoteInsertAction(QString actionName);

    /**
     * Abstract method to get the view size
     * Typically implemented as canvas()->size().
     */
    virtual QSize getViewSize() = 0;

    /**
     * Abstract method to set the view size
     * Typically implemented as canvas()->resize().
     */
    virtual void setViewSize(QSize) = 0;

    /**
     * Abstract method to get current insert-pointer time
     */
    virtual Rosegarden::timeT getInsertionTime() = 0;

    /**
     * Return the time at which the insert cursor may be found,
     * and the time signature, clef and key at that time.  Default
     * implementation is okay but slow.
     */ 
    virtual Rosegarden::timeT getInsertionTime(Rosegarden::Clef &clef,
					       Rosegarden::Key &key);

    /**
     * Abstract method to get current segment
     */
    virtual Rosegarden::Segment *getCurrentSegment() = 0;

    virtual RosegardenCanvasView* getCanvasView();
    virtual void setCanvasView(RosegardenCanvasView *cv);

    //--------------- Data members ---------------------------------

    /// The current selection of Events (for cut/copy/paste)
    Rosegarden::EventSelection* m_currentEventSelection;

    ActiveItem* m_activeItem;

    RosegardenCanvasView *m_canvasView;

    QScrollBar  *m_horizontalScrollBar;
    QVBoxLayout *m_rulerBox;
    QVBoxLayout *m_controlBox;
    QWidget     *m_topBarButtons;
    QWidget     *m_bottomBarButtons;

};

#endif
