// -*- c-basic-offset: 4 -*-

#include "staffruler.h"

#include "qcanvaslinegroupable.h"
#include "qcanvasrectanglegroupable.h"


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


StaffRuler::StaffRuler(int xPos, int yPos, QCanvas* c)
    : m_canvas(c),
      m_xPos(xPos),
      m_yPos(yPos),
      m_stepLineHeight(10),
      m_subStepLineHeight(5),
      m_mainLine(new QCanvasLine(m_canvas)),
      m_cursor(new PositionCursor(m_canvas, m_canvas))
{
    QCanvasRectangle *rulerBackground = new QCanvasRectangle(0, 0,
                                                             m_canvas->width(), m_yPos, 
                                                             m_canvas);
    QColor bgColor(105, 170, 228);
    
    rulerBackground->setBrush(bgColor);
    rulerBackground->setPen(bgColor);
    rulerBackground->setZ(-1);
    rulerBackground->show();

    m_mainLine->setPoints(0, m_yPos, m_canvas->width(), m_yPos);
    
    m_mainLine->show();
    m_cursor->show();
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
    // TODO: perhaps recycle instead

    for (unsigned int i = 0; i < m_steps.size() - 1; ++i) {
         makeStep(i, m_steps[i].first, m_steps[i + 1].first, m_steps[i].second);
    }

//     m_mainLine->setPoints
// 	(m_xPos, m_yPos,
// 	 m_xPos + int(m_steps[m_steps.size() - 1].first) + 10, m_yPos);

}

void StaffRuler::makeStep(int stepValue,
                          double stepPos, double nextStepPos,
                          unsigned short nbSubsteps)
{
    // Make step line
    //
    QCanvasLine* stepLine = new QCanvasLine(m_canvas);
    
    stepLine->setPoints(int(stepPos) + m_xPos, m_yPos,
                        int(stepPos) + m_xPos, m_yPos - m_stepLineHeight);
    
    stepLine->show();

    // Make label
    //
    QString labelText;
    labelText.setNum(stepValue);

    QCanvasText* label = new QCanvasText(labelText, m_canvas);
    label->setX(stepPos + m_xPos);
    label->setY(m_yPos + 4);
    label->setTextFlags(Qt::AlignHCenter);

    // Prepare StepElement
    StepElement stepEl(stepLine, label);
    

    // Make substeps
    //
    double incr = (nextStepPos - stepPos) / nbSubsteps;

    for (double subStepPos = (stepPos + incr);
         subStepPos <= (nextStepPos - incr);
         subStepPos += incr) {

        QCanvasLine* subStep = new QCanvasLine(m_canvas);
    
        subStep->setPoints(int(subStepPos) + m_xPos, m_yPos,
                           int(subStepPos) + m_xPos, m_yPos - m_subStepLineHeight);
        subStep->show();

        stepEl.addSubStep(subStep);
    }

    label->show();

    m_stepElements.push_back(stepEl);
}

//////////////////////////////////////////////////////////////////////

PositionCursor::PositionCursor(QCanvas* c, QObject* parent)
    : QObject(parent),
      QCanvasItemGroup(c),
      m_grip(new QCanvasRectangleGroupable(c, this)),
      m_line(new QCanvasLineGroupable(c, this))
{
    m_grip->setX(-5);
    m_grip->setY(20);
    m_grip->setSize(11, 10);
    m_grip->setBrush(magenta);
    m_line->setPoints(0, 0, 0, canvas()->height());
    m_line->setPen(magenta);
}

void PositionCursor::setPosition(unsigned int pos)
{
    setX(pos);
}

