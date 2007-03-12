/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "ClefInsertionCommand.h"

#include <klocale.h>
#include "misc/Strings.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/BaseProperties.h"
#include "document/BasicCommand.h"
#include <qstring.h>


namespace Rosegarden
{

using namespace BaseProperties;

ClefInsertionCommand::ClefInsertionCommand(Segment &segment, timeT time,
        Clef clef,
        bool shouldChangeOctave,
        bool shouldTranspose) :
        BasicCommand(getGlobalName(&clef), segment, time,
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

QString
ClefInsertionCommand::getGlobalName(Clef *)
{
    /* doesn't handle octave offset -- leave it for now
        if (clef) {
    	QString name(strtoqstr(clef->getClefType()));
    	name = name.left(1).upper() + name.right(name.length()-1);
    	return i18n("Change to %1 Cle&f...").arg(name);
        } else {
    */ 
    return i18n("Add Cle&f Change...");
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
                }
                ++i;
            }
        }
    }
}

}
