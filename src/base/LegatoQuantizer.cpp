/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "LegatoQuantizer.h"
#include "base/BaseProperties.h"
#include "base/NotationTypes.h"
#include "Selection.h"
#include "Composition.h"
#include "base/Profiler.h"

#include <iostream>
#include <cmath>
#include <cstdio> // for sprintf
#include <ctime>

using std::cout;
using std::cerr;
using std::endl;

namespace Rosegarden
{

using namespace BaseProperties;


LegatoQuantizer::LegatoQuantizer(timeT unit) :
    Quantizer(RawEventData),
    m_unit(unit)
{
    if (m_unit < 0) m_unit = Note(Note::Shortest).getDuration();
}

LegatoQuantizer::LegatoQuantizer(std::string source, std::string target, timeT unit) :
    Quantizer(source, target),
    m_unit(unit)
{
    if (m_unit < 0) m_unit = Note(Note::Shortest).getDuration();
}

LegatoQuantizer::LegatoQuantizer(const LegatoQuantizer &q) :
    Quantizer(q.m_target),
    m_unit(q.m_unit)
{
    // nothing else
}

LegatoQuantizer::~LegatoQuantizer()
{
    // nothing
}

void
LegatoQuantizer::quantizeRange(Segment *s,
			       Segment::iterator from,
			       Segment::iterator to) const
{
    Segment::iterator tmp;
    while (from != to) {
	quantizeLegatoSingle(s, from, tmp);
	from = tmp;
	if (!s->isBeforeEndMarker(from) ||
	    (s->isBeforeEndMarker(to) &&
	     ((*from)->getAbsoluteTime() >= (*to)->getAbsoluteTime()))) break;
    }
}

void
LegatoQuantizer::quantizeLegatoSingle(Segment *s, Segment::iterator i,
                                      Segment::iterator &nexti) const
{
    // Stretch each note out to reach the quantized start time of the
    // next note whose quantized start time is greater than or equal
    // to the end time of this note after quantization

    timeT t = getFromSource(*i, AbsoluteTimeValue);
    timeT d = getFromSource(*i, DurationValue);

    timeT d0(d), t0(t);

    timeT barStart = s->getBarStartForTime(t);

    t -= barStart;
    t = quantizeTime(t);
    t += barStart;

    nexti = i;
    ++nexti;

    for (Segment::iterator j = i; s->isBeforeEndMarker(j); ++j) {
	if (!(*j)->isa(Note::EventType)) continue;
	
	timeT qt = (*j)->getAbsoluteTime();
	qt -= barStart;
	qt = quantizeTime(qt);
	qt += barStart;

	if (qt >= t + d) {
	    d = qt - t;
	}
	if (qt > t) {
	    break;
	}
    }
    
    if (t0 != t || d0 != d) {
	setToTarget(s, i, t, d);
	nexti = s->findTime(t + d);
    }
}

timeT
LegatoQuantizer::quantizeTime(timeT t) const
{
    if (m_unit != 0) {
	timeT low = (t / m_unit) * m_unit;
	timeT high = low + m_unit;
	t = ((high - t > t - low) ? low : high);
    }
    return t;
}

}
