/*
    Rosegarden-4
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

#include <iostream>
#include <fstream>
#include <string>

#include <kapp.h>

#include "csoundio.h"

#include "Composition.h"
#include "BaseProperties.h"

using Rosegarden::Composition;
using Rosegarden::Segment;
using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::BaseProperties;
using Rosegarden::timeT;


CsoundExporter::CsoundExporter(QObject *parent,
                               Composition *composition,
			       std::string fileName) :
    ProgressReporter(parent, "csoundExporter"),
    m_composition(composition),
    m_fileName(fileName)
{
    // nothing else
}

CsoundExporter::~CsoundExporter()
{
    // nothing
}

static double
convertTime(Rosegarden::timeT t)
{
    return double(t) / double(Note(Note::Crotchet).getDuration());
}

bool
CsoundExporter::write()
{
    std::ofstream str(m_fileName.c_str(), std::ios::out);
    if (!str) {
        std::cerr << "CsoundExporter::write() - can't write file" << std::endl;
        return false;
    }

    str << ";; Csound score file written by Rosegarden-4\n\n";
    if (m_composition->getCopyrightNote() != "") {
	str << ";; Copyright note:\n;; "
//!!! really need to remove newlines from copyright note
	    << m_composition->getCopyrightNote() << "\n";
    }

    int trackNo = 0;
    for (Composition::iterator i = m_composition->begin();
	 i != m_composition->end(); ++i) {

        emit setProgress(int(double(trackNo++)/double(m_composition->getNbTracks()) * 100.0));
        kapp->processEvents(50);

	str << "\n;; Segment: \"" << (*i)->getLabel() << "\"\n";
	str << ";; on Track: \""
	    << m_composition->getTrackById((*i)->getTrack())->getLabel()
	    << "\"\n";
	str << ";;\n;; Inst\tTime\tDur\tPitch\tVely\n"
	    << ";; ----\t----\t---\t-----\t----\n";

	for (Segment::iterator j = (*i)->begin(); j != (*i)->end(); ++j) {

	    if ((*j)->isa(Note::EventType)) {
		
		long pitch = 0;
		(*j)->get<Int>(BaseProperties::PITCH, pitch);

		long velocity = 127;
		(*j)->get<Int>(BaseProperties::VELOCITY, velocity);

		str << "   i"
		    << (*i)->getTrack() << "\t"
		    << convertTime((*j)->getAbsoluteTime()) << "\t"
		    << convertTime((*j)->getDuration()) << "\t"
		    << 3 + (pitch / 12) << ((pitch % 12) < 10 ? ".0" : ".")
		    << pitch % 12 << "\t"
		    << velocity << "\t\n";

	    } else {
		str << ";; Event type: " << (*j)->getType() << std::endl;
	    }
	}
    }

    int tempoCount = m_composition->getTempoChangeCount();

    if (tempoCount > 0) {

	str << "\nt ";

	for (int i = 0; i < tempoCount - 1; ++i) {

	    std::pair<Rosegarden::timeT, long> tempoChange = 
		m_composition->getRawTempoChange(i);

	    timeT myTime = tempoChange.first;
	    timeT nextTime = myTime;
	    if (i < m_composition->getTempoChangeCount()-1) {
		nextTime = m_composition->getRawTempoChange(i+1).first;
	    }
	    
	    int tempo = tempoChange.second / 60;

	    str << convertTime(  myTime) << " " << tempo << " "
		<< convertTime(nextTime) << " " << tempo << " ";
	}

	str << convertTime(m_composition->getRawTempoChange(tempoCount-1).first)
	    << " "
	    << m_composition->getRawTempoChange(tempoCount-1).second/60
	    << std::endl;
    }

    str << "\ne" << std::endl;
    str.close();
    return true;
}


