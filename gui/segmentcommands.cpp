/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include "segmentcommands.h"
#include "rosedebug.h"
#include "NotationTypes.h"

using Rosegarden::Composition;
using Rosegarden::Segment;
using Rosegarden::Event;
using Rosegarden::timeT;
using Rosegarden::TrackId;


// --------- Erase Segment --------
//
SegmentEraseCommand::SegmentEraseCommand(Segment *segment) :
    SegmentCommand("Erase Segment"),
    m_composition(segment->getComposition()),
    m_segment(segment)
{
    // nothing else
}

SegmentEraseCommand::~SegmentEraseCommand()
{
    // This is the only place the Segment can safely be deleted, and
    // then only if it is not in the Composition (i.e. if we executed
    // more recently than we unexecuted)

    if (!m_segment->getComposition()) {
	delete m_segment;
    }
}


void
SegmentEraseCommand::getSegments(SegmentSet &segments)
{
    segments.insert(m_segment);
}


void
SegmentEraseCommand::execute()
{
    m_composition->detachSegment(m_segment);
}

void
SegmentEraseCommand::unexecute()
{
    m_composition->addSegment(m_segment);
}


// --------- Insert Segment --------
//
SegmentInsertCommand::SegmentInsertCommand(Composition *c,
                                           TrackId track,
                                           timeT startTime,
                                           timeT duration):
    SegmentCommand("Create Segment"),
    m_composition(c),
    m_segment(0),
    m_track(track),
    m_startTime(startTime),
    m_duration(duration)
{
}

SegmentInsertCommand::~SegmentInsertCommand()
{
    if (!m_segment->getComposition()) {
	delete m_segment;
    }
}


void
SegmentInsertCommand::getSegments(std::set<Segment *> &segments)
{
    segments.insert(m_segment);
}


void
SegmentInsertCommand::execute()
{
    if (!m_segment)
    {
        // Create and insert Segment
        //
        m_segment = new Segment();
        m_segment->setTrack(m_track);
        m_segment->setStartTime(m_startTime);
	m_composition->addSegment(m_segment);
        m_segment->setDuration(m_duration);
    }
    else
    {
        m_composition->addSegment(m_segment);
    }
    
}

void
SegmentInsertCommand::unexecute()
{
    m_composition->detachSegment(m_segment);
}

// --------- Record Segment --------
//

SegmentRecordCommand::SegmentRecordCommand(Segment *s) :
    SegmentCommand("Record"),
    m_composition(s->getComposition()),
    m_segment(s)
{
}

SegmentRecordCommand::~SegmentRecordCommand()
{
    if (!m_segment->getComposition()) {
	delete m_segment;
    }
}

void
SegmentRecordCommand::execute()
{
    if (!m_segment->getComposition()) {
	m_composition->addSegment(m_segment);
    }
}

void
SegmentRecordCommand::unexecute()
{
    m_composition->detachSegment(m_segment);
}

void
SegmentRecordCommand::getSegments(SegmentSet &segments)
{
    segments.insert(m_segment);
}


// --------- Reconfigure Segments --------
//

SegmentReconfigureCommand::SegmentReconfigureCommand(QString name) :
    SegmentCommand(name)
{
}

SegmentReconfigureCommand::~SegmentReconfigureCommand()
{
}

void
SegmentReconfigureCommand::getSegments(SegmentSet &segments)
{
    for (SegmentRecSet::iterator i = m_records.begin();
	 i != m_records.end(); ++i) {

	if (i->segment->getDuration()  != i->duration ||
	    i->segment->getStartTime() != i->startTime ||
	    i->segment->getTrack()     != i->track) {

	    segments.insert(i->segment);
	}
    }
}

void
SegmentReconfigureCommand::addSegment(Segment *segment,
				      timeT startTime,
				      timeT duration,
				      TrackId track)
{
    SegmentRec record;
    record.segment = segment;
    record.startTime = startTime;
    record.duration = duration;
    record.track = track;
    m_records.push_back(record);
}

void
SegmentReconfigureCommand::execute()
{
    swap();
}

