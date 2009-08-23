/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

//#include <Q3Canvas>
//#include <Q3CanvasItemList>
//#include <Q3CanvasLine>

#include "ControllerEventsRuler.h"
#include "ControlRuler.h"
#include "EventControlItem.h"
#include "ControlTool.h"
#include "ControlToolBox.h"
#include "ControllerEventAdapter.h"
#include "ControlRulerEventInsertCommand.h"
#include "ControlRulerEventEraseCommand.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/ControlParameter.h"
#include "base/Event.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "commands/edit/EraseCommand.h"
#include "gui/general/EditViewBase.h"
#include "gui/general/GUIPalette.h"
#include "gui/widgets/LineEdit.h"
#include "gui/widgets/InputDialog.h"
#include "document/CommandHistory.h"

#include <QMouseEvent>
#include <QColor>
#include <QPoint>
#include <QString>
#include <QValidator>
#include <QWidget>
#include <QPainter>

namespace Rosegarden
{

ControllerEventsRuler::ControllerEventsRuler(MatrixViewSegment *segment,
        RulerScale* rulerScale,
//        EditViewBase* parentView,
//        Q3Canvas* c,
        QWidget* parent,
        const ControlParameter *controller,
        const char* name) //, WFlags f)
        //: ControlRuler(segment, rulerScale, parentView, c, parent), // name, f),
        //: ControlRuler(segment, rulerScale, parentView, parent), // name, f),
        : ControlRuler(segment, rulerScale, parent), // name, f),
        m_defaultItemWidth(20),
        m_lastDrawnRect(QRectF(0,0,0,0)),
        m_moddingSegment(false)
//        m_controlLine(new Q3CanvasLine(canvas())),
//        m_controlLineShowing(false),
//        m_controlLineX(0),
//        m_controlLineY(0)
{
    // Make a copy of the ControlParameter if we have one
    //
    if (controller) {
        m_controller = new ControlParameter(*controller);
    }
    else {
        m_controller = 0;
    }

    // This is necessary to run the overloaded method, the base method has already run
    setViewSegment(segment);

    setMenuName("controller_events_ruler_menu");
//    drawBackground(); Now in paintEvent
//    init();

    RG_DEBUG << "ControllerEventsRuler::ControllerEventsRuler - " << controller->getName();
    RG_DEBUG << "Segment from " << segment->getSegment().getStartTime() << " to " << segment->getSegment().getEndTime();
    RG_DEBUG << "Position x = " << rulerScale->getXForTime(segment->getSegment().getStartTime()) << " to " << rulerScale->getXForTime(segment->getSegment().getEndTime());
}

ControllerEventsRuler::~ControllerEventsRuler()
{
    RG_DEBUG << "ControllerEventsRuler::~ControllerEventsRuler()";
    if (m_segment) m_segment->removeObserver(this);
}

bool ControllerEventsRuler::isOnThisRuler(Event *event)
{
    // Check whether the received event is of the right type/number for this ruler
    bool result = false;
    if (event->getType() == m_controller->getType()) {
        if (event->getType() == Controller::EventType) {
            try {
                if (event->get<Int>(Controller::NUMBER) ==
                        m_controller->getControllerValue())
                    result = true;
            } catch (...) {
            }
        } else {
            result = true;
        }
    }
    RG_DEBUG << "ControllerEventsRuler::isOnThisRuler - "
        << "Event type: " << event->getType() << " Controller type: " << m_controller->getType();

    return result;
}

void
ControllerEventsRuler::setSegment(Segment *segment)
{
    if (m_segment) m_segment->removeObserver(this);
    m_segment = segment;
    m_segment->addObserver(this);
    ControlRuler::setSegment(segment);
    init();
}

void
ControllerEventsRuler::setViewSegment(MatrixViewSegment *segment)
{
    RG_DEBUG << "ControllerEventsRuler::setSegment(" << segment << ")" << endl;
    setSegment(&segment->getSegment());
}

void
ControllerEventsRuler::init()
{
    // Reset range information for this controller type
    if (!m_controller)
        return;

    setMaxItemValue(m_controller->getMax());
    setMinItemValue(m_controller->getMin());

    for (Segment::iterator it = m_segment->begin();
            it != m_segment->end(); it++) {
        if (isOnThisRuler(*it)) {
            addControlItem(*it);
        }
    }
}

void ControllerEventsRuler::paintEvent(QPaintEvent *event)
{
    ControlRuler::paintEvent(event);

    if (m_lastDrawnRect != m_pannedRect) {
        EventControlItem *item;
        for (ControlItemMap::iterator it = m_controlItemMap.begin(); it != m_controlItemMap.end(); it++) {
            item = static_cast <EventControlItem *> (it->second);
            item->updateFromEvent();
        }
        m_lastDrawnRect = m_pannedRect;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QBrush brush(GUIPalette::getColour(GUIPalette::ControlItem),Qt::SolidPattern);

//    QPen highlightPen(GUIPalette::getColour(GUIPalette::SelectedElement),
//            2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    QPen pen(GUIPalette::getColour(GUIPalette::MatrixElementBorder),
            0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);

    // Use a fast vector list to record selected items that are currently visible so that they
    //  can be drawn last - can't use m_selectedItems as this covers all selected, visible or not
    std::vector<ControlItem*> selectedvector;

    painter.setBrush(brush);
    painter.setPen(pen);
    for (ControlItemList::iterator it = m_visibleItems.begin(); it != m_visibleItems.end(); ++it) {
        if (!(*it)->isSelected()) {
            painter.drawPolygon(mapItemToWidget(*it));
        } else {
            selectedvector.push_back(*it);
        }
    }

//    painter.setBrush(brush);
    pen.setColor(GUIPalette::getColour(GUIPalette::SelectedElement));
    pen.setWidthF(2.0);
    painter.setPen(pen);
    for (std::vector<ControlItem*>::iterator it = selectedvector.begin(); it != selectedvector.end(); ++it)
    {
        painter.drawPolygon(mapItemToWidget(*it));
    }

    if (m_selectionRect) {
        pen.setColor(GUIPalette::getColour(GUIPalette::MatrixElementBorder));
        pen.setWidthF(0.5);
        painter.setPen(pen);
        brush.setStyle(Qt::NoBrush);
        painter.setBrush(brush);
        painter.drawRect(mapItemToWidget(m_selectionRect));
    }
}

QString ControllerEventsRuler::getName()
{
    if (m_controller) {
        QString name = tr("Unsupported Event Type");

        if (m_controller->getType() == Controller::EventType) {
            QString hexValue;
            hexValue.sprintf("0x%x", m_controller->getControllerValue());

            name = QString("%1 (%2 / %3)").arg(strtoqstr(m_controller->getName()))
                   .arg(int(m_controller->getControllerValue()))
                   .arg(hexValue);
        } else if (m_controller->getType() == PitchBend::EventType) {
            name = tr("Pitch Bend");
        }

        return name;
    } else
        return tr("Controller Events");
}

void ControllerEventsRuler::eventAdded(const Segment*, Event *event)
{
    // Segment observer call that an event has been added
    // If it should be on this ruler and it isn't already there
    //  add a ControlItem to display it
    // Note that ControlPainter will (01/08/09) add events directly
    //  these should not be replicated by this observer mechanism
    if (isOnThisRuler(event) && !m_moddingSegment) addControlItem(event);
}

void ControllerEventsRuler::eventRemoved(const Segment*, Event *event)
{
    // Segment observer notification of a removed event
    // Could be an erase action on the ruler or an undo/redo event

    // Old code did this ... not sure why
    //    clearSelectedItems();
    //
    if (isOnThisRuler(event) && !m_moddingSegment) {
        removeControlItem(event);
        update();
    }
}

void ControllerEventsRuler::segmentDeleted(const Segment *)
{
    m_segment = 0;
}

void ControllerEventsRuler::addControlItem(Event *event)
{
    EventControlItem *controlItem = new EventControlItem(this, new ControllerEventAdapter(event), QPolygonF());
    controlItem->updateFromEvent();

    ControlRuler::addControlItem(controlItem);
}

void ControllerEventsRuler::addControlItem(float x, float y)
{
    // Adds a ControlItem in the absence of an event (used by ControlPainter)
    clearSelectedItems();
    EventControlItem *controlItem = new EventControlItem(this, new ControllerEventAdapter(0), QPolygonF());
    controlItem->reconfigure(x,y);
    controlItem->setSelected(true);
    m_selectedItems.push_back(controlItem);
    ControlRuler::addControlItem(controlItem);
}

//void ControllerEventsRuler::removeControlItem(Event *event)
//{
//    RG_DEBUG << "ControllerEventsRuler::removeControlItem()";
//    for (ControlItemMap::iterator it = m_controlItemMap.begin(); it != m_controlItemMap.end(); ++it) {
//        if (EventControlItem *item = dynamic_cast<EventControlItem*>(it->second)) {
//            if (item->getEvent() == event) {
//    //                m_controlItemList.erase(it);
//    //                m_selectedItems.remove(item);
//    //                delete item;
//                removeControlItem(it);
//                RG_DEBUG << "Control item erased";
//                break;
//            }
//        }
//    }
//}

void ControllerEventsRuler::slotSetTool(const QString &matrixtoolname)
{
    // Possible matrixtoolnames include:
    // selector, painter, eraser, mover, resizer, velocity
    QString controltoolname = "selector";
    if (matrixtoolname == "painter") controltoolname = "painter";
    if (matrixtoolname == "eraser") controltoolname = "eraser";
    if (matrixtoolname == "velocity") controltoolname = "adjuster";
    ///TODO Write mechanism to select correct control tool for the given matrix tool
//    QString controltoolname = "painter";

    ControlTool *tool = dynamic_cast<ControlTool *>(m_toolBox->getTool(controltoolname));
    if (!tool) return;
    if (m_currentTool) m_currentTool->stow();
    m_currentTool = tool;
    m_currentTool->ready();
//    emit toolChanged(name);
}

Event *ControllerEventsRuler::insertEvent(float x, float y)
{
    timeT insertTime = m_rulerScale->getTimeForX(x);

    Event* controllerEvent = new Event(m_controller->getType(), insertTime);

    long initialValue = YToValue(y);

    RG_DEBUG << "ControllerEventsRuler::insertControllerEvent() : inserting event at "
    << insertTime
    << " - initial value = " << initialValue
    << endl;

    // ask controller number to user
    long number = 0;

    if (m_controller) {
        number = m_controller->getControllerValue();
    } else {
        bool ok = false;
        QIntValidator intValidator(0, 128, this);
//         QString res = KLineEditDlg::getText(tr("Controller Event Number"), "0",
//                                             &ok, this, &intValidator);
        QString res = InputDialog::getText(this, "", tr("Controller Event Number"),
                                           LineEdit::Normal, "0", &ok);

        if (ok)
            number = res.toULong();
    }

    if (m_controller->getType() == Rosegarden::Controller::EventType)
    {
        controllerEvent->set<Rosegarden::Int>(Rosegarden::Controller::VALUE, initialValue);
        controllerEvent->set<Rosegarden::Int>(Rosegarden::Controller::NUMBER, number);
    }
    else if (m_controller->getType() == Rosegarden::PitchBend::EventType)
    {
        // Convert to PitchBend MSB/LSB
        int lsb = initialValue & 0x7f;
        int msb = (initialValue >> 7) & 0x7f;
        controllerEvent->set<Rosegarden::Int>(Rosegarden::PitchBend::MSB, msb);
        controllerEvent->set<Rosegarden::Int>(Rosegarden::PitchBend::LSB, lsb);
    }

    m_moddingSegment = true;
    m_segment->insert(controllerEvent);
    m_moddingSegment = false;

    return controllerEvent;
//    ControlRulerEventInsertCommand* command =
//        new ControlRulerEventInsertCommand(m_controller->getType(),
//                                           insertTime, number,
//                                           initialValue, *m_segment);
//
//    CommandHistory::getInstance()->addCommand(command);
}

void ControllerEventsRuler::eraseEvent(Event *event)
{
    m_moddingSegment = true;
    m_segment->eraseSingle(event);
    m_moddingSegment = false;
}

void ControllerEventsRuler::eraseControllerEvent()
{
    RG_DEBUG << "ControllerEventsRuler::eraseControllerEvent() : deleting selected events\n";

    // This command uses the SegmentObserver mechanism to bring the control item list up to date
    ControlRulerEventEraseCommand* command =
        new ControlRulerEventEraseCommand(m_selectedItems,
                                        *m_segment,
                                        m_eventSelection->getStartTime(),
                                        m_eventSelection->getEndTime());
    CommandHistory::getInstance()->addCommand(command);
    updateSelection();
}

void ControllerEventsRuler::clearControllerEvents()
{
    EventSelection *es = new EventSelection(*m_segment);

    for (Segment::iterator it = m_segment->begin(); it != m_segment->end(); ++it) {
        if (isOnThisRuler(*it)) {
            es->addEvent(*it);
        }
//        if (!(*it)->isa(Controller::EventType))
//            continue;
//        {
//            if (m_controller) // ensure we have only the controller events we want for this ruler
//            {
//                try
//                {
//                    if ((*it)->get
//                            <Int>(Controller::NUMBER)
//                            != m_controller->getControllerValue())
//                        continue;
//                } catch (...)
//                {
//                    continue;
//                }
//
//                es->addEvent(*it);
//            }
//        }
    }

    EraseCommand *command = new EraseCommand(*es);
    CommandHistory::getInstance()->addCommand(command);
}

//void ControllerEventsRuler::startControlLine()
//{
//    m_controlLineShowing = true;
//    this->setCursor(Qt::pointingHandCursor);
//}
//
//void ControllerEventsRuler::contentsMousePressEvent(QMouseEvent *e)
//{
////    if (!m_controlLineShowing) {
//        if (e->button() == Qt::MidButton)
////            m_lastEventPos = inverseMapPoint(e->pos());
//            m_lastEventPos = e->pos();
//
//        ControlRuler::mousePressEvent(e); // send super
//
////        return ;
////    }
//
//    // cancel control line mode
////    if (e->button() == Qt::RightButton) {
////        m_controlLineShowing = false;
////        m_controlLine->hide();
//
////        this->setCursor(Qt::arrowCursor);
////        return ;
////    }
//
////    if (e->button() == Qt::LeftButton) {
////        QPoint p = inverseMapPoint(e->pos());
//
////        m_controlLine->show();
////        m_controlLineX = p.x();
////        m_controlLineY = p.y();
////        m_controlLine->setPoints(m_controlLineX, m_controlLineY, m_controlLineX, m_controlLineY);
////        canvas()->update();
////    }
//}
//
//void ControllerEventsRuler::contentsMouseReleaseEvent(QMouseEvent *e)
//{
////    if (!m_controlLineShowing) {
////        if (e->button() == Qt::MidButton)
////            insertControllerEvent();
//
//        ControlRuler::mouseReleaseEvent(e); // send super
//
////        return ;
////    } else {
//        //QPoint p = inverseMapPoint(e->pos());
//
//        //timeT startTime = m_rulerScale->getTimeForX(m_controlLineX);
//        //timeT endTime = m_rulerScale->getTimeForX(p.x());
//
//        //long startValue = heightToValue(m_controlLineY - canvas()->height());
//        //long endValue = heightToValue(p.y() - canvas()->height());
//
//        //RG_DEBUG << "ControllerEventsRuler::contentsMouseReleaseEvent - "
//        //<< "starttime = " << startTime
//        //<< ", endtime = " << endTime
//        //<< ", startValue = " << startValue
//        //<< ", endValue = " << endValue
//        //<< endl;
//
//        //drawControlLine(startTime, endTime, startValue, endValue);
//
//        //m_controlLineShowing = false;
//        //m_controlLine->hide();
//        //this->setCursor(Qt::arrowCursor);
//        //canvas()->update();
//    //}
//}
//
//void ControllerEventsRuler::contentsMouseMoveEvent(QMouseEvent *e)
//{
////    if (!m_controlLineShowing) {
//        // Don't send super if we're using the middle button
//        //
//        if (e->button() == Qt::MidButton) {
////            m_lastEventPos = inverseMapPoint(e->pos());
//            m_lastEventPos = e->pos();
//            return ;
//        }
//
//        ControlRuler::mouseMoveEvent(e); // send super
////        return ;
////    }
//
////    QPoint p = inverseMapPoint(e->pos());
//
////    m_controlLine->setPoints(m_controlLineX, m_controlLineY, p.x(), p.y());
////    canvas()->update();
//
//}

//void ControllerEventsRuler::layoutItem(ControlItem* item)
//{
////    timeT itemTime = item->getElementAdapter()->getTime();
//    timeT itemTime = 0;
//
//    double x = m_rulerScale->getXForTime(itemTime) + m_viewSegmentOffset;
//
//    item->setX(x);
//
//    int width = getDefaultItemWidth(); // TODO: how to scale that ??
//
//    if (m_controller->getType() == PitchBend::EventType)
//        width /= 4;
//
//    item->setWidth(width);
//
//    //RG_DEBUG << "ControllerEventsRuler::layoutItem ControlItem x = " << x
//    //<< " - width = " << width << endl;
//}

//void
//ControllerEventsRuler::drawControlLine(timeT startTime,
                                       //timeT endTime,
                                       //int startValue,
                                       //int endValue)
//{
    //if (m_controller == 0)
        //return ;
    //if (startTime > endTime) {
        //std::swap(startTime, endTime);
        //std::swap(startValue, endValue);
    //}

    //timeT quantDur = Note(Note::Quaver).getDuration();

    //// If inserting a line of PitchBends then we want a smoother curve
    ////
    //if (m_controller->getType() == PitchBend::EventType)
        //quantDur = Note(Note::Demisemiquaver).getDuration();

    //// for the moment enter a quantized set of events
    //timeT time = startTime, newTime = 0;
    //double step = double(endValue - startValue) / double(endTime - startTime);

    //MacroCommand *macro = new MacroCommand(tr("Add line of controllers"));

    //while (time < endTime) {
        //int value = startValue + int(step * double(time - startTime));

        //// hit the buffers
        //if (value < m_controller->getMin())
            //value = m_controller->getMin();
        //else if (value > m_controller->getMax())
            //value = m_controller->getMax();

        //ControlRulerEventInsertCommand* command =
            //new ControlRulerEventInsertCommand(m_controller->getType(),
                                               //time, m_controller->getControllerValue(), value, *m_segment);

        //macro->addCommand(command);

        //// get new time - do it by quantized distances
        //newTime = (time / quantDur) * quantDur;
        //if (newTime > time)
            //time = newTime;
        //else
            //time += quantDur;
    //}

    //CommandHistory::getInstance()->addCommand(macro);
//}

}
