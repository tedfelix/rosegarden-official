// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include "musicxmlio.h"

#include "Composition.h"
#include "CompositionTimeSliceAdapter.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"
#include "Instrument.h"

#include <iostream>
#include <fstream>
#include <string>

using Rosegarden::BaseProperties;
using Rosegarden::Bool;
using Rosegarden::Clef;
using Rosegarden::Composition;
using Rosegarden::Indication;
using Rosegarden::Instrument;
using Rosegarden::Int;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::String;
using Rosegarden::timeT;
using Rosegarden::TimeSignature;


MusicXmlExporter::MusicXmlExporter(RosegardenGUIDoc *doc,
                                   std::string fileName) :
    m_doc(doc),
    m_fileName(fileName)
{
    // nothing else
}

MusicXmlExporter::~MusicXmlExporter()
{
    // nothing
}

/**
 * Take a Key-change event and return an int value
 * for that key based on the circle of fifths.
 */
int
convertKeyToFifths(Key whichKey) {
    int whichKeyTonic = whichKey.getTonicPitch();
    bool isSharp = whichKey.isSharp();
    switch (whichKeyTonic) {
        case 0:
            // Hee hee
            if (isSharp) { return 12; } else { return 0; }
        case 1:
            if (isSharp) { return 7; } else { return -5; }
        case 2:
            if (isSharp) { return 2; } else { return -10; }
        case 3:
            if (isSharp) { return 9; } else { return -3; }
        case 4:
            if (isSharp) { return 4; } else { return -8; }
        case 5:
            if (isSharp) { return 11; } else { return -1; }
        case 6:
            if (isSharp) { return 6; } else { return -6; }
        case 7:
            if (isSharp) { return 1; } else { return -11; }
        case 8:
            if (isSharp) { return 8; } else { return -4; }
        case 9:
            if (isSharp) { return 3; } else { return -9; }
        case 10:
            if (isSharp) { return 10; } else { return -2; }
        case 11:
            if (isSharp) { return 5; } else { return -7; }
    }
    // Incomplete: Error!
    return 0;
}

// converts pitch to basic note name
// Incomplete??  probably just as limited here as it was in its original home: can't
// handle B/C E/F or user-specified accidentals
char
convertPitchToName(int pitch, bool isFlatKeySignature)
{
    // shift to a->g, rather than c->b
    pitch += 3;
    pitch %= 12;

    // For flat key signatures, accidentals use next note name
    if (isFlatKeySignature &&
        (pitch == 1 || pitch == 4 ||
         pitch == 6 || pitch == 9 || pitch == 11)) {
        pitch++;
    }
    pitch = pitch % 12;

    // compensate for no c-flat or f-flat
    if (pitch > 2) {
        pitch++;
    }
    if (pitch > 8) {
        pitch++;
    }
    char pitchLetter = (char)((int)(pitch/2) + 97);
    return pitchLetter;
}

bool
needsAccidental(int pitch) {
    if (pitch > 4) {
        pitch++;
    }
    return (pitch % 2 == 1);
}

/**
 * Accidentals?
 */
void
MusicXmlExporter::writeNote(Event *e, bool isFlatKeySignature, std::ofstream &str) {
    str << "\t\t\t\t<note>" << std::endl;
    if (e->isa(Note::EventRestType)) {
        str << "\t\t\t\t<rest/>" << std::endl;
    } else {
        long pitch = 0;
        e->get<Int>(BaseProperties::PITCH, pitch);
        str << "\t\t\t\t<pitch>" << std::endl;
        str << "\t\t\t\t<step>" << convertPitchToName(pitch % 12, isFlatKeySignature) << "</step>" << std::endl;
        // Incomplete: double sharp/flats
        str << "\t\t\t\t<alter>" << (needsAccidental(pitch % 12)?(isFlatKeySignature?-1:1):0) << "</alter>" << std::endl;

        int octave = (int)(pitch / 12);
        str << "\t\t\t\t<octave>" << octave << "</octave>" << std::endl;
        str << "\t\t\t\t</pitch>" << std::endl;
    }
    str << "\t\t\t\t<duration>" << e->getDuration() << "</duration>" << std::endl;
    
    // Incomplete: will RG ever use this?
    str << "\t\t\t\t<voice>" << "1" << "</voice>" << std::endl;
    Note tmpNote = Note::getNearestNote(e->getDuration(), MAX_DOTS);
    str << "\t\t\t\t<type>" << tmpNote.getShortName() << "</type>" << std::endl;
    // could also do <stem>down</stem> if you wanted
    str << "\t\t\t\t</note>" << std::endl;
}

