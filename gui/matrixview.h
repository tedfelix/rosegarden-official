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

#include "editview.h"
#include "matrixcanvasview.h"
#include "matrixelement.h"

namespace Rosegarden { class Segment; }

class RosegardenGUIDoc;
class MatrixStaff;
class MatrixCanvasView;
class MatrixHLayout;
class MatrixVLayout;

class QMouseEvent;

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
				Rosegarden::timeT startTime,
				Rosegarden::timeT endTime);

    QCanvas* canvas() { return getCanvasView()->canvas(); }

    MatrixStaff* getStaff(int) { return m_staffs[0]; } // deal with 1 staff only
    virtual void update();

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

    /// edition tools
    void slotPaintSelected();
    void slotEraseSelected();
    void slotSelectSelected();

    /// Canvas actions slots

    /**
     * Called when a mouse press occurred on a matrix element
     * or somewhere on the staff
     */
    void mousePressed(Rosegarden::timeT time, int pitch,
                      QMouseEvent*, MatrixElement*);

    void mouseMoved(Rosegarden::timeT time, QMouseEvent*);
    void mouseReleased(Rosegarden::timeT time, QMouseEvent*);

    /**
     * Called when the mouse cursor moves over a different height on
     * the staff
     *
     * @see MatrixCanvasView#hoveredOverNoteChange()
     */
    void hoveredOverNoteChanged(const QString&);

    /**
     * Called when the mouse cursor moves over a note which is at a
     * different time on the staff
     *
     * @see MatrixCanvasView#hoveredOverNoteChange()
     */
    void hoveredOverAbsoluteTimeChanged(unsigned int);

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

    std::vector<MatrixStaff*> m_staffs;
    
    MatrixHLayout* m_hlayout;
    MatrixVLayout* m_vlayout;

    // Status bar elements
    QLabel* m_hoveredOverAbsoluteTime;
    QLabel* m_hoveredOverNoteName;

    virtual MatrixCanvasView *getCanvasView() {
	return static_cast<MatrixCanvasView *>(m_canvasView);
    }
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

    virtual EditTool* createTool(const QString& toolName);

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
    Q_OBJECT

    friend MatrixToolBox;

public:

    virtual void handleLeftButtonPress(Rosegarden::timeT,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement*);

    /**
     * Set the duration of the element
     */
    virtual void handleMouseMove(Rosegarden::timeT,
                                 int height,
                                 QMouseEvent*);

    /**
     * Actually insert the new element
     */
    virtual void handleMouseRelease(Rosegarden::timeT,
                                    int height,
                                    QMouseEvent*);

    static const QString ToolName;

public slots:
    /**
     * Set the shortest note which can be "painted"
     * on the matrix
     */
    void setResolution(Rosegarden::Note::Type);

protected:
    MatrixPainter(MatrixView*);

    MatrixElement* m_currentElement;
    MatrixStaff* m_currentStaff;

    Rosegarden::Note::Type m_resolution;
    Rosegarden::timeT m_basicDuration;
};



class MatrixEraser : public MatrixTool
{
    Q_OBJECT

    friend MatrixToolBox;

public:

    virtual void handleLeftButtonPress(Rosegarden::timeT,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement*);

    static const QString ToolName;

protected:
    MatrixEraser(MatrixView*);

    MatrixStaff* m_currentStaff;
};

#endif
