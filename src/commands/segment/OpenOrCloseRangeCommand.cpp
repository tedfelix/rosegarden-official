/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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


#include "OpenOrCloseRangeCommand.h"

#include <klocale.h>
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Selection.h"


namespace Rosegarden
{

OpenOrCloseRangeCommand::OpenOrCloseRangeCommand(Composition *composition,
        timeT rangeBegin,
        timeT rangeEnd,
        bool open) :
        KNamedCommand(i18n("Open or Close Range")),
        m_composition(composition),
        m_beginTime(rangeBegin),
        m_endTime(rangeEnd),
        m_prepared(false),
        m_opening(open)
{}

OpenOrCloseRangeCommand::~OpenOrCloseRangeCommand()
{}

void
OpenOrCloseRangeCommand::execute()
{
    timeT offset = m_endTime - m_beginTime;
    if (!m_opening)
        offset = -offset;

    if (m_opening) {
        if (offset + m_composition->getDuration() >
                m_composition->getEndMarker()) {
            m_composition->setEndMarker
            (m_composition->getBarEndForTime
             (m_composition->getDuration() + offset));
        }
    }

    if (!m_prepared) {

        timeT movingFrom = m_beginTime;
        if (!m_opening)
            movingFrom = m_endTime;

        for (Composition::iterator i = m_composition->begin();
                i != m_composition->end(); ++i) {

            if ((*i)->getStartTime() >= movingFrom) {
                m_moving.push_back(*i);
            }
        }

        m_timesigsPre = TimeSignatureSelection
                        (*m_composition, movingFrom,
                         m_composition->getEndMarker(),
                         false);

        m_temposPre = TempoSelection
                      (*m_composition, movingFrom,
                       m_composition->getEndMarker(),
                       false);

        for (TimeSignatureSelection::timesigcontainer::const_iterator i =
                    m_timesigsPre.begin(); i != m_timesigsPre.end(); ++i) {

            timeT t = i->first;
            TimeSignature sig = i->second;
            m_timesigsPost.addTimeSignature(t + offset, sig);
        }

        for (TempoSelection::tempocontainer::const_iterator i =
                    m_temposPre.begin(); i != m_temposPre.end(); ++i) {

            timeT t = i->first;
            TempoSelection::tempochange change = i->second;
            m_temposPost.addTempo(t + offset, change.first, change.second);
        }

        m_prepared = true;
    }

    for (std::vector<Segment *>::iterator i = m_moving.begin();
            i != m_moving.end(); ++i) {
        //	RG_DEBUG << "Moving segment on track " << (*i)->getTrack() << " from " << (*i)->getStartTime() << " to " << ((*i)->getStartTime() + offset) << " (current end time is " << (*i)->getEndTime() << ", end marker is " << (*i)->getEndMarkerTime() << ")" << endl;
        (*i)->setStartTime((*i)->getStartTime() + offset);
    }

    for (TimeSignatureSelection::timesigcontainer::const_iterator i =
                m_timesigsPre.begin(); i != m_timesigsPre.end(); ++i) {
        int n = m_composition->getTimeSignatureNumberAt(i->first);
        if (n >= 0)
            m_composition->removeTimeSignature(n);
    }

    for (TimeSignatureSelection::timesigcontainer::const_iterator i =
                m_timesigsPost.begin(); i != m_timesigsPost.end(); ++i) {
        m_composition->addTimeSignature(i->first, i->second);
    }

    for (TempoSelection::tempocontainer::const_iterator i =
                m_temposPre.begin(); i != m_temposPre.end(); ++i) {
        int n = m_composition->getTempoChangeNumberAt(i->first);
        if (n >= 0)
            m_composition->removeTempoChange(n);
    }

    for (TempoSelection::tempocontainer::const_iterator i =
                m_temposPost.begin(); i != m_temposPost.end(); ++i) {
        m_composition->addTempoAtTime(i->first, i->second.first, i->second.second);
    }
}

void
OpenOrCloseRangeCommand::unexecute()
{
    timeT offset = m_beginTime - m_endTime;
    if (!m_opening)
        offset = -offset;

    for (std::vector<Segment *>::iterator i = m_moving.begin();
            i != m_moving.end(); ++i) {
        (*i)->setStartTime((*i)->getStartTime() + offset);
    }

    for (TimeSignatureSelection::timesigcontainer::const_iterator i =
                m_timesigsPost.begin(); i != m_timesigsPost.end(); ++i) {
        int n = m_composition->getTimeSignatureNumberAt(i->first);
        if (n >= 0)
            m_composition->removeTimeSignature(n);
    }

    for (TimeSignatureSelection::timesigcontainer::const_iterator i =
                m_timesigsPre.begin(); i != m_timesigsPre.end(); ++i) {
        m_composition->addTimeSignature(i->first, i->second);
    }

    for (TempoSelection::tempocontainer::const_iterator i =
                m_temposPost.begin(); i != m_temposPost.end(); ++i) {
        int n = m_composition->getTempoChangeNumberAt(i->first);
        if (n >= 0)
            m_composition->removeTempoChange(n);
    }

    for (TempoSelection::tempocontainer::const_iterator i =
                m_temposPre.begin(); i != m_temposPre.end(); ++i) {
        m_composition->addTempoAtTime(i->first, i->second.first, i->second.second);
    }
}

}
