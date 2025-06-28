/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[KeyPressureRuler]"
#define RG_NO_DEBUG_PRINT

#include "KeyPressureRuler.h"

#include "base/ControlParameter.h"
#include "base/ViewElement.h"
#include "base/BaseProperties.h"
#include "base/RulerScale.h"
#include "base/SegmentPerformanceHelper.h"
#include "gui/general/GUIPalette.h"
#include "gui/rulers/EventControlItem.h"

#include <QPainter>

namespace Rosegarden
{

KeyPressureRuler::KeyPressureRuler(ViewSegment* segment,
                                   RulerScale* rulerScale,
                                   QWidget* parent,
                                   const ControlParameter *controller,
                                   const char* name) :
    ControllerEventsRuler(segment, rulerScale, parent, controller, name),
    m_notePitch(-1),
    m_noteStart(-1),
    m_noteDuration(-1)
{
}

KeyPressureRuler::~KeyPressureRuler()
{
}

void KeyPressureRuler::setElementSelection
(const std::vector<ViewElement *> &elementList)
{
    if (! m_segment) return;
    RG_DEBUG << "setElementSelection" << elementList.size();
    SegmentPerformanceHelper helper(*m_segment);
    m_notePitch = -1;
    SegmentPerformanceHelper::IteratorVector ivec;
    for (const ViewElement *element : elementList) {
        RG_DEBUG << "setElementSelection check element" << element;
        const Event* event = element->event();
        if (event->isa(Note::EventType)) {
            Segment::iterator it = m_segment->findSingle(event);
            // ignore note tied to the chosen note
            if (std::find(ivec.begin(), ivec.end(), it) != ivec.end()) continue;
            if (m_notePitch != -1) {
                RG_DEBUG << "setElementSelection more than one note";
                m_notePitch = -1;
                m_noteStart = -1;
                m_noteDuration = -1;
                break;
            }
            m_noteStart = event->getAbsoluteTime();
            m_notePitch = event->get<Int>(BaseProperties::PITCH);
            m_noteDuration = helper.getSoundingDuration(it);
            ivec = helper.getTiedNotes(it);
            RG_DEBUG << "setElementSelection settin pitch" << m_notePitch;
        }
    }

    // set element active flag
    for (ControlItemMap::iterator it = m_controlItemMap.begin();
         it != m_controlItemMap.end();
         ++it) {
        ControlItem* item = it->second.data();
        EventControlItem *ecItem = dynamic_cast<EventControlItem*>(item);
        if (ecItem == nullptr) continue;
        Event* controlEvent = ecItem->getEvent();
        long value;
        controlEvent->get<Rosegarden::Int>(Rosegarden::KeyPressure::PITCH,
                                           value);
        int cPitch = value;
        timeT controlTime = controlEvent->getAbsoluteTime();
        if (cPitch == m_notePitch &&
            controlTime >= m_noteStart &&
            controlTime < m_noteStart + m_noteDuration) {
            item->setActive(true);
        } else {
            item->setActive(false);
        }
    }

    // and clear any selection
    if (m_selectedItems.size() > 0) clearSelectedItems();
    update();
}

int KeyPressureRuler::getPitch() const
{
    RG_DEBUG << "getPitch" << m_notePitch;
    if (m_notePitch == -1) return 0; // always return a valid pitch
    return m_notePitch;
}

void KeyPressureRuler::paintEvent(QPaintEvent *event)
{
    ControlRuler::paintEvent(event);

    // If this is the first time we've drawn this view,
    //  reconfigure all items to make sure their icons
    //  come out the right size
    ///@TODO Only reconfigure all items if zoom has changed
    if (m_lastDrawnRect != m_pannedRect) {
        for (ControlItemMap::iterator it = m_controlItemMap.begin();
             it != m_controlItemMap.end();
             ++it) {
            it->second->reconfigure();
        }
        m_lastDrawnRect = m_pannedRect;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QBrush brush(GUIPalette::getColour(GUIPalette::ControlItem),Qt::SolidPattern);

    QPen pen(GUIPalette::getColour(GUIPalette::MatrixElementBorder),
            0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);

    painter.setBrush(brush);
    painter.setPen(pen);

    // only connect active items
    float lastX, lastY;
    bool first = true;
    for (ControlItemMap::iterator it = m_controlItemMap.begin();
         it != m_controlItemMap.end();
         ++it) {
        if (!it->second->active()) continue;
        QSharedPointer<ControlItem> item = it->second;
        if (first) {
            lastX = item->xStart();
            lastY = item->y();
            first = false;
            continue;
        }
        float itemX = item->xStart();
        float itemY = item->y();
        painter.drawLine(mapXToWidget(lastX), mapYToWidget(lastY),
                         mapXToWidget(itemX), mapYToWidget(lastY));
        painter.drawLine(mapXToWidget(itemX), mapYToWidget(itemY),
                         mapXToWidget(itemX), mapYToWidget(lastY));
        lastX = itemX;
        lastY = itemY;
    }

    drawItems(painter, pen, brush);
    drawSelectionRect(painter, pen, brush);
    drawRubberBand(painter);
}

void
KeyPressureRuler::setSegment(Segment *segment)
{
    ControllerEventsRuler::setSegment(segment); // create all items
    // and deactivate them
    for (ControlItemMap::iterator it = m_controlItemMap.begin();
         it != m_controlItemMap.end();
         ++it) {
        it->second->setActive(false);
    }
}

void KeyPressureRuler::getLimits(float& xmin, float& xmax)
{
    if (m_noteStart == -1) {// nothing selected
        xmin = -1;
        xmax = -1;
    } else {
        xmin = m_rulerScale->getXForTime(m_noteStart);
        xmax = m_rulerScale->getXForTime(m_noteStart + m_noteDuration);
    }
}

Event* KeyPressureRuler::getNewEvent(timeT time, long value) const
{
    Event* event = m_controller->newEvent(time, value);
    int pitch = getPitch();
    event->set<Rosegarden::Int>(Rosegarden::KeyPressure::PITCH, pitch);
    return event;
}

}
