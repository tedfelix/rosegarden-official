// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
#include "dialogs.h" // ugh -- for TempoDialog::TempoDialogAction

#include "Event.h"
#include "Selection.h"

namespace Rosegarden { class Segment; class Staff; class RulerScale; class ControlParameter; }

class QCanvasItem;
class QScrollView;
class QCanvasView;
class QVBox;
class QGridLayout;
class QVBoxLayout;
class QScrollBar;
class QTabWidget;

class KCommand;
class KToggleAction;

class RosegardenGUIDoc;
class MultiViewCommandHistory;
class BasicCommand;
class ActiveItem;
class BarButtons;
class RosegardenCanvasView;
class ControlRuler;
class PropertyControlRuler;
class ControllerEventsRuler;

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

    /**
     * Set the current event selection.
     *
     * If preview is true, sound the selection as well.
     *
     * If redrawNow is true, recolour the elements on the canvas;
     * otherwise just line up a refresh for the next paint event.
     *
     * (If the selection has changed as part of a modification to a
     * segment, redrawNow should be unnecessary and undesirable, as a
     * paint event will occur in the next event loop following the
     * command invocation anyway.)
     */
    virtual void setCurrentSelection(Rosegarden::EventSelection* s,
				     bool preview = false,
				     bool redrawNow = false) = 0;

    Rosegarden::EventSelection* getCurrentSelection()
        { return m_currentEventSelection; }

signals:
    void changeTempo(Rosegarden::timeT,  // tempo change time
                     double,             // tempo value
                     TempoDialog::TempoDialogAction); // tempo action

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

    virtual void slotStepBackward(); // default is event-by-event
    virtual void slotStepForward(); // default is event-by-event
    void slotJumpBackward();
    void slotJumpForward();
    void slotJumpToStart();
    void slotJumpToEnd();

    void slotAddTempo();
    void slotAddTimeSignature();

    virtual void slotShowVelocityControlRuler();
    virtual void slotShowControllerEventsRuler();
    virtual void slotShowPropertyControlRuler();

    // transpose
    void slotTranspose();
    void slotTransposeUp();
    void slotTransposeUpOctave();
    void slotTransposeDown();
    void slotTransposeDownOctave();


    /// Control ruler actions
    void slotInsertControlRulerItem();
    void slotEraseControlRulerItem();
    void slotClearControlRulerItem();
    void slotStartControlLineItem();
    void slotCloseControlRulerItem();

    // add control ruler
    void slotAddControlRuler(int);

protected:
    virtual Rosegarden::RulerScale* getHLayout() = 0;

    QVBox* getBottomWidget() { return m_bottomBox; }

    virtual void paintEvent(QPaintEvent* e);

    /**
     * Locate the given widget in the top bar-buttons position and
     * connect up its scrolling signals.
     */
    void setTopBarButtons(BarButtons*);

    /**
     * Locate the given widget in the bottom bar-buttons position and
     * connect up its scrolling signals.
     */
    void setBottomBarButtons(BarButtons*);

    /**
     * Locate the given widget right above the top bar-buttons and
     * connect up its scrolling signals.
     * The widget has to have a slotScrollHoriz(int) slot
     */
    void addRuler(QWidget*);

    /**
     * Add a ruler control box
     */
    void addPropertyBox(QWidget*);

    /**
     * Make a control ruler for the given property,
     */
    PropertyControlRuler* makePropertyControlRuler(Rosegarden::PropertyName propertyName);

    /**
     * Make a ruler for controller events
     */
    ControllerEventsRuler* makeControllerEventRuler(Rosegarden::ControlParameter *controller = 0);

    /**
     * Add control ruler
     */
    void addControlRuler(ControlRuler* ruler);

    /**
     * Update all control rulers
     */
    void updateControlRulers(bool updateHPos=false);

    /**
     * Set zoom factor of control rulers
     */
    void setControlRulersZoom(QWMatrix);

    /**
     * Find the control ruler for the given property name
     * if it's already been created, return 0 otherwise
     */
    PropertyControlRuler* findRuler(Rosegarden::PropertyName propertyName, int &index);

    /**
     * Show a control ruler for the given property
     * If the ruler already exists, activate the tab it's in,
     * otherwise create the ruler and add it to the control rulers tab
     * widget
     */ 
    void showPropertyControlRuler(Rosegarden::PropertyName propertyName);
    
    /**
     * Return the control ruler currently displayed, or 0 if none exist
     */
    ControlRuler* getCurrentControlRuler();
    
    /**
     * Set up those actions common to any EditView (e.g. note insertion,
     * time signatures etc)
     */
    void setupActions();

    /**
     * Set up the 'Add control ruler' sub-menu
     */
    void setupAddControlRulerMenu();

    /**
     * Do this after any other segment setup in a subordinate view
     */
    void setupControllerTabs();

    /**
     * Create an action menu for inserting notes from the PC keyboard,
     * and add it to the action collection.  This is one of the methods
     * called by setupActions().
     */
    void createInsertPitchActionMenu();

    /**
     * Get a note pitch from an action name (where the action is one of
     * those created by createInsertPitchActionMenu).  Can throw an
     * Exception to mean that the action is not an insert one.  Also
     * returns any specified accidental through the reference arg.
     */
    int getPitchFromNoteInsertAction(QString actionName,
				     Rosegarden::Accidental &acc);

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

    /**
     * Abstract method to get current staff (the returned staff will be
     * that representing the segment of getCurrentSegment())
     */
    virtual Rosegarden::Staff *getCurrentStaff() = 0;

    virtual RosegardenCanvasView* getCanvasView();
    virtual void setCanvasView(RosegardenCanvasView *cv);

    //--------------- Data members ---------------------------------

    /// The current selection of Events (for cut/copy/paste)
    Rosegarden::EventSelection* m_currentEventSelection;

    ActiveItem* m_activeItem;

    RosegardenCanvasView *m_canvasView;

    QVBoxLayout  *m_rulerBox;
    QVBoxLayout  *m_controlBox;
    QVBox        *m_bottomBox;
    BarButtons   *m_topBarButtons;
    BarButtons   *m_bottomBarButtons;
    ControlRuler *m_controlRuler;
    QTabWidget   *m_controlRulers;
    QWMatrix      m_currentRulerZoomMatrix;

    static const unsigned int RULERS_ROW;
    static const unsigned int CONTROLS_ROW;
    static const unsigned int TOPBARBUTTONS_ROW;
    static const unsigned int CANVASVIEW_ROW;
    static const unsigned int CONTROLRULER_ROW;
};

#endif
