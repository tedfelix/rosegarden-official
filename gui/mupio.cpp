/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <iostream>
#include <fstream>
#include <string>

#include <kapp.h>

#include "mupio.h"

#include "Composition.h"
#include "BaseProperties.h"
#include "Sets.h"
#include "SegmentNotationHelper.h"
#include "NotationTypes.h"
#include "Quantizer.h"

using Rosegarden::Composition;
using Rosegarden::Segment;
using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::BaseProperties;
using Rosegarden::timeT;


MupExporter::MupExporter(QObject *parent,
			 Composition *composition,
			 std::string fileName) :
    ProgressReporter(parent, "mupExporter"),
    m_composition(composition),
    m_fileName(fileName)
{
    // nothing else
}

MupExporter::~MupExporter()
{
    // nothing
}

bool
MupExporter::write()
{
    Composition *c = m_composition;

    std::ofstream str(m_fileName.c_str(), std::ios::out);
    if (!str) {
        std::cerr << "MupExporter::write() - can't write file " << m_fileName
		  << std::endl;
        return false;
    }

    str << "score\n";
    str << "\tstaffs=" << c->getNbTracks() << "\n";
    
    int ts = c->getTimeSignatureCount();
    std::pair<timeT, Rosegarden::TimeSignature> tspair;
    if (ts > 0) tspair = c->getTimeSignatureChange(0);
    str << "\ttime="
	<< tspair.second.getNumerator() << "/"
	<< tspair.second.getDenominator() << "\n";

    for (int barNo = -1; barNo < c->getNbBars(); ++barNo) {

	for (Rosegarden::TrackId trackNo = 0; trackNo < c->getNbTracks(); ++trackNo) {

	    if (barNo < 0) {
		writeClefAndKey(str, trackNo);
		continue;
	    }

	    if (barNo == 0 && trackNo == 0) {
		str << "\nmusic\n";
	    }

	    str << "\t" << trackNo+1 << ":";

	    Segment *s = 0;
	    timeT barStart = c->getBarStart(barNo);
	    timeT barEnd = c->getBarEnd(barNo);
	    
	    for (Composition::iterator ci = c->begin(); ci != c->end(); ++ci) {
		if ((*ci)->getTrack() == trackNo &&
		    (*ci)->getStartTime() < barEnd &&
		    (*ci)->getEndMarkerTime() > barStart) {
		    s = *ci;
		    break;
		}
	    }

	    Rosegarden::TimeSignature timeSig(c->getTimeSignatureAt(barStart));

	    if (!s) {
		// write empty bar
		writeInventedRests(str, timeSig, 0, barEnd - barStart);
		continue;
	    }
	    
	    if (s->getStartTime() > barStart) {
		writeInventedRests(str, timeSig,
				   0, s->getStartTime() - barStart);
	    }
	    
	    for (Segment::iterator si = s->findTime(barStart);
		 s->isBeforeEndMarker(si) &&
		     (*si)->getNotationAbsoluteTime() < barEnd; ++si) {

		if ((*si)->isa(Note::EventType)) {

		    str << " ";

		    Rosegarden::Chord chord(*s, si, c->getNotationQuantizer());
		    writeDuration(str, (*chord.getInitialNote())->
				                   getNotationDuration());

		    for (Rosegarden::Chord::iterator chi = chord.begin();
			 chi != chord.end(); ++chi) {
			writePitch(str, trackNo, **chi);
		    }

		    str << ";";

		    si = chord.getFinalElement();

		} else if ((*si)->isa(Note::EventRestType)) {

		    str << " ";
		    writeDuration(str, (*si)->getNotationDuration());
		    str << "r;";

		} // ignore all other sorts of events for now

	    }

	    if (s->getEndMarkerTime() < barEnd) {
		writeInventedRests(str, timeSig,
				   s->getEndMarkerTime() - barStart,
				   barEnd - s->getEndMarkerTime());
	    }

	    str << "\n";
	}

	if (barNo >= 0) str << "bar" << std::endl;
    }

    str << "\n" << std::endl;
    str.close();
    return true;
}


