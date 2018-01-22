/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "LinkedSegmentsCommand.h"

#include "base/Composition.h"

#include <QtGlobal>

namespace Rosegarden
{

LinkedSegmentsCommand::LinkedSegmentsCommand(const QString &name,
					     SegmentVec originalSegments,
					     Composition *composition) :
  NamedCommand(name),
  m_originalSegments(originalSegments),
  m_composition(composition),
  m_detached(true)
{
#if !defined NDEBUG
  Q_ASSERT(!m_originalSegments.empty());
  timeT startTime = m_originalSegments[0]->getStartTime();
  for (SegmentVec::const_iterator i = m_originalSegments.begin();
       i != m_originalSegments.end();
       ++i) {
    Q_ASSERT((*i)->getStartTime() == startTime);
    if (m_originalSegments.size() > 1) {
      Q_CHECK_PTR((*i)->getLinker());
    }
  }
#endif            
}

LinkedSegmentsCommand::~LinkedSegmentsCommand(void)
{
    if (m_detached) {
        for (SegmentVec::iterator i = m_newSegments.begin();
             i != m_newSegments.end();
             ++i) {
            delete *i;
        }
    }
}
void
LinkedSegmentsCommand::executeAttachDetach(void)
{
    m_composition->detachAllSegments(m_originalSegments);
    m_composition->addAllSegments(m_newSegments);
    m_detached = false; // i.e. new segments are not detached
}

void
LinkedSegmentsCommand::unexecuteAttachDetach(void)
{
    m_composition->detachAllSegments(m_newSegments);
    m_composition->addAllSegments(m_originalSegments);
    m_detached = true; // i.e. new segments are detached
}

void
LinkedSegmentsCommand::
copyAuxProperties(Segment *source, Segment *target)
{
  // Set its track, repeatingness, etc
  target->setTrack(source->getTrack());
  target->setRepeating(source->isRepeating());
  target->setDelay(source->getDelay());
  target->setRealTimeDelay(source->getRealTimeDelay());
  // !!! Not sure which is required, maybe both.
  target->setTranspose(source->getTranspose());
  target->setLinkTransposeParams(source->getLinkTransposeParams());
}

}


