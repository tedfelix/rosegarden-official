/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ControllerEventsRuler.h"

#include <klocale.h>
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
#include "ControlRuler.h"
#include "gui/general/EditViewBase.h"
#include "gui/widgets/TextFloat.h"
#include <klineeditdlg.h>
#include <qcanvas.h>
#include <qcolor.h>
#include <qpoint.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

ControllerEventsRuler::ControllerEventsRuler(Segment *segment,
        RulerScale* rulerScale,
        EditViewBase* parentView,
        QCanvas* c,
        QWidget* parent,
        const ControlParameter *controller,
        const char* name, WFlags f)
        : ControlRuler(segment, rulerScale, parentView, c, parent, name, f),
        m_defaultItemWidth(20),
        m_controlLine(new QCanvasLine(canvas())),
        m_controlLineShowing(false),
        m_controlLineX(0),
        m_controlLineY(0)
{
    // Make a copy of the ControlParameter if we have one
    //
    if (controller)
        m_controller = new ControlParameter(*controller);
    else
        m_controller = 0;

    setMenuName("controller_events_ruler_menu");
    drawBackground();
    init();
}

void
ControllerEventsRuler::setSegment(Segment *segment)
{
    RG_DEBUG << "ControllerEventsRuler::setSegment(" << segment << ")" << endl;

    m_segment->removeObserver(this);
    m_segment = segment;
    m_segment->addObserver(this);

    while (child(NULL))
        delete (child(NULL));

    drawBackground();
    init();
}

void
ControllerEventsRuler::init()
{
    // Reset range information for this controller type (for the moment
    // this assumes min is always 0.
    //
    setMaxItemValue(m_controller->getMax());

    for (Segment::iterator i = m_segment->begin();
            i != m_segment->end(); ++i) {

        // skip if not the same type of event that we're expecting
        //
        if (m_controller->getType() != (*i)->getType())
            continue;

        int width = getDefaultItemWidth();

        // Check for specific controller value if we need to
        //
        if (m_controller->getType() == Controller::EventType) {
            try {
                if ((*i)->get
                        <Int>(Controller::NUMBER)
                        != m_controller->getControllerValue())
                    continue;
            } catch (...) {
                continue;
            }
        } else if (m_controller->getType() == PitchBend::EventType)
            width /= 4;

        //RG_DEBUG << "ControllerEventsRuler: adding element\n";

        double x = m_rulerScale->getXForTime((*i)->getAbsoluteTime());
        new ControlItem(this, new ControllerEventAdapter(*i),
                        int(x + m_staffOffset), width);
    }
}

void
ControllerEventsRuler::drawBackground()
{
    // Draw some minimum and maximum controller value guide lines
    //
    QCanvasLine *topLine = new QCanvasLine(canvas());
    QCanvasLine *topQLine = new QCanvasLine(canvas());
    QCanvasLine *midLine = new QCanvasLine(canvas());
    QCanvasLine *botQLine = new QCanvasLine(canvas());
    QCanvasLine *bottomLine = new QCanvasLine(canvas());
    //m_controlLine->setPoints(m_controlLineX, m_controlLineY, m_controlLineX, m_controlLineY);
    int cHeight = canvas()->height();
    int cWidth = canvas()->width();

    topLine->setPen(QColor(127, 127, 127));
    topLine->setPoints(0, 0, cWidth, 0);
    topLine->setZ( -10);
    topLine->show();

    topQLine->setPen(QColor(192, 192, 192));
    topQLine->setPoints(0, cHeight / 4, cWidth, cHeight / 4);
    topQLine->setZ( -10);
    topQLine->show();

    midLine->setPen(QColor(127, 127, 127));
    midLine->setPoints(0, cHeight / 2, cWidth, cHeight / 2);
    midLine->setZ( -10);
    midLine->show();

    botQLine->setPen(QColor(192, 192, 192));
    botQLine->setPoints(0, 3*cHeight / 4, cWidth, 3*cHeight / 4);
    botQLine->setZ( -10);
    botQLine->show();

    bottomLine->setPen(QColor(127, 127, 127));
    bottomLine->setPoints(0, cHeight - 1, cWidth, cHeight - 1);
    bottomLine->setZ( -10);
    bottomLine->show();

    canvas()->update();
}

ControllerEventsRuler::~ControllerEventsRuler()
{}

