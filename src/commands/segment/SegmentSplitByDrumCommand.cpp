/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
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
    if (!m_newSegments.size()) {

        Segment *s = 0;

        // get the total number of colors in the map; only do this once
        int maxColors = m_composition->getSegmentColourMap().size();

        // get the color index for the template segment, so we can make the new
        // segments have contrasting colors for layer indication purposes
        int colorIndex = m_segment->getColourIndex();

        // iterate through each pitch from 0 to 127, hunting for notes
        for (MidiByte pitch = 0; pitch <= 127; ++pitch) {

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

                    if ((*i)->has(BaseProperties::PITCH) && (*i)->get<Int>(BaseProperties::PITCH) == pitch) {

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
                QString fsckIt = QString("Pitch %1").arg(pitch); // can't remember how to do this in STL
                std::string label = (m_keyMap ? m_keyMap->getMapForKeyName(pitch) : fsckIt.toStdString()) + " (" + m_segment->getLabel() + ")";
                s->setLabel(label);

                s = 0;

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
