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


#include "SegmentSplitByDrumCommand.h"

#include "base/BaseProperties.h"
#include "misc/AppendLabel.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
//#include "base/NotationQuantizer.h"
#include "base/Segment.h"
#include "base/TimeT.h"
#include "base/MidiProgram.h"
#include "base/Profiler.h"

#include <QtGlobal>

#include <vector>
#include <algorithm>


namespace Rosegarden
{


SegmentSplitByDrumCommand::SegmentSplitByDrumCommand(Segment *segment, const MidiKeyMapping *keyMap) :
        NamedCommand(tr("Split by Drum")),
        m_composition(segment->getComposition()),
        m_segment(segment),
        m_keyMap(keyMap),
        m_executed(false)
{
}

SegmentSplitByDrumCommand::~SegmentSplitByDrumCommand()
{
    if (m_executed) {
        delete m_segment;
    } else {
        for (SegmentVector::iterator i = m_newSegments.begin(); i != m_newSegments.end(); ++i) {
            delete (*i);
        }
    }
}

void
SegmentSplitByDrumCommand::execute()
{
    Profiler profiler("SplitByDrumCommand::execute", true);
    if (!m_newSegments.size()) {

        Segment *s = nullptr;

        // get the total number of colors in the map; only do this once
        int maxColors = m_composition->getSegmentColourMap().colours.size();

        // get the color index for the template segment, so we can make the new
        // segments have contrasting colors for layer indication purposes
        int colorIndex = m_segment->getColourIndex();

        PitchList pitchesUsed;

        // collect the pitches used by this segment into a vector
        // (iterate through eg. 3,000 events 1 time)
        for (Segment::iterator i = m_segment->begin();
                m_segment->isBeforeEndMarker(i); ++i) {

            // only interested in notes here
            if ((*i)->isa(Note::EventType)) {
                if ((*i)->has(BaseProperties::PITCH)) pitchesUsed.push_back((*i)->get<Int>(BaseProperties::PITCH));
            }
        }

        // sort the vector leaving just one of each
        std::sort(pitchesUsed.begin(), pitchesUsed.end());
        pitchesUsed.erase(std::unique(pitchesUsed.begin(), pitchesUsed.end()), pitchesUsed.end());


        // iterate through eg. 3,000 events n times, for a total of n + 1
        // instead of 128.  n + 1 will always be dramatically smaller than 128;
        // typically on the order of 20.
        for (PitchList::const_iterator index = pitchesUsed.begin(); index != pitchesUsed.end(); ++index) {

            // iterate through our source segment pitch by pitch, creating a new
            // destination segment to contain all notes of each pitch
            // encountered
            for (Segment::iterator i = m_segment->begin();
                    m_segment->isBeforeEndMarker(i); ++i) {

//                if ((*i)->isa(Note::EventRestType)) continue;
//                if ((*i)->isa(Indication::EventType)) continue;
//                if ((*i)->isa(Clef::EventType) continue;
//                if ((*i)->isa(Key::EventType) continue;

                
                // handle Note::EventType; implicitly ignore everything else at
                // this stage (future behavioral requirements to be determined
                // after field testing)
                if ((*i)->isa(Note::EventType)) {

                    if ((*i)->has(BaseProperties::PITCH) && (*i)->get<Int>(BaseProperties::PITCH) == (*index)) {

                        // we have an event of this pitch to put into a segment;
                        // do we have already have a segment?
                        if (!s) {

                            // no segment, so create one, and set it up with an
                            // initial drum clef, then increment the color index
                            // so the layer wheel and the ruler will help
                            // indicate which layer segment is active
                            s = new Segment;
                            timeT start = m_segment->getStartTime();
                            Event *clefEvent = Clef(Clef::TwoBar).getAsEvent(start);

                            s->setTrack(m_segment->getTrack());
                            s->setStartTime(start);
                            s->insert(clefEvent);

                            // ensure segment color contrast
                            colorIndex += 5;

                            // if we went past the end of the color map, just roll back to 0
                            if (colorIndex > maxColors) colorIndex = 0;
                            s->setColourIndex(colorIndex);

                            m_newSegments.push_back(s);
                        }

                        // insert event into new segment (we'll normalize rests
                        // later)
                        s->insert(new Event(**i));
                    }

                } else {

                    //TODO what do we do with everything else?  there could be
                    // controllers or program changes or other things, and even
                    // internal events like text dynamics...  it seems like it
                    // may be necessary to grab that stuff and store it in an
                    // additional conductor segment or something...  dunno yet

                }

            } // end segment iterator loop
        
            // end of the loop for this pitch, so if we created a segment,
            // normalize the rests, add it to the composition, then set s
            // back to 0 for the next pass
            if (s) {
                s->normalizeRests(m_segment->getStartTime(), m_segment->getEndMarkerTime());
                m_composition->addSegment(s);

                // use label from percussion key map if available, else use
                // "pitch n" for label
                QString fsckIt = tr("Pitch %1").arg(*index);
                std::string label = (m_keyMap ? m_keyMap->getMapForKeyName(*index) : fsckIt.toStdString()) + " (" + m_segment->getLabel() + ")";
                s->setLabel(label);

                s = nullptr;

            } // end segment loop

        } // end pitch loop

    } else {
        // to make redo work, we have to re-insert all of these
        for (SegmentVector::iterator i = m_newSegments.begin(); i != m_newSegments.end(); ++i) {
            m_composition->addSegment(*i);
        }
    }

    // whether we do or redo we want to detach the original segment
    m_composition->detachSegment(m_segment);
    m_executed = true;
}

void
SegmentSplitByDrumCommand::unexecute()
{
    m_composition->addSegment(m_segment);

    for (SegmentVector::iterator i = m_newSegments.begin(); i != m_newSegments.end(); ++i) {
        m_composition->detachSegment(*i);
    }
    m_executed = false;
}


} // namespace Rosegarden
