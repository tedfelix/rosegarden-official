/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "PasteConductorDataCommand.h"

#include <klocale.h>
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Selection.h"


namespace Rosegarden
{

PasteConductorDataCommand::PasteConductorDataCommand(Composition *composition,
        Clipboard *clipboard,
        timeT t) :
        KNamedCommand(i18n("Paste Tempos and Time Signatures")),
        m_composition(composition),
        m_clipboard(new Clipboard(*clipboard)),
        m_t0(t)
{}

PasteConductorDataCommand::~PasteConductorDataCommand()
{
    delete m_clipboard;
}

void
PasteConductorDataCommand::execute()
{
    //!!! current implementation of execute and unexecute require
    // that the target area of the composition be empty of tempo and
    // timesig data before the command is executed

    if (m_clipboard->hasTimeSignatureSelection()) {

        for (TimeSignatureSelection::timesigcontainer::const_iterator i =
                    m_clipboard->getTimeSignatureSelection().begin();
                i != m_clipboard->getTimeSignatureSelection().end(); ++i) {

            timeT t = i->first;
            t = t - m_clipboard->getBaseTime() + m_t0;
            TimeSignature sig = i->second;

            if (i == m_clipboard->getTimeSignatureSelection().begin() &&
                    m_composition->getTimeSignatureAt(t) == sig)
                continue;

            m_composition->addTimeSignature(t, sig);
        }
    }

    if (m_clipboard->hasTempoSelection()) {

        for (TempoSelection::tempocontainer::const_iterator i =
                    m_clipboard->getTempoSelection().begin();
                i != m_clipboard->getTempoSelection().end(); ++i) {

            timeT t = i->first;
            t = t - m_clipboard->getBaseTime() + m_t0;
            tempoT tempo = i->second.first;
            tempoT targetTempo = i->second.second;

            if (i == m_clipboard->getTempoSelection().begin() &&
                    targetTempo < 0 &&
                    m_composition->getTempoAtTime(t) == tempo)
                continue;

            m_composition->addTempoAtTime(t, tempo, targetTempo);
        }
    }
}

void
PasteConductorDataCommand::unexecute()
{
    //!!! see note above

    for (TimeSignatureSelection::timesigcontainer::const_iterator i =
                m_clipboard->getTimeSignatureSelection().begin();
            i != m_clipboard->getTimeSignatureSelection().end(); ++i) {

        timeT t = i->first;
        int n = m_composition->getTimeSignatureNumberAt(t);

        if (n >= 0 && m_composition->getTimeSignatureChange(n).first == t) {
            m_composition->removeTimeSignature(n);
        }
    }

    for (TempoSelection::tempocontainer::const_iterator i =
                m_clipboard->getTempoSelection().begin();
            i != m_clipboard->getTempoSelection().end(); ++i) {

        timeT t = i->first;
        int n = m_composition->getTempoChangeNumberAt(t);

        if (n >= 0 && m_composition->getTempoChange(n).first == t) {
            m_composition->removeTempoChange(n);
        }
    }
}

}