void
SegmentReconfigureCommand::unexecute()
{
    swap();
}

void
SegmentReconfigureCommand::swap()
{
    //!!! NB this won't actually _work_ for changes to duration
    // that make segments shorter, for the simple reason that
    // Segment::setDuration is not yet implemented for this case!

    for (SegmentRecSet::iterator i = m_records.begin();
	 i != m_records.end(); ++i) {

	// set the segment's values from the record, but set the
	// previous values back in to the record for use in the
	// next iteration of the execute/unexecute cycle

	timeT currentDuration = i->segment->getDuration();
	timeT currentStartTime = i->segment->getStartTime();
	TrackId currentTrack = i->segment->getTrack();

	if (currentDuration != i->duration) {
	    i->segment->setDuration(i->duration);
	    i->duration = currentDuration;
	}

	if (currentStartTime != i->startTime || currentTrack != i->track) {
	    i->segment->getComposition()->setSegmentStartTimeAndTrack
		(i->segment, i->startTime, i->track);
	    i->startTime = currentStartTime;
	    i->track = currentTrack;
	}
    }
}


// --------- Split Segment --------
//

SegmentSplitCommand::SegmentSplitCommand(Segment *segment,
					 timeT splitTime) :
    SegmentCommand("Split Segment"),
    m_segment(segment),
    m_newSegment(0),
    m_splitTime(splitTime)
{
}

SegmentSplitCommand::~SegmentSplitCommand()
{
    if (m_newSegment && !m_newSegment->getComposition()) {
	delete m_newSegment;
    }
}

void
SegmentSplitCommand::getSegments(SegmentSet &segments)
{
    segments.insert(m_segment);
    if (m_newSegment) segments.insert(m_newSegment);
}

void
SegmentSplitCommand::execute()
{
    if (!m_newSegment) {

	m_newSegment = new Segment;
	m_newSegment->setTrack(m_segment->getTrack());
	m_newSegment->setStartTime(m_splitTime);
	m_segment->getComposition()->addSegment(m_newSegment);

	Event *clefEvent = 0;
	Event *keyEvent = 0;

	// Copy the last occurence of clef and key
	// from the left hand side of the split (nb. timesig events
	// don't appear in segments, only in composition)
	//
	Segment::iterator it = m_segment->findTime(m_splitTime);

	while (it != m_segment->begin()) {

	    --it;

	    if (!clefEvent && (*it)->isa(Rosegarden::Clef::EventType)) {
		clefEvent = new Event(**it, m_splitTime);
	    }

	    if (!keyEvent && (*it)->isa(Rosegarden::Key::EventType)) {
		keyEvent = new Event(**it, m_splitTime);
	    }

	    if (clefEvent && keyEvent) break;
	}

	// Insert relevant meta info if we've found some
	//
	if (clefEvent)
	    m_newSegment->insert(clefEvent);

	if (keyEvent)
	    m_newSegment->insert(keyEvent);

	// Copy through the Events
	//
	it = m_segment->findTime(m_splitTime);

	if (it != m_segment->end() && (*it)->getAbsoluteTime() > m_splitTime) {
	    m_newSegment->fillWithRests((*it)->getAbsoluteTime());
	}

	while (it != m_segment->end()) {
	    m_newSegment->insert(new Event(**it));
	    ++it;
	}

	m_newSegment->setDuration(m_segment->getEndTime() - m_splitTime);
    }

    // Resize left hand Segment
    //
    //!!! should we be dividing any events that are extant during the
    // split?
    m_segment->erase(m_segment->findTime(m_splitTime), m_segment->end());
    m_segment->setDuration(m_splitTime - m_segment->getStartTime());

    // Look for a final rest and shrink it
    Segment::iterator it = m_segment->end();

    if (it != m_segment->begin() &&
	(*(--it))->isa(Rosegarden::Note::EventRestType)) {
	Event *e = new Event(**it, (*it)->getAbsoluteTime(),
			     m_splitTime - (*it)->getAbsoluteTime());
	m_segment->erase(it);
	m_segment->insert(e);
    }

    if (!m_newSegment->getComposition()) {
	m_segment->getComposition()->addSegment(m_newSegment);
    }
}

