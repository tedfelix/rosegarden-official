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

#include "NoteWidget.h"
#include "base/Event.h"
#include "base/NotationTypes.h"


namespace Rosegarden
{


EventWidget *EventWidget::create(QWidget *parent, const Event &event)
{
    if (event.getType() == Note::EventType)
        return new NoteWidget(parent, event);

    return nullptr;
}


}
