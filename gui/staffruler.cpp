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

#include "rosedebug.h"
#include "colours.h"

#include "staffruler.h"

#include "qcanvasgroupableitem.h"

// ActiveItem::ActiveItem(QCanvasItem* item)
// {
//     item->setActive(true);
// }

//////////////////////////////////////////////////////////////////////

StaffRuler::StepElement::StepElement(QCanvasLine* l, QCanvasText* t)
    : stepLine(l), label(t)
{
}

StaffRuler::StepElement::StepElement(const StepElement& e)
    : stepLine(e.stepLine), label(e.label),
      subSteps(e.subSteps)
{
}

StaffRuler::StepElement& StaffRuler::StepElement::operator=(const StaffRuler::StepElement& e)
{
    stepLine = e.stepLine;
    label = e.label;
    subSteps = e.subSteps;

    return *this;
}


void StaffRuler::StepElement::addSubStep(QCanvasLine* l)
{
    subSteps.push_back(l);
}


StaffRuler::StaffRuler(int xPos, int yPos, int thickness,
                       QCanvas* c)
    : QCanvasItemGroup(c),
      m_xPos(xPos),
      m_yPos(yPos),
      m_thickness(thickness),
      m_mainLinePos(m_thickness / 4),
      m_stepLineHeight(thickness / 6),
      m_subStepLineHeight(thickness / 10),
      m_mainLine(new QCanvasLineGroupable(c, this)),
      m_greyBackground(new QCanvasRectangleGroupable(c, this)),
      m_whiteBackground(new QCanvasRectangleGroupable(c, this)),
      m_cursor(new PositionCursor(m_mainLinePos + m_yPos + thickness/4,
				  canvas(), canvas())),
      m_barNumberFont("helvetica", 8, QFont::Normal)
{
    m_greyBackground->setX(0);
    m_greyBackground->setY(m_yPos);
    m_greyBackground->setZ(-1);
    m_greyBackground->setSize(canvas()->width(), m_thickness/4);

    m_whiteBackground->setX(0);
    m_whiteBackground->setY(m_yPos);
    m_whiteBackground->setSize(canvas()->width(), m_thickness/2);
    
    m_greyBackground->setBrush(RosegardenGUIColours::StaffRulerBackground);
    m_greyBackground->setPen(RosegardenGUIColours::StaffRulerBackground);
    m_whiteBackground->setZ(-4);

    m_whiteBackground->setBrush(white);
    m_whiteBackground->setPen(white);
    m_whiteBackground->setZ(-5);

    m_mainLine->setPoints(0, m_mainLinePos + m_yPos,
                          canvas()->width(), m_mainLinePos + m_yPos);
    m_mainLine->setZ(1);

    if (thickness < 30) thickness = 30;
    m_barNumberFont.setPixelSize(thickness / 2 - 10);

    setActive(true);
}

void StaffRuler::clearSteps()
{
    m_steps.clear();

    // TODO: recycle items instead
    for (unsigned int i = 0; i < m_stepElements.size(); ++i) {
        StepElement& stepEl = m_stepElements[i];
        delete stepEl.stepLine;
        delete stepEl.label;
        for (unsigned int j = 0; j < stepEl.subSteps.size(); ++j)
            delete stepEl.subSteps[j];
    }

    m_stepElements.clear();

}

void StaffRuler::addStep(double stepPos, unsigned short subSteps)
{
    m_steps.push_back(StepDef(stepPos, subSteps));
}

void StaffRuler::update()
{
    if (m_steps.size() == 0) return;

    // TODO: perhaps recycle instead

    double maxStepPos = 0;

    for (unsigned int i = 0; i < m_steps.size() - 1; ++i) {
	makeStep(i, m_steps[i].first, m_steps[i + 1].first, m_steps[i].second);
	maxStepPos = m_steps[i + 1].first;
    }

    m_cursor->setMinPosition(int(m_steps[0].first) + m_xPos);

    m_greyBackground->setSize((int)maxStepPos, m_greyBackground->height());
    m_whiteBackground->setSize((int)maxStepPos, m_whiteBackground->height());
    m_mainLine->setPoints(0, m_mainLinePos + m_yPos,
			  (int)maxStepPos, m_mainLinePos + m_yPos);

    setActive(true); // set steps active
}

