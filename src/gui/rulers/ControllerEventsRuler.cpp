/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ControllerEventsRuler]"

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
#include "commands/edit/EventInsertionCommand.h"
#include "commands/notation/EraseEventCommand.h"
#include "gui/general/EditViewBase.h"
#include "gui/general/GUIPalette.h"
#include "gui/widgets/LineEdit.h"
#include "gui/widgets/InputDialog.h"
#include "document/Command.h"
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


ControllerEventsRuler::ControllerEventsRuler(ViewSegment *segment,
        RulerScale* rulerScale,
        QWidget* parent,
        const ControlParameter *controller,
        const char* /* name */) //, WFlags f)
        : ControlRuler(segment, rulerScale, parent), // name, f),
        m_defaultItemWidth(20),
        m_lastDrawnRect(QRectF(0,0,0,0)),
        m_moddingSegment(false),
        m_rubberBand(new QLineF(0,0,0,0)),
        m_rubberBandVisible(false)
{
    // Make a copy of the ControlParameter if we have one
    //
    if (controller) {
        m_controller = new ControlParameter(*controller);
    }
    else {
        m_controller = 0;
    }

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
                if (event->get<Int>(Controller::NUMBER) == m_controller->getControllerValue()) result = true;
            } catch (...) {
                //
            }
        } else {
            // covers pitch bend, but isn't there a better test?
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
ControllerEventsRuler::setViewSegment(ViewSegment *segment)
{
    RG_DEBUG << "ControllerEventsRuler::setSegment(" << segment << ")" << endl;
    setSegment(&segment->getSegment());
}

void
ControllerEventsRuler::init()
{
    if (!m_controller)
        return;

    clear();
    
    // Reset range information for this controller type
    setMaxItemValue(m_controller->getMax());
    setMinItemValue(m_controller->getMin());

    for (Segment::iterator it = m_segment->begin();
            it != m_segment->end(); ++it) {
        if (isOnThisRuler(*it)) {
            addControlItem(*it);
        }
    }
    
    update();
}

void ControllerEventsRuler::paintEvent(QPaintEvent *event)
{
    ControlRuler::paintEvent(event);

    // If this is the first time we've drawn this view,
    //  reconfigure all items to make sure their icons
    //  come out the right size
    ///@TODO Only reconfigure all items if zoom has changed
    if (m_lastDrawnRect != m_pannedRect) {
        EventControlItem *item;
        for (ControlItemMap::iterator it = m_controlItemMap.begin(); it != m_controlItemMap.end(); ++it) {
            item = static_cast <EventControlItem *> (it->second);
            item->reconfigure();
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

    painter.setBrush(brush);
    painter.setPen(pen);

    QString str;
//    str = QString::fromStdString(m_controller->getName());
//    painter.drawText(10,20,str.toUpper());
    
    ControlItemMap::iterator mapIt;
    float lastX, lastY;
    lastX = m_rulerScale->getXForTime(m_segment->getStartTime());

    if (m_nextItemLeft != m_controlItemMap.end()) {
        EventControlItem *item = static_cast<EventControlItem*> (m_nextItemLeft->second);
        lastY = item->y();
    } else {
        lastY = valueToY(m_controller->getDefault());
    }
    
    mapIt = m_firstVisibleItem;
    while (mapIt != m_controlItemMap.end()) {
        EventControlItem *item = static_cast<EventControlItem*> (mapIt->second);
        painter.drawLine(mapXToWidget(lastX),mapYToWidget(lastY),
                mapXToWidget(item->xStart()),mapYToWidget(lastY));
        painter.drawLine(mapXToWidget(item->xStart()),mapYToWidget(lastY),
                mapXToWidget(item->xStart()),mapYToWidget(item->y()));
        lastX = item->xStart();
        lastY = item->y();
        if (mapIt == m_lastVisibleItem) {
            mapIt = m_controlItemMap.end();
        } else {
            ++mapIt;
        }
    }
    
    painter.drawLine(mapXToWidget(lastX),mapYToWidget(lastY),
            mapXToWidget(m_rulerScale->getXForTime(m_segment->getEndTime())*m_xScale),
            mapYToWidget(lastY));
    
    // Use a fast vector list to record selected items that are currently visible so that they
    //  can be drawn last - can't use m_selectedItems as this covers all selected, visible or not
    std::vector<ControlItem*> selectedvector;

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
    QFontMetrics fontMetrics(painter.font());
    int fontHeight = fontMetrics.height();
    int fontOffset = fontMetrics.width('+');
    
    for (std::vector<ControlItem*>::iterator it = selectedvector.begin(); it != selectedvector.end(); ++it)
    {
        // Draw the marker
        painter.drawPolygon(mapItemToWidget(*it));

        // For selected items, draw the value in text alongside the marker
        // By preference, this should sit on top of the new line that represents this value change
        str = QString::number(yToValue((*it)->y())-m_controller->getDefault());
        int x = mapXToWidget((*it)->xStart())+0.4*fontOffset;
        int y = std::max(mapYToWidget((*it)->y())-0.2f*fontHeight,float(fontHeight));
        
        painter.setPen(QPen(Qt::NoPen));
        painter.setBrush(QBrush(Qt::white));
        painter.drawRect(QRect(x,y+2,fontMetrics.width(str),-(fontMetrics.height()-2)));
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.drawText(x,y,str);
    }

    if (m_selectionRect) {
        pen.setColor(GUIPalette::getColour(GUIPalette::MatrixElementBorder));
        pen.setWidthF(0.5);
        painter.setPen(pen);
        brush.setStyle(Qt::NoBrush);
        painter.setBrush(brush);
        painter.drawRect(mapItemToWidget(m_selectionRect));
    }

    // draw the rubber band indicating where a line of controllers will go
    if (m_rubberBand && m_rubberBandVisible) {
        int x1 = mapXToWidget(m_rubberBand->x1());
        int y1 = mapYToWidget(m_rubberBand->y1());
        int x2 = mapXToWidget(m_rubberBand->x2());
        int y2 = mapYToWidget(m_rubberBand->y2());
        painter.setPen(Qt::red);
        painter.drawLine(x1, y1, x2, y2);
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
        eraseControlItem(event);
        update();
    }
}

void ControllerEventsRuler::segmentDeleted(const Segment *)
{
    m_segment = 0;
}

ControlItem* ControllerEventsRuler::addControlItem(Event *event)
{
    EventControlItem *controlItem = new EventControlItem(this, new ControllerEventAdapter(event), QPolygonF());
    controlItem->updateFromEvent();

    ControlRuler::addControlItem(controlItem);
    return controlItem;
}

ControlItem* ControllerEventsRuler::addControlItem(float x, float y)
{
    // Adds a ControlItem in the absence of an event (used by ControlPainter)
    clearSelectedItems();
    EventControlItem *item = new EventControlItem(this, new ControllerEventAdapter(0), QPolygonF());
    item->reconfigure(x,y);
    item->setSelected(true);
//    m_selectedItems.push_back(item);
//    if (isVisible(item)) {
//        m_visibleItems.push_back(item);
//    }
    ControlRuler::addControlItem(item);
    
    return item;
}

void
ControllerEventsRuler::addControlLine(float x1, float y1, float x2, float y2, bool eraseExistingControllers)
{
    std::cout << "ControllerEventsRuler::addControlLine()";
    clearSelectedItems();

    // get a timeT for one end point of our line
    timeT originTime = m_rulerScale->getTimeForX(x1);

    // get a timeT for the other end point of our line
    timeT destinationTime = m_rulerScale->getTimeForX(x2);

    // get a value for one end point of our line
    long originValue = yToValue(y1);
    
    // get a value for the other end point
    long destinationValue = yToValue(y2);

    if (originTime == destinationTime || originValue == destinationValue) return;

    // If the "anchor point" was to the right of the newly clicked destination
    // point, we're drawing a line from right to left.  We simply swap origin
    // for destination and calculate all lines as drawn from left to right, for
    // convenience and sanity.
    if (originTime > destinationTime) {
        timeT swapTime = originTime;
        originTime = destinationTime;
        destinationTime = swapTime;

        long swapValue = originValue;
        originValue = destinationValue;
        destinationValue = swapValue;
    }

    // save a list of existing events that occur within the span of the new line
    // for later deletion, if desired
    std::vector<Event*> eventsToClear;

    if (eraseExistingControllers) {
        for (Segment::iterator si = m_segment->begin();
             si != m_segment->end(); ++si) {

            timeT t = (*si)->getNotationAbsoluteTime();
            if (t >= originTime && t <= destinationTime) {
                eventsToClear.push_back(*si);
            }
        }
    }

    // rise and run
    long rise = destinationValue - originValue;
    timeT run = destinationTime - originTime;

    std::cout << "Drawing a line from origin time: " << originTime << " to " << destinationTime
              << " rising from: " << originValue << " to " << destinationValue 
              << " with a rise of: " << rise << " and run of: " << run
              << std::endl;

    // avoid divide by 0 potential, rise is always at least 1
    if (rise == 0) rise = 1;

    // are we rising or falling?
    bool rising = (rise > 0);

    // always calculate step on a positive value for rise, and make sure it's at
    // least 1
    float step = run / (float)(rising ? rise : rise * -1);

    // Trying this with pitch bend with a rise approaching the maximum over a
    // span of around four bars generated over 15,000 pitch bend events!  That's
    // super duper fine resolution, but it's too much for anything to handle.
    // Let's try to do some sensible thinning without getting too complicated...
    // bool isPitchBend = (m_controller->getType() == Rosegarden::PitchBend::EventType);
    int thinningHackCounter = 1;
    
    long intermediateValue = originValue;    
    long controllerNumber = 0;

    if (m_controller) {
        controllerNumber = m_controller->getControllerValue();
    } else {
        std::cout << "No controller number set.  Time to panic!  Line drawing aborted." << std::endl;
        return;
    }

    MacroCommand *macro = new MacroCommand(tr("Insert Line of Controllers"));

    bool failsafe = false;

    // if we're clearing existing events, add that to the macro command
    for (std::vector<Event*>::iterator ei = eventsToClear.begin();
         ei != eventsToClear.end(); ++ei) {

        // if the event was a controller or pitch bend, and it is on this ruler,
        // add it to the list
        if (((*ei)->isa(Controller::EventType) || (*ei)->isa(PitchBend::EventType))
            && isOnThisRuler(*ei)) {

            bool collapseRests = true;
            macro->addCommand(new EraseEventCommand(*m_segment,
                                                    *ei,
                                                    collapseRests));
       }
    }

    if (macro->haveCommands()) {
        CommandHistory::getInstance()->addCommand(macro);
        macro = new MacroCommand(tr("Insert Line of Controllers"));
    }

    for (float i = originTime; i <= destinationTime; i += step) {

        if (failsafe) continue;

        if (rising) intermediateValue++;
        else intermediateValue--;

        if (rising && intermediateValue > destinationValue) failsafe = true;
        else if (!rising && intermediateValue < destinationValue) failsafe = true;

//        std::cout << "creating event at time: " << i << " of value: " << intermediateValue << std::endl;
//        continue;

        Event *controllerEvent = new Event(m_controller->getType(), (timeT) i);

        if (m_controller->getType() == Rosegarden::Controller::EventType) {

            controllerEvent->set<Rosegarden::Int>(Rosegarden::Controller::VALUE, intermediateValue);
            controllerEvent->set<Rosegarden::Int>(Rosegarden::Controller::NUMBER, controllerNumber);

        } else if (m_controller->getType() == Rosegarden::PitchBend::EventType)   {

            // always set the first and last events, then only set every 25th,
            // 50th, and 75th event for pitch bend
            if (thinningHackCounter++ > 100) thinningHackCounter = 1;
            if (thinningHackCounter != 25 &&
                thinningHackCounter != 50 &&
                thinningHackCounter != 75 &&
                i != originTime           &&
                i != destinationTime) continue;

            std::cout << "intermediate value: " << intermediateValue << std::endl;

            // Convert to PitchBend MSB/LSB
            int lsb = intermediateValue & 0x7f;
            int msb = (intermediateValue >> 7) & 0x7f;
            controllerEvent->set<Rosegarden::Int>(Rosegarden::PitchBend::MSB, msb);
            controllerEvent->set<Rosegarden::Int>(Rosegarden::PitchBend::LSB, lsb);
        }

        if (failsafe) std::cout << "intermediate value: " << intermediateValue << " exceeded target: " << destinationValue << std::endl;

        macro->addCommand(new EventInsertionCommand (*m_segment, controllerEvent));
    }

    m_moddingSegment = true;
    CommandHistory::getInstance()->addCommand(macro);
    m_moddingSegment = false;
    
    // How else to re-initialize and bring things into view?  I'm missing
    // something, but this works...
    init();
}

void
ControllerEventsRuler::drawRubberBand(float x1, float y1, float x2, float y2)
{
    delete m_rubberBand;
    m_rubberBand = new QLineF(x1, y1, x2, y2);
    m_rubberBandVisible = true;
    repaint();
}

void
ControllerEventsRuler::stopRubberBand()
{
    m_rubberBandVisible = false;
    repaint();
}

void ControllerEventsRuler::slotSetTool(const QString &matrixtoolname)
{
    // Possible matrixtoolnames include:
    // selector, painter, eraser, mover, resizer, velocity
    QString controltoolname = "selector";
    if (matrixtoolname == "painter") controltoolname = "painter";
    if (matrixtoolname == "eraser") controltoolname = "eraser";
    if (matrixtoolname == "velocity") controltoolname = "adjuster";
    if (matrixtoolname == "mover") controltoolname = "mover";

    ControlTool *tool = dynamic_cast<ControlTool *>(m_toolBox->getTool(controltoolname));
    if (!tool) return;
    if (m_currentTool) m_currentTool->stow();
    m_currentTool = tool;
    m_currentTool->ready();
//    emit toolChanged(name);
}

Event *ControllerEventsRuler::insertEvent(float x, float y)
{
    timeT insertTime = m_rulerScale->getTimeForX(x/m_xScale);

    Event* controllerEvent = new Event(m_controller->getType(), insertTime);

    long initialValue = yToValue(y);

    RG_DEBUG << "ControllerEventsRuler::insertControllerEvent() : inserting event at "
    << insertTime
    << " - initial value = " << initialValue
    << endl;

    // ask controller number to user
    long number = 0;

    if (m_controller) {
        number = m_controller->getControllerValue();
    } else {

        //!!!
        // Weird.  I've never seen this in action in eight years.  I guess this
        // is some out there failsafe that never gets used in practice.  The
        // code looks wrong anyway.  0 to 128?
        //
        // Noting it with raised eyebrows and moving along for now.

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
    m_selectedItems.clear();
    updateSelection();
}


}
