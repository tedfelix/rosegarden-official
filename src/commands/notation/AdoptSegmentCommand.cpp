/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "AdoptSegmentCommand.h"

#include "gui/editors/notation/NotationView.h"

#include <QString>

namespace Rosegarden
{
AdoptSegmentCommand::
AdoptSegmentCommand(QString name, // 
		    NotationView &view,
		    Segment *segment,
		    bool into) :
  NamedCommand(name),
  m_view(view),
  m_segment(segment),
  m_into(into),
  m_detached(false),
  m_viewDestroyed(false)
{
    QObject::connect(&view, SIGNAL(destroyed()), this, SLOT(viewDestroyed()));
}
  
AdoptSegmentCommand::
~AdoptSegmentCommand()
{
    if (m_detached) {
        delete m_segment;
    }
}
void
AdoptSegmentCommand::slotViewdestroyed()
{ m_viewDestroyed = true; }

void
AdoptSegmentCommand::execute()
{
    if (m_into) { adopt(); }
    else { unadopt(); }
}
  
void
AdoptSegmentCommand::unexecute()
{
    if (m_into) { unadopt(); }
    else { adopt(); }
}
  
void
AdoptSegmentCommand::adopt()
{
    if (m_viewDestroyed) { return; }
    m_view.adoptSegment(m_segment);
    m_detached = false;
}

void
AdoptSegmentCommand::unadopt()
{
    if (m_viewDestroyed) { return; }
    m_view.unadoptSegment(m_segment);
    m_detached = true;
}

}



