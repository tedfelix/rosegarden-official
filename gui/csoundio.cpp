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

#include "csoundio.h"

#include "Composition.h"
#include "BaseProperties.h"

#include <iostream>
#include <fstream>
#include <string>

using Rosegarden::Composition;
using Rosegarden::Segment;
using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::BaseProperties;
using Rosegarden::timeT;


CsoundExporter::CsoundExporter(Composition *composition,
			       std::string fileName) :
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
    std::ofstream str(m_fileName.c_str(), ios::out);
    if (!str) {
        std::cerr << "CsoundExporter::write() - can't write file" << endl;
        return false;
    }

    int instrument = 1;

    for (Composition::iterator i = m_composition->begin();
	 i != m_composition->end(); ++i) {

	for (Segment::iterator j = (*i)->begin(); j != (*i)->end(); ++j) {

	    if ((*j)->isa(Note::EventType)) {
		
		long pitch = 0;
		(*j)->get<Int>(BaseProperties::PITCH, pitch);

		long velocity = 127;
		(*j)->get<Int>(BaseProperties::VELOCITY, velocity);

		str << "i" << instrument << "\t"
		    << convertTime((*j)->getAbsoluteTime()) << "\t"
		    << convertTime((*j)->getDuration()) << "\t"
		    << 3 + (pitch / 12) << ((pitch % 12) < 10 ? ".0" : ".")
		    << pitch % 12 << "\t"
		    << velocity << "\t\n";
	    }
	}

	++instrument;
    }

    int tempoCount = m_composition->getTempoChangeCount();

    if (tempoCount > 0) {

	str << "t ";

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
	    << endl;
    }

    str << "e" << endl;
    str.close();
    return true;
}


