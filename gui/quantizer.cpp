
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "rosedebug.h"
#include "quantizer.h"
#include "NotationTypes.h"
#include "notationproperties.h"

Quantizer::Quantizer()
{
    // empty
}

void
Quantizer::quantize(Track::iterator from,
                    Track::iterator to)
{
    Track::iterator it = from;

    while (it != to) {

        quantize( (*it) );

        ++it;
    }
}

void
Quantizer::quantize(Event *el)
{
    Event::timeT drt = el->getDuration();

    kdDebug(KDEBUG_AREA) << "Quantizer applying to event of duration " << drt << endl;

    int high, low;
    quantize(drt, high, low);

    int qd;

    if ((high - drt) > (drt - low)) {
        qd = low;
    } else {
        qd = high;
    }

    Note note = Note::getNearestNote(qd);

    el->setMaybe<Int>(P_NOTE_TYPE, note.getNoteType());
    el->setMaybe<Bool>(P_NOTE_DOTTED, note.isDotted());
    el->setMaybe<Int>(P_QUANTIZED_DURATION, qd);

    kdDebug(KDEBUG_AREA) << "Quantized to duration : "
                          << qd << " - note : " << note.getNoteType()
			 << ", dotted : " << note.isDotted() << "\n";
}

void
Quantizer::quantize(Event::timeT drt, int &high, int &low)
{
    //!!! no dottedness -- NotationTypes stuff can help more here

    int d, ld = Note(Note::Shortest).getDuration();

    Note lowNote(Note::getNearestNote(drt));
    low = lowNote.getDuration();
    high = 1000000; //!!!

    try {
        Note highNote(lowNote.isDotted() ?
                      lowNote.getNoteType()+1 : 
                      lowNote.getNoteType(),     !lowNote.isDotted());
        if (highNote.getDuration() > drt) high = highNote.getDuration();

    } catch (Note::BadType) {
        // lowNote is already the longest there is
    }

    return;
}

