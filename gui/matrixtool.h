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