void
SegmentSplitCommand::unexecute()
{
    if (m_segment->getEndTime() < m_newSegment->getFirstEventTime()) {
	m_segment->fillWithRests(m_newSegment->getFirstEventTime());
    }

    for (Segment::iterator it = m_newSegment->begin();
	 it != m_newSegment->end(); ++it) {
	m_segment->insert(new Event(**it));
    }

    m_segment->getComposition()->detachSegment(m_newSegment);
}




SegmentChangeQuantizationCommand::SegmentChangeQuantizationCommand(Rosegarden::StandardQuantization *sq) :
    SegmentCommand(name(sq)),
    m_quantization(sq)
{
    // nothing
}

SegmentChangeQuantizationCommand::~SegmentChangeQuantizationCommand()
{
    // nothing
}

void
SegmentChangeQuantizationCommand::execute()
{
    for (unsigned int i = 0; i < m_records.size(); ++i) {

	SegmentRec &rec = m_records[i];

	if (m_quantization) {

	    rec.oldQuantizer =
		new Rosegarden::Quantizer(rec.segment->getQuantizer());
	    rec.segment->setQuantizeLevel(*m_quantization);

	    rec.wasQuantized = rec.segment->hasQuantization();
	    rec.segment->setQuantization(true);

	} else {

	    rec.wasQuantized = rec.segment->hasQuantization();
	    rec.segment->setQuantization(false);
	}
    }
}

void
SegmentChangeQuantizationCommand::unexecute()
{
    for (unsigned int i = 0; i < m_records.size(); ++i) {

	SegmentRec &rec = m_records[i];

	if (m_quantization) {

	    if (!rec.wasQuantized) rec.segment->setQuantization(false);

	    rec.segment->setQuantizeLevel(*rec.oldQuantizer);
	    delete rec.oldQuantizer;
	    rec.oldQuantizer = 0;

	} else {

	    if (rec.wasQuantized) rec.segment->setQuantization(true);
	}
    }
}

void
SegmentChangeQuantizationCommand::addSegment(Rosegarden::Segment *s)
{
    SegmentRec rec;
    rec.segment = s;
    rec.oldQuantizer = 0;
    rec.wasQuantized = false; // shouldn't matter what we initialise this to
    m_records.push_back(rec);
}
    
void
SegmentChangeQuantizationCommand::getSegments(SegmentSet &segments)
{
    for (unsigned int i = 0; i < m_records.size(); ++i) {
	segments.insert(m_records[i].segment);
    }
}

QString
SegmentChangeQuantizationCommand::name(Rosegarden::StandardQuantization *sq)
{
    if (!sq) {
	return "Unquantize";
    } else {
	return QString("Quantize to ") + sq->name.c_str();
    }
}



// --------- Add Time Signature --------
// 

AddTimeSignatureCommand::~AddTimeSignatureCommand()
{
    if (m_oldTimeSignature) delete m_oldTimeSignature;
}

void
AddTimeSignatureCommand::execute()
{
    int oldIndex = m_composition->getTimeSignatureNumberAt(m_time);
    if (oldIndex >= 0) {
	std::pair<timeT, Rosegarden::TimeSignature> data =
	    m_composition->getTimeSignatureChange(oldIndex);
	if (data.first == m_time) {
	    m_oldTimeSignature = new Rosegarden::TimeSignature(data.second);
	}
    }

    m_timeSigIndex = m_composition->addTimeSignature(m_time, m_timeSignature);
}

void
AddTimeSignatureCommand::unexecute()
{
    m_composition->removeTimeSignature(m_timeSigIndex);
    if (m_oldTimeSignature) {
	m_composition->addTimeSignature(m_time, *m_oldTimeSignature);
    }
}


void
AddTempoChangeCommand::execute()
{
    m_composition->addTempo(m_time, m_tempo);
}

void
AddTempoChangeCommand::unexecute()
{
    int num = m_composition->getTempoChangeNumberAt(m_time);
    m_composition->removeTempoChange(num);
}
