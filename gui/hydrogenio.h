// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#ifndef _HYDROGENIO_H_
#define _HYDROGENIO_H_

#include <iostream>
#include <string>

#include <qxml.h>
#include <qtextstream.h>
#include <qobject.h>

#include "progressreporter.h"

class Rosegarden::Studio;
class Rosegarden::Composition;


/**
 * Hydrogen drum machine file importer
 */

class HydrogenLoader : public ProgressReporter
{
public:
    HydrogenLoader(Rosegarden::Studio *,
            QObject *parent = 0, const char *name = 0);

    /**
      * Load and parse the Hydrogen file \a fileName, and write it into the
      * given Composition (clearing the existing segment data first).
      * Return true for success.
      */
    bool load(const QString& fileName, Rosegarden::Composition &);

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::Studio      *m_studio;
    std::string              m_fileName;

private:
    static const int MAX_DOTS = 4;
    static const Rosegarden::PropertyName SKIP_PROPERTY;
};


class HydrogenXMLHandler : public QXmlDefaultHandler
{
public:
    HydrogenXMLHandler(Rosegarden::Composition *comp,
            Rosegarden::InstrumentId drumInstrument = Rosegarden::MidiInstrumentBase + 9);

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
    Rosegarden::Composition *m_composition;
    Rosegarden::InstrumentId m_drumInstrument;

    bool                     m_inNote;
    bool                     m_inInstrument;
    bool                     m_inPattern;

    // Pattern attributes
    //
    std::string              m_patternName;
    int                      m_patternSize;

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

    Rosegarden::Segment     *m_segment;
    Rosegarden::TrackId      m_currentTrackNb;
    bool                     m_segmentAdded;
};


#endif /* _HYDROGENIO_H_ */
