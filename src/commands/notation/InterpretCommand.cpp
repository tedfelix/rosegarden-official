/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "InterpretCommand.h"

#include "base/Composition.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "misc/Debug.h"
#include "base/Quantizer.h"
#include "base/Segment.h"
#include "base/Sets.h"
#include "base/BaseProperties.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;

const int InterpretCommand::NoInterpretation  = 0;
const int InterpretCommand::GuessDirections   = (1<<0);
const int InterpretCommand::ApplyTextDynamics = (1<<1);
const int InterpretCommand::ApplyHairpins     = (1<<2);
const int InterpretCommand::StressBeats       = (1<<3);
const int InterpretCommand::Articulate        = (1<<4);
const int InterpretCommand::AllInterpretations= (1<<5) - 1;

InterpretCommand::~InterpretCommand()
{
    for (IndicationMap::iterator i = m_indications.begin();
            i != m_indications.end(); ++i) {
        delete i->second;
    }
}

void
InterpretCommand::modifySegment()
{
    // Of all the interpretations, Articulate is the only one that
    // changes event times or durations.  This means we must apply it
    // last, as the selection cannot be used after it's been applied,
    // because the events in the selection will have been recreated
    // with their new timings.

    // The default velocity for new notes is 100, and the range is
    // 0-127 (in practice this seems to be roughly logarithmic rather
    // than linear, though perhaps that's an illusion).

    // We should only apply interpretation to those events actually
    // selected, but when applying things like hairpins and text
    // dynamics we need to take into account all dynamics that may
    // cover our events even if they're not selected or are not within
    // the time range of the selection at all.  So first we'd better
    // find all the likely indications, starting at (for the sake of
    // argument) three bars before the start of the selection:

    Segment &segment(getSegment());

    timeT t = m_selection->getStartTime();
    for (int i = 0; i < 3; ++i)
        t = segment.getBarStartForTime(t);

    Segment::iterator itr = segment.findTime(t);

    while (itr != segment.end()) {
        timeT eventTime = (*itr)->getAbsoluteTime();
        if (eventTime > m_selection->getEndTime())
            break;
        if ((*itr)->isa(Indication::EventType)) {
            m_indications[eventTime] = new Indication(**itr);
        }
        ++itr;
    }

    //!!! need the option of ignoring current velocities or adjusting
    //them: at the moment ApplyTextDynamics ignores them and the
    //others adjust them

    if (m_interpretations & GuessDirections)
        guessDirections();
    if (m_interpretations & ApplyTextDynamics)
        applyTextDynamics();
    if (m_interpretations & ApplyHairpins)
        applyHairpins();
    if (m_interpretations & StressBeats)
        stressBeats();
    if (m_interpretations & Articulate)
        articulate();

    //!!! Finally, in future we should extend this to allow
    // indications on one segment (e.g. top line of piano staff) to
    // affect another (e.g. bottom line).  All together now: "Even
    // X11 Rosegarden could do that!"
}

void
InterpretCommand::guessDirections()
{
    //...
}

void
InterpretCommand::applyTextDynamics()
{
    // laborious

    Segment &segment(getSegment());
    int velocity = 100;

    timeT startTime = m_selection->getStartTime();
    timeT endTime = m_selection->getEndTime();

    for (Segment::iterator i = segment.begin();
            segment.isBeforeEndMarker(i); ++i) {

        timeT t = (*i)->getAbsoluteTime();

        if (t > endTime)
            break;

        if (Text::isTextOfType(*i, Text::Dynamic)) {

            std::string text;
            if ((*i)->get
                    <String>(Text::TextPropertyName, text)) {
                velocity = getVelocityForDynamic(text);
            }
        }

        if (t >= startTime &&
                (*i)->isa(Note::EventType) && m_selection->contains(*i)) {
            (*i)->set
            <Int>(VELOCITY, velocity);
        }
    }
}

