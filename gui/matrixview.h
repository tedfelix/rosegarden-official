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
#include "linedstaff.h"

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
     * Returns the layout x coordinate of the element (not the same
     * as the canvas x coordinate, which is assigned by the staff
     * depending on its own location)
     */
    double getLayoutX() const { return m_layoutX; }

    /**
     * Returns the layout y coordinate of the element (not the same
     * as the canvas y coordinate, which is assigned by the staff
     * depending on its own location)
     */
    double getLayoutY() const { return m_layoutY; }

    /**
     * Sets the layout x coordinate of the element (to be translated
     * to canvas coordinate according to the staff's location)
     */
    void setLayoutX(double x) { m_layoutX = x; }

    /**
     * Sets the layout y coordinate of the element (to be translated
     * to canvas coordinate according to the staff's location)
     */
    void setLayoutY(double y) { m_layoutY = y; }

    /**
     * Returns the actual x coordinate of the element on the canvas
     */
    double getCanvasX() const { return m_canvasRect->x(); }

    /**
     * Returns the actual y coordinate of the element on the canvas
     */
    double getCanvasY() const { return m_canvasRect->y(); }

    /**
     * Sets the x coordinate of the element on the canvas
     */
    void setCanvasX(double x) { m_canvasRect->setX(x); }

    /**
     * Sets the y coordinate of the element on the canvas
     */
    void setCanvasY(double y) { m_canvasRect->setY(y); }

    /**
     * Sets the width of the rectangle on the canvas
     */
    void setWidth(int w)   { m_canvasRect->setSize(w, m_canvasRect->height()); }

    /**
     * Sets the height of the rectangle on the canvas
     */
    void setHeight(int h)   { m_canvasRect->setSize(m_canvasRect->width(), h); }

    /// Returns true if the wrapped event is a note
    bool isNote() const;

protected:

    //--------------- Data members ---------------------------------

    QCanvasRectangle* m_canvasRect;

    double m_layoutX;
    double m_layoutY;
};


class MatrixCanvasView : public QCanvasView
{
    Q_OBJECT

public:
    MatrixCanvasView(MatrixStaff&, QCanvas *viewing=0, QWidget *parent=0,
                     const char *name=0, WFlags f=0);

    ~MatrixCanvasView();

signals:

    void itemPressed(int pitch, Rosegarden::timeT time,
                     QMouseEvent*, MatrixElement*);

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

typedef std::vector<double> BarData;

class MatrixHLayout : public Rosegarden::HorizontalLayoutEngine<MatrixElement>
{
public:
    MatrixHLayout(double scaleFactor);

    virtual ~MatrixHLayout();

    void setScaleFactor(double scaleFactor) {
        m_scaleFactor = scaleFactor;
    }

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

    double m_scaleFactor;
    double m_totalWidth;
};

//------------------------------------------------------------

typedef Rosegarden::ViewElementList<MatrixElement> MatrixElementList;

class MatrixStaff : public LinedStaff<MatrixElement>
{
public:
    MatrixStaff(QCanvas *, Rosegarden::Segment *, int id, int vResolution);
    virtual ~MatrixStaff();

protected:
    virtual int getLineCount() const;
    virtual int getLegerLineCount() const;
    virtual int getBottomLineHeight() const;
    virtual int getHeightPerLine() const;

    /**
     * Override from Rosegarden::Staff<T>
     * Wrap only notes and time sig changes
     */
    virtual bool wrapEvent(Rosegarden::Event*);

public:
    LinedStaff<MatrixElement>::setResolution;

    int getElementHeight() { return m_resolution; }

    virtual void positionElements(Rosegarden::timeT from = -1,
                                  Rosegarden::timeT to = -1);
};


//------------------------------------------------------------

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

    /// Canvas actions slots

    /**
     * Called when a mouse press occurred on a matrix element
     * or somewhere on the staff
     */
    void itemPressed(int pitch, Rosegarden::timeT time,
                     QMouseEvent*, MatrixElement*);

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
