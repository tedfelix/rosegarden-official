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
    void setWidth(int w)   { m_canvasRect->setSize(m_canvasRect->height(), w); }

    /**
     * Sets the height of the rectangle computed by the layout engine
     */
    void setHeight(int h)   { m_canvasRect->setSize(h, m_canvasRect->width()); }

    /// Returns true if the wrapped event is a note
    bool isNote() const;

protected:
    QCanvasRectangle* m_canvasRect;
};


class MatrixCanvasView : public QCanvasView
{
public:
    MatrixCanvasView(QCanvas *viewing=0, QWidget *parent=0,
                     const char *name=0, WFlags f=0);

    ~MatrixCanvasView();

};

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

protected:
    double m_pitchScaleFactor;
    double m_staffIdScaleFactor;
};

class MatrixHLayout : public Rosegarden::HorizontalLayoutEngine<MatrixElement>
{
public:
    MatrixHLayout(unsigned int durationScaleFactor = 2);
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
    virtual unsigned int getBarLineCount(StaffType &staff) ;

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
    double m_totalWidth;
    double m_durationScaleFactor;
};

//------------------------------------------------------------

typedef Rosegarden::ViewElementList<MatrixElement> MatrixElementList;

class MatrixStaff : public Rosegarden::Staff<MatrixElement>
{
public:
    MatrixStaff(QCanvas*, Rosegarden::Segment*, unsigned int id);

    void renderElements(MatrixElementList::iterator from,
			MatrixElementList::iterator to);

    /**
     * Call renderElements(from, to) on the whole staff.
     */
    void renderElements();

    int getId() { return m_id; }

protected:
    QCanvas* m_canvas;

    unsigned int m_id;
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

    virtual bool applyLayout();

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

    //--------------- Data members ---------------------------------

    MatrixCanvasView* m_canvasView;

    std::vector<MatrixStaff*> m_staffs;
    
    MatrixHLayout* m_hLayout;
    MatrixVLayout* m_vLayout;
};



#endif