int
InterpretCommand::getVelocityForDynamic(std::string text)
{
    int velocity = 100;

    // should do case-insensitive matching with whitespace
    // removed.  can surely be cleverer about this too!

    if (text == "ppppp")
        velocity = 10;
    else if (text == "pppp")
        velocity = 20;
    else if (text == "ppp")
        velocity = 30;
    else if (text == "pp")
        velocity = 40;
    else if (text == "p")
        velocity = 60;
    else if (text == "mp")
        velocity = 80;
    else if (text == "mf")
        velocity = 90;
    else if (text == "f")
        velocity = 105;
    else if (text == "ff")
        velocity = 110;
    else if (text == "fff")
        velocity = 115;
    else if (text == "ffff")
        velocity = 120;
    else if (text == "fffff")
        velocity = 125;
    else if (text == "d5")
        velocity = 5;
    else if (text == "d10")
        velocity = 10;
    else if (text == "d15")
        velocity = 15;
    else if (text == "d20")
        velocity = 20;
    else if (text == "d25")
        velocity = 25;
    else if (text == "d30")
        velocity = 30;
    else if (text == "d35")
        velocity = 35;
    else if (text == "d40")
        velocity = 40;
    else if (text == "d45")
        velocity = 45;
    else if (text == "d50")
        velocity = 50;
    else if (text == "d55")
        velocity = 55;
    else if (text == "d60")
        velocity = 60;
    else if (text == "d65")
        velocity = 65;
    else if (text == "d70")
        velocity = 70;
    else if (text == "d75")
        velocity = 75;
    else if (text == "d80")
        velocity = 80;
    else if (text == "d85")
        velocity = 85;
    else if (text == "d90")
        velocity = 90;
    else if (text == "d95")
        velocity = 95;
    else if (text == "d100")
        velocity = 100;
    else if (text == "d105")
        velocity = 105;
    else if (text == "d110")
        velocity = 110;
    else if (text == "d115")
        velocity = 115;
    else if (text == "d120")
        velocity = 120;
    else if (text == "d125")
        velocity = 125;
    
    NOTATION_DEBUG << "InterpretCommand::getVelocityForDynamic: unrecognised dynamic " << text << endl;

    return velocity;
}

void
InterpretCommand::applyHairpins()
{
    Segment &segment(getSegment());
    int velocityToApply = -1;

    for (EventSelection::eventcontainer::iterator ecitr =
                m_selection->getSegmentEvents().begin();
            ecitr != m_selection->getSegmentEvents().end(); ++ecitr) {

        Event *e = *ecitr;
        if (Text::isTextOfType(e, Text::Dynamic)) {
            velocityToApply = -1;
        }
        if (!e->isa(Note::EventType))
            continue;
        bool crescendo = true;

        IndicationMap::iterator inditr =
            findEnclosingIndication(e, Indication::Crescendo);

        // we can't be in both crescendo and decrescendo -- at least,
        // not meaningfully

        if (inditr == m_indications.end()) {
            inditr = findEnclosingIndication(e, Indication::Decrescendo);
            if (inditr == m_indications.end()) {
                if (velocityToApply > 0) {
                    e->set
                    <Int>(VELOCITY, velocityToApply);
                }
                continue;
            }
            crescendo = false;
        }

        // The starting velocity for the indication is easy -- it's
        // just the velocity of the last note at or before the
        // indication begins that has a velocity

        timeT hairpinStartTime = inditr->first;
        // ensure we scan all of the events at this time:
        Segment::iterator itr(segment.findTime(hairpinStartTime + 1));
        while (itr == segment.end() ||
                (*itr)->getAbsoluteTime() > hairpinStartTime ||
                !(*itr)->isa(Note::EventType) ||
                !(*itr)->has(VELOCITY)) {
            if (itr == segment.begin()) {
                itr = segment.end();
                break;
            }
            --itr;
        }

        long startingVelocity = 100;
        if (itr != segment.end()) {
            (*itr)->get
            <Int>(VELOCITY, startingVelocity);
        }

        // The ending velocity is harder.  If there's a dynamic change
        // directly after the hairpin, then we want to use that
        // dynamic's velocity unless it opposes the hairpin's
        // direction.  If there isn't, or it does oppose the hairpin,
        // we should probably make the degree of change caused by the
        // hairpin depend on its total duration.

        long endingVelocity = startingVelocity;
        timeT hairpinEndTime = inditr->first +
                               inditr->second->getIndicationDuration();
        itr = segment.findTime(hairpinEndTime);
        while (itr != segment.end()) {
            if (Text::isTextOfType(*itr, Text::Dynamic)) {
                std::string text;
                if ((*itr)->get
                        <String>(Text::TextPropertyName, text)) {
                    endingVelocity = getVelocityForDynamic(text);
                    break;
                }
            }
            if ((*itr)->getAbsoluteTime() >
                    (hairpinEndTime + Note(Note::Crotchet).getDuration()))
                break;
            ++itr;
        }

        if (( crescendo && (endingVelocity < startingVelocity)) ||
                (!crescendo && (endingVelocity > startingVelocity))) {
            // we've got it wrong; prefer following the hairpin to
            // following whatever direction we got the dynamic from
            endingVelocity = startingVelocity;
            // and then fall into the next conditional to set up the
            // velocities
        }

        if (endingVelocity == startingVelocity) {
            // calculate an ending velocity based on starting velocity
            // and hairpin duration (okay, we'll leave that bit for later)
            endingVelocity = startingVelocity * (crescendo ? 120 : 80) / 100;
        }

        double proportion =
            (double(e->getAbsoluteTime() - hairpinStartTime) /
             double(hairpinEndTime - hairpinStartTime));
        long velocity =
            int((endingVelocity - startingVelocity) * proportion +
                startingVelocity);

        NOTATION_DEBUG << "InterpretCommand::applyHairpins: velocity of note at " << e->getAbsoluteTime() << " is " << velocity << " (" << proportion << " through hairpin from " << startingVelocity << " to " << endingVelocity << ")" << endl;
        if (velocity < 10)
            velocity = 10;
        if (velocity > 127)
            velocity = 127;
        e->set
        <Int>(VELOCITY, velocity);
        velocityToApply = velocity;
    }
}

