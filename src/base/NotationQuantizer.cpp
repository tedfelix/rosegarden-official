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

#include "NotationQuantizer.h"
#include "base/BaseProperties.h"
#include "base/NotationTypes.h"
#include "Selection.h"
#include "Composition.h"
#include "Sets.h"
#include "base/Profiler.h"

#include <iostream>
#include <cmath>
#include <cstdio> // for sprintf
#include <ctime>

using std::cout;
using std::cerr;
using std::endl;

//#define DEBUG_NOTATION_QUANTIZER 1

namespace Rosegarden {

using namespace BaseProperties;

class NotationQuantizer::Impl
{
public:
    Impl(NotationQuantizer *const q) :
	m_unit(Note(Note::Demisemiquaver).getDuration()),
	m_simplicityFactor(13),
	m_maxTuplet(3),
	m_articulate(true),
	m_q(q),
	m_provisionalBase("notationquantizer-provisionalBase"),
	m_provisionalAbsTime("notationquantizer-provisionalAbsTime"),
	m_provisionalDuration("notationquantizer-provisionalDuration"),
	m_provisionalNoteType("notationquantizer-provisionalNoteType"),
	m_provisionalScore("notationquantizer-provisionalScore")
    { }

    Impl(const Impl &i) :
	m_unit(i.m_unit),
	m_simplicityFactor(i.m_simplicityFactor),
	m_maxTuplet(i.m_maxTuplet),
	m_articulate(i.m_articulate),
	m_q(i.m_q),
	m_provisionalBase(i.m_provisionalBase),
	m_provisionalAbsTime(i.m_provisionalAbsTime),
	m_provisionalDuration(i.m_provisionalDuration),
	m_provisionalNoteType(i.m_provisionalNoteType),
	m_provisionalScore(i.m_provisionalScore)
    { }

    class ProvisionalQuantizer : public Quantizer {
	// This class exists only to pick out the provisional abstime
	// and duration values from half-quantized events, so that we
	// can treat them using the normal Chord class
    public:
	ProvisionalQuantizer(Impl *i) : Quantizer("blah", "blahblah"), m_impl(i) { }
	timeT getQuantizedDuration(const Event *e) const override {
	    return m_impl->getProvisional((Event *)e, DurationValue);
	}
	timeT getQuantizedAbsoluteTime(const Event *e) const override {
	    timeT t = m_impl->getProvisional((Event *)e, AbsoluteTimeValue);
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "ProvisionalQuantizer::getQuantizedAbsoluteTime: returning " << t << endl;
#endif
	    return t;
	}

    private:
	Impl *m_impl;
    };

    void quantizeRange(Segment *,
		       Segment::iterator,
		       Segment::iterator) const;

    void quantizeAbsoluteTime(Segment *, Segment::iterator) const;
    long scoreAbsoluteTimeForBase(Segment *, const Segment::iterator &,
				  int depth, timeT base, timeT sigTime,
				  timeT t, timeT d, int noteType,
				  const Segment::iterator &,
				  const Segment::iterator &,
				  bool &right) const;
    void quantizeDurationProvisional(Segment *, Segment::iterator) const;
    void quantizeDuration(Segment *, Chord &) const;

    void scanTupletsInBar(Segment *,
			  timeT barStart, timeT barDuration,
			  timeT wholeStart, timeT wholeDuration,
			  const std::vector<int> &divisions) const;
    void scanTupletsAt(Segment *, Segment::iterator, int depth,
		       timeT base, timeT barStart,
		       timeT tupletStart, timeT tupletBase) const;
    bool isValidTupletAt(Segment *, const Segment::iterator &,
			 int depth, timeT base, timeT sigTime,
			 timeT tupletBase) const;
    
    void setProvisional(Event *, ValueType value, timeT t) const;
    timeT getProvisional(Event *, ValueType value) const;
    void unsetProvisionalProperties(Event *) const;

    timeT m_unit;
    int m_simplicityFactor;
    int m_maxTuplet;
    bool m_articulate;
    bool m_contrapuntal;

private:
    NotationQuantizer *const m_q;

