/***************************************************************************
                          quantizer.cpp  -  description
                             -------------------
    begin                : Thu Aug 17 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "rosedebug.h"
#include "quantizer.h"
#include "notepixmapfactory.h"
#include "NotationTypes.h"

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
    Event::timeT drt = el->duration();

    int high, low;
    quantize(drt, high, low);

    int qd;

    if ((high - drt) > (drt - low)) {
        qd = low;
    } else {
        qd = high;
    }

    Note note = Note::getNearestNote(qd);

    el->set<Int>("Notation::NoteType", note.getType());
    el->set<Bool>("Notation::NoteDotted", note.isDotted());
    el->set<Int>("QuantizedDuration", qd);

    kdDebug(KDEBUG_AREA) << "Quantized to duration : "
                          << qd << " - note : " << note.getType()
			 << ", dotted : " << note.isDotted() << "\n";
}

void
Quantizer::quantize(Event::timeT drt, int &high, int &low)
{
    //!!! no dottedness -- NotationTypes stuff can help more here

    int d, ld = Note(Note::Shortest).getDuration();

    for (Note::Type t = Note::Shortest; t < Note::Longest; ++t) {
        d = Note(t+1).getDuration();
        if (d > drt) {
            low = ld;
            high = d;
            return;
        }
        ld = d;
    }

    low = Note(Note::Longest).getDuration();
    high = 1000000; //!!!
    return;
}

