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
//#define RG_NO_DEBUG_PRINT

#include "KeyPressureRuler.h"

#include "base/ViewElement.h"
#include "base/BaseProperties.h"

namespace Rosegarden
{

KeyPressureRuler::KeyPressureRuler(ViewSegment* segment,
                                   RulerScale* rulerScale,
                                   QWidget* parent,
                                   const ControlParameter *controller,
                                   const char* name) :
    ControllerEventsRuler(segment, rulerScale, parent, controller, name),
    m_notePitch(-1),
    m_noteStart(0),
    m_noteDuration(0)
{
}

KeyPressureRuler::~KeyPressureRuler()
{
}

void KeyPressureRuler::setElementSelection
(const std::vector<ViewElement *> &elementList)
{
    RG_DEBUG << "setElementSelection" << elementList.size();
    m_notePitch = -1;
    for (const ViewElement *element : elementList) {
        const Event* event = element->event();
        if (event->isa(Note::EventType)) {
            if (m_notePitch != -1) {
                RG_DEBUG << "setElementSelection more than one note";
                m_notePitch = -1;
                break;
            }
            m_notePitch = event->get<Int>(BaseProperties::PITCH);
            m_noteStart = event->getAbsoluteTime();
            m_noteDuration = event->getDuration();
            RG_DEBUG << "setElementSelection settin pitch" << m_notePitch;
        }
    }

}

int KeyPressureRuler::getPitch()
{
    RG_DEBUG << "getPitch" << m_notePitch;
    if (m_notePitch == -1) return 0; // always return a valid pitch
    return m_notePitch;
}

}
