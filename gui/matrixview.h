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

#ifndef MATRIXVIEW_H
#define MATRIXVIEW_H

#include <vector>

#include <qcanvas.h>
#include <kmainwindow.h>

#include "Staff.h"
#include "LayoutEngine.h"

#include "editview.h"

namespace Rosegarden { class Segment; }

class RosegardenGUIDoc;
class MatrixStaff;

class MatrixElement : public Rosegarden::ViewElement
{
public:
    MatrixElement(Rosegarden::Event *event);

    virtual ~MatrixElement();

    void setCanvas(QCanvas* c);

    /**
     * Returns the X coordinate of the element, as computed by the
     * layout. This is not the coordinate of the associated canvas
     * item.
     *
     * @see getEffectiveX()
     */
    double getLayoutX() { return m_canvasRect->x(); }

    /**
     * Returns the Y coordinate of the element, as computed by the
     * layout. This is not the coordinate of the associated canvas
     * item.
     *
     * @see getEffectiveY()
     */
    double getLayoutY() { return m_canvasRect->y(); }

    /**
     * Sets the X coordinate which was computed by the layout engine
     * @see getLayoutX()
     */
    void setLayoutX(double x) { m_canvasRect->setX(x); }

    /**
     * Sets the Y coordinate which was computed by the layout engine
     * @see getLayoutY()
     */
    void setLayoutY(double y) { m_canvasRect->setY(y); }

    /**
     * Sets the width of the rectangle computed by the layout engine
     */
    void setWidth(int w)   { m_canvasRect->setSize(w, m_canvasRect->height()); }

    /**
     * Sets the height of the rectangle computed by the layout engine
     */
    void setHeight(int h)   { m_canvasRect->setSize(m_canvasRect->width(), h); }

    /// Returns true if the wrapped event is a note
    bool isNote() const;

protected:

    //--------------- Data members ---------------------------------

    QCanvasRectangle* m_canvasRect;
};


class MatrixCanvasView : public QCanvasView
{
    Q_OBJECT

public:
    MatrixCanvasView(MatrixStaff&, QCanvas *viewing=0, QWidget *parent=0,
                     const char *name=0, WFlags f=0);

    ~MatrixCanvasView();

signals:

    void itemPressed(Rosegarden::timeT time, int pitch,
                     QMouseEvent*,
                     MatrixElement*);

    void itemReleased(Rosegarden::timeT time, QMouseEvent*);

    void itemResized(Rosegarden::timeT time, QMouseEvent*);

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
     * Compute the time and pitch corresponding to the event's
     * location
     */
    void eventTimePitch(QMouseEvent*, Rosegarden::timeT&, int& pitch);

    //--------------- Data members ---------------------------------

    MatrixStaff& m_staff;
};

//------------------------------------------------------------
//                       Layouts
//------------------------------------------------------------

class MatrixVLayout : public Rosegarden::VerticalLayoutEngine<MatrixElement>
{
public:
    MatrixVLayout();

    virtual ~MatrixVLayout();

    /**
     * Resets internal data stores for all staffs
     */
    virtual void reset();

    /**
     * Resets internal data stores for a specific staff
     */
    virtual void resetStaff(StaffType &staff);

    /**
     * Precomputes layout data for a single staff, updating any
     * internal data stores associated with that staff and updating
     * any layout-related properties in the events on the staff's
     * segment.
     */
    virtual void scanStaff(StaffType &staff);

    /**
     * Computes any layout data that may depend on the results of
     * scanning more than one staff.  This may mean doing most of
     * the layout (likely for horizontal layout) or nothing at all
     * (likely for vertical layout).
     */
    virtual void finishLayout();

    static const unsigned int maxMIDIPitch;

protected:
    //--------------- Data members ---------------------------------


};

//------------------------------

typedef std::vector<QCanvasLine *> HLineList;
typedef std::vector<double> BarData;

class MatrixHLayout : public Rosegarden::HorizontalLayoutEngine<MatrixElement>
{
public:
    MatrixHLayout();

    virtual ~MatrixHLayout();

    /**
     * Resets internal data stores for all staffs
     */
    virtual void reset();

    /**
     * Resets internal data stores for a specific staff
     */
    virtual void resetStaff(StaffType &staff);

    /**
     * Returns the total length of all elements once layout is done.
     * This is the x-coord of the end of the last element on the
     * longest staff
     */
    virtual double getTotalWidth();

    /**
     * Returns the total number of bar lines on the given staff
     */
    virtual unsigned int getBarLineCount(StaffType &staff);

    /**
     * Returns the x-coordinate of the given bar number (zero-based)
     * on the given staff
     */
    virtual double getBarLineX(StaffType &staff, unsigned int barNo);

