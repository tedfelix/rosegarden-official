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

#ifndef STAFFRULER_H
#define STAFFRULER_H

#include <vector>
#include <qcanvas.h>

#include "qcanvasitemgroup.h"

class QCanvasRectangleGroupable;
class QCanvasLineGroupable;

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


class PositionCursor : public QObject, public QCanvasItemGroup, public ActiveItem
{
    Q_OBJECT

public:
    PositionCursor(int gripHeight, QCanvas*, QObject* parent = 0);

    int getPosition() const { return int(x()); }

    int getMinPosition() const { return m_minPos; }
    void setMinPosition(int p);

    // ActiveItem interface
    virtual void handleMousePress(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);
    virtual void handleMouseRelease(QMouseEvent*);

public slots:
    void setPosition(int pos);

signals:
    void positionChange(int pos);

protected:
    QCanvasRectangleGroupable* m_grip;
    QCanvasLineGroupable* m_line;

    int m_minPos;
};

//////////////////////////////////////////////////////////////////////

/**
 * The StaffRuler is a ruler shown above the staff in the Notation
 * window.
 *
 * It also deals with the position cursor
 */
class StaffRuler : public QCanvasItemGroup, public ActiveItem
{
public:
    typedef std::pair<double, unsigned short> StepDef;
    typedef std::vector<StepDef> Steps;

    StaffRuler(int xPos, int yPos, QCanvas*);

    void clearSteps();
    void addStep(double stepPos, unsigned short subSteps);

    /// Re-create all the steps and substeps
    void update();

    void setCursorPosition(unsigned int pos) { m_cursor->setPosition(pos); }
    unsigned int getCursorPosition() const   { return int(m_cursor->x());  }
    
    PositionCursor* getCursor() { return m_cursor; }

    /// reset ruler width to canvas width
    void resize();

    // ActiveItem interface
    virtual void handleMousePress(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);
    virtual void handleMouseRelease(QMouseEvent*);

protected:
    struct StepElement
    {
        StepElement(QCanvasLine* l, QCanvasText* t);
        StepElement(const StepElement&);
        StepElement& operator=(const StepElement& e);

        void addSubStep(QCanvasLine*);

        QCanvasLine* stepLine;
        QCanvasText* label;
        std::vector<QCanvasLine*> subSteps;
    };
    
    void makeStep(int stepValue,
                  double pos, double nextStepPos,
                  unsigned short nbSubsteps);

    int m_xPos; // mainLine X pos
    int m_yPos; // mainLine Y pos

    int m_stepLineHeight,
        m_subStepLineHeight;

    QCanvasLineGroupable* m_mainLine;
    QCanvasRectangleGroupable* m_background;

    PositionCursor* m_cursor;

    Steps m_steps;
    
    std::vector<StepElement> m_stepElements;
};

#endif
