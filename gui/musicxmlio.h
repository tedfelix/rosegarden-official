// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2002
        Hans Kieserman      <hkieserman@mail.com>
    with heavy lifting from csoundio as it was on 13/5/2002.

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _MUSICXMLIO_H_
#define _MUSICXMLIO_H_

#include <iostream>
#include <fstream>
#include <set>
#include <string>
#include <vector>
#include "Event.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "rosegardenguidoc.h"
#include "progressreporter.h"

/**
 * MusicXml scorefile export
 */

class MusicXmlExporter : public ProgressReporter
{
public:
    typedef std::multiset<Rosegarden::Event*, Rosegarden::Event::EventCmp> eventstartlist;
    typedef std::multiset<Rosegarden::Event*, Rosegarden::Event::EventEndCmp> eventendlist;
public:
    MusicXmlExporter(QObject *parent, RosegardenGUIDoc *, std::string fileName);
    ~MusicXmlExporter();

    bool write();

protected:
    RosegardenGUIDoc *m_doc;
    std::string m_fileName;
    void writeClef(Rosegarden::Event *event, std::ofstream &str);
    void writeKey(Rosegarden::Event *event, std::ofstream &str);
    void writeTime(Rosegarden::TimeSignature timeSignature, std::ofstream &str);
    void writeNote(Rosegarden::Event *e, Rosegarden::timeT lastNoteTime,
		   const Rosegarden::Key &key, std::ofstream &str);

 private:
    static const int MAX_DOTS = 4;
};


#endif /* _MUSICXMLIO_H_ */
