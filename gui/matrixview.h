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

#ifndef MATRIXVIEW_H
#define MATRIXVIEW_H

#include <vector>

#include <qcanvas.h>
#include <kmainwindow.h>

#include "Staff.h"
#include "RulerScale.h"
#include "SnapGrid.h"

#include "editview.h"
#include "matrixcanvasview.h"
#include "matrixelement.h"
#include "matrixhlayout.h"
#include "matrixvlayout.h"

namespace Rosegarden { 
    class Segment;
    class EventSelection;
    class MappedEvent;
}

class RosegardenGUIDoc;
class MatrixStaff;
class MatrixCanvasView;
class PianoKeyboard;
class MatrixParameterBox;

class QMouseEvent;
class QLabel;

typedef std::vector<MatrixElement*> SelectedElements;

/**
 * Matrix ("Piano Roll") View
 *
 * Note: we currently display only one staff
 */
class MatrixView : public EditView
{
    Q_OBJECT
public:
    MatrixView(RosegardenGUIDoc *doc,
               std::vector<Rosegarden::Segment *> segments,
               QWidget *parent);

    virtual ~MatrixView();

    virtual bool applyLayout(int staffNo = -1);

    virtual void refreshSegment(Rosegarden::Segment *segment,
				Rosegarden::timeT startTime = 0,
				Rosegarden::timeT endTime = 0);

    QCanvas* canvas() { return getCanvasView()->canvas(); }

    void setCanvasCursor(const QCursor &cursor) {
	getCanvasView()->viewport()->setCursor(cursor);
    }

//     void setPositionTracking(bool t) {
// 	getCanvasView()->setPositionTracking(t);
//     }

    MatrixStaff* getStaff(int) { return m_staffs[0]; } // deal with 1 staff only
    virtual void updateView();

    void setCurrentSelection(Rosegarden::EventSelection* s);
    Rosegarden::EventSelection* getCurrentSelection()
        { return m_currentEventSelection; }

    // Play a Note Event using the keyPressed() signal
    //
    void playNote(Rosegarden::Event *event);

    // Play a preview (same as above but a simpler interface)
    //
    void playPreview(int pitch);

    Rosegarden::SnapGrid &getSnapGrid() { return m_snapGrid; }

    // Selection of current matrix elements
    //
    SelectedElements& getSelectedElements() { return m_selectedElements; }

    // Set entire selection
    //
    void setSelectedElements(const SelectedElements &eS);

    // Add
    //
    bool addElementToSelection(MatrixElement *mE);

    // Removal is a two part process to keep the integrity of
    // the vector
    //
    void queueElementForDeselection(MatrixElement *mE);
    void processDeselections();

    // Is this element in the current selection?
    //
    bool isElementSelected(MatrixElement *mE);

signals:    
    /**
     * Emitted when the selection has been cut or copied
     *
     * @see MatrixSelector#hideSelection
     */
    void usedSelection();

    // Play a pressed key 
    //
    void keyPressed(Rosegarden::MappedEvent*);


public slots:

    /**
     * put the indicationed text/object into the clipboard and remove * it
     * from the document
     */
    virtual void slotEditCut();

    /**
     * put the indicationed text/object into the clipboard
     */
    virtual void slotEditCopy();

    /**
     * paste the clipboard into the document
     */
    virtual void slotEditPaste();

    /*
     * Delete the current selection
     */
    void slotEditDelete();

    /// edition tools
    void slotPaintSelected();
    void slotEraseSelected();
    void slotSelectSelected();
    void slotMoveSelected();
    void slotResizeSelected();

    /// transforms
    void slotTransformsQuantize();

    /// Canvas actions slots

    /**
     * Called when a mouse press occurred on a matrix element
     * or somewhere on the staff
     */
    void slotMousePressed(Rosegarden::timeT time, int pitch,
                      QMouseEvent*, MatrixElement*);

    void slotMouseMoved(Rosegarden::timeT time, int pitch, QMouseEvent*);
    void slotMouseReleased(Rosegarden::timeT time, int pitch, QMouseEvent*);

    /**
     * Called when the mouse cursor moves over a different height on
     * the staff
     *
     * @see MatrixCanvasView#hoveredOverNoteChanged()
     */
    void slotHoveredOverNoteChanged(const QString&);

    /**
     * Called when the mouse cursor moves over a different key on
     * the piano keyboard
     *
     * @see PianoKeyboard#hoveredOverKeyChanged()
     */
    void slotHoveredOverKeyChanged(unsigned int);

    /**
     * Called when the mouse cursor moves over a note which is at a
     * different time on the staff
     *
     * @see MatrixCanvasView#hoveredOverNoteChange()
     */
    void slotHoveredOverAbsoluteTimeChanged(unsigned int);

    /// Set the time pointer position during playback
    void slotSetPointerPosition(Rosegarden::timeT time);

    /// Set the insertion pointer position (from the bottom LoopRuler)
    void slotSetInsertCursorPosition(Rosegarden::timeT position);

    // Catch the keyboard being pressed
    //
    void slotKeyPressed(unsigned int y, bool repeating);

    // Handle scrolling between view and PianoKeyboard
    //
    void slotVerticalScrollPianoKeyboard(int y);

    // close
    //
    void closeWindow();

protected:

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */
    virtual void slotSaveOptions();

    /**
     * read general Options again and initialize all variables like the recent file list
     */
    virtual void readOptions();

    /**
     * create menus and toolbars
     */
    virtual void setupActions();

    /**
     * setup status bar
     */
    virtual void initStatusBar();

    /**
     * Return the size of the MatrixCanvasView
     */
    virtual QSize getViewSize();

    /**
     * Set the size of the MatrixCanvasView
     */
    virtual void setViewSize(QSize);

    virtual MatrixCanvasView *getCanvasView();

    //--------------- Data members ---------------------------------

//     RefreshStack m_refreshStack;

    /// The current selection of Events (for cut/copy/paste)
    Rosegarden::EventSelection* m_currentEventSelection;

    // vector of selected MatrixElements
    //
    SelectedElements m_selectedElements;
    SelectedElements m_deselectionQueue;

    std::vector<MatrixStaff*> m_staffs;

    MatrixHLayout m_hlayout;
    MatrixVLayout m_vlayout;
    Rosegarden::SnapGrid m_snapGrid;

    // Status bar elements
    QLabel* m_hoveredOverAbsoluteTime;
    QLabel* m_hoveredOverNoteName;
    QLabel *m_selectionCounter;

    /**
     * used in slotHoveredOverKeyChanged to track moves over the piano
     * keyboard
     */
    int m_previousEvPitch;

    MatrixCanvasView    *m_canvasView;
    QScrollView         *m_pianoView;
    PianoKeyboard       *m_pianoKeyboard;

    // The last note we sent in case we're swooshing up and
    // down the keyboard and don't want repeat notes sending
    //
    Rosegarden::MidiByte m_lastNote;

    Rosegarden::PropertyName m_selectedProperty;

    // The parameter box
    //
    MatrixParameterBox *m_parameterBox;
};

#endif