    /**
     * Precomputes layout data for a single staff, updating any
     * internal data stores associated with that staff and updating
     * any layout-related properties in the events on the staff's
     * segment.
     */
    virtual void scanStaff(StaffType&);

    /**
     * Computes any layout data that may depend on the results of
     * scanning more than one staff.  This may mean doing most of
     * the layout (likely for horizontal layout) or nothing at all
     * (likely for vertical layout).
     */
    virtual void finishLayout();

protected:

    //--------------- Data members ---------------------------------

    BarData m_barData;
    HLineList m_hlines;

    double m_totalWidth;
};

//------------------------------------------------------------

typedef Rosegarden::ViewElementList<MatrixElement> MatrixElementList;

class MatrixStaff : public Rosegarden::Staff<MatrixElement>
{
    typedef std::vector<QCanvasLine *> StaffLineList;

public:
    MatrixStaff(QCanvas*, Rosegarden::Segment*, unsigned int id,
                unsigned int pitchScaleFactor = defaultPitchScaleFactor);

    ~MatrixStaff();

    void renderElements(MatrixElementList::iterator from,
			MatrixElementList::iterator to);

    /**
     * Call renderElements(from, to) on the whole staff.
     */
    void renderElements();

    int getId() { return m_id; }

    void setPitchScaleFactor(unsigned int f) { m_pitchScaleFactor = f; }
    unsigned int getPitchScaleFactor()       { return m_pitchScaleFactor; }

    void setTimeScaleFactor(float f) { m_timeScaleFactor = f; }
    float getTimeScaleFactor()       { return m_timeScaleFactor; }

    void setTimeResolution(float f) { m_timeResolution = f; }
    float getTimeResolution()       { return m_timeResolution; }

    /**
     * This must be called each time the canvas is resized
     */
    void resizeStaffHLines();

    void setBarData(const BarData& bd) { m_barData = bd; }

    Rosegarden::timeT xToTime(double x);

    int yToPitch(double y);

    static const unsigned int nbHLines;

    static const unsigned int defaultPitchScaleFactor;

protected:

    /**
     * Override from Rosegarden::Staff<T>
     * Wrap only notes and time sig changes
     */
    virtual bool wrapEvent(Rosegarden::Event*);

    /// Create staff's vertical lines
    void createLines();


    //--------------- Data members ---------------------------------

    StaffLineList m_staffHLines, m_staffVLines;

    QCanvas* m_canvas;

    unsigned int m_id;

    unsigned int m_pitchScaleFactor;
    float m_timeScaleFactor;
    float m_timeResolution;

    BarData m_barData;

    unsigned int m_currentBarLength;
};


//------------------------------------------------------------

class MatrixView : public EditView
{
    Q_OBJECT
public:
    MatrixView(RosegardenGUIDoc *doc,
               std::vector<Rosegarden::Segment *> segments,
               QWidget *parent);

    virtual ~MatrixView();

    virtual bool applyLayout(int staffNo = -1);

    QCanvas* canvas() { return m_canvasView->canvas(); }

public slots:

    /**
     * undo
     */
    virtual void slotEditUndo();

    /**
     * redo
     */
    virtual void slotEditRedo();
    
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

    /// edition tools
    void slotPaintSelected();
    void slotEraseSelected();
    void slotSelectSelected();

protected:

    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */
    virtual void saveOptions();

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

    //--------------- Data members ---------------------------------

    MatrixCanvasView* m_canvasView;

    std::vector<MatrixStaff*> m_staffs;
    
    MatrixHLayout* m_hlayout;
    MatrixVLayout* m_vlayout;
};

//////////////////////////////////////////////////////////////////////
//                     MatrixToolBox
//////////////////////////////////////////////////////////////////////

#include "edittool.h"

class MatrixToolBox : public EditToolBox
{
public:
    MatrixToolBox(MatrixView* parent);

protected:

    virtual EditTool* createTool(const QString& toolName) = 0;

    //--------------- Data members ---------------------------------

    MatrixView* m_mParentView;
};

//////////////////////////////////////////////////////////////////////
//                     MatrixTools
//////////////////////////////////////////////////////////////////////

class MatrixTool : public EditTool
{
public:
//     virtual void ready();

protected:
    MatrixTool(const QString& menuName, MatrixView*);

    //--------------- Data members ---------------------------------

    MatrixView* m_mParentView;
};

class MatrixPainter : public MatrixTool
{
    friend MatrixToolBox;

public:

    virtual void handleLeftButtonPress(int height,
                                       Rosegarden::timeT,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement*);

//     virtual void handleMouseRelease(QMouseEvent*);

    static const QString ToolName;

protected:
    MatrixPainter(MatrixView*);
};

#endif