void
MusicXmlExporter::writeKey(Event *event, std::ofstream &str) {
    Key whichKey(*event);
    str << "\t\t\t\t<key>" << std::endl;
    str << "\t\t\t\t<fifths>" << convertKeyToFifths(whichKey) << "</fifths>" << std::endl;
    str << "\t\t\t\t<mode>";                
    if (whichKey.isMinor()) {
        str << "minor";
    } else {
        str << "major";
    }
    str << "</mode>" << std::endl;
    str << "\t\t\t\t</key>" << std::endl;
}

void
MusicXmlExporter::writeTime(TimeSignature timeSignature, std::ofstream &str) {
    str << "\t\t\t\t<time>" << std::endl;
    str << "\t\t\t\t<beats>" << timeSignature.getNumerator() << "</beats>" << std::endl;
    str << "\t\t\t\t<beat-type>" << timeSignature.getDenominator() << "</beat-type>" << std::endl;
    str << "\t\t\t\t</time>" << std::endl;
}

void
MusicXmlExporter::writeClef(Event *event, std::ofstream &str) {
    str << "\t\t\t\t<clef>" << std::endl;
    std::string whichClef(event->get<String>(Clef::ClefPropertyName));
    if (whichClef == Clef::Treble) {
        str << "\t\t\t\t<sign>G</sign>" << std::endl;
        str << "\t\t\t\t<line>2</line>" << std::endl;
    } else if (whichClef == Clef::Tenor) {
        str << "\t\t\t\t<sign>C</sign>" << std::endl;
        str << "\t\t\t\t<line>3</line>" << std::endl;
    } else if (whichClef == Clef::Alto) {
        str << "\t\t\t\t<sign>C</sign>" << std::endl;
        str << "\t\t\t\t<line>4</line>" << std::endl;
    } else if (whichClef == Clef::Bass) {
        str << "\t\t\t\t<sign>F</sign>" << std::endl;
        str << "\t\t\t\t<line>4</line>" << std::endl;
    }
    str << "\t\t\t\t</clef>" << std::endl;
}

/** 
 * Incomplete: This would be cleaner via an XML parser
 */
