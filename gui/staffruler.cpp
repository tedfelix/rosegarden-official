// -*- c-basic-offset: 4 -*-

#include "rosedebug.h"

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


StaffRuler::StaffRuler(int xPos, int yPos,
                       QCanvas* c)
    : QCanvasItemGroup(c),
      m_xPos(xPos),
      m_yPos(yPos),
      m_thickness(15),
      m_mainLinePos(m_yPos + m_thickness),
      m_stepLineHeight(10),
      m_subStepLineHeight(5),
      m_mainLine(new QCanvasLineGroupable(c, this)),
      m_greyBackground(new QCanvasRectangleGroupable(c, this)),
      m_whiteBackground(new QCanvasRectangleGroupable(c, this)),
      m_cursor(new PositionCursor(m_mainLinePos + m_yPos + 20, canvas(), canvas()))
{
    m_greyBackground->setX(0);
    m_greyBackground->setY(m_yPos);
    m_greyBackground->setSize(canvas()->width(), m_thickness);

    m_whiteBackground->setX(0);
    m_whiteBackground->setY(m_yPos);
    m_whiteBackground->setSize(canvas()->width(), m_thickness + 15);

    QColor bgColor(212, 212, 212); // light grey
    
    m_greyBackground->setBrush(bgColor);
    m_greyBackground->setPen(bgColor);
    m_greyBackground->show();

    m_whiteBackground->setBrush(white);
    m_whiteBackground->setPen(white);
    m_whiteBackground->show();
    m_whiteBackground->setZ(-1);

    m_mainLine->setPoints(0, m_mainLinePos,
                          canvas()->width(), m_mainLinePos);
    //m_mainLine->setPen(red); // DEBUG

    m_mainLine->show();
    m_cursor->show();

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

    for (unsigned int i = 0; i < m_steps.size() - 1; ++i) {
         makeStep(i, m_steps[i].first, m_steps[i + 1].first, m_steps[i].second);
    }

    m_cursor->setMinPosition(int(m_steps[0].first) + m_xPos);

//     m_mainLine->setPoints
// 	(m_xPos, m_yPos,
// 	 m_xPos + int(m_steps[m_steps.size() - 1].first) + 10, m_yPos);

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
    stepLine->show();

    // Make label
    //
    QString labelText;
    labelText.setNum(stepValue);

    QCanvasText* label = new QCanvasTextGroupable(labelText, canvas(), this);
    label->setX(stepPos + m_xPos);
    label->setY(m_mainLinePos + 4 + m_yPos);
    label->setTextFlags(Qt::AlignHCenter);

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
        subStep->show();

        stepEl.addSubStep(subStep);
    }

    label->show();

    m_stepElements.push_back(stepEl);
}

void StaffRuler::resize()
{
    m_greyBackground->setSize(canvas()->width(), m_greyBackground->height());
    m_whiteBackground->setSize(canvas()->width(), m_whiteBackground->height());
    m_mainLine->setPoints(0, m_yPos, canvas()->width(), m_yPos);
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
    m_grip->setBrush(magenta);
    m_line->setPoints(0, 0, 0, canvas()->height());
    m_line->setPen(magenta);

    setActive(true);
}

void PositionCursor::setPosition(int pos)
{
    setX((pos > getMinPosition()) ? pos : getMinPosition());
}

void PositionCursor::handleMousePress(QMouseEvent*)
{
}

void PositionCursor::handleMouseMove(QMouseEvent* e)
{
    if (e->x() > getMinPosition())
        setPosition(e->x());
    else
        setPosition(getMinPosition());
}

void PositionCursor::handleMouseRelease(QMouseEvent* e)
{
    if (e->x() > getMinPosition())
        setPosition(e->x());

    emit positionChange(getPosition());
}

void PositionCursor::setMinPosition(int p)
{
    m_minPos = p;

    if (getPosition() < getMinPosition())
        setPosition(getMinPosition());
}

void PositionCursor::setGripHeight(int p)
{
    m_grip->setY(p);
}