void
MupExporter::writeClefAndKey(std::ofstream &str, Rosegarden::TrackId trackNo)
{
    Composition *c = m_composition;

    for (Composition::iterator i = c->begin(); i != c->end(); ++i) {
	if ((*i)->getTrack() == trackNo) {

	    Rosegarden::Clef clef;
	    Rosegarden::Key key;
	    (Rosegarden::SegmentNotationHelper(**i)).
		getClefAndKeyAt((*i)->getStartTime(), clef, key);

	    str << "staff " << trackNo+1 << "\n";

	    if (clef.getClefType() == Rosegarden::Clef::Treble) {
		str << "\tclef=treble\n";
	    } else if (clef.getClefType() == Rosegarden::Clef::Alto) {
		str << "\tclef=alto\n";
	    } else if (clef.getClefType() == Rosegarden::Clef::Tenor) {
		str << "\tclef=tenor\n";
	    } else if (clef.getClefType() == Rosegarden::Clef::Bass) {
		str << "\tclef=bass\n";
	    }

	    str << "\tkey=" << key.getAccidentalCount()
		<< (key.isSharp() ? "#" : "&")
		<< (key.isMinor() ? "minor" : "major") << std::endl;

	    m_clefKeyMap[trackNo] = ClefKeyPair(clef, key);

	    return;
	}
    }
}

void
MupExporter::writeInventedRests(std::ofstream &str,
				Rosegarden::TimeSignature &timeSig,
				timeT offset,
				timeT duration)
{
    str << " ";
    Rosegarden::DurationList dlist;
    timeSig.getDurationListForInterval(dlist, duration, offset);
    for (Rosegarden::DurationList::iterator i = dlist.begin();
	 i != dlist.end(); ++i) {
	writeDuration(str, *i);
	str << "r;";
    }
}

void
MupExporter::writePitch(std::ofstream &str, Rosegarden::TrackId trackNo,
			Rosegarden::Event *event)
{
    long pitch = 0;
    if (!event->get<Int>(BaseProperties::PITCH, pitch)) {
	str << "c"; // have to write something, or it won't parse
	return;
    }

    Rosegarden::Accidental accidental = Rosegarden::Accidentals::NoAccidental;
    (void)event->get<String>(BaseProperties::ACCIDENTAL, accidental);

    // mup octave: treble clef is in octave 4?

    ClefKeyPair ck;
    ClefKeyMap::iterator ckmi = m_clefKeyMap.find(trackNo);
    if (ckmi != m_clefKeyMap.end()) ck = ckmi->second;
/*!!!
    Rosegarden::NotationDisplayPitch ndp(pitch, ck.first, ck.second, accidental);
    int placeInScale, accidentals, octave;
    ndp.getInScale(ck.first, ck.second, placeInScale, accidentals, octave);

    str << "cdefgab"[placeInScale];
    
    switch (accidentals) {
    case -2: str << "&&"; break;
    case -1: str << "&";  break;
    case  1: str << "#";  break;
    case  2: str << "x";  break;
    }

    str << octave + 1;
*/

    Rosegarden::Pitch p(pitch, accidental);
    Rosegarden::Accidental acc(p.getAccidental(ck.second.isSharp()));
    int noteInScale(p.getNoteInScale(ck.second));
    int octave(p.getOctave());

    str << "cdefgab"[noteInScale];
    
    if (accidental == Rosegarden::Accidentals::DoubleFlat) str << "&&";
    else if (accidental == Rosegarden::Accidentals::Flat) str << "&";
    else if (accidental == Rosegarden::Accidentals::Sharp) str << "#";
    else if (accidental == Rosegarden::Accidentals::DoubleSharp) str << "##";

    str << octave + 1;
}

void
MupExporter::writeDuration(std::ofstream &str, Rosegarden::timeT duration)
{
    Note note(Note::getNearestNote(duration, 2));
    int n = Note::Semibreve - note.getNoteType();
    if (n < 0) str << "1/" << (1 << (-n));
    else str << (1 << n);
    for (int d = 0; d < note.getDots(); ++d) str << ".";
}

