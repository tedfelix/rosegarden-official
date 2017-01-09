/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ClefInsertionCommand.h"

#include "misc/Strings.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/BaseProperties.h"
#include "document/BasicCommand.h"
#include "base/Selection.h"
#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;

ClefInsertionCommand::ClefInsertionCommand(Segment &segment, timeT time,
        Clef clef,
        bool shouldChangeOctave,
        bool shouldTranspose) :
        BasicCommand(getThisGlobalName(&clef), segment, time,
                     ((shouldChangeOctave || shouldTranspose) ?
                      segment.getEndTime() : time + 1)),
        m_clef(clef),
        m_shouldChangeOctave(shouldChangeOctave),
        m_shouldTranspose(shouldTranspose),
        m_lastInsertedEvent(0)
{
    // nothing
}

ClefInsertionCommand::~ClefInsertionCommand()
{
    // nothing
}

EventSelection *
ClefInsertionCommand::getSubsequentSelection()
{
    EventSelection *selection = new EventSelection(getSegment());
    selection->addEvent(getLastInsertedEvent());
    return selection;
}

QString 
ClefInsertionCommand::getThisGlobalName(Clef *clef)
{
    return getGlobalName(clef);
}

QString
ClefInsertionCommand::getGlobalName(Clef *)
{
    /* doesn't handle octave offset -- leave it for now
        if (clef) {
    	QString name(strtoqstr(clef->getClefType()));
    	name = name.left(1).toUpper() + name.right(name.length()-1);
    	return tr("Change to %1 Cle&f...").arg(name);
        } else {
    */ 
    return tr("Add Cle&f Change...");
    /*
        }
    */
}

timeT
ClefInsertionCommand::getRelayoutEndTime()
{
    // Inserting a clef can change the y-coord of every subsequent note
    return getSegment().getEndTime();
}

void
ClefInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    Clef oldClef(getSegment().getClefAtTime(getStartTime()));

    Segment::iterator i = getSegment().findTime(getStartTime());
    while (getSegment().isBeforeEndMarker(i)) {
        if ((*i)->getAbsoluteTime() > getStartTime()) {
            break;
        }
        if ((*i)->isa(Clef::EventType)) {
            getSegment().erase(i);
            break;
        }
        ++i;
    }

    i = helper.insertClef(getStartTime(), m_clef);
    if (i != helper.segment().end())
        m_lastInsertedEvent = *i;

    if (m_clef != oldClef) {

        int semitones = 0;

        if (m_shouldChangeOctave) {
            semitones += 12 * (m_clef.getOctave() - oldClef.getOctave());
        }
        if (m_shouldTranspose) {
            semitones -= m_clef.getPitchOffset() - oldClef.getPitchOffset();
        }

        if (semitones != 0) {
            while (i != helper.segment().end()) {
                if ((*i)->isa(Note::EventType)) {
                    long pitch = 0;
                    if ((*i)->get
                            <Int>(PITCH, pitch)) {
                        pitch += semitones;
                        (*i)->set
                        <Int>(PITCH, pitch);
                    }
                } else if (*i != m_lastInsertedEvent && (*i)->isa(Clef::EventType)) {
		    // Stop changing octaves when next clef is encountered.
		    break;
                }
                ++i;
            }
        }
    }
}

ClefLinkInsertionCommand::ClefLinkInsertionCommand(Segment &segment,
                                                   timeT time,
                                                   Clef clef,
                                                   bool shouldChangeOctave,
                                                   bool shouldTranspose) : 
    ClefInsertionCommand(segment,time,clef,shouldChangeOctave,shouldTranspose)
{
    setUpdateLinks(false);
};

ClefLinkInsertionCommand::~ClefLinkInsertionCommand()
{
    // nothing
}

QString 
ClefLinkInsertionCommand::getThisGlobalName(Clef *clef)
{
    return getGlobalName(clef);
}

QString
ClefLinkInsertionCommand::getGlobalName(Clef */* clef */) 
{ 
    return tr("Add Cl&ef Change for linked segment..."); 
}

void
ClefLinkInsertionCommand::modifySegment()
{
    ClefInsertionCommand::modifySegment();
    if (m_lastInsertedEvent && m_lastInsertedEvent->isa(Clef::EventType)) {
        //add a property so this event is ignored when updating linked segs
        m_lastInsertedEvent->set<Bool>(LINKED_SEGMENT_IGNORE_UPDATE, true);
    }
}

}
