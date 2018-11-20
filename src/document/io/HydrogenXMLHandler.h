
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_HYDROGENXMLHANDLER_H
#define RG_HYDROGENXMLHANDLER_H

#include "HydrogenLoader.h"
#include "base/MidiProgram.h"
#include "base/Track.h"
#include <string>
#include <QString>
#include <vector>
#include <qxml.h>
#include "misc/Version.h"

#include <QCoreApplication>


class QXmlAttributes;


namespace Rosegarden
{

class Segment;
class Composition;


class HydrogenXMLHandler : public QXmlDefaultHandler
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::HydrogenXMLHandler)

public:
    HydrogenXMLHandler(Composition *comp,
            InstrumentId drumInstrument = MidiInstrumentBase + 9);

    /** 
      * Overloaded handler functions
      */
    bool startDocument() override;
    bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts) override;

    bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName) override;

    bool characters(const QString& ch) override;

    bool endDocument () override;

private:
    virtual bool startElement_093(const QString& namespaceURI,
                                  const QString& localName,
                                  const QString& qName,
                                  const QXmlAttributes& atts);
    virtual bool endElement_093(const QString& namespaceURI,
                                const QString& localName,
                                const QString& qName);
    virtual bool characters_093(const QString& ch);

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
