/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AddLayerCommand.h"

#include "base/Segment.h"
#include "base/Composition.h"
//#include "base/Studio.h"
#include "base/Track.h"
//#include "base/Colour.h"
//#include "base/ColourMap.h"
//#include "document/RosegardenDocument.h"


#include <QApplication>

namespace Rosegarden
{

AddLayerCommand::AddLayerCommand(Segment *segment, Composition &composition):
        NamedCommand(tr("Add Layer")),
        m_segment(segment),
        m_composition(composition),
        m_detached(false)
{}

AddLayerCommand::~AddLayerCommand()
{
    if (m_detached) {
        delete m_segment;
    }
}

Segment *
AddLayerCommand::getSegment() const
{
    return m_segment;
}

void
AddLayerCommand::execute()
{ 
    if (!m_segment) return;

    Segment *layer = new Segment();

    layer->setTrack(m_segment->getTrack());
    layer->setStartTime(m_segment->getStartTime());
    m_composition.addSegment(layer);
    layer->setEndTime(m_segment->getEndTime());

    std::string label = m_segment->getLabel();
    label += tr(" - layer").toStdString();
    layer->setLabel(label);

    layer->setHighestPlayable(m_segment->getHighestPlayable());
    layer->setLowestPlayable(m_segment->getLowestPlayable());
    layer->setTranspose(m_segment->getTranspose());

    // segments have an initial clef, not put in by segment_insert_command
    // apparently (because that's what this file used to be a copy of); must
    // have done it at times when that command was invoked...  need to pick that
    // code up, create a clef, make it invisible, and insert it into the
    // segment, either here or out there somewhere in calling code
    
    // can we iterate through the segment and pre-set all the rests to be
    // invisible, or do they not exist until the notation editor calculates
    // them?  maybe this has to be done externally to this command, and hey ho,
    // maye we can pre-set the micro-position hoopty on the damn things to avoid
    // having to do the rest height jiggling in NPF!!! (that would be crude, and
    // incapable of using multiple levels for layer 1 2 3 4, but I've
    // practically never used more than two overlapping segments, and it's worth
    // trying to deliver a practical amount of progress here and now)

    // get the total number of colors in the map
    int maxColors = m_composition.getSegmentColourMap().size();

    // get the color index for the segment used as the template for the new
    // empty one we're creating
    int index = m_segment->getColourIndex();

    // with the default color map (the only one I care about anymore) a
    // difference of +5 guarantees enough contrast to make the segment changer
    // widget, raw note ruler, and track headers show enough contrast to be
    // useful as an indication that this segment is not the same as the one it
    // is patterned after
    index += 5;

    // if we went past the end of the color map, just roll back to 0, because
    // this will happen so infrequently in use it's not worth a lot of fancy
    // handling, and anyway 0 will be a contrast from what's sitting on the end
    // of the standard color map, so it will still be functional
    if (index > maxColors) index = 0;
    layer->setColourIndex(index);
    
    // now what other gotchas are there?

    // now m_segment goes from being the input template to what we'll return if
    // asked
    m_segment = layer;
    m_detached = false;
}

void
AddLayerCommand::unexecute()
{
    m_composition.detachSegment(m_segment);
    m_detached = true;
}

}