void
InterpretCommand::stressBeats()
{
    Composition *c = getSegment().getComposition();

    for (EventSelection::eventcontainer::iterator itr =
                m_selection->getSegmentEvents().begin();
            itr != m_selection->getSegmentEvents().end(); ++itr) {

        Event *e = *itr;
        if (!e->isa(Note::EventType))
            continue;

        timeT t = e->getNotationAbsoluteTime();
        TimeSignature timeSig = c->getTimeSignatureAt(t);
        timeT barStart = getSegment().getBarStartForTime(t);
        int stress = timeSig.getEmphasisForTime(t - barStart);

        // stresses are from 0 to 4, so we add 12% to the velocity
        // at the maximum stress, subtract 4% at the minimum
        int velocityChange = stress * 4 - 4;

        // do this even if velocityChange == 0, in case the event
        // has no velocity yet
        long velocity = 100;
        e->get
        <Int>(VELOCITY, velocity);
        velocity += velocity * velocityChange / 100;
        if (velocity < 10)
            velocity = 10;
        if (velocity > 127)
            velocity = 127;
        e->set
        <Int>(VELOCITY, velocity);
    }
}

void
InterpretCommand::articulate()
{
    // Basic articulations:
    //
    // -- Anything marked tenuto or within a slur gets 100% of its
    //    nominal duration (that's what we need the quantizer for,
    //    to get the display nominal duration), and its velocity
    //    is unchanged.
    //
    // -- Anything marked marcato gets 60%, or 70% if slurred (!),
    //    and gets an extra 15% of velocity.
    //
    // -- Anything marked staccato gets 55%, or 70% if slurred,
    //    and unchanged velocity.
    //
    // -- Anything marked staccatissimo gets 30%, or 50% if slurred (!),
    //    and loses 5% of velocity.
    //
    // -- Anything marked sforzando gains 35% of velocity.
    //
    // -- Anything marked with an accent gains 30% of velocity.
    //
    // -- Anything marked rinforzando gains 15% of velocity and has
    //    its full duration.  Guess we really need to use some proper
    //    controllers here.
    //
    // -- Anything marked down-bow gains 5% of velocity, anything
    //    marked up-bow loses 5%.
    //
    // -- Anything unmarked and unslurred, or marked tenuto and
    //    slurred, gets 90% of duration.

    std::set
        <Event *> toErase;
    std::set
        <Event *> toInsert;
    Segment &segment(getSegment());

    for (EventSelection::eventcontainer::iterator ecitr =
                m_selection->getSegmentEvents().begin();
            ecitr != m_selection->getSegmentEvents().end(); ++ecitr) {

        Event *e = *ecitr;
        if (!e->isa(Note::EventType))
            continue;
        Segment::iterator itr = segment.findSingle(e);
        Chord chord(segment, itr, m_quantizer);

        // the things that affect duration
        bool staccato = false;
        bool staccatissimo = false;
        bool marcato = false;
        bool tenuto = false;
        bool rinforzando = false;
        bool slurred = false;

        int velocityChange = 0;

        std::vector<Mark> marks(chord.getMarksForChord());

        for (std::vector<Mark>::iterator i = marks.begin();
                i != marks.end(); ++i) {

            if (*i == Marks::Accent) {
                velocityChange += 30;
            } else if (*i == Marks::Tenuto) {
                tenuto = true;
            } else if (*i == Marks::Staccato) {
                staccato = true;
            } else if (*i == Marks::Staccatissimo) {
                staccatissimo = true;
                velocityChange -= 5;
            } else if (*i == Marks::Marcato) {
                marcato = true;
                velocityChange += 15;
            } else if (*i == Marks::Sforzando) {
                velocityChange += 35;
            } else if (*i == Marks::Rinforzando) {
                rinforzando = true;
                velocityChange += 15;
            } else if (*i == Marks::DownBow) {
                velocityChange += 5;
            } else if (*i == Marks::UpBow) {
                velocityChange -= 5;
            }
        }

        IndicationMap::iterator inditr =
            findEnclosingIndication(e, Indication::Slur);

        if (inditr != m_indications.end())
            slurred = true;
        if (slurred) {
            // last note in a slur should be treated as if unslurred
            timeT slurEnd =
                inditr->first + inditr->second->getIndicationDuration();
            if (slurEnd == e->getNotationAbsoluteTime() + e->getNotationDuration() ||
                    slurEnd == e->getAbsoluteTime() + e->getDuration()) {
                slurred = false;
            }
            /*!!!
            	    Segment::iterator slurEndItr = segment.findTime(slurEnd);
            	    if (slurEndItr != segment.end() &&
            		(*slurEndItr)->getNotationAbsoluteTime() <=
            		            e->getNotationAbsoluteTime()) {
            		slurred = false;
            	    } 
            */
        }

        int durationChange = 0;

        if (slurred) {
            //!!! doesn't seem to be picking up slurs correctly
            if (tenuto)
                durationChange = -10;
            else if (marcato || staccato)
                durationChange = -30;
            else if (staccatissimo)
                durationChange = -50;
            else
                durationChange = 0;
        } else {
            if (tenuto)
                durationChange = 0;
            else if (marcato)
                durationChange = -40;
            else if (staccato)
                durationChange = -45;
            else if (staccatissimo)
                durationChange = -70;
            else if (rinforzando)
                durationChange = 0;
            else
                durationChange = -10;
        }

        NOTATION_DEBUG << "InterpretCommand::modifySegment: chord has " << chord.size() << " notes in it" << endl;

        for (Chord::iterator ci = chord.begin();
                ci != chord.end(); ++ci) {

            e = **ci;

            NOTATION_DEBUG << "InterpretCommand::modifySegment: For note at " << e->getAbsoluteTime() << ", velocityChange is " << velocityChange << " and durationChange is " << durationChange << endl;

            // do this even if velocityChange == 0, in case the event
            // has no velocity yet
            long velocity = 100;
            e->get
            <Int>(VELOCITY, velocity);
            velocity += velocity * velocityChange / 100;
            if (velocity < 10)
                velocity = 10;
            if (velocity > 127)
                velocity = 127;
            e->set
            <Int>(VELOCITY, velocity);

            timeT duration = e->getNotationDuration();

            // don't mess with the duration of a tied note
            bool tiedForward = false;
            e->get<Bool>(TIED_FORWARD, tiedForward);
            if (tiedForward) {
                durationChange = 0;
                NOTATION_DEBUG << "InterpretCommand::modifySegment: Tied forward note encountered, durationChange is " << durationChange << endl;
            }

            timeT newDuration = duration + duration * durationChange / 100;

            // this comparison instead of "durationChange != 0"
            // because we want to permit the possibility of resetting
            // the performance duration of a note (that's perhaps been
            // articulated wrongly) based on the notation duration:

            timeT eventDuration = e->getDuration();

            if (eventDuration != newDuration) {

                NOTATION_DEBUG << "InterpretCommand::modifySegment: durationChange is " << durationChange << "; e->getDuration() is " << eventDuration << "; newDuration is " << newDuration << endl;

                if (toErase.find(e) == toErase.end()) {

                    //!!! deal with tuplets

                    Event *newEvent = new Event(*e,
                                                e->getAbsoluteTime(),
                                                newDuration,
                                                e->getSubOrdering(),
                                                e->getNotationAbsoluteTime(),
                                                duration);
                    toInsert.insert(newEvent);
                    toErase.insert(e);
                }
            }
        }

        // what we want to do here is jump our iterator to the final
        // element in the chord -- but that doesn't work because we're
        // iterating through the selection, not the segment.  So for
        // now we just accept the fact that notes in chords might be
        // processed multiple times (slow) and added into the toErase
        // set more than once (hence the nasty tests in the loop just
        // after the close of this loop).
    }

    for (std::set
                <Event *>::iterator j = toErase.begin(); j != toErase.end(); ++j) {
            Segment::iterator jtr(segment.findSingle(*j));
            if (jtr != segment.end())
                segment.erase(jtr);
        }

    for (std::set
                <Event *>::iterator j = toInsert.begin(); j != toInsert.end(); ++j) {
            segment.insert(*j);
        }
}

InterpretCommand::IndicationMap::iterator

InterpretCommand::findEnclosingIndication(Event *e,
        std::string type)
{
    // a bit slow, but let's wait and see whether it's a bottleneck
    // before we worry about that

    timeT t = e->getAbsoluteTime();
    IndicationMap::iterator itr = m_indications.lower_bound(t);

    while (1) {
        if (itr != m_indications.end()) {

            if (itr->second->getIndicationType() == type &&
                    itr->first <= t &&
                    itr->first + itr->second->getIndicationDuration() > t) {
                return itr;
            }
        }
        if (itr == m_indications.begin())
            break;
        --itr;
    }

    return m_indications.end();
}

}
