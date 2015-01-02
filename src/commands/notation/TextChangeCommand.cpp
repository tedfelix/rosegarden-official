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


#include "TextChangeCommand.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "document/BasicCommand.h"


namespace Rosegarden
{

TextChangeCommand::TextChangeCommand(Segment &segment,
                                     Event *event,
                                     Text text) :
        BasicCommand(tr("Edit Text"), segment,
                     event->getAbsoluteTime(), event->getAbsoluteTime() + 1,
                     true),  // bruteForceRedo
        m_event(event),
        m_text(text)
{
    // nothing
}

TextChangeCommand::~TextChangeCommand()
{}

void
TextChangeCommand::modifySegment()
{
    m_event->set
    <String>(Text::TextTypePropertyName, m_text.getTextType());
    m_event->set
    <String>(Text::TextPropertyName, m_text.getText());
}

}
