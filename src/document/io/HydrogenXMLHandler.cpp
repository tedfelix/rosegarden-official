/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[HydrogenXMLHandler]"

#include "HydrogenXMLHandler.h"

#include "base/Event.h"
#include "base/BaseProperties.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Track.h"
#include <QString>


namespace Rosegarden
{

HydrogenXMLHandler::HydrogenXMLHandler(Composition *composition,
                                       InstrumentId drumIns):
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
{}

bool
HydrogenXMLHandler::startDocument()
{
    RG_DEBUG << "HydrogenXMLHandler::startDocument";

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
    m_instrumentVolumes.clear();
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


bool HydrogenXMLHandler::startElement_093(const QString& /*namespaceURI*/,
                                          const QString& /*localName*/,
                                          const QString& qName,
                                          const QXmlAttributes& /*atts*/)
{
    QString lcName = qName.toLower();

    RG_DEBUG << "HydrogenXMLHandler::startElement - " << lcName;

    if (lcName == "note") {

        if (m_inInstrument)
            return false;

        m_inNote = true;

    } else if (lcName == "instrument") {

        // Beware instrument attributes inside Notes
        if (!m_inNote)
            m_inInstrument = true;
    } else if (lcName == "pattern") {
        m_inPattern = true;
        m_segmentAdded = false; // flag the segments being added
    } else if (lcName == "sequence") {

        // Create a new segment and set some flags
        //
        m_segment = new Segment();
        m_newSegment = true;
        m_inSequence = true;
    }

    m_currentProperty = lcName;

    return true;
}

bool
HydrogenXMLHandler::startElement(const QString& /*namespaceURI*/,
                                 const QString& /*localName*/,
                                 const QString& qName,
                                 const QXmlAttributes& /*atts*/)
{
 bool rc=false;
 QXmlAttributes DummyAttr;
 QString DummyQString;

 if (m_version=="") {
   /* no version yet, use 093 */
   rc=startElement_093(DummyQString, DummyQString, qName, DummyAttr);
 }
 else {
   /* select version dependant function */
   rc=startElement_093(DummyQString, DummyQString, qName, DummyAttr);
 }
 return rc;
}

bool
HydrogenXMLHandler::endElement_093(const QString& /*namespaceURI*/,
                                   const QString& /*localName*/,
                                   const QString& qName)
{
    QString lcName = qName.toLower();

    if (lcName == "note") {

        RG_DEBUG << "HydrogenXMLHandler::endElement - Hydrogen Note : position = " << m_position
        << ", velocity = " << m_velocity
        << ", panL = " << m_panL
        << ", panR = " << m_panR
        << ", pitch = " << m_pitch
        << ", instrument = " << m_instrument
        << endl;

        timeT barLength = m_composition->getBarEnd(m_currentBar) -
                          m_composition->getBarStart(m_currentBar);

        timeT pos = m_composition->getBarStart(m_currentBar) +
                    timeT(
                        double(m_position) / double(m_patternSize) * double(barLength));

        // Insert a rest if we've got a new segment
        //
        if (m_newSegment) {
            Event *restEvent = new Event(Note::EventRestType,
                                         m_composition->getBarStart(m_currentBar),
                                         pos - m_composition->getBarStart(m_currentBar),
                                         Note::EventRestSubOrdering);
            m_segment->insert(restEvent);
            m_newSegment = false;
        }

        // Create and insert this event
        //
        Event *noteEvent = new Event(Note::EventType,
                                     pos, Note(Note::Semiquaver).getDuration());

        // get drum mapping from instrument and calculate velocity
        noteEvent->set
        <Int>(
            BaseProperties::PITCH, 36 + m_instrument);
        noteEvent->set
        <Int>(BaseProperties::VELOCITY,
              int(127.0 * m_velocity * m_volume *
                  m_instrumentVolumes[m_instrument]));
        m_segment->insert(noteEvent);

        m_inNote = false;

    } else if (lcName == "instrument" && m_inInstrument) {

        RG_DEBUG << "HydrogenXMLHandler::endElement - Hydrogen Instrument : id = " << m_id
        << ", muted = " << m_muted
        << ", volume = " << m_instrumentVolumes[m_instrument]
        << ", filename = \"" << m_fileName << "\""
        << endl;

        m_inInstrument = false;

    } else if (lcName == "pattern") {
        m_inPattern = false;

        if (m_segmentAdded) {

            // Add a blank track to demarcate patterns
            //
            Track *track = new Track
                           (m_currentTrackNb, m_drumInstrument, m_currentTrackNb,
                            "<blank spacer>", false);
            m_currentTrackNb++;
            m_composition->addTrack(track);

            std::vector<TrackId> trackIds;
            trackIds.push_back(track->getId());
            m_composition->notifyTracksAdded(trackIds);

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

            Track *track = new Track
                           (m_currentTrackNb, m_drumInstrument, m_currentTrackNb,
                            m_patternName, false);
            m_currentTrackNb++;

            // Enforce start and end markers for this bar so that we have a
            // whole bar unit segment.
            //
            m_segment->setEndMarkerTime(m_composition->getBarEnd(m_currentBar));
            QString label = QString("%1 - %2 %3 %4").arg(strtoqstr(m_patternName))
                            .arg(strtoqstr(m_sequenceName))
                            .arg(tr(" imported from Hydrogen ")).arg(strtoqstr(m_version));
            m_segment->setLabel(qstrtostr(label));

            m_composition->addTrack(track);

            std::vector<TrackId> trackIds;
            trackIds.push_back(track->getId());
            m_composition->notifyTracksAdded(trackIds);

            m_composition->addSegment(m_segment);
            m_segment = 0;

            m_segmentAdded = true;
        }

        m_inSequence = false;

    } else if (lcName == "version") {
        // up to now we can only read files of version 0.9.3 and earlier
        // there was an xml format change  in 0.9.4
        Version canHandleVersion, versionInFile;

        canHandleVersion.qstrtoversion("0.9.3");
        versionInFile.qstrtoversion(strtoqstr(m_version));

        RG_DEBUG << "HydrogenXMLHandler::endElement version " << m_version;
        RG_DEBUG << "ch_major: " << canHandleVersion.Major() << 
                    "  ch_minor: " << canHandleVersion.Minor() << 
                    "  ch_micro: " << canHandleVersion.Micro() << endl;
        RG_DEBUG << "if_major: " << versionInFile.Major() << 
                    "  if_minor: " << versionInFile.Minor() << 
                    "  if_micro: " << versionInFile.Micro() << endl;

        bool bCanHandleFile=(versionInFile<=canHandleVersion);

        if (bCanHandleFile==true) {
          // go on, this is a good version 
          RG_DEBUG << "HydrogenXMLHandler::endElement version: version ok ";
        }
        else {
          // error 
          RG_DEBUG << "HydrogenXMLHandler::endElement version: bad version (file created with hydrogen version " << m_version << " can not be parsed)";
          return false;
        }
    }

    return true;
}

bool
HydrogenXMLHandler::endElement(const QString& /*namespaceURI*/,
                               const QString& /*localName*/,
                               const QString& qName)
{
 bool rc=false;
 QString DummyQString;

 if (m_version=="") {
   /* no version yet, use 093 */
   rc=endElement_093(DummyQString, DummyQString, qName);
 }
 else {
   /* select version dependant function */
   rc=endElement_093(DummyQString, DummyQString, qName);
 }
 return rc;
}

bool
HydrogenXMLHandler::characters_093(const QString& chars)
{
    QString ch = chars.trimmed();
    if (ch == "")
        return true;

    if (m_inNote) {
        if (m_currentProperty == "position") {
            m_position = ch.toInt();
        } else if (m_currentProperty == "velocity") {
            m_velocity = qstrtodouble(ch);
        } else if (m_currentProperty == "pan_L") {
            m_panL = qstrtodouble(ch);
        } else if (m_currentProperty == "pan_R") {
            m_panR = qstrtodouble(ch);
        } else if (m_currentProperty == "pitch") {
            m_pitch = qstrtodouble(ch);
        } else if (m_currentProperty == "instrument") {
            m_instrument = ch.toInt();

            // Standard kit conversion - hardcoded conversion for Hyrdogen's default
            // drum kit.  The m_instrument mapping for low values maps well onto the
            // kick drum GM kit starting point (MIDI pitch = 36).
            //
            switch (m_instrument) {
            case 11:  // Cowbell
                m_instrument = 20;
                break;
            case 12:  // Ride Jazz
                m_instrument = 15;
                break;
            case 14:  // Ride Rock
                m_instrument = 17;
                break;
            case 15:  // Crash Jazz
                m_instrument = 16;
                break;

            default:
                break;
            }

        }
    } else if (m_inInstrument) {
        if (m_currentProperty == "id") {
            m_id = ch.toInt();
        } else if (m_currentProperty == "ismuted") {
            if (ch.toLower() == "true")
                m_muted = true;
            else
                m_muted = false;
        } else if (m_currentProperty == "filename") {
            m_fileName = qstrtostr(chars); // don't strip whitespace from the filename
        } else if (m_currentProperty == "volume") {
            m_instrumentVolumes.push_back(qstrtodouble(ch));
        }


    } else if (m_inPattern) {

        // Pattern attributes

        if (m_currentProperty == "name") {
            if (m_inSequence)
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

            m_bpm = qstrtodouble(ch);
            m_composition->addTempoAtTime
            (0, Composition::getTempoForQpm(m_bpm));

        } else if (m_currentProperty == "volume") {
            m_volume = qstrtodouble(ch);
        } else if (m_currentProperty == "name") {
            m_name = qstrtostr(chars);
        } else if (m_currentProperty == "author") {
            m_author = qstrtostr(chars);
        } else if (m_currentProperty == "notes") {
            m_notes = qstrtostr(chars);
        } else if (m_currentProperty == "mode") {
            if (ch.toLower() == "song")
                m_songMode = true;
            else
                m_songMode = false;
        }
    }

    return true;
}

bool
HydrogenXMLHandler::characters(const QString& chars)
{
 bool rc=false;

 if (m_version=="") {
   /* no version yet, use 093 */
   rc=characters_093(chars);
 }
 else {
   /* select version dependant function */
   rc=characters_093(chars);
 }
 return rc;
}

bool
HydrogenXMLHandler::endDocument()
{
    RG_DEBUG << "HydrogenXMLHandler::endDocument";
    return true;
}

}
