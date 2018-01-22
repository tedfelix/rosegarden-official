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

#define RG_MODULE_STRING "[MultiKeyInsertionCommand]"

#include "MultiKeyInsertionCommand.h"

#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "KeyInsertionCommand.h"
#include <QString>
#include "document/RosegardenDocument.h"
#include "base/Studio.h"
#include "misc/Debug.h"


namespace Rosegarden
{

MultiKeyInsertionCommand::MultiKeyInsertionCommand(RosegardenDocument* doc,
        timeT time,
        Key key,
        bool convert,
        bool transpose,
        bool transposeKey,
	bool ignorePercussion) :
        MacroCommand(getGlobalName(&key))
{
   Composition &c = doc->getComposition();
   Studio &s = doc->getStudio();

    for (Composition::iterator i = c.begin(); i != c.end(); ++i) {
        Segment *segment = *i;

	Instrument *instrument = s.getInstrumentFor(segment);
	// if (instrument) {
	//    RG_DEBUG <<
	//                "PERC DEBUG: instrument->isPercussion " << instrument->isPercussion() <<
	//                " ignorePercussion " << ignorePercussion << endl << endl << endl;
	//}
	if (instrument) if (instrument->isPercussion() && ignorePercussion) continue;

        // no harm in using getEndTime instead of getEndMarkerTime here:
        if (segment->getStartTime() <= time && segment->getEndTime() > time) {
            addCommand(new KeyInsertionCommand(*segment, time, key, convert, transpose, transposeKey,
	                                       ignorePercussion));
        } else if (segment->getStartTime() > time) {
            addCommand(new KeyInsertionCommand(*segment, segment->getStartTime(),
                                               key, convert, transpose, transposeKey, ignorePercussion));
        }
    }
}

MultiKeyInsertionCommand::~MultiKeyInsertionCommand()
{
    // nothing
}

}
