/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2025 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[EventWidget]"
#define RG_NO_DEBUG_PRINT

#include "EventWidget.h"

#include "EditEvent.h"
#include "NoteWidget.h"
#include "RestWidget.h"
#include "ControllerWidget.h"
#include "ProgramChangeWidget.h"
#include "PitchBendWidget.h"
#include "ChannelPressureWidget.h"
#include "KeyPressureWidget.h"

#include "base/Event.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"


namespace Rosegarden
{


EventWidget *EventWidget::create(EditEvent *parent, const Event &event)
{
    const std::string type = event.getType();

    if (type == Note::EventType)
        return new NoteWidget(parent, event);
    if (type == Note::EventRestType)
        return new RestWidget(parent, event);
    if (type == Controller::EventType)
        return new ControllerWidget(parent, event);
    if (type == ProgramChange::EventType)
        return new ProgramChangeWidget(parent, event);
    if (type == PitchBend::EventType)
        return new PitchBendWidget(parent, event);
    if (type == ChannelPressure::EventType)
        return new ChannelPressureWidget(parent, event);
    if (type == KeyPressure::EventType)
        return new KeyPressureWidget(parent, event);
    //SystemExclusive::EventType

    // ??? Probably won't do these.  Notation editor handles these better.
    //Clef::EventType
    //Key::EventType
    //Indication::EventType
    //Text::EventType
    //Guitar::Chord::EventType

    return nullptr;
}

EventWidget::EventWidget(EditEvent *parent) :
    QWidget(parent)
{
}


}