bool
MusicXmlExporter::write()
{
    Composition *composition = &m_doc->getComposition();

    std::ofstream str(m_fileName.c_str(), std::ios::out);
    if (!str) {
        std::cerr << "MusicXmlExporter::write() - can't write file " << m_fileName << std::endl;
        return false;
    }

    // XML header information
    str << "<?xml version=\"1.0\"?>" << std::endl;
    str << "<!DOCTYPE score-partwise PUBLIC \"-//Recordare//DTD MusicXML 0.6 Partwise//EN\" \"http://www.musicxml.org/dtds/partwise.dtd\">" << std::endl;
    // MusicXml header information
    str << "<score-partwise>" << std::endl;
    str << "\t<work> <work-title>" << m_fileName <<
        "</work-title></work> " << std::endl;
    // Movement, etc. info goes here
    str << "\t<identification> " << std::endl;
    if (composition->getCopyrightNote() != "") {
        str << "\t\t<rights>"
            // Incomplete: need to XML-encode from copyright note
            << composition->getCopyrightNote() << "</rights>" << std::endl;
    }
    str << "\t\t<encoding>" << std::endl;
    // Incomplete: Insert date!
    //    str << "\t\t\t<encoding-date>" << << "</encoding-date>" << std::endl;
    str << "\t\t\t<software>Rosegarden 4</software>" << std::endl;
    str << "\t\t</encoding>" << std::endl;
    str << "\t</identification> " << std::endl;

    // MIDI information
    str << "\t<part-list>" << std::endl;
    Composition::trackcontainer * tracks = composition->getTracks();
    for (Composition::trackiterator i = tracks->begin();
         i != tracks->end(); ++i) {
        // Incomplete: What about all the other Midi stuff?
        // Incomplete: (Future) GUI to set labels if they're not already
        Instrument * trackInstrument = (&m_doc->getStudio())->getInstrumentById((*i).second->getInstrument());
        str << "\t\t<score-part id=\"" << (*i).first << "\">" << std::endl;
        str << "\t\t\t<part-name>" << (*i).second->getLabel() << "</part-name>" << std::endl;
        if (trackInstrument) {
            str << "\t\t\t<score-instrument id=\"" << trackInstrument->getName() << "\">" << std::endl;
            str << "\t\t\t\t<instrument-name>" << trackInstrument->getType() << "</instrument-name>" << std::endl;
            str << "\t\t\t</score-instrument>" << std::endl;
            str << "\t\t\t<midi-instrument id=\"" << trackInstrument->getName() << "\">" << std::endl;
            str << "\t\t\t\t<midi-channel>" << ((unsigned int)trackInstrument->getMidiChannel()) << "</midi-channel>" << std::endl;
            str << "\t\t\t\t<midi-program>" << ((unsigned int)trackInstrument->getProgramChange()) << "</midi-program>" << std::endl;
            str << "\t\t\t</midi-instrument>" << std::endl;
        }
        str << "\t\t</score-part>" << std::endl;

    } // end track iterator
    str << "\t</part-list>" << std::endl;
    
    // Notes!
    // Write out all segments for each Track
    bool startedPart = false;
    for (Composition::trackiterator j = tracks->begin();
         j != tracks->end(); ++j) {
        // Code courtesy docs/code/iterators.txt
        Rosegarden::CompositionTimeSliceAdapter::TrackSet trackSet;
        // Incomplete: get the track info for each track (i.e. this should
        // be in an iterator loop) into the track set
        trackSet.insert((*j).first);
        Rosegarden::CompositionTimeSliceAdapter adapter(composition, trackSet);
        if (startedPart) {
            str << "\t</part>" << std::endl;
            startedPart = true;
        }
        str << "\t<part id=\"" << (*j).first << "\">" << std::endl;
        int oldMeasureNumber = -1;
        bool startedAttributes = false;
        bool isFlatKeySignature = false;
        TimeSignature prevTimeSignature;

        for (Rosegarden::CompositionTimeSliceAdapter::iterator k = adapter.begin();
             k != adapter.end(); ++k) {
            Event *event = *k;
            timeT absoluteTime = event->getAbsoluteTime();
            // Open a new measure if necessary
            // Incomplete: How does MusicXML handle non-contiguous measures?
            int measureNumber = composition->getBarNumber(absoluteTime);

            if (oldMeasureNumber < 0) {
                str << "\t\t<measure number=\""<< measureNumber << "\">" << std::endl;
                str << "\t\t\t<attributes>" << std::endl;
                // Divisions is divisions of crotchet (quarter-note) on which all
                // note-lengths are based
                str << "\t\t\t\t<divisions>" << Note(Note::Crotchet).getDuration() << "</divisions>" << std::endl;
                prevTimeSignature = composition->getTimeSignatureAt(composition->getStartMarker());
                writeTime(prevTimeSignature, str);
                startedAttributes = true;
            } else if (measureNumber > oldMeasureNumber) {
                if (startedAttributes) {
                    str << "\t\t\t</attributes>" << std::endl;
                    startedAttributes = false;
                }
                str << "\t\t</measure>\n" << std::endl;
                str << "\t\t<measure number=\""<< measureNumber << "\">" << std::endl;
            } else if (measureNumber < oldMeasureNumber) {
                // Incomplete: Error!
            }
            oldMeasureNumber = measureNumber;
            
            // process event
            if (event->isa(Key::EventType)) {
                if (!startedAttributes) {
                    str << "\t\t\t<attributes>" << std::endl;
                    startedAttributes = true;
                }
                writeKey(event, str);
                Key whichKey(*event);
                if (whichKey.isSharp()) {
                    isFlatKeySignature = false;
                } else {
                    isFlatKeySignature = true;
                }
            } else if (event->isa(Clef::EventType)) {
                if (!startedAttributes) {
                    str << "\t\t\t<attributes>" << std::endl;
                    startedAttributes = true;
                }
                writeClef(event, str);
            } else if (event->isa(Note::EventRestType) ||
                event->isa(Note::EventType)) {
                // Random TimeSignature events in the middle of nowhere will
                // be ignored, for better or worse
                TimeSignature timeSignature = composition->getTimeSignatureAt(absoluteTime);
                if (timeSignature.getNumerator() != prevTimeSignature.getNumerator() ||
                    timeSignature.getDenominator() != prevTimeSignature.getDenominator()) {
                    if (!startedAttributes) {
                        str << "\t\t\t<attributes>" << std::endl;
                        startedAttributes = true;
                    }
                    writeTime(timeSignature, str);
                }
                if (startedAttributes) {
                    str << "\t\t\t</attributes>" << std::endl;
                    startedAttributes = false;
                }

                writeNote(event, isFlatKeySignature, str);
            }
        }
        if (oldMeasureNumber > 0) { // no events at all
            str << "\t\t</measure>\n" << std::endl;
        }
    }
    str << "\t</part>" << std::endl;

    str << "</score-partwise>" << std::endl;
    str.close();
    return true;
}
