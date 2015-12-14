/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[FigurationSourceMap]"

#include "FigurationSourceMap.h"
#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/NotationQuantizer.h"
#include "base/NotationRules.h"
#include "base/NotationTypes.h"
#include "base/Property.h"
#include "base/Selection.h"
#include "base/figuration/FigChord.h"
#include "base/figuration/RelativeEvent.h"
#include "misc/Debug.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace Rosegarden
{
/***** Internal types *****/

/// @typedef pitchT
/// The types we retrieve from particular properties
/// @author Tom Breton (Tehom)
typedef PropertyDefn<Int>::basic_type pitchT;
typedef PropertyDefn<Int>::basic_type velocityT;


/// @class RelativeNote
/// Class to describe a relative note
/// @author Tom Breton (Tehom)
class RelativeNote : public RelativeEvent
{
public:
    RelativeNote(int i, Event *e, timeT startTime) :
        RelativeEvent(e, startTime),
        m_index(i)
    {};
    virtual Event   *getAsEvent(timeT baseTime, const Key key,
                                const FigChord *notes);
    virtual pitchT getResultPitch(const Key key, const Pitch & basePitch)=0;
private:
    int              m_index;
};

/// @class ChromaticRelativeNote
/// Class to describe a chromatically relative note
/// @author Tom Breton (Tehom)
class ChromaticRelativeNote : public RelativeNote
{
public:
    ChromaticRelativeNote(int i, Event *e, timeT startTime,
                          const Pitch &basePitch) :
        RelativeNote(i, e, startTime),
        m_interval(e->get<Int>(BaseProperties::PITCH) -
                   basePitch.getPerformancePitch()) {

        // The larger the interval, the worse the score.
        const int absInterval = abs(m_interval);
        const int penalty = (100 * (absInterval % 12)) + (absInterval / 12);
        setScore(-penalty);
    };
    static pitchT addChromaticInterval(const Pitch & basePitch, int interval);
    virtual pitchT getResultPitch(const Key key, const Pitch & basePitch);
private:
    int  m_interval;
};

/// @class DiatonicRelativeNote
/// Class to describe a diatonically relative note
/// @author Tom Breton (Tehom)
class DiatonicRelativeNote : public RelativeNote
{
public:
    DiatonicRelativeNote(int i, Event *e, timeT startTime,
                         const Key key, const Pitch basePitch);
    static pitchT addDiatonicInterval(const Key key,
                                      const Pitch & basePitch,
                                      int interval);
    virtual pitchT getResultPitch(const Key key, const Pitch & basePitch);
private:
    int  m_interval;
};

/// @class ProximityNote
/// Class to describe a proximity note, ie one whose pitch is the
/// nearest chord tone (counting octaves etc as unisons)
/// @author Tom Breton (Tehom)
class ProximityNote : public RelativeEvent
{
    friend class UnparamaterizedRelativeEventAdder;

    typedef std::vector<int> IndexVector;
    typedef int PitchNoOctave;
    typedef std::vector<PitchNoOctave> PitchNoOctaveVector;

    // Fold a range of [-11,11] to a range of [-6,5], changing only
    // the octave component, not the pitch-modulo-octave component.
    static int doubleOctaveToBalanced(int i)
    {
        if (i >= 6) { return i - 12; }
        else if (i < -6) { return i + 12; }
        return i;
    }
    class TonePressure {
    public:
        enum Pressure { high, normal, low, numStates, };

        TonePressure(PitchNoOctave pitch) :
            m_pitch(pitch)
            {}
        void addIndex(int index)
        {
            m_originalTones.push_back(index);
            sort(m_originalTones.begin(), m_originalTones.end());
        }
        // Unused now.
        PitchNoOctave getPitch(void) { return m_pitch; }
        int getPenalty(PitchNoOctave pitch)
        {
            // Direction of subtraction here isn't important since
            // we'll square it.  We must convert "round the corner"
            // comparisons like 0 vs 11 into the range -6 to 5.
            int difference = doubleOctaveToBalanced(pitch - m_pitch);
            return difference * difference;
        }
        static int getMovementPenalty(TonePressure &from,
                                      TonePressure &to,
                                      PitchNoOctaveVector &pitches,
                                      bool forwards)
        {
            const int index =
                forwards ?
                from.m_originalTones.back() :
                from.m_originalTones.front();
                
            const PitchNoOctave ourPitch = pitches[index];
                            
            const int oldPenalty =
                from.getPenalty(ourPitch);

            const int newPenalty =
                to.getPenalty(ourPitch);
                            
            return newPenalty - oldPenalty;
        }
        static void moveIndexForwards(TonePressure &from,
                                      TonePressure &to)
        {
            moveIndexForwards(from.m_originalTones, to.m_originalTones);
        }
        static void moveIndexBackwards(TonePressure &to,
                                       TonePressure &from)
        {
            moveIndexBackwards(to.m_originalTones, from.m_originalTones);
        }        

        Pressure getState(size_t min, size_t max)
        {
            if (m_originalTones.size() > max) {
                return high;
            } else if (m_originalTones.size() < min) {
                return low;
            } else { return normal; }
        }
        void insertPitchForEachIndex(PitchNoOctaveVector * finalMapping)
        {
            for (IndexVector::iterator j = m_originalTones.begin();
                 j != m_originalTones.end();
                 ++j) {
                const PitchNoOctave pitch = m_pitch;
                (*finalMapping)[*j] = pitch;
            }
        }

    private:
        // Safe even if "from" is empty.
        static void moveIndexForwards(IndexVector &from,
                                      IndexVector &to)
        {
            if (from.empty()) { return; }
            const int index = from.back();
            from.pop_back();
            to.insert(to.begin(), index);
        }

        // Safe even if "from" is empty.
        static void moveIndexBackwards(IndexVector &to,
                                       IndexVector &from)
        {
            if (from.empty()) { return; }
            const int index = from.front();
            from.erase(from.begin());
            to.push_back(index);
        }        

        IndexVector     m_originalTones;
        PitchNoOctave	m_pitch;
    };
    typedef std::vector<TonePressure> TonePressureVector;
    class SharedData {
    public:
        SharedData(void) :
            m_oldFigChord(0),
            m_pitchDeltas(0)
            {}

        void init(PitchNoOctaveVector &unorderedPitches)
        {
            // Caller guarantees that unorderedPitches has no
            // duplicates.
            
            m_originalPitches = unorderedPitches;
            sort(m_originalPitches.begin(),
                 m_originalPitches.end());
            // Now m_originalPitches is in its final form: sorted and
            // without duplicates (because unorderedPitches has no
            // duplicates)

            // We already know how big m_indexes will be.
            m_indexes.resize(m_originalPitches.size());

            typedef PitchNoOctaveVector::iterator iterator;
            for (iterator i = unorderedPitches.begin();
                 i != unorderedPitches.end();
                 ++i) {
                iterator found = find(m_originalPitches.begin(),
                                      m_originalPitches.end(),
                                      *i);

                Q_ASSERT_X(found != m_originalPitches.end(),
                           "SharedData::init",
                           "didn't find this pitch");
                const int targetIndex = i - unorderedPitches.begin();
                const int sourceIndex = found - m_originalPitches.begin();
                m_indexes[targetIndex] = sourceIndex;
            }

            // Now m_indexes is in its final form: Any ProximityNote's
            // m_index, thru m_indexes, indexes the corresponding
            // element of m_originalPitches, and each is indexed by
            // some element of m_indexes (true because
            // m_originalPitches has no duplicates)

            // m_pitchDeltas and m_oldFigChord are not set here, they
            // remain null pointers.
        }

        TonePressureVector getTonePressureVector(const FigChord* notes)
        {
            // Temporary container, easier than doing this work on
            // TonePressure objects.
            PitchNoOctaveVector notePitchesInOrder;

            // Add all the pitches.  It's easier to explicitly loop
            // and inline the conversion than to build a conversion
            // object and pass it to "transform".
            for (FigChord::const_iterator i = notes->begin();
                 i != notes->end();
                 ++i) {
                const Event *toneEvent = **i;
                const pitchT pitch =
                    toneEvent->get<Int>(BaseProperties::PITCH);
                const PitchNoOctave pitchNoOctave = pitch % 12;
                notePitchesInOrder.push_back(pitchNoOctave);
            }

            // Put the pitches in order.
            sort(notePitchesInOrder.begin(),notePitchesInOrder.end());

            // Make a corresponding TonePressureVector.  This ctor
            // works because TonePressure has a ctor taking just
            // PitchNoOctave.
            return
                TonePressureVector(notePitchesInOrder.begin(),
                                   notePitchesInOrder.end());
        }

        void update(const FigChord* notes)
        {
            // If the old data is still valid, done.
            if (m_oldFigChord == notes) { return; }

            Q_ASSERT_X(!notes->empty(),
                       "SharedData::update",
                       "the chord has no notes");

            if (m_pitchDeltas) { delete m_pitchDeltas; }
            m_pitchDeltas = 0;
            m_oldFigChord = notes;

            TonePressureVector tones = getTonePressureVector(notes);

            // Place each index with the chord tone that best matches
            // it.  We iterate over m_indexes so that we can know
            // indexes to place, rather than pitches.
            for (IndexVector::const_iterator i = m_indexes.begin();
                 i != m_indexes.end();
                 ++i) {
                const PitchNoOctave ourPitch = m_originalPitches[*i];

                // Find the closest chord tone (modulo octaves)
                int leastPenalty = 1000000;
                TonePressure *bestTonePressure = 0;
                for (TonePressureVector::iterator j = tones.begin();
                     j != tones.end();
                     ++j
                     ) {
                    TonePressure *tonePressure = &*j;
                    const int penalty = tonePressure->getPenalty(ourPitch);
                    if (penalty < leastPenalty) {
                        bestTonePressure = tonePressure;
                        leastPenalty = penalty;
                    }
                }

                // Since notes isn't empty, something must have beaten
                // the initial leastPenalty.
                Q_CHECK_PTR(bestTonePressure);

                // Add this index to the best match.  TonePressure
                // maintains the sorting invariant.
                bestTonePressure->addIndex(*i);
            }

            // Now "tones" is fully populated with indexes, but they
            // may be unevenly distributed.

            // Figure out the extremes of population.  For instance,
            // if 4 chord tones are distributed among 3 original
            // indexes, each chord tone may have 1 or 2 associated
            // indexes, no more or fewer.

            const float exactRatio =
                float(m_indexes.size()) / float(tones.size());
            const int maximumOriginals = ceil(exactRatio);
            const int minimumOriginals = floor(exactRatio);

            // Try to balance tones' mapping to original tones, while
            // still preserving a good match of pitches.
            while (true)
            {
                enum Strategy { NoOp, Up, Down, };
                
                int leastPenalty = 1000000;

                // Variables about the best next operation.  It's
                // possible that none is chosen, but in that case
                // chosenStrategy remains NoOp so the loop ends and
                // the other "chosenX" variables aren't used.
                Strategy chosenStrategy = NoOp;
                int chosenIndex = -1;
                int chosenCascadeIncrement = 0;
                
                // Scan by integers, so we can easily grab 2 elements
                // at once and do modular lookup.
                for (size_t i = 0; i < tones.size(); ++i) {
                    const int highIndex = (i + 1) % tones.size();
                    TonePressure &tonePressureA = tones.at(i);
                    TonePressure &tonePressureB = tones.at(highIndex);

                    // Decide what we could do to resolve
                    /// over/underpressure.
                    TonePressure::Pressure AState =
                        tonePressureA.getState(minimumOriginals,
                                               maximumOriginals);
                    TonePressure::Pressure BState =
                        tonePressureB.getState(minimumOriginals,
                                               maximumOriginals);

                    Strategy strategies
                        [TonePressure::numStates]
                        [TonePressure::numStates] = {
                        // **********  [][high], [][normal], [][low]
                        /* [high]  [] */ { NoOp, Up,    Up, },
                        /* [normal][] */ { Down, NoOp,  Up, },
                        /* [low]   [] */ { Down, Down, NoOp, },
                    };
                    Strategy strategy = strategies[AState][BState];

                    // What direction to cascade in, if any (See
                    // below).  The matches between high and low don't
                    // cascade because there's no right direction for
                    // them to cascade in.
                    int cascadeIncrements
                        [TonePressure::numStates]
                        [TonePressure::numStates] = {
                        // **********  [][high], [][normal], [][low]
                        /* [high]  [] */ { 0,  1,  0, },
                        /* [normal][] */ {-1,  0, -1, },
                        /* [low]   [] */ { 0,  1,  0, },
                    };
                    
                    switch (strategy) {
                    case NoOp:
                        break;
                    case Up:
                        {
                            // Cheat: We don't calculate cascade
                            // penalties, just singleton penalties.
                            const int penalty =
                                TonePressure::getMovementPenalty(tonePressureA,
                                                                 tonePressureB,
                                                                 m_originalPitches,
                                                                 true);
                            
                            if (penalty < leastPenalty) {
                                leastPenalty = penalty;
                                chosenStrategy = strategy;
                                chosenIndex = i;
                                chosenCascadeIncrement =
                                    cascadeIncrements[AState][BState];
                            }
                        }
                        break;
                    case Down:
                        {
                            // Cheat: We don't calculate cascade
                            // penalties, just singleton penalties.
                            const int penalty =
                                TonePressure::getMovementPenalty(tonePressureB,
                                                                 tonePressureA,
                                                                 m_originalPitches,
                                                                 false);
                            
                            if (penalty < leastPenalty) {
                                leastPenalty = penalty;
                                chosenStrategy = strategy;
                                chosenIndex = i;
                                chosenCascadeIncrement =
                                    cascadeIncrements[AState][BState];
                            }
                        }
                        break;
                    }
                }

                // If we have no way to improve the balance, the outer
                // loop is done.
                if (chosenStrategy == NoOp) { break; }

                
                // Do the "fix" operation at least once.
                
                // Then we may loop.  We do this to prevent indexes
                // bouncing forever between a pair of nodes, each
                // iteration undoing the previous iteration's
                // "improvement".  To prevent this, we continue until
                // the node we just changed doesn't have the same
                // pressure state that we are fixing, thus it won't
                // bounce.

                // We are guaranteed to find at least one such node.
                // That's because we chose limits such that if any
                // node is low pressure, there exists at least one
                // node that, after losing an index, still won't be
                // low pressure, and similarly for high
                // pressure/gaining an index.

                // This prevention works even if we have to skip some
                // empty nodes, because we are always pulling at least
                // one index towards the underpopulated group of
                // nodes, which will eventually populate them enough.
                
                // Matches between high and low don't loop and we
                // don't "fix" normal/normal, so there's always a
                // distinct state that we are fixing.

                for (int i = chosenIndex;true;) {

                    const int highIndex = (i + 1) % tones.size();
                    TonePressure &tonePressureA = tones.at(i);
                    TonePressure &tonePressureB = tones.at(highIndex);

                    bool breakInnerLoop;
                    switch (chosenStrategy) {
                    case Up: {
                        // Safe even for empty sources.
                        TonePressure::moveIndexForwards(tonePressureA,
                                                        tonePressureB);

                        switch (chosenCascadeIncrement) {
                        case 1:
                            // Going forwards and moving indexes
                            // forwards: Node B might become high
                            // pressure.
                            breakInnerLoop =
                                (tonePressureB.getState(minimumOriginals,
                                                        maximumOriginals)
                                 != TonePressure::high);
                            break;

                        case -1:
                            // Going backwards but moving indexes
                            // forwards: Node A might become low
                            // pressure.
                            breakInnerLoop =
                                (tonePressureA.getState(minimumOriginals,
                                                        maximumOriginals)
                                 != TonePressure::low);
                            break;

                        case 0:
                        default:
                            // Not cascading: stop after the very
                            // first one.
                            breakInnerLoop = true;
                            break;
                        }
                        break;
                    }
                    case Down: {
                        // Safe even for empty sources.
                        TonePressure::moveIndexBackwards(tonePressureA,
                                                         tonePressureB);
                        switch (chosenCascadeIncrement) {
                        case 1:
                            // Going forwards but moving indexes
                            // backwards: Node B might become low
                            // pressure.
                            breakInnerLoop =
                                (tonePressureB.getState(minimumOriginals,
                                                        maximumOriginals)
                                 != TonePressure::low);
                            break;

                        case -1:
                            // Going backwards and moving indexes
                            // backwards:  Node A might become high
                            // pressure.
                            breakInnerLoop =
                                (tonePressureA.getState(minimumOriginals,
                                                        maximumOriginals)
                                 != TonePressure::high);
                            break;

                        case 0:
                        default:
                            // Not cascading: stop after the very
                            // first one.
                            breakInnerLoop = true;
                            break;
                        }

                        break;
                    }
                        
                    case NoOp:
                        throw Exception("Shouldn't see NoOp here.");
                        break;
                    }

                    // Break out of the inner loop, but let the outer
                    // loop continue.
                    if (breakInnerLoop) { break; }
                    
                    Q_ASSERT(chosenCascadeIncrement != 0);

                    // chosenCascadeIncrement is 1 or -1, so we are
                    // actually taking single steps.
                    i += chosenCascadeIncrement;

                    // Effectively "i %= tones.size();" except that
                    // had problems with negative i
                    if (i >= (int)tones.size()) { i -= tones.size(); }
                    else if (i < 0) { i += tones.size(); }
                }
            }

            // Now "tones" is balanced.

            // Make an object to contain the target pitches, in the
            // same order as m_pitchDeltas and m_originalPitches.
            PitchNoOctaveVector targetPitches;
            targetPitches.resize(m_indexes.size());

            // Insert pitches into it.
            for (TonePressureVector::iterator i = tones.begin();
                 i != tones.end();
                 ++i)
                { i->insertPitchForEachIndex(&targetPitches); }

            // Make a new final mapping object.
            int * finalMapping = new int[m_indexes.size()];

            // Populate it with the pitch deltas.
            for (size_t i = 0; i < m_originalPitches.size(); ++i) {
                int pitchDelta =
                    targetPitches[i] - m_originalPitches[i];
                finalMapping[i] = doubleOctaveToBalanced(pitchDelta);
            }

            // Finally we have the pitch delta mapping object that
            // ProximityNote::getAsEvent needs.
            m_pitchDeltas = finalMapping;
        }

        PitchNoOctave getPitchDelta(int index)
        {
            const int pitchDelta = m_pitchDeltas[m_indexes[index]];
            return pitchDelta;
        }
        
    private:

        // A layer of indirection.  Every ProximityNote in the
        // relevant figuration contains an index into m_indexes.  That
        // entry of m_indexes itself indexes m_originalPitches and
        // m_pitchDeltas.  The corresponding entry of
        // m_originalPitches contains that ProximityNote's original
        // pitch (used for calculating m_pitchDeltas).
        IndexVector m_indexes;

        // All the original pitches of the figuration, modulo octaves,
        // in order and without repititions.  Each is indexed by some
        // member of m_indexes.
        PitchNoOctaveVector m_originalPitches;

        // Cached values, initially null pointers.  When we see a new
        // figChord, we make a new m_pitchDeltas.

        // Cached: The figchord that determined the most recent final
        // mapping.
        const FigChord *m_oldFigChord;

        // Cached: The most recent final mapping.  All the pitch
        // deltas to use, in the same respective order as
        // m_originalPitches.
        const int *m_pitchDeltas;
    };
    
public:
    ProximityNote(Event *e, timeT startTime, int index,
                  SharedData *sharedData) :
        RelativeEvent(e, startTime),
        m_index(index),
        m_sharedData(sharedData)
    { setScore(0); };
    virtual Event   *getAsEvent(timeT baseTime, const Key key,
                                const FigChord *notes);
private:
    int m_index;
    SharedData *m_sharedData;
};

/// @class RelativeNonnote
/// Class to describe a relative note
/// @author Tom Breton (Tehom)
class RelativeNonnote : public RelativeEvent
{
public:
    RelativeNonnote(Event *e, timeT startTime) :
        RelativeEvent(e, startTime)
    {};
    virtual Event   *getAsEvent(timeT baseTime, const Key key,
                                const FigChord *notes);
};

/***** Internal types to add complete collections of notes *****/

class BaseRelativeEventAdder
{
protected:
    typedef FigurationSourceMap::UnsolvedNote UnsolvedNote;

public:
    virtual UnsolvedNote add(Event *e)=0;
    virtual ~BaseRelativeEventAdder(void) {};

protected:
    BaseRelativeEventAdder(timeT startTime) :
        m_startTime(startTime)
    {}
    
    // !!! Taking over from FigurationSourceMap
    // I take ownership of "e"
    static UnsolvedNote trivialUnsolvedNote(RelativeEvent *e) {
        UnsolvedNote trivialContainer;
        trivialContainer.insert(e);
        return trivialContainer;
    };

    timeT m_startTime;
};

class ParamaterizedRelativeEventAdder : public BaseRelativeEventAdder
{
public:
    ParamaterizedRelativeEventAdder(timeT startTime, Segment *s,
                                    const FigChord *parameterChord) :
        BaseRelativeEventAdder(startTime),
        m_parameterChord(parameterChord),
        m_key(s->getKeyAtTime(startTime))
        {}

    // This shoudl actually become getPossibleRelations.
    virtual UnsolvedNote add(Event *e)
    {
        if (e->isa(Note::EventType)) {
            return FigurationSourceMap::getPossibleRelations(e,
                                                             m_parameterChord, m_key, m_startTime);
        } else {
            return
                trivialUnsolvedNote(new RelativeNonnote(e, m_startTime));
        }
    }
    
    const FigChord *m_parameterChord;
    const Key m_key;
};

class UnparamaterizedRelativeEventAdder : public BaseRelativeEventAdder
{
public:    
    UnparamaterizedRelativeEventAdder(timeT startTime) :
        BaseRelativeEventAdder(startTime),
        m_sharedData(new ProximityNote::SharedData)
        {}
        
    virtual ~UnparamaterizedRelativeEventAdder(void)
    {
        // Destroying the adder triggers setting up the shared data,
        // using what we collected during the adder's lifetime.
        
        // When we get here, we've placed all the events.  Now to
        // set shared data correctly.
        m_sharedData->init(m_pitchesInOccurenceOrder);
    }
private:
    // Return this pitch's index in m_pitchesInOccurenceOrder, adding
    // it there if it's not already.
    int getPitchIndex(pitchT pitch)
    {
        typedef ProximityNote::PitchNoOctaveVector::iterator
            iterator;
        const pitchT pitchNoOctave = pitch % 12;
        const iterator found = find(m_pitchesInOccurenceOrder.begin(),
                                    m_pitchesInOccurenceOrder.end(),
                                    pitchNoOctave);
        if (found == m_pitchesInOccurenceOrder.end()) {
            // Make a new entry
            m_pitchesInOccurenceOrder.push_back(pitchNoOctave);
            return m_pitchesInOccurenceOrder.size() - 1;
        } else {
            // Use the old entry.
            return found - m_pitchesInOccurenceOrder.begin();
        }
    }
        
    virtual UnsolvedNote add(Event *e)
        {
            if (e->isa(Note::EventType)) {
                int index = getPitchIndex(e->get<Int>(BaseProperties::PITCH));
                return 
                    trivialUnsolvedNote(new ProximityNote(e,
                                                          m_startTime,
                                                          index,
                                                          m_sharedData));
            } else {
                return
                    trivialUnsolvedNote(new RelativeNonnote(e, m_startTime));
                    }
        }

    // I share this with many ProximityNotes
    ProximityNote::SharedData         *m_sharedData;
    ProximityNote::PitchNoOctaveVector m_pitchesInOccurenceOrder;
};

/// @typedef RelativeEventVec
/// Containers for RelativeEvent
/// @author Tom Breton (Tehom)
typedef std::vector<RelativeEvent *> RelativeEventVec;

/***** End of internal types *****/

/***** Helper functions *****/
    
/// True if Event a has higher pitch than Event b.  
/// @param a and b both must be notes.
/// @author Tom Breton (Tehom)
int higherPitch(Event *a, Event* b)
{
    if (!a->has(BaseProperties::PITCH) ||
        !b->has(BaseProperties::PITCH)) {
        throw Exception("Shouldn't get a note that has no pitch.");
    }

    return
        a->get<Int>(BaseProperties::PITCH) >
        b->get<Int>(BaseProperties::PITCH);
}

/***** Methods for DiatonicRelativeNote *****/

/// Make a DiatonicRelativeNote for Event e
/// @param i indexes some tone in the parameter chord.
/// @param basePitch is the pitch of that tone
/// @param startTime is the time the figuration started
/// @param key is the current key
/// @author Tom Breton (Tehom)
DiatonicRelativeNote::DiatonicRelativeNote(int i,
                                           Event *e,
                                           timeT startTime,
                                           const Key key,
                                           const Pitch basePitch) :
    RelativeNote(i, e, startTime)
{
    const int octaveBase = 0;
    const Pitch thisPitch(*e);
    // This folds ocatve information in, so (say) a major ninth gives
    // 8 rather than 1.
    int relativeNoteInScale =
        thisPitch.getNoteInScale(key) -
        basePitch.getNoteInScale(key);
    int relativeOctave =
        thisPitch.getOctave(octaveBase) -
        basePitch.getOctave(octaveBase);

    // Separate note in scale from note in octave.  First we add so
    // many octaves that it's surely positive.  We remove the same
    // number in the calculation of relativeOctave.
    relativeNoteInScale += 7 * 10;
    relativeOctave += (relativeNoteInScale / 7) - 10;
    relativeNoteInScale %= 7;
    
    m_interval = relativeNoteInScale + (7 * relativeOctave);
    // Big penalty for accidentals in key, because diatonics aren't
    // meant to handle them.
    const int accidentalPenalty =
        (thisPitch.getDisplayAccidental(key) ==
         Accidentals::NoAccidental) ? 0 : 10000;

    // Unisons, octaves etc aren't right for handling perfect
    // intervals because addDiatonicInterval removes accidentals.
    int unisonPenalty =
        (relativeNoteInScale == 0) ? 50000 : 0;

    const int penalty =
        abs(relativeNoteInScale) +
        abs(relativeOctave) +
        accidentalPenalty +
        unisonPenalty;
    setScore(-penalty);
};

/// Return the pitch of basePitch raised by a diatonic interval.
/// @param key is the key to govern diatonicity
/// @param basePitch is a Pitch
/// @param interval can be positive or negative
/// @author Tom Breton (Tehom)
pitchT
DiatonicRelativeNote::addDiatonicInterval(const Key key,
                                          const Pitch & basePitch,
                                          int interval)
{
    // This code does a lot of calculation that almost belongs to
    // Pitch, but can't easily be done thru the Pitch interface.
    const pitchT pitch = basePitch.getPerformancePitch();
    const int pitchInC = Key::DefaultKey.transposeFrom(pitch, key);
    const int noteInOctave = pitchInC % 12;
    const int oldOctave = pitchInC / 12;
    const int oldStepInScale =
        (key.isMinor()) ?
        steps_Cminor_harmonic[noteInOctave] :
        steps_Cmajor[noteInOctave];
    
    // Add the interval
    int newStepInScale = oldStepInScale + interval;

    // Ensure newStepInScale is within (0,6), remembering any
    // adjustment for octave.
    // Make sure it's positive so that operations / and % round
    // consistently.
    newStepInScale += 7 * 10;
    const int octavesGained = ((newStepInScale / 7) - 10);
    newStepInScale %= 7;

    const int newNoteInScale = 
        (key.isMinor()) ?
        scale_Cminor_harmonic[newStepInScale] :
        scale_Cmajor[newStepInScale];
    const int newOctave = oldOctave + octavesGained;

    const int newPitchInC = newNoteInScale + (newOctave * 12);
    return key.transposeFrom(newPitchInC, Key::DefaultKey);
}

/// Return pitch relative to basePitch
/// @param key is the key to govern diatonicity
/// @param e must be a note.
/// @author Tom Breton (Tehom)
pitchT
DiatonicRelativeNote::getResultPitch(const Key key, const Pitch & basePitch)
{
    return addDiatonicInterval(key, basePitch, m_interval);
}
/***** Methods for ChromaticRelativeNote *****/

/// Return the pitch of e, raised by a chromatic interval.
/// @param e must be a note.
/// @param interval can be positive or negative
/// @author Tom Breton (Tehom)
pitchT
ChromaticRelativeNote::addChromaticInterval(const Pitch & basePitch,
                                            int interval)
{
    return basePitch.getPerformancePitch() + interval;
}

/// Return the pitch relative to e
/// @param key is ignored
/// @param e must be a note.
/// @author Tom Breton (Tehom)
pitchT
ChromaticRelativeNote::getResultPitch(const Key, const Pitch & basePitch)
{
    return addChromaticInterval(basePitch, m_interval);
}

/***** Methods for RelativeNote *****/    

/// Return a new note event relative to baseNote.
/// @param baseTime is the baseline time.  
/// @param baseNote must be a note.  Its internal time is ignored.
/// @param key is the key to govern diatonicity
/// @author Tom Breton (Tehom)
Event *
RelativeNote::getAsEvent(timeT baseTime, const Key key,
                         const FigChord *notes)
{
    // Figure out time
    timeT newStartTime = getAbsoluteTime(baseTime);

    // Figure out pitch
    Event *baseNote = *notes->at(m_index);
    const Pitch basePitch(*baseNote);
    pitchT pitch =
        getResultPitch(key, basePitch);

    // Figure out velocity
    velocityT velocity = 
        baseNote->get<Int>(BaseProperties::VELOCITY) +
        (m_bareEvent)->get<Int>(BaseProperties::VELOCITY) -
        100;
    // Clamp to legal MIDI velocities
    if (velocity < 0)
        { velocity = 0; }
    if (velocity > 127)
        { velocity = 127; }
    
    // Create a corresponding note
    Event *newNote = new Event(*(m_bareEvent), newStartTime);
    newNote->set<Int>(BaseProperties::PITCH, pitch, true);
    newNote->set<Int>(BaseProperties::VELOCITY, velocity, true);
    return newNote;
}

/***** Methods for ProximityNote *****/

Event *
ProximityNote::getAsEvent(timeT baseTime, const Key /*key*/,
                          const FigChord *notes)
{
    m_sharedData->update(notes);

    const timeT newStartTime = getAbsoluteTime(baseTime);
    Event *newNote = new Event(*(m_bareEvent), newStartTime);

    const pitchT bareEventPitch =
        m_bareEvent->get<Int>(BaseProperties::PITCH);

    pitchT newPitch = bareEventPitch +
        m_sharedData->getPitchDelta(m_index);

    newNote->set<Int>(BaseProperties::PITCH, newPitch, true);
    return newNote;
}

/***** Methods for RelativeNonnote *****/

Event *
RelativeNonnote::getAsEvent(timeT baseTime, const Key /*key*/,
                            const FigChord */*notes*/)
{
    return new Event(*m_bareEvent, getAbsoluteTime(baseTime));
}

/*** FigurationSourceMap ***/

/// Return a set of possible generators for Event e
/// @author Tom Breton (Tehom)
FigurationSourceMap::UnsolvedNote
FigurationSourceMap::getPossibleRelations(Event *e,
					  const FigChord *parameterChord,
					  const Key key,
					  timeT startTime)
{
    UnsolvedNote possibleRelations;
    possibleRelations.clear();

    for (unsigned int i = 0; i < parameterChord->size(); ++i) {
        const Event *toneEvent = *parameterChord->at(i);
        const Pitch basePitch(*toneEvent);
        const unsigned int NbRelationTypes = 2;
        // !!! SIMPLIFY ME
        for (unsigned int j = 0; j < NbRelationTypes; ++j) {
            RelativeNote * relation;
            switch (j) {
            case 0:
                relation =
                    new DiatonicRelativeNote(i, e, startTime,
                                             key, basePitch);
                break;
            case 1:
                relation =
                    new ChromaticRelativeNote(i, e, startTime,
                                              basePitch);
                break;
            default:
                throw Exception("Got a relation type we don't recognize.");
                break;
            }
            possibleRelations.insert(relation);
        }
    }

    return possibleRelations;
}

/*** FigurationSourceMap ***/
  
/// Get any figuration objects from segment s into figs
/// @author Tom Breton (Tehom)
FigurationVector
FigurationSourceMap::
getFigurations(Segment *s)
{
    RG_DEBUG << "Looking for figuration in segment " << s->getLabel()
             << endl;
    Q_CHECK_PTR(s);

    FigurationVector figs;
    FigChord  *parameterChord = 0;
    for (Segment::iterator i = s->begin();
         i != s->end();
         ++i) {

        if ((*i)->isa(Indication::EventType)) {
            Indication indication(**i);
            const std::string type = indication.getIndicationType();

            // We found an indication of a parameter chord.  Now look
            // for the chord itself.
            if (type == Indication::FigParameterChord)
                {
                    // The chord here will parameterize all
                    // figurations until we see another parameter
                    // chord..
                    RG_DEBUG << "Got a parameter chord"
                             << endl;
                    // Remove any old one.
                    if (parameterChord) { delete parameterChord; }
                    parameterChord = new FigChord(*s, i);
                } else if (type == Indication::Figuration) {

                // There is a figuration here.  It's relative to
                // the preceding parameter chord.
                RG_DEBUG << "Got a figuration"
                         << endl;
            
                // If there's no parameter chord yet, this is a
                // proximity-note figuration
                bool parameterized =
                    parameterChord && !parameterChord->empty();

                unsigned int NbParameters =
                    parameterized ? parameterChord->size() : 0;

                /*** Collect all events that start during it ***/

                UnsolvedFiguration  notesToAccountFor;
                
                // Get its bounding times
                timeT figDuration  = (*i)->getDuration();
                timeT figStartTime = (*i)->getAbsoluteTime();
                timeT figEndTime   = figStartTime + figDuration;

                TimeSignature timeSignature = 
                    s->getComposition()->getTimeSignatureAt(figStartTime);

                // If the indication takes zero time we wouldn't
                // collect anything because we don't collect events
                // that start exactly at the end.
                if (figDuration == 0) { continue; }

                BaseRelativeEventAdder *eventAdder =
                    parameterized ?
                    dynamic_cast<BaseRelativeEventAdder *>
                    (new ParamaterizedRelativeEventAdder(figStartTime,
                                                        s, parameterChord)) :
                    dynamic_cast<BaseRelativeEventAdder *>
                    (new UnparamaterizedRelativeEventAdder(figStartTime));
                
                // We allow any events that start within the interval,
                // but not exactly at the end of it.  So we don't
                // re-use iterator i, because it might already be past
                // relevant events.  And since we allow events whose
                // durations extend past the end of the indication, we
                // don't check for duration.
                Segment::iterator startOfFig = s->findTime(figStartTime);
                Segment::iterator endOfFig   = s->findTime(figEndTime);

                for (Segment::iterator j = startOfFig;
                     j != endOfFig;
                     ++j) {
                    Event *e = (*j);
                    notesToAccountFor.insert(eventAdder->add(e));
                }

                delete eventAdder;

                // We want to not inspect any of these events again,
                // so arrange to continue the big loop from the next
                // event.
                i = endOfFig;

                RelativeEventVec    events;
                for (UnsolvedFiguration::iterator j =
                         notesToAccountFor.begin();
                     j != notesToAccountFor.end();
                     ++j) {
                    // NB, we are emptying hs and deleting all its
                    // members except the one relation that we keep.
                    UnsolvedNote hs = (*j);
                    RelativeEvent * bestRelation = 0;
                    int bestScore = -1000000;
                    for (UnsolvedNote::iterator k = hs.begin();
                         k != hs.end();
                         ++k) {
                        RelativeEvent * relation = *k;
                        int score = relation->getScore();
                        if (score > bestScore) {
                            // Delete any previous bestRelation.
                            if (bestRelation) { delete bestRelation; }
                            bestRelation = relation;
                            bestScore = score;
                        } else
                            { delete relation; }
                    }
                    if (bestRelation) {
                        RG_DEBUG << "Found a preferred relation"
                                 << endl;
                        events.push_back(bestRelation);
                    }
                }
                Figuration figurationAll =
                    { events, figDuration, NbParameters,
                      timeSignature.getNumerator(),
                      timeSignature.getDenominator(),
                    };
                figs.push_back(figurationAll);
            }
        }
    }

    if (parameterChord)
        { delete parameterChord; }
    return figs;
}

Figuration *
FigurationSourceMap::
findMatch(FigurationVector& figVector,
          int timeSigNumerator,
          int timeSigDenominator,
          unsigned int NbParameters)
{
    for (FigurationVector::iterator iterFig = figVector.begin();
         iterFig != figVector.end();
         ++iterFig)
    {
        Figuration& fig = *iterFig;
        // If we find a match, use it and end the loop.
        if ((fig.m_timesigNumerator == timeSigNumerator) &&
            (fig.m_timesigDenominator == timeSigDenominator) &&
            ((fig.m_NbParameters == NbParameters) || fig.m_NbParameters == 0)) {
            return &fig;
        }
    }
    // If we never found a match, signal that with NULL
    return 0;
}

}