void StaffRuler::makeStep(int stepValue,
                          double stepPos, double nextStepPos,
                          unsigned short nbSubsteps)
{
//     kdDebug(KDEBUG_AREA) << "StaffRuler::makeStep: stepValue = " << stepValue
// 			 << ", stepPos = " << stepPos << ", nextStepPos = "
// 			 << nextStepPos << ", nbSubsteps = " << nbSubsteps
// 			 << endl;

    if (stepPos == nextStepPos) return; // yes, this can happen

    // Make step line
    //
    QCanvasLineGroupable* stepLine = new QCanvasLineGroupable(canvas(), this);
    
    stepLine->setPoints(int(stepPos) + m_xPos, 0,
                        int(stepPos) + m_xPos, -m_stepLineHeight);
    stepLine->setY(m_mainLinePos + m_yPos);

    // Make label
    //
    QString labelText;
    labelText.setNum(stepValue);

    QCanvasText* label = new QCanvasTextGroupable(labelText, canvas(), this);
    label->setX(stepPos + m_xPos);
    label->setY(m_mainLinePos + m_yPos + 4);
    label->setTextFlags(Qt::AlignHCenter);
    label->setFont(m_barNumberFont);

    // Prepare StepElement
    StepElement stepEl(stepLine, label);
    

    // Make substeps
    //
    double incr = (nextStepPos - stepPos) / nbSubsteps;

    for (double subStepPos = (stepPos + incr);
         subStepPos <= (nextStepPos - incr);
         subStepPos += incr) {

        QCanvasLineGroupable* subStep = new QCanvasLineGroupable(canvas(), this);
    
        subStep->setPoints(int(subStepPos) + m_xPos, 0,
                           int(subStepPos) + m_xPos, -m_subStepLineHeight);
        subStep->setY(m_mainLinePos + m_yPos);

        stepEl.addSubStep(subStep);
    }

    m_stepElements.push_back(stepEl);
}

void StaffRuler::resize()
{
    m_greyBackground->setSize(canvas()->width(), m_greyBackground->height());
    m_whiteBackground->setSize(canvas()->width(), m_whiteBackground->height());
    m_mainLine->setPoints(0, m_mainLinePos + m_yPos,
			  canvas()->width(), m_mainLinePos + m_yPos);
}

void StaffRuler::setXPos(int xpos)
{
    double deltaX = xpos - m_xPos;

    moveBy(deltaX, 0);
    m_xPos = xpos;
}

void StaffRuler::setYPos(int ypos)
{
    double deltaY = ypos - m_yPos;

    moveBy(0, deltaY);
    m_yPos = ypos;

    m_cursor->setGripHeight(m_mainLinePos + m_yPos + 20);
}

void StaffRuler::handleMousePress(QMouseEvent* e)
{
    setCursorPosition(e->x());
}

void StaffRuler::handleMouseMove(QMouseEvent* e)
{
    setCursorPosition(e->x());
}

void StaffRuler::handleMouseRelease(QMouseEvent* e)
{
    setCursorPosition(e->x());
}



//////////////////////////////////////////////////////////////////////

PositionCursor::PositionCursor(int gripHeight, QCanvas* c, QObject* parent)
    : QObject(parent),
      QCanvasItemGroup(c),
      m_grip(new QCanvasRectangleGroupable(c, this)),
      m_line(new QCanvasLineGroupable(c, this)),
      m_minPos(0)
{
    m_grip->setX(-5);
    m_grip->setY(gripHeight);
    m_grip->setSize(11, 10);
    m_grip->setBrush(RosegardenGUIColours::InsertCursor);
    m_line->setPoints(0, 0, 0, canvas()->height());
    m_line->setPen(RosegardenGUIColours::InsertCursor);

    setActive(true);
}

void PositionCursor::slotSetPosition(int pos)
{
    // stealthly readjust length in case the canvas has changed height
    m_line->setPoints(0, 0, 0, canvas()->height());

    setX((pos > getMinPosition()) ? pos : getMinPosition());
}

void PositionCursor::handleMousePress(QMouseEvent*)
{
    m_line->setPoints(0, 0, 0, canvas()->height());
}

void PositionCursor::handleMouseMove(QMouseEvent* e)
{
    if (e->x() > getMinPosition())
       slotSetPosition(e->x());
    else
       slotSetPosition(getMinPosition());
}

void PositionCursor::handleMouseRelease(QMouseEvent* e)
{
    if (e->x() > getMinPosition())
       slotSetPosition(e->x());

    emit positionChange(getPosition());
}

void PositionCursor::setMinPosition(int p)
{
    m_minPos = p;

    if (getPosition() < getMinPosition())
       slotSetPosition(getMinPosition());
}

void PositionCursor::setGripHeight(int p)
{
    m_grip->setY(p);
}

