// -*- c-basic-offset: 4 -*-

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

#ifndef _MUPIO_H_
#define _MUPIO_H_

#include <string>
#include "progressreporter.h"
#include "Event.h"
#include "Track.h"
#include "NotationTypes.h"
#include <fstream>

namespace Rosegarden { class Composition; }


/**
 * Mup file export
 */

class MupExporter : public ProgressReporter
{
public:
    MupExporter(QObject *parent, Rosegarden::Composition *, std::string fileName);
    ~MupExporter();

    bool write();

protected:
    Rosegarden::timeT writeBar(std::ofstream &, 
			       Rosegarden::Composition *,
			       Rosegarden::Segment *,
			       Rosegarden::timeT, Rosegarden::timeT,
			       Rosegarden::TimeSignature &,
			       Rosegarden::TrackId);
    void writeClefAndKey(std::ofstream &, Rosegarden::TrackId trackNo);
    void writeInventedRests(std::ofstream &,
			    Rosegarden::TimeSignature &timeSig,
			    Rosegarden::timeT offset,
			    Rosegarden::timeT duration);
    void writePitch(std::ofstream &, Rosegarden::TrackId, Rosegarden::Event *event);
    void writeDuration(std::ofstream &, Rosegarden::timeT duration);

    typedef std::pair<Rosegarden::Clef, Rosegarden::Key> ClefKeyPair;
    typedef std::map<Rosegarden::TrackId, ClefKeyPair> ClefKeyMap;
    ClefKeyMap m_clefKeyMap;

    Rosegarden::Composition *m_composition;
    std::string m_fileName;
};

#endif