QString ControllerEventsRuler::getName()
{
    if (m_controller) {
        QString name = i18n("Unsupported Event Type");

        if (m_controller->getType() == Controller::EventType) {
            QString hexValue;
            hexValue.sprintf("0x%x", m_controller->getControllerValue());

            name = QString("%1 (%2 / %3)").arg(strtoqstr(m_controller->getName()))
                   .arg(int(m_controller->getControllerValue()))
                   .arg(hexValue);
        } else if (m_controller->getType() == PitchBend::EventType) {
            name = i18n("Pitch Bend");
        }

        return name;
    } else
        return i18n("Controller Events");
}

void ControllerEventsRuler::eventAdded(const Segment*, Event *e)
{
    if (e->getType() != m_controller->getType())
        return ;

    // Check for specific controller value if we need to
    //
    if (e->getType() == Controller::EventType) {
        try {
            if (e->get
                    <Int>(Controller::NUMBER) !=
                    m_controller->getControllerValue())
                return ;
        } catch (...) {
            return ;
        }
    }

    RG_DEBUG << "ControllerEventsRuler::elementAdded()\n";

    double x = m_rulerScale->getXForTime(e->getAbsoluteTime());

    int width = getDefaultItemWidth();

    if (m_controller->getType() == PitchBend::EventType)
        width /= 4;

    new ControlItem(this, new ControllerEventAdapter(e), int(x + m_staffOffset), width);
}

void ControllerEventsRuler::eventRemoved(const Segment*, Event *e)
{
    if (e->getType() != m_controller->getType())
        return ;

    clearSelectedItems();

    QCanvasItemList allItems = canvas()->allItems();

    for (QCanvasItemList::Iterator it = allItems.begin(); it != allItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
            ControllerEventAdapter * adapter = dynamic_cast<ControllerEventAdapter*>(item->getElementAdapter());
            if (adapter->getEvent() == e) {
                delete item;
                break;
            }
        }
    }
}

void ControllerEventsRuler::insertControllerEvent()
{
    timeT insertTime = m_rulerScale->getTimeForX(m_lastEventPos.x());


    // compute initial value from cursor height
    //
    long initialValue = heightToValue(m_lastEventPos.y() - canvas()->height());

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
        QString res = KLineEditDlg::getText(i18n("Controller Event Number"), "0",
                                            &ok, this, &intValidator);
        if (ok)
            number = res.toULong();
    }

    ControlRulerEventInsertCommand* command =
        new ControlRulerEventInsertCommand(m_controller->getType(),
                                           insertTime, number,
                                           initialValue, *m_segment);

    m_parentEditView->addCommandToHistory(command);
}

void ControllerEventsRuler::eraseControllerEvent()
{
    RG_DEBUG << "ControllerEventsRuler::eraseControllerEvent() : deleting selected events\n";

    ControllerEventEraseCommand* command =
        new ControllerEventEraseCommand(m_selectedItems,
                                        *m_segment,
                                        m_eventSelection->getStartTime(),
                                        m_eventSelection->getEndTime());
    m_parentEditView->addCommandToHistory(command);
    updateSelection();
}

void ControllerEventsRuler::clearControllerEvents()
{
    EventSelection *es = new EventSelection(*m_segment);

    for (Segment::iterator it = m_segment->begin(); it != m_segment->end(); ++it) {
        if (!(*it)->isa(Controller::EventType))
            continue;
        {
            if (m_controller) // ensure we have only the controller events we want for this ruler
            {
                try
                {
                    if ((*it)->get
                            <Int>(Controller::NUMBER)
                            != m_controller->getControllerValue())
                        continue;
                } catch (...)
                {
                    continue;
                }

                es->addEvent(*it);
            }
        }
    }

    EraseCommand *command = new EraseCommand(*es);
    m_parentEditView->addCommandToHistory(command);

}

void ControllerEventsRuler::startControlLine()
{
    m_controlLineShowing = true;
    this->setCursor(Qt::pointingHandCursor);
}

