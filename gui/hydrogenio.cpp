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

#include <kmessagebox.h>
#include <klocale.h>

#include "Studio.h"
#include "Composition.h"
#include "Event.h"
#include "BaseProperties.h"
#include "NotationTypes.h"
#include "MidiTypes.h"

#include "hydrogenio.h"
#include "rosestrings.h"

/**
 * Hydrogen drum machine importer
 */

HydrogenLoader::HydrogenLoader(Rosegarden::Studio *studio,
        QObject *parent, const char *name):
    ProgressReporter(parent, name),
    m_studio(studio)
{
}


bool 
HydrogenLoader::load(const QString& fileName, Rosegarden::Composition &comp)
{
    m_composition = &comp;
    comp.clear();

    QFile file(fileName);
    if (!file.open(IO_ReadOnly))
    {
        return false;
    }

    m_studio->unassignAllInstruments();

    HydrogenXMLHandler handler(m_composition);

    QXmlInputSource source(file);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);

    bool ok = reader.parse(source);

    return ok;
}


// ---- HydrogenXMLHandler ----
//
//
using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::Int;

HydrogenXMLHandler::HydrogenXMLHandler(Rosegarden::Composition *composition,
        Rosegarden::InstrumentId drumIns):
    m_composition(composition),
    m_drumInstrument(drumIns),
    m_inNote(false),
    m_inInstrument(false),
    m_inPattern(false),
    m_inSequence(false),
    m_patternName(""),
    m_patternSize(0),
    m_sequenceName(""),
    m_position(0),
    m_velocity(0.0),
    m_panL(0.0),
    m_panR(0.0),
    m_pitch(0.0),
    m_instrument(0),
    m_id(0),
    m_muted(false),
    m_fileName(""),
    m_bpm(0),
    m_volume(0.0),
    m_name(""),
    m_author(""),
    m_notes(""),
    m_songMode(false),
    m_version(""),
    m_currentProperty(""),
    m_segment(0),
    m_currentTrackNb(0),
    m_segmentAdded(false),
    m_currentBar(0),
    m_newSegment(false)
{
}

bool
HydrogenXMLHandler::startDocument()
{
    RG_DEBUG << "HydrogenXMLHandler::startDocument" << endl;

    m_inNote = false;
    m_inInstrument = false;
    m_inPattern = false;
    m_inSequence = false;

    // Pattern attributes
    //
    m_patternName = "";
    m_patternSize = 0;

    // Sequence attributes
    //
    m_sequenceName = "";

    // Note attributes
    //
    m_position = 0;
    m_velocity = 0.0;
    m_panL = 0.0;
    m_panR = 0.0;
    m_pitch = 0.0;
    m_instrument = 0;

    // Instrument attributes
    //
    m_id = 0;
    m_muted = false;
    m_fileName = "";

    // Global attributes
    //
    m_bpm = 0;
    m_volume = 0.0;
    m_name = "";
    m_author = "";
    m_notes = "";
    m_songMode = false;
    m_version = "";

    m_currentProperty = "";

    m_segment = 0;
    m_currentTrackNb = 0;
    m_segmentAdded = 0;
    m_currentBar = 0;
    m_newSegment = false;

    return true;
}

bool
HydrogenXMLHandler::startElement(const QString& /*namespaceURI*/,
                                 const QString& /*localName*/,
                                 const QString& qName,
                                 const QXmlAttributes& /*atts*/)
{
    QString lcName = qName.lower();

    if (lcName == "note") {

        if (m_inInstrument) return false;

        m_inNote = true;

    } else if (lcName == "instrument") {

        // Beware instrument attributes inside Notes
        if (!m_inNote) m_inInstrument = true;
    } else if (lcName == "pattern") {
        m_inPattern = true;
        m_segmentAdded = false; // flag the segments being added
    } else if (lcName == "sequence") {

        // Create a new segment and set some flags
        //
        m_segment = new Rosegarden::Segment();
        m_newSegment = true;
        m_inSequence = true;
    }

    m_currentProperty = lcName;

    return true;
}

