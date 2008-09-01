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


#include "MusicXmlExporter.h"

#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/CompositionTimeSliceAdapter.h"
#include "base/Event.h"
#include "base/Instrument.h"
#include "base/NotationTypes.h"
#include "base/XmlExportable.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/general/ProgressReporter.h"
#include <QObject>

namespace Rosegarden
{

using namespace BaseProperties;

MusicXmlExporter::MusicXmlExporter(QObject *parent,
                                   RosegardenGUIDoc *doc,
                                   std::string fileName) :
        ProgressReporter(parent, "musicXmlExporter"),
        m_doc(doc),
        m_fileName(fileName)
{
    // nothing else
}

MusicXmlExporter::~MusicXmlExporter()
{
    // nothing
}

void
MusicXmlExporter::writeNote(Event *e, timeT lastNoteTime,
                            AccidentalTable &accTable,
                            const Clef &clef,
                            const Rosegarden::Key &key,
                            std::ofstream &str)
{
    str << "\t\t\t<note>" << std::endl;

    Pitch pitch(64);
    Accidental acc;
    Accidental displayAcc;
    bool cautionary;
    Accidental processedDisplayAcc;

    if (e->isa(Note::EventRestType)) {
        str << "\t\t\t\t<rest/>" << std::endl;

    } else {

        // Order of MusicXML elements within a note:
        // chord
        // pitch
        // duration
        // tie
        // instrument
        // voice
        // type
        // dot(s)
        // accidental
        // time modification
        // stem
        // notehead
        // staff
        // beam
        // notations
        // lyric

        if (e->getNotationAbsoluteTime() == lastNoteTime) {
            str << "\t\t\t\t<chord/>" << std::endl;
        } else {
            accTable.update();
        }

        str << "\t\t\t\t<pitch>" << std::endl;

        long p = 0;
        e->get<Int>(PITCH, p);
        pitch = p;

        str << "\t\t\t\t\t<step>" << pitch.getNoteName(key) << "</step>" << std::endl;

        acc = pitch.getAccidental(key.isSharp());
        displayAcc = pitch.getDisplayAccidental(key);

        cautionary = false;
        processedDisplayAcc =
            accTable.processDisplayAccidental
            (displayAcc, pitch.getHeightOnStaff(clef, key), cautionary);

        // don't handle cautionary accidentals here:
        if (cautionary)
            processedDisplayAcc = Accidentals::NoAccidental;

        if (acc == Accidentals::DoubleFlat) {
            str << "\t\t\t\t\t<alter>-2</alter>" << std::endl;
        } else if (acc == Accidentals::Flat) {
            str << "\t\t\t\t\t<alter>-1</alter>" << std::endl;
        } else if (acc == Accidentals::Sharp) {
            str << "\t\t\t\t\t<alter>1</alter>" << std::endl;
        } else if (acc == Accidentals::DoubleSharp) {
            str << "\t\t\t\t\t<alter>2</alter>" << std::endl;
        }

        int octave = pitch.getOctave( -1);
        str << "\t\t\t\t\t<octave>" << octave << "</octave>" << std::endl;

        str << "\t\t\t\t</pitch>" << std::endl;
    }

    // Since there's no way to provide the performance absolute time
    // for a note, there's also no point in providing the performance
    // duration, even though it might in principle be of interest
    str << "\t\t\t\t<duration>" << e->getNotationDuration() << "</duration>" << std::endl;

    if (!e->isa(Note::EventRestType)) {

        if (e->has(TIED_BACKWARD) &&
                e->get
                <Bool>(TIED_BACKWARD)) {
            str << "\t\t\t\t<tie type=\"stop\"/>" << std::endl;
        }
        if (e->has(TIED_FORWARD) &&
                e->get
                <Bool>(TIED_FORWARD)) {
            str << "\t\t\t\t<tie type=\"start\"/>" << std::endl;
        }

        // Incomplete: will RG ever use this?
        str << "\t\t\t\t<voice>" << "1" << "</voice>" << std::endl;
    }

    Note note = Note::getNearestNote(e->getNotationDuration());

    static const char *noteNames[] = {
        "64th", "32nd", "16th", "eighth", "quarter", "half", "whole", "breve"
    };

    int noteType = note.getNoteType();
    if (noteType < 0 || noteType >= int(sizeof(noteNames) / sizeof(noteNames[0]))) {
        std::cerr << "WARNING: MusicXmlExporter::writeNote: bad note type "
        << noteType << std::endl;
        noteType = 4;
    }

    str << "\t\t\t\t<type>" << noteNames[noteType] << "</type>" << std::endl;
    for (int i = 0; i < note.getDots(); ++i) {
        str << "\t\t\t\t<dot/>" << std::endl;
    }

    if (!e->isa(Note::EventRestType)) {

        if (processedDisplayAcc == Accidentals::DoubleFlat) {
            str << "\t\t\t\t<accidental>flat-flat</accidental>" << std::endl;
        } else if (processedDisplayAcc == Accidentals::Flat) {
            str << "\t\t\t\t<accidental>flat</accidental>" << std::endl;
        } else if (processedDisplayAcc == Accidentals::Natural) {
            str << "\t\t\t\t<accidental>natural</accidental>" << std::endl;
        } else if (processedDisplayAcc == Accidentals::Sharp) {
            str << "\t\t\t\t<accidental>sharp</accidental>" << std::endl;
        } else if (processedDisplayAcc == Accidentals::DoubleSharp) {
            str << "\t\t\t\t<accidental>double-sharp</accidental>" << std::endl;
        }

        bool haveNotations = false;
        if (e->has(TIED_BACKWARD) &&
                e->get
                <Bool>(TIED_BACKWARD)) {
            if (!haveNotations) {
                str << "\t\t\t\t<notations>" << std::endl;
                haveNotations = true;
            }
            str << "\t\t\t\t\t<tied type=\"stop\"/>" << std::endl;
        }
        if (e->has(TIED_FORWARD) &&
                e->get
                <Bool>(TIED_FORWARD)) {
            if (!haveNotations) {
                str << "\t\t\t\t<notations>" << std::endl;
                haveNotations = true;
            }
            str << "\t\t\t\t\t<tied type=\"start\"/>" << std::endl;
        }
        if (haveNotations) {
            str << "\t\t\t\t</notations>" << std::endl;
        }
    }

    // could also do <stem>down</stem> if you wanted
    str << "\t\t\t</note>" << std::endl;
}

void
MusicXmlExporter::writeKey(Rosegarden::Key whichKey, std::ofstream &str)
{
    str << "\t\t\t\t<key>" << std::endl;
    str << "\t\t\t\t<fifths>"
        << (whichKey.isSharp() ? "" : "-")
        << (whichKey.getAccidentalCount()) << "</fifths>" << std::endl;
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
MusicXmlExporter::writeTime(TimeSignature timeSignature, std::ofstream &str)
{
    str << "\t\t\t\t<time>" << std::endl;
    str << "\t\t\t\t<beats>" << timeSignature.getNumerator() << "</beats>" << std::endl;
    str << "\t\t\t\t<beat-type>" << timeSignature.getDenominator() << "</beat-type>" << std::endl;
    str << "\t\t\t\t</time>" << std::endl;
}

void
MusicXmlExporter::writeClef(Clef whichClef, std::ofstream &str)
{
    str << "\t\t\t\t<clef>" << std::endl;
    if (whichClef == Clef::Treble) {
        str << "\t\t\t\t<sign>G</sign>" << std::endl;
        str << "\t\t\t\t<line>2</line>" << std::endl;
    } else if (whichClef == Clef::Alto) {
        str << "\t\t\t\t<sign>C</sign>" << std::endl;
        str << "\t\t\t\t<line>3</line>" << std::endl;
    } else if (whichClef == Clef::Tenor) {
        str << "\t\t\t\t<sign>C</sign>" << std::endl;
        str << "\t\t\t\t<line>4</line>" << std::endl;
    } else if (whichClef == Clef::Bass) {
        str << "\t\t\t\t<sign>F</sign>" << std::endl;
        str << "\t\t\t\t<line>4</line>" << std::endl;
    }
    str << "\t\t\t\t</clef>" << std::endl;
}

std::string
MusicXmlExporter::numToId(int num)
{
    int base = num % 52;
    char c;
    if (base < 26) c = 'A' + char(base);
    else c = 'a' + char(base - 26);
    std::string s;
    s += c;
    while (num / 52 > 0) {
        s += c;
        num /= 52;
    }
    return s;
}

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
    str << "<!DOCTYPE score-partwise PUBLIC \"-//Recordare//DTD MusicXML 1.1 Partwise//EN\" \"http://www.musicxml.org/dtds/partwise.dtd\">" << std::endl;
    // MusicXml header information
    str << "<score-partwise>" << std::endl;
    str << "\t<work> <work-title>" << XmlExportable::encode(m_fileName)
        << "</work-title></work> " << std::endl;
    // Movement, etc. info goes here
    str << "\t<identification> " << std::endl;
    if (composition->getCopyrightNote() != "") {
        str << "\t\t<rights>"
        << XmlExportable::encode(composition->getCopyrightNote())
        << "</rights>" << std::endl;
    }
    str << "\t\t<encoding>" << std::endl;
    // Incomplete: Insert date!
    //    str << "\t\t\t<encoding-date>" << << "</encoding-date>" << std::endl;
    str << "\t\t\t<software>Rosegarden v" VERSION "</software>" << std::endl;
    str << "\t\t</encoding>" << std::endl;
    str << "\t</identification> " << std::endl;

    // MIDI information
    str << "\t<part-list>" << std::endl;
    Composition::trackcontainer& tracks = composition->getTracks();

    int trackNo = 0;
    timeT lastNoteTime = -1;

    for (Composition::trackiterator i = tracks.begin();
            i != tracks.end(); ++i) {
        // Incomplete: What about all the other Midi stuff?
        // Incomplete: (Future) GUI to set labels if they're not already
        Instrument * trackInstrument = (&m_doc->getStudio())->getInstrumentById((*i).second->getInstrument());
        str << "\t\t<score-part id=\"" << numToId((*i).first) << "\">" << std::endl;
        str << "\t\t\t<part-name>" << XmlExportable::encode((*i).second->getLabel()) << "</part-name>" << std::endl;
        if (trackInstrument) {
/*
  Removing this stuff for now.  It doesn't work, because the ids are
  are expected to be non-numeric names that refer to elements
  elsewhere that define the actual instruments.  I think.

            str << "\t\t\t<score-instrument id=\"" << trackInstrument->getName() << "\">" << std::endl;
            str << "\t\t\t\t<instrument-name>" << trackInstrument->getType() << "</instrument-name>" << std::endl;
            str << "\t\t\t</score-instrument>" << std::endl;
            str << "\t\t\t<midi-instrument id=\"" << trackInstrument->getName() << "\">" << std::endl;
            str << "\t\t\t\t<midi-channel>" << ((unsigned int)trackInstrument->getMidiChannel() + 1) << "</midi-channel>" << std::endl;
            if (trackInstrument->sendsProgramChange()) {
                str << "\t\t\t\t<midi-program>" << ((unsigned int)trackInstrument->getProgramChange() + 1) << "</midi-program>" << std::endl;
            }
            str << "\t\t\t</midi-instrument>" << std::endl;
*/
        }
        str << "\t\t</score-part>" << std::endl;

        emit setProgress(int(double(trackNo++) / double(tracks.size()) * 20.0));
        rgapp->refreshGUI(50);

    } // end track iterator
    str << "\t</part-list>" << std::endl;

    // Notes!
    // Write out all segments for each Track
    trackNo = 0;

    for (Composition::trackiterator j = tracks.begin();
            j != tracks.end(); ++j) {

        bool startedPart = false;

        // Code courtesy docs/code/iterators.txt
        CompositionTimeSliceAdapter::TrackSet trackSet;

        // Incomplete: get the track info for each track (i.e. this should
        // be in an iterator loop) into the track set
        trackSet.insert((*j).first);
        CompositionTimeSliceAdapter adapter(composition, trackSet);

        int oldMeasureNumber = -1;
        bool startedAttributes = false;
        Rosegarden::Key key;
        Clef clef;
        AccidentalTable accTable(key, clef);
        TimeSignature prevTimeSignature;

        bool timeSigPending = false;
        bool keyPending = false;
        bool clefPending = false;

        for (CompositionTimeSliceAdapter::iterator k = adapter.begin();
                k != adapter.end(); ++k) {

            Event *event = *k;
            timeT absoluteTime = event->getNotationAbsoluteTime();

            if (!startedPart) {
                str << "\t<part id=\"" << numToId((*j).first) << "\">" << std::endl;
                startedPart = true;
            }

            // Open a new measure if necessary
            // Incomplete: How does MusicXML handle non-contiguous measures?

            int measureNumber = composition->getBarNumber(absoluteTime);

            TimeSignature timeSignature = composition->getTimeSignatureAt(absoluteTime);

            if (measureNumber != oldMeasureNumber) {

                if (startedAttributes) {
                    
                    // rather bizarrely, MusicXML appears to require
                    // key, time, clef in that order

                    if (keyPending) {
                        writeKey(key, str);
                        keyPending = false;
                    }
                    if (timeSigPending) {
                        writeTime(prevTimeSignature, str);
                        timeSigPending = false;
                    }
                    if (clefPending) {
                        writeClef(clef, str);
                        clefPending = false;
                    }

                    str << "\t\t\t</attributes>" << std::endl;
                    startedAttributes = false;
                }

                while (measureNumber > oldMeasureNumber) {

                    bool first = (oldMeasureNumber < 0);

                    if (!first) {
                        if (startedAttributes) {
                            str << "\t\t\t</attributes>" << std::endl;
                        }                            
                        str << "\t\t</measure>\n" << std::endl;
                    }

                    ++oldMeasureNumber;

                    str << "\t\t<measure number=\"" << (oldMeasureNumber + 1) << "\">" << std::endl;

                    if (first) {
                        str << "\t\t\t<attributes>" << std::endl;
                        // Divisions is divisions of crotchet (quarter-note) on which all
                        // note-lengths are based
                        str << "\t\t\t\t<divisions>" << Note(Note::Crotchet).getDuration() << "</divisions>" << std::endl;
                        startedAttributes = true;
                        timeSigPending = true;
                    }
                }

                accTable = AccidentalTable(key, clef);
            }

            oldMeasureNumber = measureNumber;

            if (timeSignature != prevTimeSignature) {
                prevTimeSignature = timeSignature;
                timeSigPending = true;
                if (!startedAttributes) {
                    str << "\t\t\t<attributes>" << std::endl;
                    startedAttributes = true;
                }
            }

            // process event
            if (event->isa(Rosegarden::Key::EventType)) {

                if (!startedAttributes) {
                    str << "\t\t\t<attributes>" << std::endl;
                    startedAttributes = true;
                }
                key = Rosegarden::Key(*event);
                keyPending = true;
                accTable = AccidentalTable(key, clef);

            } else if (event->isa(Clef::EventType)) {

                if (!startedAttributes) {
                    str << "\t\t\t<attributes>" << std::endl;
                    startedAttributes = true;
                }
                clef = Clef(*event);
                clefPending = true;
                accTable = AccidentalTable(key, clef);

            } else if (event->isa(Note::EventRestType) ||
                       event->isa(Note::EventType)) {
                
                if (startedAttributes) {
                
                    if (keyPending) {
                        writeKey(key, str);
                        keyPending = false;
                    }
                    if (timeSigPending) {
                        writeTime(prevTimeSignature, str);
                        timeSigPending = false;
                    }
                    if (clefPending) {
                        writeClef(clef, str);
                        clefPending = false;
                    }

                    str << "\t\t\t</attributes>" << std::endl;
                    startedAttributes = false;
                }

                writeNote(event, lastNoteTime, accTable, clef, key, str);

                if (event->isa(Note::EventType)) {
                    lastNoteTime = event->getNotationAbsoluteTime();
                } else if (event->isa(Note::EventRestType)) {
                    lastNoteTime = -1;
                }
            }
        }

        if (startedPart) {
            if (startedAttributes) {
                
                if (keyPending) {
                    writeKey(key, str);
                    keyPending = false;
                }
                if (timeSigPending) {
                    writeTime(prevTimeSignature, str);
                    timeSigPending = false;
                }
                if (clefPending) {
                    writeClef(clef, str);
                    clefPending = false;
                }
                
                str << "\t\t\t</attributes>" << std::endl;
                startedAttributes = false;
            }

            str << "\t\t</measure>" << std::endl;
            str << "\t</part>" << std::endl;
        }

        emit setProgress(20 +
                         int(double(trackNo++) / double(tracks.size()) * 80.0));
        rgapp->refreshGUI(50);
    }

    str << "</score-partwise>" << std::endl;
    str.close();
    return true;
}

}
