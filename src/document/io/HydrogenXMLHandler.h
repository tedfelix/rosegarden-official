
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_HYDROGENXMLHANDLER_H_
#define _RG_HYDROGENXMLHANDLER_H_

#include "HydrogenLoader.h"
#include "base/MidiProgram.h"
#include "base/Track.h"
#include <string>
#include <qstring.h>
#include <vector>
#include <qxml.h>


class QXmlAttributes;


namespace Rosegarden
{

class Segment;
class Composition;


class HydrogenXMLHandler : public QXmlDefaultHandler
{
public:
    HydrogenXMLHandler(Composition *comp,
            InstrumentId drumInstrument = MidiInstrumentBase + 9);

    /** 
      * Overloaded handler functions
      */
    virtual bool startDocument();
    virtual bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts);

    virtual bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName);

    virtual bool characters(const QString& ch);

    virtual bool endDocument ();

protected:
    Composition *m_composition;
    InstrumentId m_drumInstrument;

    bool                     m_inNote;
    bool                     m_inInstrument;
    bool                     m_inPattern;
    bool                     m_inSequence;

    // Pattern attributes
    //
    std::string              m_patternName;
    int                      m_patternSize;

    // Sequence attributes
    //
    std::string              m_sequenceName;

    // Note attributes
    //
    int                      m_position;
    double                   m_velocity;
    double                   m_panL;
    double                   m_panR;
    double                   m_pitch;
    int                      m_instrument;

    // Instrument attributes
    //
    int                      m_id;
    bool                     m_muted;
    std::vector<double>      m_instrumentVolumes;
    std::string              m_fileName;

    // Global attributes
    //
    double                   m_bpm;
    double                   m_volume;
    std::string              m_name;
    std::string              m_author;
    std::string              m_notes;
    bool                     m_songMode;  // Song mode or pattern mode?
    std::string              m_version;

    //
    QString                  m_currentProperty;

    Segment     *m_segment;
    TrackId      m_currentTrackNb;
    bool                     m_segmentAdded;
    int                      m_currentBar;
    bool                     m_newSegment;

    SegmentMap               m_segmentMap;

};



}

#endif
