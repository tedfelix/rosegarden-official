/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "BasicQuantizer.h"
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

const std::string Quantizer::RawEventData = "";
const std::string Quantizer::DefaultTarget = "DefaultQ";
const std::string Quantizer::GlobalSource = "GlobalQ";
const std::string Quantizer::NotationPrefix = "Notation";

BasicQuantizer::BasicQuantizer(timeT unit, bool doDurations,
                               int swing, int iterate) :
    Quantizer(RawEventData),
    m_unit(unit),
    m_durations(doDurations),
    m_swing(swing),
    m_iterate(iterate)
{
    if (m_unit < 0) m_unit = Note(Note::Shortest).getDuration();
}

BasicQuantizer::BasicQuantizer(std::string source, std::string target,
                               timeT unit, bool doDurations,
                               int swing, int iterate) :
    Quantizer(source, target),
    m_unit(unit),
    m_durations(doDurations),
    m_swing(swing),
    m_iterate(iterate)
{
    if (m_unit < 0) m_unit = Note(Note::Shortest).getDuration();
}

BasicQuantizer::BasicQuantizer(const BasicQuantizer &q) :
    Quantizer(q.m_target),
    m_unit(q.m_unit),
    m_durations(q.m_durations),
    m_swing(q.m_swing),
    m_iterate(q.m_iterate)
{
    // nothing else
}

BasicQuantizer::~BasicQuantizer()
{
    // nothing
}

void
BasicQuantizer::quantizeSingle(Segment *s, Segment::iterator i) const
{
    timeT d = getFromSource(*i, DurationValue);

    if (d == 0 && (*i)->isa(Note::EventType)) {
        s->erase(i);
        return;
    }

    if (m_unit == 0) return;

    timeT t = getFromSource(*i, AbsoluteTimeValue);
    timeT d0(d), t0(t);

    timeT barStart = s->getBarStartForTime(t);

    t -= barStart;

    int n = t / m_unit;
    timeT low = n * m_unit;
    timeT high = low + m_unit;
    timeT swingOffset = (m_unit * m_swing) / 300;

    if (high - t > t - low) {
        t = low;
    } else {
        t = high;
        ++n;
    }

    if (n % 2 == 1) {
        t += swingOffset;
    }
    
    if (m_durations && d != 0) {

        low = (d / m_unit) * m_unit;
        high = low + m_unit;

        if (low > 0 && (high - d > d - low)) {
            d = low;
        } else {
            d = high;
        }

        int n1 = n + d / m_unit;

        if (n % 2 == 0) { // start not swung
            if (n1 % 2 == 0) { // end not swung
                // do nothing
            } else { // end swung
                d += swingOffset;
            }
        } else { // start swung
            if (n1 % 2 == 0) { // end not swung
                d -= swingOffset;
            } else {
                // do nothing
            }
        }
    }

    t += barStart;

    timeT t1(t), d1(d);
    t = (t - t0) * m_iterate / 100 + t0;
    d = (d - d0) * m_iterate / 100 + d0;

    // if an iterative quantize results in something much closer than
    // the shortest actual note resolution we have, just snap it
    if (m_iterate != 100) {
        timeT close = Note(Note::Shortest).getDuration()/2;
        if (t >= t1 - close && t <= t1 + close) t = t1;
        if (d >= d1 - close && d <= d1 + close) d = d1;
    }

    if (t0 != t || d0 != d) setToTarget(s, i, t, d);
}


std::vector<timeT>
BasicQuantizer::getStandardQuantizations()
{
    checkStandardQuantizations();
    return m_standardQuantizations;
}

void
BasicQuantizer::checkStandardQuantizations()
{
    if (!m_standardQuantizations.empty())
        return;

    // For each note type from semibreve to hemidemisemiquaver
    for (Note::Type nt = Note::Semibreve; nt >= Note::Shortest; --nt) {

        // For semiquavers and smaller, offer the triplet variation
        int i1 = (nt < Note::Quaver ? 1 : 0);

        // For the base note (0) and the triplet variation (1)
        for (int i = 0; i <= i1; ++i) {

            // Compute divisor, e.g. crotchet is 4, quaver is 8...
            int divisor = (1 << (Note::Semibreve - nt));

            // If we're doing the triplet variation, adjust the divisor
            if (i)
                divisor = divisor * 3 / 2;

            // Compute the number of MIDI clocks.
            timeT unit = Note(Note::Semibreve).getDuration() / divisor;

            m_standardQuantizations.push_back(unit);
        }
    }
}    

timeT
BasicQuantizer::getStandardQuantization(Segment *s)
{
    checkStandardQuantizations();
    timeT unit = -1;

    for (Segment::iterator i = s->begin(); s->isBeforeEndMarker(i); ++i) {

        if (!(*i)->isa(Rosegarden::Note::EventType)) continue;
        timeT myUnit = getUnitFor(*i);
        if (unit < 0 || myUnit < unit) unit = myUnit;
    }

    return unit;
}

timeT
BasicQuantizer::getStandardQuantization(EventSelection *s)
{
    checkStandardQuantizations();
    timeT unit = -1;

    if (!s) return 0;

    for (EventSelection::eventcontainer::iterator i =
             s->getSegmentEvents().begin();
         i != s->getSegmentEvents().end(); ++i) {

        if (!(*i)->isa(Rosegarden::Note::EventType)) continue;
        timeT myUnit = getUnitFor(*i);
        if (unit < 0 || myUnit < unit) unit = myUnit;
    }

    return unit;
}

timeT
BasicQuantizer::getUnitFor(Event *e)
{
    timeT absTime = e->getAbsoluteTime();
    timeT myQuantizeUnit = 0;
    
    // m_quantizations is in descending order of duration;
    // stop when we reach one that divides into the note's time
    
    for (size_t i = 0; i < m_standardQuantizations.size(); ++i) {
        if (absTime % m_standardQuantizations[i] == 0) {
            myQuantizeUnit = m_standardQuantizations[i];
            break;
        }
    }

    return myQuantizeUnit;
}

std::vector<timeT>
BasicQuantizer::m_standardQuantizations;


}
