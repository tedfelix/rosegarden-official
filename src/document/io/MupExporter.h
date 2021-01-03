
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MUPEXPORTER_H
#define RG_MUPEXPORTER_H

#include "base/Track.h"
#include <map>
#include <string>
#include <utility>
#include "base/Event.h"
#include "base/NotationTypes.h"
#include <fstream>


class QObject;


namespace Rosegarden
{

class TimeSignature;
class Segment;
class Event;
class Composition;


/**
 * Mup file export
 */

class MupExporter
{
public:
    MupExporter(QObject *parent, Composition *, std::string fileName);
    ~MupExporter();

    bool write();

protected:
    timeT writeBar(std::ofstream &, 
                               Composition *,
                               Segment *,
                               timeT, timeT,
                               TimeSignature &,
                               TrackId);
    void writeClefAndKey(std::ofstream &, TrackId trackNo);
    void writeInventedRests(std::ofstream &,
                            TimeSignature &timeSig,
                            timeT offset,
                            timeT duration);
    void writePitch(std::ofstream &, TrackId, Event *event);
    void writeDuration(std::ofstream &, timeT duration);

    typedef std::pair<Clef, Rosegarden::Key> ClefKeyPair;
    typedef std::map<TrackId, ClefKeyPair> ClefKeyMap;
    ClefKeyMap m_clefKeyMap;

    Composition *m_composition;
    std::string m_fileName;
};


}

#endif
