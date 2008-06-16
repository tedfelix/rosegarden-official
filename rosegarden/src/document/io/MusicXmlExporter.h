
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    This file is Copyright 2002
        Hans Kieserman      <hkieserman@mail.com>
    with heavy lifting from csoundio as it was on 13/5/2002.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_MUSICXMLEXPORTER_H_
#define _RG_MUSICXMLEXPORTER_H_

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "gui/general/ProgressReporter.h"
#include <fstream>
#include <set>
#include <string>


class QObject;


namespace Rosegarden
{

class RosegardenGUIDoc;
class Key;
class Clef;
class AccidentalTable;


/**
 * MusicXml scorefile export
 */

class MusicXmlExporter : public ProgressReporter
{
public:
    typedef std::multiset<Event*, Event::EventCmp> eventstartlist;
    typedef std::multiset<Event*, Event::EventEndCmp> eventendlist;
public:
    MusicXmlExporter(QObject *parent, RosegardenGUIDoc *, std::string fileName);
    ~MusicXmlExporter();

    bool write();

protected:
    RosegardenGUIDoc *m_doc;
    std::string m_fileName;
    void writeClef(Rosegarden::Clef, std::ofstream &str);
    void writeKey(Rosegarden::Key, std::ofstream &str);
    void writeTime(TimeSignature timeSignature, std::ofstream &str);
    void writeNote(Event *e, timeT lastNoteTime,
                   AccidentalTable &table,
                   const Clef &clef,
                   const Rosegarden::Key &key,
                   std::ofstream &str);

    std::string numToId(int);
};



}

#endif