    PropertyName m_provisionalBase;
    PropertyName m_provisionalAbsTime;
    PropertyName m_provisionalDuration;
    PropertyName m_provisionalNoteType;
    PropertyName m_provisionalScore;
};

NotationQuantizer::NotationQuantizer() :
    Quantizer(NotationPrefix),
    m_impl(new Impl(this))
{
    // nothing else 
}

NotationQuantizer::NotationQuantizer(std::string source, std::string target) :
    Quantizer(source, target),
    m_impl(new Impl(this))
{
    // nothing else 
}

NotationQuantizer::NotationQuantizer(const NotationQuantizer &q) :
    Quantizer(q.m_target),
    m_impl(new Impl(*q.m_impl))
{
    // nothing else
}

NotationQuantizer::~NotationQuantizer()
{
    delete m_impl;
}

void
NotationQuantizer::setUnit(timeT unit) 
{
    m_impl->m_unit = unit;
}

timeT
NotationQuantizer::getUnit() const 
{
    return m_impl->m_unit;
}

void
NotationQuantizer::setMaxTuplet(int m) 
{
    m_impl->m_maxTuplet = m;
}

int
NotationQuantizer::getMaxTuplet() const 
{
    return m_impl->m_maxTuplet;
}

void
NotationQuantizer::setSimplicityFactor(int s) 
{
    m_impl->m_simplicityFactor = s;
}

int
NotationQuantizer::getSimplicityFactor() const 
{
    return m_impl->m_simplicityFactor;
}

void
NotationQuantizer::setContrapuntal(bool c) 
{
    m_impl->m_contrapuntal = c;
}

bool
NotationQuantizer::getContrapuntal() const 
{
    return m_impl->m_contrapuntal;
}

void
NotationQuantizer::setArticulate(bool a) 
{
    m_impl->m_articulate = a;
}

bool
NotationQuantizer::getArticulate() const 
{
    return m_impl->m_articulate;
}

void
NotationQuantizer::Impl::setProvisional(Event *e, ValueType v, timeT t) const
{
    if (v == AbsoluteTimeValue) {
	e->setMaybe<Int>(m_provisionalAbsTime, t);
    } else {
	e->setMaybe<Int>(m_provisionalDuration, t);
    }
}

timeT
NotationQuantizer::Impl::getProvisional(Event *e, ValueType v) const
{
    timeT t;
    if (v == AbsoluteTimeValue) {
	t = e->getAbsoluteTime();
	e->get<Int>(m_provisionalAbsTime, t);
    } else {
	t = e->getDuration();
	e->get<Int>(m_provisionalDuration, t);
    }
    return t;
}

void
NotationQuantizer::Impl::unsetProvisionalProperties(Event *e) const
{
    e->unset(m_provisionalBase);
    e->unset(m_provisionalAbsTime);
    e->unset(m_provisionalDuration);
    e->unset(m_provisionalNoteType);
    e->unset(m_provisionalScore);
}

void
NotationQuantizer::Impl::quantizeAbsoluteTime(Segment *s, Segment::iterator i) const
{
    Profiler profiler("NotationQuantizer::Impl::quantizeAbsoluteTime");

    Composition *comp = s->getComposition();
    
    TimeSignature timeSig;
    timeT t = m_q->getFromSource(*i, AbsoluteTimeValue);
    timeT sigTime = comp->getTimeSignatureAt(t, timeSig);

    timeT d = getProvisional(*i, DurationValue);
    int noteType = Note::getNearestNote(d).getNoteType();
    (*i)->setMaybe<Int>(m_provisionalNoteType, noteType);

    int maxDepth = 8 - noteType;
    if (maxDepth < 4) maxDepth = 4;
    std::vector<int> divisions;
    timeSig.getDivisions(maxDepth, divisions);
    if (timeSig == TimeSignature()) // special case for 4/4
	divisions[0] = 2;

    // At each depth of beat subdivision, we find the closest match
    // and assign it a score according to distance and depth.  The
    // calculation for the score should accord "better" scores to
    // shorter distance and lower depth, but it should avoid giving
    // a "perfect" score to any combination of distance and depth
    // except where both are 0.  Also, the effective depth is
    // 2 more than the value of our depth counter, which counts
    // from 0 at a point where the effective depth is already 1.
    
    timeT base = timeSig.getBarDuration();

    timeT bestBase = -2;
    long bestScore = 0;
    bool bestRight = false;

#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "quantizeAbsoluteTime: t is " << t << ", d is " << d << endl;
#endif

    // scoreAbsoluteTimeForBase wants to know the previous starting
    // note (N) and the previous starting note that ends (roughly)
    // before this one starts (N').  Much more efficient to calculate
    // them once now before the loop.

    static timeT shortTime = Note(Note::Shortest).getDuration();
    
    Segment::iterator j(i);
    Segment::iterator n(s->end()), nprime(s->end());
    for (;;) {
	if (j == s->begin()) break;
	--j;
	if ((*j)->isa(Note::EventType)) {
	    if (n == s->end()) n = j;
	    if ((*j)->getAbsoluteTime() + (*j)->getDuration() + shortTime/2
		<= (*i)->getAbsoluteTime()) {
		nprime = j;
		break;
	    }
	}
    }
    
#ifdef DEBUG_NOTATION_QUANTIZER
    if (n != s->end() && n != nprime) {
	cout << "found n (distinct from nprime) at " << (*n)->getAbsoluteTime() << endl;
    }
    if (nprime != s->end()) {
	cout << "found nprime at " << (*nprime)->getAbsoluteTime()
	     << ", duration " << (*nprime)->getDuration() << endl;
    }
#endif

    for (int depth = 0; depth < maxDepth; ++depth) {

	base /= divisions[depth];
	if (base < m_unit) break;
	bool right = false;
	long score = scoreAbsoluteTimeForBase(s, i, depth, base, sigTime,
					      t, d, noteType, n, nprime, right);

	if (depth == 0 || score < bestScore) {
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << " [*]";
#endif
	    bestBase = base;
	    bestScore = score;
	    bestRight = right;
	}

#ifdef DEBUG_NOTATION_QUANTIZER
	cout << endl;
#endif
    }

    if (bestBase == -2) {
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "Quantizer::quantizeAbsoluteTime: weirdness: no snap found" << endl;
#endif
    } else {
	// we need to snap relative to the time sig, not relative
	// to the start of the whole composition
	t -= sigTime;

	t = (t / bestBase) * bestBase;
	if (bestRight) t += bestBase;

/*
	timeT low = (t / bestBase) * bestBase;
	timeT high = low + bestBase;
	t = ((high - t > t - low) ? low : high);
*/

	t += sigTime;
	
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "snap base is " << bestBase << ", snapped to " << t << endl;
#endif
    }

    setProvisional(*i, AbsoluteTimeValue, t);
    (*i)->setMaybe<Int>(m_provisionalBase, bestBase);
    (*i)->setMaybe<Int>(m_provisionalScore, bestScore);
}

long
NotationQuantizer::Impl::scoreAbsoluteTimeForBase(Segment *s,
						  const Segment::iterator & /* i */,
						  int depth,
						  timeT base,
						  timeT sigTime,
						  timeT t,
						  timeT d,
						  int noteType,
						  const Segment::iterator &n,
						  const Segment::iterator &nprime,
						  bool &wantRight)
    const
{
    Profiler profiler("NotationQuantizer::Impl::scoreAbsoluteTimeForBase");

    // Lower score is better.
    
    static timeT shortTime = Note(Note::Shortest).getDuration();

    double simplicityFactor(m_simplicityFactor);
    simplicityFactor -= Note::Crotchet - noteType;
    if (simplicityFactor < 10) simplicityFactor = 10;

    double effectiveDepth = pow(depth + 2, simplicityFactor / 10);

    //!!! use velocity to adjust the effective depth as well? -- louder
    // notes are more likely to be on big boundaries.  Actually, perhaps
    // introduce a generally-useful "salience" score a la Dixon et al

    long leftScore = 0;

    for (int ri = 0; ri < 2; ++ri) {

	bool right = (ri == 1);

	long distance = (t - sigTime) % base;
	if (right) distance = base - distance;
	long score = long((distance + shortTime / 2) * effectiveDepth);
    
	double penalty1 = 1.0;
    
	// seriously penalise moving a note beyond its own end time
	if (d > 0 && right && distance >= d * 0.9) {
	    penalty1 = double(distance) / d + 0.5;
	}
    
	double penalty2 = 1.0;

	// Examine the previous starting note (N), and the previous
	// starting note that ends before this one starts (N').
    
	// We should penalise moving this note to before the performed end
	// of N' and seriously penalise moving it to the same quantized
	// start time as N' -- but we should encourage moving it to the
	// same time as the provisional end of N', or to the same start
	// time as N if N != N'.
	
	if (!right) {
	    if (n != s->end()) {
		if (n != nprime) {
		    timeT nt = getProvisional(*n, AbsoluteTimeValue);
		    if (t - distance == nt) penalty2 = penalty2 * 2 / 3;
		}
		if (nprime != s->end()) {
		    timeT npt = getProvisional(*nprime, AbsoluteTimeValue);
		    timeT npd = getProvisional(*nprime, DurationValue);
		    if (t - distance <= npt) penalty2 *= 4;
		    else if (t - distance < npt + npd) penalty2 *= 2;
		    else if (t - distance == npt + npd) penalty2 = penalty2 * 2 / 3;
		}
	    }
	}
    
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "  depth/eff/dist/t/score/pen1/pen2/res: " << depth << "/" << effectiveDepth << "/" << distance << "/" << (right ? t + distance : t - distance) << "/" << score << "/" << penalty1 << "/" << penalty2 << "/" << (score * penalty1 * penalty2);
	if (right) cout << " -> ";
	else cout << " <- ";
	if (ri == 0) cout << endl;
#endif
    
	score = long(score * penalty1);
	score = long(score * penalty2);
    
	if (ri == 0) {
	    leftScore = score;
	} else {
	    if (score < leftScore) {
		wantRight = true;
		return score;
	    } else {
		wantRight = false;
		return leftScore;
	    }
	}
    }

    return leftScore;
}
    
void
NotationQuantizer::Impl::quantizeDurationProvisional(Segment *, Segment::iterator i)
    const
{
    Profiler profiler("NotationQuantizer::Impl::quantizeDurationProvisional");

    // Calculate a first guess at the likely notation duration based
    // only on its performed duration, without considering start time.

    timeT d = m_q->getFromSource(*i, DurationValue);
    if (d == 0) {
	setProvisional(*i, DurationValue, d);
	return;
    }

    Note shortNote = Note::getNearestNote(d, 2);

    timeT shortTime = shortNote.getDuration();
    timeT time = shortTime;

    if (shortTime != d) {

	Note longNote(shortNote);

	if ((shortNote.getDots() > 0 ||
	     shortNote.getNoteType() == Note::Shortest)) { // can't dot that
	    
	    if (shortNote.getNoteType() < Note::Longest) {
		longNote = Note(shortNote.getNoteType() + 1, 0);
	    }
	
	} else {
	    longNote = Note(shortNote.getNoteType(), 1);
	}
	
	timeT longTime = longNote.getDuration();
	
	// we should prefer to round up to a note with fewer dots rather
	// than down to one with more

	//!!! except in dotted time, etc -- we really want this to work on a
	// similar attraction-to-grid basis to the abstime quantization

	if ((longNote.getDots() + 1) * (longTime - d) <
	    (shortNote.getDots() + 1) * (d - shortTime)) {
	    time = longTime;
	}
    }

    setProvisional(*i, DurationValue, time);

    if ((*i)->has(BEAMED_GROUP_TUPLET_BASE)) {
	// We're going to recalculate these, and use our own results
	(*i)->unset(BEAMED_GROUP_ID);
	(*i)->unset(BEAMED_GROUP_TYPE);
	(*i)->unset(BEAMED_GROUP_TUPLET_BASE);
	(*i)->unset(BEAMED_GROUP_TUPLED_COUNT);
	(*i)->unset(BEAMED_GROUP_UNTUPLED_COUNT);
//!!!	(*i)->unset(TUPLET_NOMINAL_DURATION);
    }
}

void
NotationQuantizer::Impl::quantizeDuration(Segment *s, Chord &c) const
{
    static int totalFracCount = 0;
    static float totalFrac = 0;

    Profiler profiler("NotationQuantizer::Impl::quantizeDuration");

#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "quantizeDuration: chord has " << c.size() << " notes" << endl;
#endif

    Composition *comp = s->getComposition();
    
    TimeSignature timeSig;
//    timeT t = m_q->getFromSource(*c.getInitialElement(), AbsoluteTimeValue);
//    timeT sigTime = comp->getTimeSignatureAt(t, timeSig);

    timeT d = getProvisional(*c.getInitialElement(), DurationValue);
    int noteType = Note::getNearestNote(d).getNoteType();
    int maxDepth = 8 - noteType;
    if (maxDepth < 4) maxDepth = 4;
    std::vector<int> divisions;
    timeSig.getDivisions(maxDepth, divisions);

    Segment::iterator nextNote = c.getNextNote();
    timeT nextNoteTime =
	(s->isBeforeEndMarker(nextNote) ?
	 getProvisional(*nextNote, AbsoluteTimeValue) :
	 s->getEndMarkerTime());

    timeT nonContrapuntalDuration = 0;
    
    for (Chord::iterator ci = c.begin(); ci != c.end(); ++ci) {

	if (!(**ci)->isa(Note::EventType)) continue;
	if ((**ci)->has(m_provisionalDuration) &&
	    (**ci)->has(BEAMED_GROUP_TUPLET_BASE)) {
	    // dealt with already in tuplet code, we'd only mess it up here
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "not recalculating duration for tuplet" << endl;
#endif
	    continue;
	}
	
	timeT ud = 0;

	if (!m_contrapuntal) {
	    // if not contrapuntal, give all notes in chord equal duration
	    if (nonContrapuntalDuration > 0) {
#ifdef DEBUG_NOTATION_QUANTIZER 
		cout << "setting duration trivially to " << nonContrapuntalDuration << endl;
#endif
		setProvisional(**ci, DurationValue, nonContrapuntalDuration);
		continue;
	    } else {
		// establish whose duration to use, then set it at the
		// bottom after it's been quantized
		Segment::iterator li = c.getLongestElement();
		if (li != s->end()) ud = m_q->getFromSource(*li, DurationValue);
		else ud = m_q->getFromSource(**ci, DurationValue);
	    }
	} else {
	    ud = m_q->getFromSource(**ci, DurationValue);
	}

	timeT qt = getProvisional(**ci, AbsoluteTimeValue);

#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "note at time " << (**ci)->getAbsoluteTime() << " (provisional time " << qt << ")" << endl;
#endif

	timeT base = timeSig.getBarDuration();
	std::pair<timeT, timeT> bases;
	for (int depth = 0; depth < maxDepth; ++depth) {
	    if (base >= ud) {
		bases = std::pair<timeT, timeT>(base / divisions[depth], base);
	    }
	    base /= divisions[depth];
	}

#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "duration is " << ud << ", probably between "
		  << bases.first << " and " << bases.second << endl;
#endif

	timeT qd = getProvisional(**ci, DurationValue);

	timeT spaceAvailable = nextNoteTime - qt;
	
	if (spaceAvailable > 0) {
	    float frac = float(ud) / float(spaceAvailable);
	    totalFrac += frac;
	    totalFracCount += 1;
	}

	if (!m_contrapuntal && qd > spaceAvailable) {

	    qd = Note::getNearestNote(spaceAvailable).getDuration();

#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "non-contrapuntal segment, rounded duration down to "
		 << qd << " (as only " << spaceAvailable << " available)"
		 << endl;
#endif

	} else {

	    //!!! Note longer than the longest note we have.  Deal with
	    //this -- how?  Quantize the end time?  Split the note?
	    //(Prefer to do that in a separate phase later if requested.)
	    //Leave it as it is?  (Yes, for now.)
	    if (bases.first == 0) return;
	    
	    timeT absTimeBase = bases.first;
	    (**ci)->get<Int>(m_provisionalBase, absTimeBase);

	    spaceAvailable = std::min(spaceAvailable, 
				      comp->getBarEndForTime(qt) - qt);

	    // We have a really good possibility of staccato if we have a
	    // note on a boundary whose base is double the note duration
	    // and there's nothing else until the next boundary and we're
	    // shorter than about a quaver (i.e. the base is a quaver or
	    // less)
	    
	    if (qd*2 <= absTimeBase && (qd*8/3) >= absTimeBase &&
		bases.second == absTimeBase) {
		
		if (nextNoteTime >= qt + bases.second) {
#ifdef DEBUG_NOTATION_QUANTIZER
		    cout << "We rounded to " << qd
			 << " but we're on " << absTimeBase << " absTimeBase"
			 << " and the next base is " << bases.second
			 << " and we have room for it, so"
			 << " rounding up again" << endl;
#endif
		    qd = bases.second;
		}
		
	    } else {
		
		// Alternatively, if we rounded down but there's space to
		// round up, consider doing so
		
		//!!! mark staccato if necessary, and take existing marks into account
		
		Note note(Note::getNearestNote(qd));
		
		if (qd < ud || (qd == ud && note.getDots() == 2)) {
		    
		    if (note.getNoteType() < Note::Longest) {
			
			if (bases.second <= spaceAvailable) {
#ifdef DEBUG_NOTATION_QUANTIZER
			    cout << "We rounded down to " << qd
				 << " but have room for " << bases.second
				 << ", rounding up again" << endl;
#endif
			    qd = bases.second;
			} else {
#ifdef DEBUG_NOTATION_QUANTIZER
			    cout << "We rounded down to " << qd
				 << "; can't fit " << bases.second << endl;
#endif
			}			
		    }
		}
	    }
	}

	setProvisional(**ci, DurationValue, qd);
	if (!m_contrapuntal) nonContrapuntalDuration = qd;
    }

#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "totalFrac " << totalFrac << ", totalFracCount " << totalFracCount << ", avg " << (totalFracCount > 0 ? (totalFrac / totalFracCount) : 0) << endl;
#endif
}


void
NotationQuantizer::Impl::scanTupletsInBar(Segment *s,
					  timeT barStart,
					  timeT barDuration,
					  timeT wholeStart,
					  timeT wholeEnd,
					  const std::vector<int> &divisions) const
{
    Profiler profiler("NotationQuantizer::Impl::scanTupletsInBar");

    //!!! need to further constrain the area scanned so as to cope with
    // partial bars

    timeT base = barDuration;

    for (int depth = -1; depth < int(divisions.size()) - 2; ++depth) {

	if (depth >= 0) base /= divisions[depth];
	if (base <= Note(Note::Semiquaver).getDuration()) break;

#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "\nscanTupletsInBar: trying at depth " << depth << " (base " << base << ")" << endl;
#endif

	// check for triplets if our next divisor is 2 and the following
	// one is not 3

	if (divisions[depth+1] != 2 || divisions[depth+2] == 3) continue;

	timeT tupletBase = base / 3;
	timeT tupletStart = barStart;

	while (tupletStart < barStart + barDuration) {

	    timeT tupletEnd = tupletStart + base;
	    if (tupletStart < wholeStart || tupletEnd > wholeEnd) {
		tupletStart = tupletEnd;
		continue;
	    }

#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "scanTupletsInBar: testing " << tupletStart << "," << base << " at tuplet base " << tupletBase << endl;
#endif

	    // find first note within a certain distance whose start time
	    // quantized to tupletStart or greater
	    Segment::iterator j = s->findTime(tupletStart - tupletBase / 3);
	    timeT jTime = tupletEnd;

	    while (s->isBeforeEndMarker(j) &&
		   (!(*j)->isa(Note::EventType) ||
		    !(*j)->get<Int>(m_provisionalAbsTime, jTime) ||
		    jTime < tupletStart)) {
		if ((*j)->getAbsoluteTime() > tupletEnd + tupletBase / 3) {
		    break;
		}
		++j;
	    }

	    if (jTime >= tupletEnd) { // nothing to make tuplets of
#ifdef DEBUG_NOTATION_QUANTIZER
		cout << "scanTupletsInBar: nothing here" << endl;
#endif
		tupletStart = tupletEnd;
		continue;
	    }

	    scanTupletsAt(s, j, depth+1, base, barStart,
			  tupletStart, tupletBase);

	    tupletStart = tupletEnd;
	}
    }
}
	

void
NotationQuantizer::Impl::scanTupletsAt(Segment *s,
				       Segment::iterator i,
				       int depth,
				       timeT base,
				       timeT sigTime,
				       timeT tupletStart,
				       timeT tupletBase) const
{
    Profiler profiler("NotationQuantizer::Impl::scanTupletsAt");

    Segment::iterator j = i;
    timeT tupletEnd = tupletStart + base;
    timeT jTime = tupletEnd;

    std::vector<Event *> candidates;
    int count = 0;

    while (s->isBeforeEndMarker(j) &&
	   ((*j)->isa(Note::EventRestType) ||
	    ((*j)->get<Int>(m_provisionalAbsTime, jTime) &&
	     jTime < tupletEnd))) {
	
	if (!(*j)->isa(Note::EventType)) { ++j; continue; }

#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "scanTupletsAt time " << jTime << " (unquantized "
	     << (*j)->getAbsoluteTime() << "), found note" << endl;
#endif

	// reject any group containing anything already a tuplet
	if ((*j)->has(BEAMED_GROUP_TUPLET_BASE)) {
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "already made tuplet here" << endl;
#endif
	    return;
	}

	timeT originalBase;

	if (!(*j)->get<Int>(m_provisionalBase, originalBase)) {
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "some notes not provisionally quantized, no good" << endl;
#endif
	    return;
	}

	if (originalBase == base) {
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "accepting note at original base" << endl;
#endif
	    candidates.push_back(*j);
	} else if (((jTime - sigTime) % base) == 0) {
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "accepting note that happens to lie on original base" << endl;
#endif
	    candidates.push_back(*j);
	} else {

	    // This is a note that did not quantize to the original base
	    // (the first note in the tuplet would have, but we can't tell
	    // anything from that).  Reject the entire group if it fails
	    // any of the likelihood tests for tuplets.

	    if (!isValidTupletAt(s, j, depth, base, sigTime, tupletBase)) {
#ifdef DEBUG_NOTATION_QUANTIZER
		cout << "no good" << endl;
#endif
		return;
	    }

	    candidates.push_back(*j);
	    ++count;
	}

	++j;
    }

    // must have at least one note that is not already quantized to the
    // original base
    if (count < 1) {
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "scanTupletsAt: found no note not already quantized to " << base << endl;
#endif
	return;
    }

#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "scanTupletsAt: Tuplet group of duration " << base << " starting at " << tupletStart << endl;
#endif

    // Woo-hoo!  It looks good.

    int groupId = s->getNextId();
    std::map<int, bool> multiples;

    for (std::vector<Event *>::iterator ei = candidates.begin();
	 ei != candidates.end(); ++ei) {

	//!!! Interesting -- we can't modify rests here, but Segment's
	// normalizeRests won't insert the correct sort of rest for us...
	// what to do?
	//!!! insert a tupleted rest, and prevent Segment::normalizeRests
	// from messing about with it
	if (!(*ei)->isa(Note::EventType)) continue;
	(*ei)->set<String>(BEAMED_GROUP_TYPE, GROUP_TYPE_TUPLED);

	//!!! This is too easy, because we rejected any notes of
	//durations not conforming to a single multiple of the
	//tupletBase in isValidTupletAt

	(*ei)->set<Int>(BEAMED_GROUP_ID, groupId);
	(*ei)->set<Int>(BEAMED_GROUP_TUPLET_BASE, base/2); //!!! wrong if tuplet count != 3
	(*ei)->set<Int>(BEAMED_GROUP_TUPLED_COUNT, 2); //!!! as above
	(*ei)->set<Int>(BEAMED_GROUP_UNTUPLED_COUNT, base/tupletBase);

	timeT t = (*ei)->getAbsoluteTime();
	t -= tupletStart;
	timeT low = (t / tupletBase) * tupletBase;
	timeT high = low + tupletBase;
	t = ((high - t > t - low) ? low : high);

	multiples[t / tupletBase] = true;

	t += tupletStart;

	setProvisional(*ei, AbsoluteTimeValue, t);
	setProvisional(*ei, DurationValue, tupletBase);
    }

    // fill in with tupleted rests

    for (int m = 0; m < base / tupletBase; ++m) {

	if (multiples[m]) continue;

	timeT absTime = tupletStart + m * tupletBase;
	timeT duration = tupletBase;
//!!!	while (multiples[++m]) duration += tupletBase;

	Event *rest = new Event(Note::EventRestType, absTime, duration);

	rest->set<String>(BEAMED_GROUP_TYPE, GROUP_TYPE_TUPLED);
	rest->set<Int>(BEAMED_GROUP_ID, groupId);
	rest->set<Int>(BEAMED_GROUP_TUPLET_BASE, base/2); //!!! wrong if tuplet count != 3
	rest->set<Int>(BEAMED_GROUP_TUPLED_COUNT, 2); //!!! as above
	rest->set<Int>(BEAMED_GROUP_UNTUPLED_COUNT, base/tupletBase);

	m_q->m_toInsert.push_back(rest);
    }
}

bool
NotationQuantizer::Impl::isValidTupletAt(Segment *s,
					 const Segment::iterator &i,
					 int depth,
					 timeT /* base */,
					 timeT sigTime,
					 timeT tupletBase) const
{
    Profiler profiler("NotationQuantizer::Impl::isValidTupletAt");

    //!!! This is basically wrong; we need to be able to deal with groups
    // that contain e.g. a crotchet and a quaver, tripleted.

    timeT ud = m_q->getFromSource(*i, DurationValue);

    if (ud > (tupletBase * 5 / 4)) {
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "\nNotationQuantizer::isValidTupletAt: note too long at "
	     << (*i)->getDuration() << " (tupletBase is " << tupletBase << ")"
	     << endl;
#endif
	return false; // too long
    }

    //!!! This bit is a cop-out.  It means we reject anything that looks
    // like it's going to have rests in it.  Bah.
    if (ud <= (tupletBase * 3 / 8)) {
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "\nNotationQuantizer::isValidTupletAt: note too short at "
	     << (*i)->getDuration() << " (tupletBase is " << tupletBase << ")"
	     << endl;
#endif
	return false;
    }

    long score = 0;
    if (!(*i)->get<Int>(m_provisionalScore, score)) return false;

    timeT t = m_q->getFromSource(*i, AbsoluteTimeValue);
    timeT d = getProvisional(*i, DurationValue);
    int noteType = (*i)->get<Int>(m_provisionalNoteType);

    //!!! not as complete as the calculation we do in the original scoring
    bool dummy;
    long tupletScore = scoreAbsoluteTimeForBase
	(s, i, depth, tupletBase, sigTime, t, d, noteType, s->end(), s->end(), dummy);
#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "\nNotationQuantizer::isValidTupletAt: score " << score
	 << " vs tupletScore " << tupletScore << endl;
#endif
    return (tupletScore < score);
}
				 

void
NotationQuantizer::quantizeRange(Segment *s,
				 Segment::iterator from,
				 Segment::iterator to) const
{
    m_impl->quantizeRange(s, from, to);
}

void
NotationQuantizer::Impl::quantizeRange(Segment *s,
				       Segment::iterator from,
				       Segment::iterator to) const
{
    Profiler *profiler = new Profiler("NotationQuantizer::Impl::quantizeRange");

/*
    clock_t start = clock();
*/
    int events = 0, notes = 0, passes = 0;
    int setGood = 0, setBad = 0;
    
#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "NotationQuantizer::Impl::quantizeRange: from time "
	      << (from == s->end() ? -1 : (*from)->getAbsoluteTime())
	      << " to "
	      << (to == s->end() ? -1 : (*to)->getAbsoluteTime())
	      << endl;
#endif

    timeT segmentEndTime = s->getEndMarkerTime();

    // This process does several passes over the data.  It's assumed
    // that this is not going to be invoked in any really time-critical
    // place.

    // Calculate absolute times on the first pass, so that we know
    // which things are chords.  We need to assign absolute times to
    // all events, but we only need do durations for notes.

    PropertyName provisionalBase("notationquantizer-provisionalBase");

    // We don't use setToTarget until we have our final values ready,
    // as it erases and replaces the events.  Just set the properties.

    // Set a provisional duration to each note first

    for (Segment::iterator i = from; i != to; ++i) {

	++events;
	if ((*i)->isa(Note::EventRestType)) continue;
	if ((*i)->isa(Note::EventType)) ++notes;
	quantizeDurationProvisional(s, i);
    }
    ++passes;

    // now do the absolute-time calculation

    timeT wholeStart = 0, wholeEnd = 0;

    Segment::iterator i = from;

    for (Segment::iterator nexti = i; i != to; i = nexti) {

	++nexti;

	if ((*i)->isa(Note::EventRestType)) {
	    if (i == from) ++from;
	    s->erase(i);
	    continue;
	}

	quantizeAbsoluteTime(s, i);

	timeT t0 = (*i)->get<Int>(m_provisionalAbsTime);
	timeT t1 = (*i)->get<Int>(m_provisionalDuration) + t0;
	if (wholeStart == wholeEnd) {
	    wholeStart = t0;
	    wholeEnd = t1;
	} else if (t1 > wholeEnd) {
	    wholeEnd = t1;
	}
    }
    ++passes;

    // now we've grouped into chords, look for tuplets next

    Composition *comp = s->getComposition();

    if (m_maxTuplet >= 2) {

	std::vector<int> divisions;
	comp->getTimeSignatureAt(wholeStart).getDivisions(7, divisions);

	for (int barNo = comp->getBarNumber(wholeStart);
	     barNo <= comp->getBarNumber(wholeEnd); ++barNo) {

	    bool isNew = false;
	    TimeSignature timeSig = comp->getTimeSignatureInBar(barNo, isNew);
	    if (isNew) timeSig.getDivisions(7, divisions);
	    scanTupletsInBar(s, comp->getBarStart(barNo),
			     timeSig.getBarDuration(),
			     wholeStart, wholeEnd, divisions);
	}
	++passes;
    }
    
    ProvisionalQuantizer provisionalQuantizer((Impl *)this);

    for (i = from; i != to; ++i) {

	if (!(*i)->isa(Note::EventType)) continue;

	// could potentially supply clef and key here, but at the
	// moment Chord doesn't do anything with them (unlike
	// NotationChord) and we don't have any really clever
	// ideas for how to use them here anyway
//	Chord c(*s, i, m_q);
	Chord c(*s, i, &provisionalQuantizer);

	quantizeDuration(s, c);

	bool ended = false;
	for (Segment::iterator ci = c.getInitialElement();
	     s->isBeforeEndMarker(ci); ++ci) {
	    if (ci == to) ended = true;
	    if (ci == c.getFinalElement()) break;
	}
	if (ended) break;

	i = c.getFinalElement();
    }
    ++passes;

    // staccato (we now do slurs separately, in SegmentNotationHelper::autoSlur)

    if (m_articulate) {

	for (i = from; i != to; ++i) {

	    if (!(*i)->isa(Note::EventType)) continue;

	    timeT qd = getProvisional(*i, DurationValue);
	    timeT ud = m_q->getFromSource(*i, DurationValue);

	    if (ud < (qd * 3 / 4) &&
		qd <= Note(Note::Crotchet).getDuration()) {
		Marks::addMark(**i, Marks::Staccato, true);
	    } else if (ud > qd) {
		Marks::addMark(**i, Marks::Tenuto, true);
	    }	    
	}
	++passes;
    }

    i = from;

    for (Segment::iterator nexti = i; i != to; i = nexti) {

	++nexti;

	if ((*i)->isa(Note::EventRestType)) continue;

	timeT t = getProvisional(*i, AbsoluteTimeValue);
	timeT d = getProvisional(*i, DurationValue);

	unsetProvisionalProperties(*i);

	if ((*i)->getAbsoluteTime() == t &&
	    (*i)->getDuration() == d) ++setBad;
	else ++setGood;

#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "Setting to target at " << t << "," << d << endl;
#endif

	m_q->setToTarget(s, i, t, d);
    }
    ++passes;
/*
    cerr << "NotationQuantizer: " << events << " events ("
	 << notes << " notes), " << passes << " passes, "
	 << setGood << " good sets, " << setBad << " bad sets, "
	 << ((clock() - start) * 1000 / CLOCKS_PER_SEC) << "ms elapsed"
	 << endl;
*/
    if (s->getEndTime() < segmentEndTime) {
	s->setEndMarkerTime(segmentEndTime);
    }

    delete profiler; // on heap so it updates before the next line:
    Profiles::getInstance()->dump();

}	
    
    
}

