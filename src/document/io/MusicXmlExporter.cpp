/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    This file is Copyright 2002
        Hans Kieserman      <hkieserman@mail.com>
    with heavy lifting from csoundio as it was on 13/5/2002.

    More or less complete rewrite (Aug 2011)
        Niek van den Berg   <niekjvandenberg@gmail.com>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MusicXmlExporter]"

#include "MusicXmlExporter.h"

#include "misc/ConfigGroups.h"
#include "misc/Debug.h"
#include "base/StaffExportTypes.h"

#include "rosegarden-version.h"

#include <QSettings>

#include <sstream>
#include <iostream>

namespace Rosegarden
{

using namespace BaseProperties;

MusicXmlExporter::MidiInstrument::
MidiInstrument(Instrument * instrument, int pitch) :
    channel(instrument->hasFixedChannel() ?
            (int(instrument->getNaturalChannel()) + 1) :
            -1),
    program(int(instrument->getProgramChange()) + 1),
    unpitched(pitch)
{ }

MusicXmlExporter::MusicXmlExporter(RosegardenMainWindow *parent,
                                   RosegardenDocument *doc,
                                   std::string fileName) :
        ProgressReporter(parent),
        m_doc(doc),
        m_fileName(fileName)
{
    m_composition = &m_doc->getComposition();
    m_view = parent->getView();
    readConfigVariables();
}

MusicXmlExporter::~MusicXmlExporter()
{
    // nothing
}

void
MusicXmlExporter::readConfigVariables(void)
{
    // grab settings info
    QSettings settings;

    settings.beginGroup(NotationOptionsConfigGroup);
    int accOctaveMode = settings.value("accidentaloctavemode", 1).toInt() ;
    m_octaveType =
        (accOctaveMode == 0 ? AccidentalTable::OctavesIndependent :
         accOctaveMode == 1 ? AccidentalTable::OctavesCautionary :
         AccidentalTable::OctavesEquivalent);

    int accBarMode = settings.value("accidentalbarmode", 0).toInt() ;
    m_barResetType =
        (accBarMode == 0 ? AccidentalTable::BarResetNone :
         accBarMode == 1 ? AccidentalTable::BarResetCautionary :
         AccidentalTable::BarResetExplicit);
    settings.endGroup();

    settings.beginGroup(MusicXMLExportConfigGroup);
    m_exportSelection = settings.value("mxmlexportselection",
                                       EXPORT_NONMUTED_TRACKS).toUInt() ;
    m_mxmlDTDType = settings.value("mxmldtdtype", DTD_PARTWISE).toUInt() ;
    m_MusicXmlVersion = settings.value("mxmlversion",
                                       MUSICXML_VERSION_1_1).toUInt() ;
    m_multiStave = settings.value("mxmlmultistave", MULTI_STAVE_NONE).toUInt();
    m_exportStaffBracket = settings.value("mxmlexportstaffgroup", 0).toBool();
    m_exportPercussion = settings.value("mxmlexportpercussion", 0).toUInt();
    m_mxmlUseOctaveShift = settings.value("mxmluseoctaveshift", 0).toBool();

    settings.endGroup();
}

bool
MusicXmlExporter::isPercussionTrack(Track *track)
{
    bool percussion = false;
    Instrument *instrument =  m_doc->getStudio().getInstrumentFor(track);
    if (instrument) {
        percussion = instrument->isPercussion();

        if (percussion && (m_exportPercussion == EXPORT_PERCUSSION_AS_NOTES)) {
            percussion = false;
        }
    }
    return percussion;
}

bool
MusicXmlExporter::exportTrack(Track *track)
{
    if (track->getInstrument() < MidiInstrumentBase) {
        // Not a MIDI track!
        return false;
    }

    bool percussion = isPercussionTrack(track);
    if (percussion && ! m_exportPercussion) {
        // Percussion track but don't export percussion tracks!
        return false;
    }

    // hasSegments was never used.
    // bool hasSegments = false;
    // for (Composition::iterator s = m_composition->begin();
    //      s != m_composition->end(); ++s) {
    //     if ((*s)->getTrack() == track->getId()) {
    //         hasSegments = true;
    //         break;
    //     }
    // }

    if (m_exportSelection == EXPORT_ALL_TRACKS) {
        // Obvious.
        return true;
    }

    if (m_exportSelection == EXPORT_NONMUTED_TRACKS) {
        // Is track muted?
        return !track->isMuted();
    }

    if (m_exportSelection == EXPORT_SELECTED_TRACK) {
        return track->getId() == m_composition->getSelectedTrack();
    }

    if (m_exportSelection == EXPORT_SELECTED_SEGMENTS) {
        //
        // Check whather the track contains selected segments. If there are
        // no selected segments, skip the track.
        //
        bool selectedSegments = false;
        if ((m_view != NULL) && (m_view->haveSelection())) {
            //
            // Check whether the current segment is in the list of selected segments.
            //
            SegmentSelection selection = m_view->getSelection();
            for (SegmentSelection::iterator it = selection.begin(); it != selection.end(); ++it) {
                if ((*it)->getTrack() == track->getId()) {
                    selectedSegments = true;
                    break;
                }
            }
        }
        return selectedSegments;
    }

    return false;
}

void
MusicXmlExporter::writeHeader(std::ostream &str)
{
    Configuration metadata = m_composition->getMetadata();

    //! NOTE Is the usage of work/movement/credits correct???
    if (metadata.has("title")) {
        str << "  <work>" << std::endl;
        str << "    <work-title>" << XmlExportable::encode(metadata.get<String>("title"))
            << "</work-title>" << std::endl;
        str << "  </work>" << std::endl;
    }

    if (metadata.has("subtitle")) {
        str << "  <movement-title>"
            << "    " << XmlExportable::encode(metadata.get<String>("subtitle"))
            << "  </movement-title>" << std::endl;
    }

    str << "  <identification>" << std::endl;

    if (metadata.has("composer")) {
        str << "    <creator type=\"composer\">"
            << XmlExportable::encode(metadata.get<String>("composer"))
            << "</creator>" << std::endl;
    }
    if (metadata.has("poet")) {
        str << "    <creator type=\"lyricist\">"
            << XmlExportable::encode(metadata.get<String>("poet"))
            << "</creator>" << std::endl;
    }
    if (metadata.has("arranger")) {
        str << "    <creator type=\"arranger\">"
            << XmlExportable::encode(metadata.get<String>("arranger"))
            << "</creator>" << std::endl;
    }
    if (m_composition->getCopyrightNote() != "") {
        str << "    <rights>"
            << XmlExportable::encode(m_composition->getCopyrightNote())
            << "</rights>" << std::endl;
    }

    str << "    <encoding>" << std::endl;
    str << "      <software>Rosegarden v" VERSION "</software>" << std::endl;
    str << "      <encoding-date>"
        << QDateTime::currentDateTime().toString("yyyy-MM-dd")
        << "</encoding-date>" << std::endl;
    str << "    </encoding>" << std::endl;
    str << "  </identification>" << std::endl;
}

MusicXmlExportHelper*
MusicXmlExporter::initalisePart(timeT compositionEndTime, int curTrackPos,
                                   bool &exporting, bool &retValue)
{
    TrackVector tracks;
    std::string name;
    Track *track = 0;
    Track *curTrack = 0;
    bool inMultiStaffGroup = false;
    InstrumentId instrument = 0;
    bool found = false;
    retValue = false;
    exporting = false;

    // For each track
    for (int trackPos = 0;
         (track = m_composition->getTrackByPosition(trackPos)) != 0; ++trackPos) {
        qApp->processEvents();

        if (trackPos == curTrackPos) curTrack = track;
        if (!inMultiStaffGroup) {
            if (((m_multiStave == MULTI_STAVE_CURLY) &&
                        (track->getStaffBracket() == Brackets::CurlyOn)) ||
                ((m_multiStave == MULTI_STAVE_CURLY_SQUARE) &&
                        ((track->getStaffBracket() == Brackets::CurlyOn) ||
                         (track->getStaffBracket() == Brackets::CurlySquareOn)))) {
                inMultiStaffGroup = true;
                instrument = track->getInstrument();
            }
        }
        if (inMultiStaffGroup) {
            if (instrument == track->getInstrument()) {
                if (exportTrack(track)) {
                    tracks.push_back(track->getId());
                    if (trackPos == curTrackPos) {
                        found = true;
                        if (tracks.size() == 1) {
                            std::stringstream id;
                            id << "P" << curTrack->getId();
                            name = id.str();
                            retValue = false;
                        } else
                            retValue = true;
                    }
                }
            }
            if (((m_multiStave == MULTI_STAVE_CURLY) &&
                        (track->getStaffBracket() == Brackets::CurlyOff)) ||
                ((m_multiStave == MULTI_STAVE_CURLY_SQUARE) &&
                        ((track->getStaffBracket() == Brackets::CurlyOff) ||
                         (track->getStaffBracket() == Brackets::CurlySquareOff)))) {
                inMultiStaffGroup = false;
                if (found) {
                    exporting = exportTrack(curTrack);
                    return new MusicXmlExportHelper(name, tracks, isPercussionTrack(track),
                                            m_exportSelection == EXPORT_SELECTED_SEGMENTS,
                                            compositionEndTime, m_composition, m_view,
                                            m_octaveType, m_barResetType);
                }
                else {
                    tracks.clear();
                }
            }
        }
    }
    retValue = true;
    if ((exporting = exportTrack(curTrack))) {
        std::stringstream id;
        id << "P" << curTrack->getId();
        name = id.str();
        tracks.push_back(curTrack->getId());
        retValue = false;
    }
    return new MusicXmlExportHelper(name, tracks, isPercussionTrack(curTrack),
                            m_exportSelection == EXPORT_SELECTED_SEGMENTS,
                            compositionEndTime, m_composition, m_view,
                            m_octaveType, m_barResetType);
}

MusicXmlExporter::PartsVector
MusicXmlExporter::writeScorePart(timeT compositionEndTime, std::ostream &str)
{
    std::string squareOpen  = "    <part-group type=\"start\" number=\"1\">\n"
                              "      <group-symbol>bracket</group-symbol>\n"
                              "      <group-barline>yes</group-barline>\n"
                              "    </part-group>\n";
    std::string squareClose = "    <part-group type=\"stop\" number=\"1\"/>\n";
    std::string curlyOpen  = "    <part-group type=\"start\" number=\"2\">\n"
                             "      <group-symbol>brace</group-symbol>\n"
                             "      <group-barline>yes</group-barline>\n"
                             "    </part-group>\n";
    std::string curlyClose = "    <part-group type=\"stop\" number=\"2\"/>\n";

    PartsVector parts;

    str << "  <part-list>" << std::endl;

    int writeSquareOpen = 0;
    int writeSquareClose = 0;
    int writeCurlyOpen = 0;
    int writeCurlyClose = 0;
    Track *track = 0;
    // For each Track in the Composition
    for (int trackPos = 0;
         (track = m_composition->getTrackByPosition(trackPos)) != 0; ++trackPos) {
        qApp->processEvents();

        bool exporting = false;
        bool inMultiStaffGroup = false;
        MusicXmlExportHelper* ctx = initalisePart(compositionEndTime, trackPos,
                                             exporting, inMultiStaffGroup);
        ctx->setUseOctaveShift(m_mxmlUseOctaveShift);
        if (m_exportStaffBracket) {
            switch (track->getStaffBracket()) {

            case Brackets::SquareOn:
                if(writeSquareOpen == 0) writeSquareOpen = 1;
                break;

            case Brackets::SquareOff:
                if(writeSquareOpen > 1) writeSquareClose = 1;
                else writeSquareOpen = 0;
                break;

            case Brackets::SquareOnOff:
                if (exporting) {
                    if(writeSquareOpen  == 0) writeSquareOpen = 1;
                    if(writeSquareClose == 0) writeSquareClose = 1;
                }
                break;

            case Brackets::CurlyOn:
                if ((writeCurlyOpen == 0) && !ctx->isMultiStave()) writeCurlyOpen = 1;
                break;

            case Brackets::CurlyOff:
                if ((writeCurlyOpen > 1) && !ctx->isMultiStave()) writeCurlyClose = 1;
                else writeCurlyOpen = 0;
                break;

            case Brackets::CurlySquareOn:
                if (writeSquareOpen == 0) writeSquareOpen = 1;
                if ((writeCurlyOpen == 0) && !ctx->isMultiStave()) writeCurlyOpen = 1;
                break;

            case Brackets::CurlySquareOff:
                if (writeSquareOpen > 1) writeSquareClose = 1;
                else writeSquareOpen = 0;
                if ((writeCurlyOpen > 1) && !ctx->isMultiStave()) writeCurlyClose = 1;
                else writeCurlyOpen = 0;
                break;
            }
        }

        if (exporting) {
            if (writeCurlyOpen == 1) {
                str << curlyOpen;
                writeCurlyOpen = 2;
            }
            if (writeSquareOpen == 1) {
                str << squareOpen;
                writeSquareOpen = 2;
            }
        }

        if (!inMultiStaffGroup) {
            str << "    <score-part id=\"P" << track->getId() << "\">" << std::endl;
            str << "      <part-name>" << track->getLabel() << "</part-name>" << std::endl;

            Instrument *instrument = m_doc->getStudio().getInstrumentFor(track);
            if (instrument != NULL) {
                InstrumentMap instruments;
                if (isPercussionTrack(track)) {
                    // For each Segment in the Composition
                    for (Composition::iterator s = m_composition->begin();
                         s != m_composition->end(); ++s) {
                        if ((*s)->getTrack() != track->getId()) continue;
                        // For each Event in the Segment
                        for (Segment::iterator e = (*s)->begin();
                            e != (*s)->end(); ++e) {
                            qApp->processEvents();
                            if ((*e)->isa(Rosegarden::Note::EventType)) {
                                int pitch = (*e)->get<Int>(BaseProperties::PITCH);
                                std::stringstream id;
                                id << "P" << track->getId() << "-I" << pitch+1;
                                InstrumentMap::iterator i = instruments.find(id.str());
                                if ((i == instruments.end()) && instrument->getKeyMapping()) {
                                    std::string n = instrument->getKeyMapping()->getMapForKeyName(pitch);
                                    str << "      <score-instrument id=\"" << id.str() << "\">" << std::endl;
                                    str << "        <instrument-name>" << n << "</instrument-name>" << std::endl;
                                    str << "      </score-instrument>" << std::endl;

                                    MidiInstrument mi(instrument, pitch);
                                    instruments[id.str()] = mi;
                                }
                            }
                        }
                    }
                } else {
                    std::stringstream id;
                    id << "P" << track->getId() << "-I" << int(instrument->getId());
                    str << "      <score-instrument id=\"" << id.str() << "\">" << std::endl;
                    str << "        <instrument-name>" << ctx->getPartName() << "</instrument-name>" << std::endl;
                    str << "      </score-instrument>" << std::endl;
                    MidiInstrument mi(instrument, -1);
                    instruments[id.str()] = mi;
                }
                for (InstrumentMap::iterator i = instruments.begin();
                     i != instruments.end(); ++i) {
                    str << "      <midi-instrument id=\"" << (*i).first << "\">" << std::endl;
                    int channelMaybe = (*i).second.channel;
                    if (channelMaybe >= 0) {
                        str << "        <midi-channel>"
                            << channelMaybe
                            << "</midi-channel>"
                            << std::endl;
                    }
                    str << "        <midi-program>" << (*i).second.program << "</midi-program>" << std::endl;
                    if ((*i).second.unpitched >= 0) {
                        str << "        <midi-unpitched>" << (*i).second.unpitched+1 << "</midi-unpitched>" << std::endl;
                    }
                    str << "      </midi-instrument>" << std::endl;
                }
                ctx->setInstrumentCount(instruments.size());
            }
            str << "    </score-part>" << std::endl;
            parts.push_back(ctx);
        } else
            delete ctx;

        if (writeSquareClose == 1) {
            str << squareClose;
            writeSquareClose = 0;
            writeSquareOpen = 0;
        }
        if (writeCurlyClose == 1) {
            str << curlyClose;
            writeCurlyClose = 0;
            writeCurlyOpen = 0;
        }

    } // for (int trackPos = 0....
    str << "  </part-list>" << std::endl;
    return parts;
}

bool
MusicXmlExporter::write()
{
    std::ofstream str(m_fileName.c_str(), std::ios::out);
    if (!str) {
        std::cerr << "MusicXmlExporter::write() - can't write file " << m_fileName << std::endl;
        return false;
    }
    std::cerr << "writing MusicXML to " << m_fileName << std::endl;
    std::string version;
    switch (m_MusicXmlVersion) {

    case 0:
        version = "1.1";
        break;

    case 1:
        version = "2.0";
        break;

    default:
        version = "1.1";
        break;
    }

    // Find out the printed length of the composition
    timeT compositionStartTime = 0;
    timeT compositionEndTime = 0;
    for ( Composition::iterator i = m_composition->begin(); i != m_composition->end(); ++i) {

        // Allow some oportunities for user to cancel
        if (isOperationCancelled()) {
            return false;
        }

        if (compositionStartTime > (*i)->getStartTime()) {
            compositionStartTime = (*i)->getStartTime();
        }
        if (compositionEndTime < (*i)->getEndMarkerTime()) {
            compositionEndTime = (*i)->getEndMarkerTime();
        }
    }
    bool pickup = compositionStartTime < 0;

    if (m_mxmlDTDType == DTD_PARTWISE) {
        // XML header information
        str << "<?xml version=\"1.0\"?>" << std::endl;
        str << "<!DOCTYPE score-partwise PUBLIC \"-//Recordare//DTD MusicXML " << version
            << " Partwise//EN\" \"http://www.musicxml.org/dtds/partwise.dtd\">" << std::endl;
        str << "<score-partwise version=\"" << version << "\">" << std::endl;

        // Write the MusicXML header
        writeHeader(str);

        // ??? This takes a long time with large files.
        PartsVector parts = writeScorePart(compositionEndTime, str);
//         for (PartsVector::iterator c = parts.begin(); c != parts.end(); c++)
//             (*c)->printSummary();

        for (PartsVector::iterator c = parts.begin(); c != parts.end(); ++c) {
            str << "  <part id=\"" << (*c)->getPartName() << "\">" << std::endl;
            int bar = pickup ? -1 : 0;
            while (m_composition->getBarStart(bar) < compositionEndTime) {
                qApp->processEvents();

                // Allow some oportunities for user to cancel
                if (isOperationCancelled()) {return false;}

                emit setValue(int(double(bar) /
                              double(m_composition->getNbTracks()) * 100.0));

                str << "    <measure number=\"" << bar+1 << "\"";
                if (bar < 0) str << " implicit=\"yes\"";
                str << ">" << std::endl;
                (*c)->writeEvents(bar, str);
                str << "    </measure>" << std::endl;
                bar++;
            } // while (m_composition->getBarEnd(bar) < ...
            str << "  </part>" << std::endl;
        } // for (int trackPos = 0....
        str << "</score-partwise>" << std::endl;
        for (PartsVector::iterator c = parts.begin(); c != parts.end(); ++c)
            delete *c;
    } else {  // DTD_TIMEWISE
        // XML header information
        str << "<?xml version=\"1.0\"?>" << std::endl;
        str << "<!DOCTYPE score-timewise PUBLIC \"-//Recordare//DTD MusicXML " << version
            << " Timewise//EN\" \"http://www.musicxml.org/dtds/timewise.dtd\">" << std::endl;
        str << "<score-timewise version=\"" << version << "\">" << std::endl;

        // Write the MusicXML header
        writeHeader(str);

        // ??? This takes a long time with large files.
        PartsVector parts = writeScorePart(compositionEndTime, str);
//         for (PartsVector::iterator c = parts.begin(); c != parts.end(); c++)
//             (*c)->printSummary();

        int bar = 0;
        while (m_composition->getBarStart(bar) < compositionEndTime) {
            str << "  <measure number=\"" << bar+1 << "\">" << std::endl;
            for (PartsVector::iterator c = parts.begin(); c != parts.end(); ++c) {
                qApp->processEvents();

                // Allow some oportunities for user to cancel
                if (isOperationCancelled()) {return false;}

                emit setValue(int(double(bar) /
                              double(m_composition->getNbTracks()) * 100.0));

                str << "    <part id=\"" << (*c)->getPartName() << "\">" << std::endl;
                (*c)->writeEvents(bar, str);
                str << "    </part>" << std::endl;
            } // for (int trackPos = 0....
            str << "  </measure>" << std::endl;
            bar++;
        } // while (m_composition->getBarEnd(bar) < ...
        str << "</score-timewise>" << std::endl;
        for (PartsVector::iterator c = parts.begin(); c != parts.end(); ++c)
            delete *c;
    }
    str.close();
    std::cerr << "MusicXML generated.\n";
    return true;
}

}