void ControllerEventsRuler::contentsMousePressEvent(QMouseEvent *e)
{
    if (!m_controlLineShowing) {
        if (e->button() == MidButton)
            m_lastEventPos = inverseMapPoint(e->pos());

        ControlRuler::contentsMousePressEvent(e); // send super

        return ;
    }

    // cancel control line mode
    if (e->button() == RightButton) {
        m_controlLineShowing = false;
        m_controlLine->hide();

        this->setCursor(Qt::arrowCursor);
        return ;
    }

    if (e->button() == LeftButton) {
        QPoint p = inverseMapPoint(e->pos());

        m_controlLine->show();
        m_controlLineX = p.x();
        m_controlLineY = p.y();
        m_controlLine->setPoints(m_controlLineX, m_controlLineY, m_controlLineX, m_controlLineY);
        canvas()->update();
    }
}

void ControllerEventsRuler::contentsMouseReleaseEvent(QMouseEvent *e)
{
    if (!m_controlLineShowing) {
        if (e->button() == MidButton)
            insertControllerEvent();

        ControlRuler::contentsMouseReleaseEvent(e); // send super

        return ;
    } else {
        QPoint p = inverseMapPoint(e->pos());

        timeT startTime = m_rulerScale->getTimeForX(m_controlLineX);
        timeT endTime = m_rulerScale->getTimeForX(p.x());

        long startValue = heightToValue(m_controlLineY - canvas()->height());
        long endValue = heightToValue(p.y() - canvas()->height());

        RG_DEBUG << "ControllerEventsRuler::contentsMouseReleaseEvent - "
        << "starttime = " << startTime
        << ", endtime = " << endTime
        << ", startValue = " << startValue
        << ", endValue = " << endValue
        << endl;

        drawControlLine(startTime, endTime, startValue, endValue);

        m_controlLineShowing = false;
        m_controlLine->hide();
        this->setCursor(Qt::arrowCursor);
        canvas()->update();
    }
}

void ControllerEventsRuler::contentsMouseMoveEvent(QMouseEvent *e)
{
    if (!m_controlLineShowing) {
        // Don't send super if we're using the middle button
        //
        if (e->button() == MidButton) {
            m_lastEventPos = inverseMapPoint(e->pos());
            return ;
        }

        ControlRuler::contentsMouseMoveEvent(e); // send super
        return ;
    }

    QPoint p = inverseMapPoint(e->pos());

    m_controlLine->setPoints(m_controlLineX, m_controlLineY, p.x(), p.y());
    canvas()->update();

}

void ControllerEventsRuler::layoutItem(ControlItem* item)
{
    timeT itemTime = item->getElementAdapter()->getTime();

    double x = m_rulerScale->getXForTime(itemTime) + m_staffOffset;

    item->setX(x);

    int width = getDefaultItemWidth(); // TODO: how to scale that ??

    if (m_controller->getType() == PitchBend::EventType)
        width /= 4;

    item->setWidth(width);

    //RG_DEBUG << "ControllerEventsRuler::layoutItem ControlItem x = " << x
    //<< " - width = " << width << endl;
}

void
ControllerEventsRuler::drawControlLine(timeT startTime,
                                       timeT endTime,
                                       int startValue,
                                       int endValue)
{
    if (m_controller == 0)
        return ;
    if (startTime > endTime) {
        std::swap(startTime, endTime);
        std::swap(startValue, endValue);
    }

    timeT quantDur = Note(Note::Quaver).getDuration();

    // If inserting a line of PitchBends then we want a smoother curve
    //
    if (m_controller->getType() == PitchBend::EventType)
        quantDur = Note(Note::Demisemiquaver).getDuration();

    // for the moment enter a quantized set of events
    timeT time = startTime, newTime = 0;
    double step = double(endValue - startValue) / double(endTime - startTime);

    KMacroCommand *macro = new KMacroCommand(i18n("Add line of controllers"));

    while (time < endTime) {
        int value = startValue + int(step * double(time - startTime));

        // hit the buffers
        if (value < m_controller->getMin())
            value = m_controller->getMin();
        else if (value > m_controller->getMax())
            value = m_controller->getMax();

        ControlRulerEventInsertCommand* command =
            new ControlRulerEventInsertCommand(m_controller->getType(),
                                               time, m_controller->getControllerValue(), value, *m_segment);

        macro->addCommand(command);

        // get new time - do it by quantized distances
        newTime = (time / quantDur) * quantDur;
        if (newTime > time)
            time = newTime;
        else
            time += quantDur;
    }

    m_parentEditView->addCommandToHistory(macro);
}

}
