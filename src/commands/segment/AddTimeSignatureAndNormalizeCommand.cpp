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


#include "AddTimeSignatureAndNormalizeCommand.h"

#include "AddTimeSignatureCommand.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "commands/notation/MakeRegionViableCommand.h"


namespace Rosegarden
{

AddTimeSignatureAndNormalizeCommand::AddTimeSignatureAndNormalizeCommand
(Composition *composition, timeT time, TimeSignature timeSig) :
        KMacroCommand(AddTimeSignatureCommand::getGlobalName())
{
    addCommand(new AddTimeSignatureCommand(composition, time, timeSig));

    // only up to the next time signature
    timeT nextTimeSigTime(composition->getDuration());

    int index = composition->getTimeSignatureNumberAt(time);
    if (composition->getTimeSignatureCount() > index + 1) {
        nextTimeSigTime = composition->getTimeSignatureChange(index + 1).first;
    }

    for (Composition::iterator i = composition->begin();
         i != composition->end(); ++i) {

        if ((*i)->getType() != Segment::Internal) continue;

        timeT startTime = (*i)->getStartTime();
        timeT endTime = (*i)->getEndTime();

        if (startTime >= nextTimeSigTime || endTime <= time)
            continue;

        // "Make Notes Viable" splits and ties notes at barlines, and
        // also does a rest normalize.  It's what we normally want
        // when adding a time signature.

        addCommand(new MakeRegionViableCommand
                   (**i,
                    std::max(startTime, time),
                    std::min(endTime, nextTimeSigTime)));
    }
}

AddTimeSignatureAndNormalizeCommand::~AddTimeSignatureAndNormalizeCommand()
{
    // well, nothing really
}

}
