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

#ifndef MATRIXTOOL_H
#define MATRIXTOOL_H

//////////////////////////////////////////////////////////////////////
//                     MatrixToolBox
//////////////////////////////////////////////////////////////////////

#include "NotationTypes.h"

#include "edittool.h"

class MatrixView;
class MatrixElement;
class MatrixStaff;

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

    friend class MatrixToolBox;

public:

    virtual void handleLeftButtonPress(Rosegarden::timeT,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement*);

    /**
     * Set the duration of the element
     */
    virtual bool handleMouseMove(Rosegarden::timeT,
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
    void slotSetResolution(Rosegarden::Note::Type);

protected:
    MatrixPainter(MatrixView*);
    MatrixPainter(QString name, MatrixView*);

    MatrixElement* m_currentElement;
    MatrixStaff* m_currentStaff;

    Rosegarden::Note::Type m_resolution;
    Rosegarden::timeT m_basicDuration;
};



class MatrixEraser : public MatrixTool
{
    friend class MatrixToolBox;

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

#include "Selection.h"
class QCanvasRectangle;

class MatrixSelector : public MatrixTool
{
    Q_OBJECT

    friend class MatrixToolBox;

public:

    virtual void handleLeftButtonPress(Rosegarden::timeT,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement*);

    /**
     * Set the duration of the element
     */
    virtual bool handleMouseMove(Rosegarden::timeT,
                                 int height,
                                 QMouseEvent*);

    /**
     * Actually insert the new element
     */
    virtual void handleMouseRelease(Rosegarden::timeT,
                                    int height,
                                    QMouseEvent*);

    /**
     * Create the selection rect
     *
     * We need this because MatrixView deletes all QCanvasItems
     * along with it. This happens before the MatrixSelector is
     * deleted, so we can't delete the selection rect in
     * ~MatrixSelector because that leads to double deletion.
     */
    virtual void ready();

    /**
     * Delete the selection rect.
     */
    virtual void stow();

    /**
     * Returns the currently selected events
     *
     * The returned result is owned by the caller
     */
    Rosegarden::EventSelection* getSelection();

    static const QString ToolName;

public slots:
    /**
     * Hide the selection rectangle
     *
     * Should be called after a cut or a copy has been
     * performed
     */
    void slotHideSelection();
    
protected:
    MatrixSelector(MatrixView*);

    void setViewCurrentSelection();
    
    //--------------- Data members ---------------------------------

    QCanvasRectangle* m_selectionRect;
    bool m_updateRect;

    int m_clickedStaff;
    MatrixStaff* m_currentStaff;

    MatrixElement* m_clickedElement;
};


class MatrixMover : public MatrixTool
{
    friend class MatrixToolBox;

public:
    virtual void handleLeftButtonPress(Rosegarden::timeT,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement*);

    /**
     * Set the duration of the element
     */
    virtual bool handleMouseMove(Rosegarden::timeT,
                                 int height,
                                 QMouseEvent*);

    /**
     * Actually insert the new element
     */
    virtual void handleMouseRelease(Rosegarden::timeT,
                                    int height,
                                    QMouseEvent*);

    static const QString ToolName;

protected:
    MatrixMover(MatrixView*);

    MatrixElement* m_currentElement;
    MatrixStaff* m_currentStaff;
};


class MatrixResizer : public MatrixPainter
{
    friend class MatrixToolBox;

public:
    virtual void handleLeftButtonPress(Rosegarden::timeT,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement*);

    /**
     * Set the duration of the element
     */
    virtual bool handleMouseMove(Rosegarden::timeT,
                                 int height,
                                 QMouseEvent*);

    /**
     * Actually insert the new element
     */
    virtual void handleMouseRelease(Rosegarden::timeT,
                                    int height,
                                    QMouseEvent*);

    static const QString ToolName;

protected:
    MatrixResizer(MatrixView*);
};


#endif