bool
HydrogenXMLHandler::endElement(const QString& /*namespaceURI*/,
                               const QString& /*localName*/,
                               const QString& qName)
{
    QString lcName = qName.lower();

    if (lcName == "note") {

        RG_DEBUG << "HydrogenXMLHandler::endElement - Hydrogen Note : position = " << m_position
                 << ", velocity = " << m_velocity
                 << ", panL = " << m_panL
                 << ", panR = " << m_panR
                 << ", pitch = " << m_pitch
                 << ", instrument = " << m_instrument
                 << endl;

        Rosegarden::timeT barLength = m_composition->getBarEnd(m_currentBar) - 
            m_composition->getBarStart(m_currentBar);

        Rosegarden::timeT pos = m_composition->getBarStart(m_currentBar) + 
            Rosegarden::timeT(
                    double(m_position)/double(m_patternSize) * double(barLength));

        // Insert a rest if we've got a new segment
        //
        if (m_newSegment)
        {
            Event *restEvent = new Event(Rosegarden::Note::EventRestType,
                                         m_composition->getBarStart(m_currentBar), 
                                         pos - m_composition->getBarStart(m_currentBar),
                                         Rosegarden::Note::EventRestSubOrdering);
            m_segment->insert(restEvent);
            m_newSegment = false;
        }

        // Create and insert this event
        //
        Event *noteEvent = new Event(Rosegarden::Note::EventType, 
            pos, Note(Note::Semiquaver).getDuration());

        // get drum mapping from instrument and calculate velocity
        noteEvent->set<Int>(Rosegarden::BaseProperties::PITCH, 36 + m_instrument); 
        noteEvent->set<Int>(Rosegarden::BaseProperties::VELOCITY, int(127.0 * m_velocity)); 
        m_segment->insert(noteEvent);

        m_inNote = false;

    } else if (lcName == "instrument" && m_inInstrument) {

        RG_DEBUG << "HydrogenXMLHandler::endElement - Hydrogen Instrument : id = " << m_id
                 << ", muted = " << m_muted
                 << ", filename = \"" << m_fileName << "\""
                 << endl;

        m_inInstrument = false;

    } else if (lcName == "pattern") {
        m_inPattern = false;

        if (m_segmentAdded) {

            // Add a blank track to demarcate patterns
            //
            Rosegarden::Track *track = new Rosegarden::Track
                (m_currentTrackNb, m_drumInstrument, m_currentTrackNb,
                 "<blank spacer>", false);
            m_currentTrackNb++;
            m_composition->addTrack(track);

            m_segmentAdded = false;

            // Each pattern has it's own bar so that the imported
            // song shows off each pattern a bar at a time.
            //
            m_currentBar++; 
        }
    
    } else if (lcName == "sequence") {

        // If we're closing out a sequencer tab and we have a m_segment then 
        // we should close up and add that segment.  Only create if we have
        // some Events in it
        //
        if (m_segment->size() > 0) {

            m_segment->setTrack(m_currentTrackNb);

            Rosegarden::Track *track = new Rosegarden::Track
                (m_currentTrackNb, m_drumInstrument, m_currentTrackNb,
                 m_patternName, false);
            m_currentTrackNb++;

            // Enforce start and end markers for this bar so that we have a 
            // whole bar unit segment.
            //
            m_segment->setEndMarkerTime(m_composition->getBarEnd(m_currentBar));
            QString label = QString("%1 - %2 %3 %4").arg(strtoqstr(m_patternName))
                .arg(strtoqstr(m_sequenceName))
                .arg(i18n(" imported from Hydrogen ")).arg(strtoqstr(m_version));
            m_segment->setLabel(qstrtostr(label));

            m_composition->addTrack(track);
            m_composition->addSegment(m_segment);
            m_segment = 0;

            m_segmentAdded = true;
        }

        m_inSequence = false;

    }

    return true;
}

bool 
HydrogenXMLHandler::characters(const QString& chars)
{
    QString ch = chars.stripWhiteSpace();
    if (ch == "") return true;

    if (m_inNote)
    {
        if (m_currentProperty == "position") {
            m_position = ch.toInt();
        } else if (m_currentProperty == "velocity") {
            m_velocity = ch.toDouble();
        } else if (m_currentProperty == "pan_L") {
            m_panL = ch.toDouble();
        } else if (m_currentProperty == "pan_R") {
            m_panR = ch.toDouble();
        } else if (m_currentProperty == "pitch") {
            m_pitch = ch.toDouble();
        } else if (m_currentProperty == "instrument") {
            m_instrument = ch.toInt();

            // Standard kit conversion - hardcoded conversion for Hyrdogen's default
            // drum kit.  The m_instrument mapping for low values maps well onto the
            // kick drum GM kit starting point (MIDI pitch = 36).
            //
            switch(m_instrument)
            {
                case 11: // Cowbell
                    m_instrument = 20;
                    break;
                case 12: // Ride Jazz
                    m_instrument = 15;
                    break;
                case 14: // Ride Rock
                    m_instrument = 17;
                    break;
                case 15: // Crash Jazz
                    m_instrument = 16;
                    break;

                default:
                    break;
            }

        }
    } else if (m_inInstrument)
    {
        if (m_currentProperty == "id") {
            m_id = ch.toInt();
        } else if (m_currentProperty == "ismuted") {
            if (ch.lower() == "true") m_muted = true;
            else m_muted = false;
        } else if (m_currentProperty == "filename") {
            m_fileName = qstrtostr(chars); // don't strip whitespace from the filename
        }
    } else if (m_inPattern) {

        // Pattern attributes

        if (m_currentProperty == "name") {
           if  (m_inSequence)
               m_sequenceName = qstrtostr(chars);
           else
               m_patternName = qstrtostr(chars);
        } else if (m_currentProperty == "size") {
            m_patternSize = ch.toInt();
        }

    } else {

        // Global attributes
        if (m_currentProperty == "version") {
            m_version = qstrtostr(chars);
        } else if (m_currentProperty == "bpm") {

            m_bpm = ch.toDouble();
            m_composition->addTempo(0, m_bpm);

        } else if (m_currentProperty == "volume") {
            m_volume = ch.toDouble();
        } else if (m_currentProperty == "name") {
            m_name = qstrtostr(chars);
        } else if (m_currentProperty == "author") {
            m_author = qstrtostr(chars);
        } else if (m_currentProperty == "notes") {
            m_notes = qstrtostr(chars);
        } else if (m_currentProperty == "mode") {
            if (ch.lower() == "song") m_songMode = true;
            else m_songMode = false;
        }
    }

    return true;
}

bool 
HydrogenXMLHandler::endDocument()
{
    RG_DEBUG << "HydrogenXMLHandler::endDocument" << endl;
    return true;
}

