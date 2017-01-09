/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
  Rosegarden
  A MIDI and audio sequencer and musical notation editor.
  Copyright 2000-2017 the Rosegarden development team.
 
  This file is Copyright 2002
  Hans Kieserman      <hkieserman@mail.com>
  with heavy lifting from csoundio as it was on 13/5/2002.

  Numerous additions and bug fixes by
  Michael McIntyre    <dmmcintyr@users.sourceforge.net>

  Some restructuring by Chris Cannam.

  Massive brain surgery, fixes, improvements, and additions by
  Heikki Junes

  Other copyrights also apply to some parts of this work.  Please
  see the AUTHORS file and individual file headers for details.
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[LilyPondExporter]"

#include "LilyPondExporter.h"
#include "LilyPondSegmentsContext.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/Configuration.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/NotationTypes.h"
#include "base/PropertyName.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/Sets.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "base/NotationQuantizer.h"
#include "base/Marker.h"
#include "base/StaffExportTypes.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/application/RosegardenMainViewWidget.h"
#include "gui/editors/notation/NotationProperties.h"
#include "gui/editors/notation/NotationView.h"
#include "gui/editors/guitar/Chord.h"
#include "gui/general/ProgressReporter.h"

#include "rosegarden-version.h"

#include <QSettings>
#include <QMessageBox>
#include <QFileInfo>
#include <QObject>
#include <QRegExp>
#include <QString>
#include <QTextCodec>
#include <QApplication>

#include <sstream>
#include <algorithm>
#include <limits>

namespace Rosegarden
{

const char* headerDedication() { return "dedication"; }
const char* headerTitle() { return "title"; }
const char* headerSubtitle() { return "subtitle"; }
const char* headerSubsubtitle() { return "subsubtitle"; }
const char* headerPoet() { return "poet"; }
const char* headerComposer() { return "composer"; }
const char* headerMeter() { return "meter"; }
const char* headerOpus() { return "opus"; }
const char* headerArranger() { return "arranger"; }
const char* headerInstrument() { return "instrument"; }
const char* headerPiece() { return "piece"; }
const char* headerCopyright() { return "copyright"; }
const char* headerTagline() { return "tagline"; }

using namespace BaseProperties;

LilyPondExporter::LilyPondExporter(RosegardenDocument *doc,
                                   const SegmentSelection &selection,
                                   const std::string &fileName,
                                   NotationView *parent) :
    ProgressReporter(parent),
    m_doc(doc),
    m_fileName(fileName),
    m_lastClefFound(Clef::Treble),
    m_selection(selection),
    SKIP_PROPERTY("LilyPondExportSkipThisEvent")
{
    m_composition = &m_doc->getComposition();
    m_studio = &m_doc->getStudio();
    m_notationView = parent;

    readConfigVariables();
    m_language = LilyPondLanguage::create(m_exportNoteLanguage);
}

void
LilyPondExporter::readConfigVariables(void)
{
    // grab settings info
    QSettings settings;
    settings.beginGroup(LilyPondExportConfigGroup);

    m_paperSize = settings.value("lilypapersize", PAPER_A4).toUInt();
    m_paperLandscape = qStrToBool(settings.value("lilypaperlandscape", "false"));
    m_fontSize = settings.value("lilyfontsize", FONT_20).toUInt();
    m_raggedBottom = qStrToBool(settings.value("lilyraggedbottom", "false"));
    m_exportEmptyStaves = qStrToBool(settings.value("lilyexportemptystaves", "false"));
    m_useShortNames = qStrToBool(settings.value("lilyuseshortnames", "true"));
    m_exportSelection = settings.value("lilyexportselection", EXPORT_NONMUTED_TRACKS).toUInt();
    if (settings.value("lilyexporteditedsegments", "false").toBool()) {
        m_exportSelection = EXPORT_EDITED_SEGMENTS;
    }
    m_exportLyrics = settings.value("lilyexportlyrics", EXPORT_LYRICS_LEFT).toUInt();
    m_exportTempoMarks = settings.value("lilyexporttempomarks", EXPORT_NONE_TEMPO_MARKS).toUInt();
    m_exportBeams = qStrToBool(settings.value("lilyexportbeamings", "false"));
    m_exportStaffGroup = qStrToBool(settings.value("lilyexportstaffbrackets", "true"));

    m_languageLevel = settings.value("lilylanguage", LILYPOND_VERSION_2_12).toUInt();
    m_exportMarkerMode = settings.value("lilyexportmarkermode", EXPORT_NO_MARKERS).toUInt();
    m_exportNoteLanguage = settings.value("lilyexportnotelanguage", LilyPondLanguage::NEDERLANDS).toUInt();
    m_chordNamesMode = qStrToBool(settings.value("lilychordnamesmode", "false"));
//    m_repeatMode = settings.value("lilyrepeatmode", REPEAT_BASIC).toUInt();
    m_repeatMode = settings.value("lilyexportrepeat", "true").toBool() ? REPEAT_VOLTA : REPEAT_UNFOLD;
    m_voltaBar = settings.value("lilydrawbaratvolta", "true").toBool();
    m_cancelAccidentals = settings.value("lilycancelaccidentals", "false").toBool();
    m_fingeringsInStaff = settings.value("lilyfingeringsinstaff", "true").toBool();
    settings.endGroup();
}

// Return true if @p event is allowed to start or end a beam group.
// Only called for GROUP_TYPE_BEAMED.
static bool canStartOrEndBeam(Event *event)
{
    // Rests cannot start or end beams
    if (!event->isa(Note::EventType))
        return false;

    // Is it really beamed? quarter and longer notes cannot be
    // (ex: bug #1705430, beaming groups erroneous after merging notes)
    // HJJ: This should be fixed in notation engine,
    //      after which the workaround below should be removed.
    const int noteType = event->get<Int>(NOTE_TYPE);
    if (noteType >= Note::QuarterNote)
        return false;

    return true;
}

Event *LilyPondExporter::nextNoteInGroup(Segment *s, Segment::iterator it, const std::string &groupType, int barEnd) const
{
    Event *event = *it;
    long currentGroupId = -1;
    event->get<Int>(BEAMED_GROUP_ID, currentGroupId);
    Q_ASSERT(currentGroupId != -1);
    const bool tuplet = groupType == GROUP_TYPE_TUPLED;
    const bool graceNotesGroup = event->has(IS_GRACE_NOTE) && event->get<Bool>(IS_GRACE_NOTE);
    timeT currentTime = m_composition->getNotationQuantizer()->getQuantizedAbsoluteTime(event);
    int subOrdering = event->getSubOrdering();

    ++it;
    for ( ; s->isBeforeEndMarker(it) ; ++it ) {
        event = *it;

        if (event->getNotationAbsoluteTime() >= barEnd)
            break;

        // Grace notes shouldn't break the beaming group of real notes
        const bool isGrace = (event->has(IS_GRACE_NOTE) && event->get<Bool>(IS_GRACE_NOTE));
        if (!graceNotesGroup && isGrace)
            continue;

        if (event->has(SKIP_PROPERTY))
            continue;

        const bool isNote = event->isa(Note::EventType);

        // Rests at the end of a beam are not included into it.
        // Rests in the middle of a beam are included, so we keep looking.
        // Same thing for symbols, etc.
        if (!tuplet && !isNote)
            continue;
        // Tuplets don't get broken by non-notes (e.g. pitchbend)
        if (tuplet && (!isNote && !event->isa(Note::EventRestType)))
            continue;

        // Within a chord, keep moving ahead
        const timeT eventTime = m_composition->getNotationQuantizer()->getQuantizedAbsoluteTime(event);
        if (eventTime == currentTime && subOrdering == event->getSubOrdering()) {
            continue;
        }
        currentTime = eventTime;
        subOrdering = event->getSubOrdering();

        long newGroupId = -1;
        event->get<Int>(BEAMED_GROUP_ID, newGroupId);

        if (!tuplet && !canStartOrEndBeam(event)) {
            newGroupId = -1;
        }

        if (newGroupId == -1 || newGroupId != currentGroupId) {
            return 0;
        }
        return event;
    }
    return 0;
}

LilyPondExporter::~LilyPondExporter()
{
    delete(m_language);
}

bool
LilyPondExporter::isSegmentToPrint(Segment *seg)
{
    bool currentSegmentSelected = false;
    if ((m_exportSelection == EXPORT_SELECTED_SEGMENTS) && !m_selection.empty()) {
        //
        // Check whether the current segment is in the list of selected segments.
        //
        for (SegmentSelection::iterator it = m_selection.begin(); it != m_selection.end(); ++it) {
            if ((*it) == seg) currentSegmentSelected = true;
        }
    } else if ((m_exportSelection == EXPORT_EDITED_SEGMENTS) && (m_notationView != NULL)) {
        currentSegmentSelected = m_notationView->hasSegment(seg);
    }

    // Check whether the track is a non-midi track.
    Track *track = m_composition->getTrackById(seg->getTrack());
    InstrumentId instrumentId = track->getInstrument();
    bool isMidiTrack = instrumentId >= MidiInstrumentBase;

    // Look for various printing selection modes
    bool ok1 = m_exportSelection == EXPORT_ALL_TRACKS;
    bool ok2 = (m_exportSelection == EXPORT_NONMUTED_TRACKS) && (!track->isMuted());
    bool ok3 = (m_exportSelection == EXPORT_SELECTED_TRACK)
               && (track->getId() == m_composition->getSelectedTrack());
    bool ok4 = (m_exportSelection == EXPORT_SELECTED_SEGMENTS) && currentSegmentSelected;
    bool ok5 = (m_exportSelection == EXPORT_EDITED_SEGMENTS) && currentSegmentSelected;

    // Skip non-midi tracks and return true if segment is selected
    return isMidiTrack && (ok1 || ok2 || ok3 || ok4 || ok5);
}


void
LilyPondExporter::handleStartingPreEvents(eventstartlist &preEventsToStart,
                                          std::ofstream &str)
{
    eventstartlist::iterator m = preEventsToStart.begin();

    while (m != preEventsToStart.end()) {

        try {
            Indication i(**m);

            if (i.getIndicationType() == Indication::QuindicesimaUp) {
                str << "\\ottava #2 ";
            } else if (i.getIndicationType() == Indication::OttavaUp) {
                str << "\\ottava #1 ";
            } else if (i.getIndicationType() == Indication::OttavaDown) {
                str << "\\ottava #-1 ";
            } else if (i.getIndicationType() == Indication::QuindicesimaDown) {
                str << "\\ottava #-2 ";
            }

        } catch (Event::BadType) {
            // Not an indication
        } catch (Event::NoData e) {
            RG_WARNING << "Bad indication: " << e.getMessage();
        }

        eventstartlist::iterator n(m);
        ++n;
        preEventsToStart.erase(m);
        m = n;
    }
}

void
LilyPondExporter::handleStartingPostEvents(eventstartlist &postEventsToStart,
                                           std::ofstream &str)
{
    eventstartlist::iterator m = postEventsToStart.begin();

    while (m != postEventsToStart.end()) {

        try {
            Indication i(**m);

            if (i.getIndicationType() == Indication::Slur) {
                if ((*m)->get
                    <Bool>(NotationProperties::SLUR_ABOVE))
                    str << "^( ";
                else
                    str << "_( ";
            } else if (i.getIndicationType() == Indication::PhrasingSlur) {
                if ((*m)->get
                    <Bool>(NotationProperties::SLUR_ABOVE))
                    str << "^\\( ";
                else
                    str << "_\\( ";
            } else if (i.getIndicationType() == Indication::Crescendo) {
                str << "\\< ";
            } else if (i.getIndicationType() == Indication::Decrescendo) {
                str << "\\> ";
// Don't seem useful and sometimes are harmful
//             } else if (i.getIndicationType() == Indication::QuindicesimaUp) {
//                 // #(set-octavation 2) ... #(set-octavation 0)
//                 //str << "\\ottava #2 ";
//             } else if (i.getIndicationType() == Indication::OttavaUp) {
//                 // #(set-octavation 1) ... #(set-octavation 0)
//                 str << "\\ottava #1 ";
//             } else if (i.getIndicationType() == Indication::OttavaDown) {
//                 // #(set-octavation -1) ... #(set-octavation 0)
//                 str << "\\ottava #-1 ";
//             } else if (i.getIndicationType() == Indication::QuindicesimaDown) {
//                 // #(set-octavation -2) ... #(set-octavation 0)
//                 str << "\\ottava #-2 ";
            } else if (i.getIndicationType() == Indication::TrillLine) {
                str << "\\startTrillSpan ";
            }

        } catch (Event::BadType) {
            // Not an indication
            // Check for sustainDown or sustainUp events
            if ((*m)->isa(Controller::EventType) &&
                (*m)->has(Controller::NUMBER) &&
                (*m)->has(Controller::VALUE)) {
                if ((*m)->get <Int>(Controller::NUMBER) == 64) {
                    //
                    // As a first approximation, any positive value for
                    // the pedal event results in a new "Ped." marking.
                    //
                    // If the pedals have been entered with a midi piano,
                    // the pedal may have continuous values from 0 to 127
                    // and there may appear funny output with plenty of 
                    // "Ped." marks indicating the change of pedal pressure.
                    //
                    // One could use the following code to make the pedal
                    // marks transparent, but the invisible syntax has to
                    // be put before the note, while the pedal syntax goes
                    // after the note. Therefore, the following does not work:
                    //
                    //   c' \sustainUp \once \overr...#'transparent \sustainDown
                    //
                    // If a solution which allows to hide the pedal marks,
                    // the example code below which shows how to hide the marks
                    // can be removed.
                    //
                    /*
                     *if ((*m)->has(INVISIBLE) && (*m)->get <Bool>(INVISIBLE)) {
                     *    str << "\\once \\override Staff.SustainPedal #'transparent = ##t ";
                     *}
                     */

                    // NOTE: sustain syntax changed in LilyPond 2.12
                    if ((*m)->get <Int>(Controller::VALUE) > 0) {
                        str << "\\sustain" << (m_languageLevel < LILYPOND_VERSION_2_12 ? "Down " : "On ");
                    } else {
                        str << "\\sustain" << (m_languageLevel < LILYPOND_VERSION_2_12 ? "Up " : "Off ");
                    }
                }
            }
        } catch (Event::NoData e) {
            RG_WARNING << "Bad indication: " << e.getMessage();
        }

        eventstartlist::iterator n(m);
        ++n;
        postEventsToStart.erase(m);
        m = n;
    }
}

void
LilyPondExporter::handleEndingPreEvents(eventendlist &preEventsInProgress,
                                        const Segment::iterator &j,
                                        std::ofstream &str)
{
    eventendlist::iterator k = preEventsInProgress.begin();

    while (k != preEventsInProgress.end()) {

        // Increment before use.  This avoids invalidating k if the element
        // at l is erased.
        eventendlist::iterator l(k++);

        // Handle and remove all the relevant events in value()
        // This assumes all deferred events are indications

        try {
            Indication i(**l);

            timeT indicationEnd =
                (*l)->getNotationAbsoluteTime() + i.getIndicationDuration();
            timeT eventEnd =
                (*j)->getNotationAbsoluteTime() + (*j)->getNotationDuration();

            if (indicationEnd < eventEnd ||
                ((i.getIndicationType() == Indication::Slur ||
                  i.getIndicationType() == Indication::PhrasingSlur) &&
                 indicationEnd == eventEnd)) {

                if (i.getIndicationType() == Indication::QuindicesimaUp) {
                    str << "\\ottava #0 ";
                } else if (i.getIndicationType() == Indication::OttavaUp) {
                    str << "\\ottava #0 ";
                } else if (i.getIndicationType() == Indication::OttavaDown) {
                    str << "\\ottava #0 ";
                } else if (i.getIndicationType() == Indication::QuindicesimaDown) {
                    str << "\\ottava #0 ";
                }

                preEventsInProgress.erase(l);
            }

        } catch (Event::BadType) {
            // not an indication

        } catch (Event::NoData e) {
            RG_WARNING << "Bad indication: " << e.getMessage();
        }
    }
}

void
LilyPondExporter::handleEndingPostEvents(eventendlist &postEventsInProgress,
                                         const Segment::iterator &j,
                                         std::ofstream &str)
{
    eventendlist::iterator k = postEventsInProgress.begin();

    while (k != postEventsInProgress.end()) {

        // Increment before use.  This avoids invalidating k if the element
        // at l is erased.
        eventendlist::iterator l(k++);

        // Handle and remove all the relevant events in value()
        // This assumes all deferred events are indications

        try {
            Indication i(**l);

            timeT indicationEnd =
                (*l)->getNotationAbsoluteTime() + i.getIndicationDuration();
            timeT eventEnd =
                (*j)->getNotationAbsoluteTime() + (*j)->getNotationDuration();

            if (indicationEnd < eventEnd ||
                ((i.getIndicationType() == Indication::Slur ||
                  i.getIndicationType() == Indication::PhrasingSlur) &&
                 indicationEnd == eventEnd)) {

                if (i.getIndicationType() == Indication::Slur) {
                    str << ") ";
                } else if (i.getIndicationType() == Indication::PhrasingSlur) {
                    str << "\\) ";
                } else if (i.getIndicationType() == Indication::Crescendo ||
                           i.getIndicationType() == Indication::Decrescendo) {
                    str << "\\! ";
                } else if (i.getIndicationType() == Indication::TrillLine) {
                    str << "\\stopTrillSpan ";
                }

                postEventsInProgress.erase(l);
            }

        } catch (Event::BadType) {
            // not an indication

        } catch (Event::NoData e) {
            RG_WARNING << "Bad indication: " << e.getMessage();
        }
    }
}

std::string
LilyPondExporter::convertPitchToLilyNoteName(int pitch, Accidental accidental,
                                             const Rosegarden::Key &key)
{
    Pitch p(pitch, accidental);
    char noteName = (char)tolower(p.getNoteName(key));
    Accidental acc = p.getAccidental(key);
    std::string lilyNote = m_language->getLilyNote(noteName, acc);
    return lilyNote;
}

std::string
LilyPondExporter::convertPitchToLilyNote(int pitch, Accidental accidental,
                                         const Rosegarden::Key &key)
{
    // calculate note name and write note
    std::string lilyNote = convertPitchToLilyNoteName(pitch, accidental, key);

    // generate and write octave marks
    std::string octaveMarks = "";
    int octave = (int)(pitch / 12);

    // tweak the octave break for B# / Cb
    Pitch p(pitch, accidental);
    char noteName = (char)tolower(p.getNoteName(key));
    Accidental acc = p.getAccidental(key);
    if (noteName == 'b' &&
        (acc == Accidentals::Sharp || acc == Accidentals::DoubleSharp)) {
        octave--;
    } else if (noteName == 'c' &&
               (acc == Accidentals::Flat || acc == Accidentals::DoubleFlat)) {
        octave++;
    }

    if (octave < 4) {
        for (; octave < 4; octave++)
            octaveMarks += ",";
    } else {
        for (; octave > 4; octave--)
            octaveMarks += "\'";
    }

    lilyNote += octaveMarks;

    return lilyNote;
}

std::string
LilyPondExporter::composeLilyMark(std::string eventMark, bool stemUp)
{

    std::string inStr = "", outStr = "";
    std::string prefix = (stemUp) ? "_" : "^";

    // shoot text mark straight through unless it's sf or rf
    if (Marks::isTextMark(eventMark)) {
        inStr = protectIllegalChars(Marks::getTextFromMark(eventMark));

        if (inStr == "sf") {
            inStr = "\\sf";
        } else if (inStr == "rf") {
            inStr = "\\rfz";
        } else {
            inStr = "\\markup { \\italic " + inStr + " } ";
        }

        outStr = prefix + inStr;

    } else if (Marks::isFingeringMark(eventMark)) {

        // fingering marks: use markup syntax only for non-trivial fingerings

        inStr = protectIllegalChars(Marks::getFingeringFromMark(eventMark));

        if (inStr != "0" && inStr != "1" && inStr != "2" && inStr != "3" && inStr != "4" && inStr != "5" && inStr != "+") {
            inStr = "\\markup { \\finger \"" + inStr + "\" } ";
        }

        outStr = prefix + inStr;

    } else {
        outStr = "-";

        // use full \accent format for everything, even though some shortcuts
        // exist, for the sake of consistency
        if (eventMark == Marks::Accent) {
            outStr += "\\accent";
        } else if (eventMark == Marks::Tenuto) {
            outStr += "\\tenuto";
        } else if (eventMark == Marks::Staccato) {
            outStr += "\\staccato";
        } else if (eventMark == Marks::Staccatissimo) {
            outStr += "\\staccatissimo";
        } else if (eventMark == Marks::Marcato) {
            outStr += "\\marcato";
        } else if (eventMark == Marks::Open) {
            outStr += "\\open";
        } else if (eventMark == Marks::Stopped) {
            outStr += "\\stopped";
        } else if (eventMark == Marks::Harmonic) {
            outStr += "\\flageolet"; // flageolets are violin harmonics, apparently
        } else if (eventMark == Marks::Trill) {
            outStr += "\\trill";
        } else if (eventMark == Marks::LongTrill) {
            // span trill up to the next note:
            // tweak the beginning of the next note using an invisible rest having zero length
            outStr += "\\startTrillSpan s4*0 \\stopTrillSpan";
        } else if (eventMark == Marks::Turn) {
            outStr += "\\turn";
        } else if (eventMark == Marks::Pause) {
            outStr += "\\fermata";
        } else if (eventMark == Marks::UpBow) {
            outStr += "\\upbow";
        } else if (eventMark == Marks::DownBow) {
            outStr += "\\downbow";
        } else if (eventMark == Marks::Mordent) {
            outStr += "\\mordent";
        } else if (eventMark == Marks::MordentInverted) {
            outStr += "\\prall";
        } else if (eventMark == Marks::MordentLong) {
            outStr += "\\prallmordent";
        } else if (eventMark == Marks::MordentLongInverted) {
            outStr += "\\prallprall";
        } else {
            outStr = "";
            RG_WARNING << "LilyPondExporter::composeLilyMark() - unhandled mark: " << eventMark;
        }
    }

    return outStr;
}

std::string
LilyPondExporter::indent(const int &column)
{
    std::string outStr = "";
    for (int c = 1; c <= column; c++) {
        outStr += "    ";
    }
    return outStr;
}

std::string
LilyPondExporter::protectIllegalChars(std::string inStr)
{

    QString tmpStr = strtoqstr(inStr);

    tmpStr.replace(QRegExp("&"), "\\&");
    tmpStr.replace(QRegExp("\\^"), "\\^");
    tmpStr.replace(QRegExp("%"), "\\%");
    tmpStr.replace(QRegExp("<"), "\\<");
    tmpStr.replace(QRegExp(">"), "\\>");
    tmpStr.replace(QRegExp("\\["), "");
    tmpStr.replace(QRegExp("\\]"), "");
    tmpStr.replace(QRegExp("\\{"), "");
    tmpStr.replace(QRegExp("\\}"), "");
    tmpStr.replace(QRegExp("\""), "\\\"");

    //
    // LilyPond uses utf8 encoding.
    //
    return tmpStr.toUtf8().data();
}

struct MarkerComp {
    // Sort Markers by time
    // Perhaps this should be made generic with a template?
    bool operator()(Marker *a, Marker *b) { 
        return a->getTime() < b->getTime();
    }
};

bool
LilyPondExporter::write()
{
    m_warningMessage = "";
    QString tmpName = strtoqstr(m_fileName);

    // dmm - modified to act upon the filename itself, rather than the whole
    // path; fixes bug #855349

    // split name into parts:
    QFileInfo nfo(tmpName);
    QString dirName = nfo.path();
    QString baseName = nfo.fileName();

    // sed LilyPond-choking chars out of the filename proper
    bool illegalFilename = (baseName.contains(' ') || baseName.contains("\\"));
    baseName.replace(QRegExp(" "), "");
    baseName.replace(QRegExp("\\\\"), "");
    baseName.replace(QRegExp("'"), "");
    baseName.replace(QRegExp("\""), "");

    // cat back together
    tmpName = dirName + '/' + baseName;

    if (illegalFilename) {
        int reply = QMessageBox::question(
                dynamic_cast<QWidget*>(qApp),
                baseName, 
                QObject::tr("LilyPond does not allow spaces or backslashes in filenames.\n\n"
                            "Would you like to use\n\n %1\n\n instead?").arg(tmpName),
                QMessageBox::Yes |QMessageBox::Cancel,
                QMessageBox::Cancel);
        if (reply != QMessageBox::Yes)
            return false;
    }

    std::ofstream str(qstrtostr(tmpName).c_str(), std::ios::out);
    if (!str) {
        RG_WARNING << "LilyPondExporter::write() - can't write file " << tmpName;
        m_warningMessage = QObject::tr("Export failed.  The file could not be opened for writing.");
        return false;
    }

    str << "% This LilyPond file was generated by Rosegarden " << protectIllegalChars(VERSION) << std::endl;

    str << m_language->getImportStatement();

    switch (m_languageLevel) {

    case LILYPOND_VERSION_2_6:
        str << "\\version \"2.6.0\"" << std::endl;
        break;

    case LILYPOND_VERSION_2_8:
        str << "\\version \"2.8.0\"" << std::endl;
        break;

    case LILYPOND_VERSION_2_10:
        str << "\\version \"2.10.0\"" << std::endl;
        break;

    case LILYPOND_VERSION_2_12:
        str << "\\version \"2.12.0\"" << std::endl;
        break;

    case LILYPOND_VERSION_2_14:
        str << "\\version \"2.14.0\"" << std::endl;
        break;

    default:
        // force the default version if there was an error
        RG_WARNING << "ERROR: Unknown language level " << m_languageLevel
                  << ", using \\version \"2.14.0\" instead";
        str << "\\version \"2.14.0\"" << std::endl;
        m_languageLevel = LILYPOND_VERSION_2_14;
    }

    // LilyPond \header block

    // set indention level to make future changes to horizontal layout less
    // tedious, ++col to indent a new level, --col to de-indent
    int col = 0;

    // grab user headers from metadata
    Configuration metadata = m_composition->getMetadata();
    std::vector<std::string> propertyNames = metadata.getPropertyNames();

    // open \header section if there's metadata to grab, and if the user
    // wishes it
    if (!propertyNames.empty()) {
        str << "\\header {" << std::endl;
        col++;  // indent+

        bool userTagline = false;

        for (size_t index = 0; index < propertyNames.size(); ++index) {
            std::string property = propertyNames [index];
            if (property == headerDedication() || property == headerTitle() ||
                property == headerSubtitle() || property == headerSubsubtitle() ||
                property == headerPoet() || property == headerComposer() ||
                property == headerMeter() || property == headerOpus() ||
                property == headerArranger() || property == headerInstrument() ||
                property == headerPiece() || property == headerCopyright() ||
                property == headerTagline()) {
                std::string header = protectIllegalChars(metadata.get<String>(property));
                if (property == headerCopyright()) {
                    // replace a (c) or (C) with a real Copyright symbol
                    size_t posCpy = header.find("(c)");
                    if (posCpy == std::string::npos) posCpy = header.find("(C)");
                    if (posCpy != std::string::npos) {
                        std::string leftOfCpy = header.substr(0, posCpy);
                        std::string rightOfCpy = header.substr(posCpy + 3);
                        str << indent(col) << property << " =  \\markup { \"" << leftOfCpy << "\""
                            << "\\char ##x00A9" << "\"" << rightOfCpy << "\" }" << std::endl;
                    } else {
                        if (header != "") {
                            str << indent(col) << property << " = \""
                                << header << "\"" << std::endl;
                        }
                    }
                } else if (header != "") {
                    str << indent(col) << property << " = \"" << header << "\"" << std::endl;
                    // let users override defaults, but allow for providing
                    // defaults if they don't:
                    if (property == headerTagline())
                        userTagline = true;
                }
            }
        }

        // default tagline
        if (!userTagline) {
            str << indent(col) << "tagline = \""
                << "Created using Rosegarden " << protectIllegalChars(VERSION) << " and LilyPond"
                << "\"" << std::endl;
        }

        // close \header
        str << indent(--col) << "}" << std::endl;
    }

    // LilyPond \paper block (optional)
    if (m_raggedBottom) {
        str << indent(col) << "\\paper {" << std::endl;
        str << indent(++col) << "ragged-bottom=##t" << std::endl;
        str << indent(--col) << "}" << std::endl;
    }

    // LilyPond music data!   Mapping:
    // LilyPond Voice = Rosegarden Segment
    // LilyPond Staff = Rosegarden Track
    // (not the cleanest output but maybe the most reliable)

    // paper/font sizes
    int font = m_fontSize + FONT_OFFSET;
    str << indent(col) << "#(set-global-staff-size " << font << ")" << std::endl;

    // write user-specified paper type as default paper size
    std::string paper = "";
    switch (m_paperSize) {
    case PAPER_A3 :
        paper += "a3";
        break;
    case PAPER_A4 :
        paper += "a4";
        break;
    case PAPER_A5 :
        paper += "a5";
        break;
    case PAPER_A6 :
        paper += "a6";
        break;
    case PAPER_LEGAL :
        paper += "legal";
        break;
    case PAPER_LETTER :
        paper += "letter";
        break;
    case PAPER_TABLOID :
        paper += "tabloid";
        break;
    case PAPER_NONE :
        paper = "";
        break; // "do not specify"
    }
    if (paper != "") {
        str << indent(col) << "#(set-default-paper-size \"" << paper << "\"" 
            << (m_paperLandscape ? " 'landscape" : "") << ")"
            << std::endl;
    }

    // Define exceptions for ChordNames context: c:3
    if (m_chordNamesMode) {
        str << "chExceptionMusic = { <c e>-\\markup { \\super \"3\"} }" << std::endl;
        str << "chExceptions = #(append (sequential-music-to-chord-exceptions chExceptionMusic #t) ignatzekExceptions)" << std::endl;
    }

    // Find out the printed length of the composition
    Composition::iterator i = m_composition->begin();
    if ((*i) == NULL) {
        // The composition is empty!
        str << indent(col) << "\\score {" << std::endl;
        str << indent(++col) << "% no segments found" << std::endl;
        // bind staffs with or without staff group bracket
        str << indent(col) // indent
            << "<<" << " s4 " << ">>" << std::endl;
        str << indent(col) << "\\layout { }" << std::endl;
        str << indent(--col) << "}" << std::endl;
        m_warningMessage = QObject::tr("Export succeeded, but the composition was empty.");
        return false;
    }
    timeT compositionStartTime = (*i)->getStartTime();
    timeT compositionEndTime = (*i)->getEndMarkerTime();
    for (; i != m_composition->end(); ++i) {
        
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

    // Gather all segments in a place where it will be possible
    // to see the repetitions in a global context and to compute
    // the place of the different voices in the Lilypond score.
    LilyPondSegmentsContext lsc(this, m_composition);
    for (Composition::iterator i = m_composition->begin();
            i != m_composition->end(); ++i) {
        if (isSegmentToPrint(*i)) {
            lsc.addSegment(*i);
        }
    }

    // Don't continue if lsc is empty
    if (lsc.containsNoSegment()) {
        switch (m_exportSelection) {
          
            case EXPORT_ALL_TRACKS :
                // We should have already exited this method if the composition is empty 
                m_warningMessage = "No segments found while exporting all the"
                                   " tracks : THIS IS A BUG.";
                break;
                
            case EXPORT_NONMUTED_TRACKS :
                m_warningMessage = QObject::tr("Export of unmuted tracks failed.  There"
                                      " are no unmuted tracks or no segments on"
                                      " them.");
                break;
                
            case EXPORT_SELECTED_TRACK :
                m_warningMessage = QObject::tr("Export of selected track failed.  There"
                                      " are no segments on the selected track.");
                break;
                
            case EXPORT_SELECTED_SEGMENTS :
                m_warningMessage = QObject::tr("Export of selected segments failed.  No"
                                      " segments are selected.");
                break;
                
            case EXPORT_EDITED_SEGMENTS :
                // Notation editor can't be open without any segment inside
                m_warningMessage = "No segments found while exporting the"
                                   " edited segments : THIS IS A BUG.";
                break;
                
            default :
                m_warningMessage = "Abnormal m_exportSelection value :"
                                   " THIS IS A BUG.";
        }

        return false;
    }

    // Look for repeating segments
    lsc.precompute();

    // If needed, compute offsets of segments following a repeating one
    // in LilyPond score
    if (m_repeatMode == REPEAT_VOLTA) {
        lsc.fixRepeatStartTimes();
    }

    // If needed, compute offsets in LilyPond score of segments following
    // a repeat with volta coming from linked segments.
    //!!! TODO : Use an other switch than m_repeatMode 
    if (m_repeatMode == REPEAT_VOLTA) {
        lsc.fixVoltaStartTimes();
    }


    // If any segment is not starting at a bar boundary, adapted
    // \partial or \skip keywords must be added to the output file.
    // We have to know what segment is starting first to compute
    // such \partial and \skip parameters.
    // We can't rely on compositionStartTime to compute such data
    // because the first segment of the composition may be being not printed.
    // Following code is finding out the start time of the first segment
    // being printed.
    timeT firstSegmentStartTime = lsc.getFirstSegmentStartTime();


    // define global context which is common for all staffs
    str << indent(col++) << "global = { " << std::endl;
    TimeSignature timeSignature = m_composition->
        getTimeSignatureAt(m_composition->getStartMarker());

    int leftBar = 0;
    int rightBar = leftBar;
    if (m_repeatMode != REPEAT_VOLTA) {   ///!!! Quick hack to remove the last blank measure
        /// The old way : look all bars successively to find time signature and
        /// write it in a LilyPond comment except for the very first one.
        /// The time is computed from the composition start time. This is wrong as 
        /// the composition start time may be outside the exported time range.
        /// Nevertheless I have no time to fix it now. So sometimes it may work and
        /// sometimes not work...
        /// When using this code with repeats, the writeskip() to the end of the
        /// composition is writing a blank measure at the end of the score.
        do {
            // Allow some oportunities for user to cancel
            if (isOperationCancelled()) {
                return false;
            }
            
            bool isNew = false;
            m_composition->getTimeSignatureInBar(rightBar + 1, isNew);

            if (isNew || (m_composition->getBarStart(rightBar + 1) >= compositionEndTime)) {
                //  - set initial time signature; further time signature changes
                //    are defined within the segments, because they may be hidden
                str << indent(col) << (leftBar == 0 ? "" : "% ") << "\\time "
                    << timeSignature.getNumerator() << "/"
                    << timeSignature.getDenominator() << std::endl;
                //  - place skips upto the end of the composition;
                //    this justifies the printed staffs
                str << indent(col);
                timeT leftTime = m_composition->getBarStart(leftBar);
                timeT rightTime = m_composition->getBarStart(rightBar + 1);
                // Check for a partial measure in the beginning of the composition
                if (leftTime < compositionStartTime) {
                    leftTime = compositionStartTime;
                }
                // Check for a partial measure in the end of the composition
                if (rightTime > compositionEndTime) {
                    rightTime = compositionEndTime;
                };
                writeSkip(timeSignature, leftTime, rightTime - leftTime, false, str);
                str << " %% " << (leftBar + 1) << "-" << (rightBar + 1) << std::endl;

                timeSignature = m_composition->getTimeSignatureInBar(rightBar + 1, isNew);
                leftBar = rightBar + 1;
            }
        } while (m_composition->getBarStart(++rightBar) < compositionEndTime);
    } else {    /// Quick hack to remove the last blank measure
        /// The preliminary new way: The timesignature are all ignored except
        /// the first one and the writeSkip() is computed to the end of the
        /// score taking into acount the repeats rather than to the end of
        /// the composition.
        timeSignature = m_composition->
            getTimeSignatureAt(lsc.getFirstSegmentStartTime());
        //  - set initial time signature; further time signature changes
        //    are defined within the segments, because they may be hidden
        str << indent(col) << "\\time "
            << timeSignature.getNumerator() << "/"
            << timeSignature.getDenominator() << std::endl;
        //  - place skips upto the end of the composition;
        //    this justifies the printed staffs
        str << indent(col);
        writeSkip(timeSignature, lsc.getFirstSegmentStartTime(),
                  lsc.getLastSegmentEndTime() - lsc.getFirstSegmentStartTime(),
                  false, str);
        str << std::endl;
    }   /// Quick hack to remove the last blank measure

    str << indent(--col) << "}" << std::endl;

    // time signatures changes are in segments, reset initial value
    timeSignature = m_composition->
        getTimeSignatureAt(m_composition->getStartMarker());

    // All the tempo changes are included in "globalTempo" context.
    // This context contains only skip notes between the tempo changes.
    // First tempo marking should still be include in \midi{ } block (prior to 2.10).
    // If tempo marks are printed in future, they should probably be
    // included in this context and the note duration in the tempo
    // mark should be according to the time signature. (hjj)
    int tempoCount = m_composition->getTempoChangeCount();

    if (tempoCount > 0) {

        timeT prevTempoChangeTime = m_composition->getStartMarker();
        int tempo = int(Composition::getTempoQpm(m_composition->getTempoAtTime(prevTempoChangeTime)));
        bool tempoMarksInvisible = false;

        str << indent(col++) << "globalTempo = {" << std::endl;
        if (m_exportTempoMarks == EXPORT_NONE_TEMPO_MARKS && tempoMarksInvisible == false) {
            str << indent(col) << "\\override Score.MetronomeMark #'transparent = ##t" << std::endl;
            tempoMarksInvisible = true;
        }
        str << indent(col) << "\\tempo 4 = " << tempo << "  ";
        int prevTempo = tempo;

        for (int i = 0; i < tempoCount; ++i) {

            // Allow some oportunities for user to cancel
            if (isOperationCancelled()) {
                return false;
            }

            std::pair<timeT, tempoT> tempoChange =
                m_composition->getTempoChange(i);

            timeT tempoChangeTime = tempoChange.first;

            tempo = int(Composition::getTempoQpm(tempoChange.second));

            // First tempo change may be before the first segment.
            // Do not apply it before the first segment appears.
            if (tempoChangeTime < compositionStartTime) {
                tempoChangeTime = compositionStartTime;
            } else if (tempoChangeTime >= compositionEndTime) {
                tempoChangeTime = compositionEndTime;
            }
            if (prevTempoChangeTime < compositionStartTime) {
                prevTempoChangeTime = compositionStartTime;
            } else if (prevTempoChangeTime >= compositionEndTime) {
                prevTempoChangeTime = compositionEndTime;
            }
            writeSkip(m_composition->getTimeSignatureAt(tempoChangeTime),
                      tempoChangeTime, tempoChangeTime - prevTempoChangeTime, false, str);
            // add new \tempo only if tempo was changed
            if (tempo != prevTempo) {
                if (m_exportTempoMarks == EXPORT_FIRST_TEMPO_MARK && tempoMarksInvisible == false) {
                    str << std::endl << indent(col) << "\\override Score.MetronomeMark #'transparent = ##t";
                    tempoMarksInvisible = true;
                }
                str << std::endl << indent(col) << "\\tempo 4 = " << tempo << "  ";
            }

            prevTempo = tempo;
            prevTempoChangeTime = tempoChangeTime;
            if (prevTempoChangeTime == compositionEndTime)
                break;
        }
        // First tempo change may be before the first segment.
        // Do not apply it before the first segment appears.
        if (prevTempoChangeTime < compositionStartTime) {
            prevTempoChangeTime = compositionStartTime;
        }
        if (m_repeatMode != REPEAT_VOLTA) {   ///!!! Quick hack bis to remove the last blank measure
            /// The writeSkip() is just not called when exporting repeats
            writeSkip(m_composition->getTimeSignatureAt(prevTempoChangeTime),
                      prevTempoChangeTime, compositionEndTime - prevTempoChangeTime, false, str);
        }   /// Quick hack bis to remove the last blank measure
        str << std::endl;
        str << indent(--col) << "}" << std::endl;
    }
    // Markers
    // Skip until marker, make sure there's only one marker per measure
    if (m_exportMarkerMode != EXPORT_NO_MARKERS) {
        str << indent(col++) << "markers = {" << std::endl;
        timeT prevMarkerTime = 0;

        // Need the markers sorted by time
        Composition::markercontainer markers(m_composition->getMarkers()); // copy
        std::sort(markers.begin(), markers.end(), MarkerComp());
        Composition::markerconstiterator i_marker = markers.begin();

        while  (i_marker != markers.end()) {
            // Allow some oportunities for user to cancel
            if (isOperationCancelled()) {
                return false;
            }
        
            timeT markerTime = m_composition->getBarStartForTime((*i_marker)->getTime());
            RG_DEBUG << "Marker: " << (*i_marker)->getTime() << " previous: " << prevMarkerTime;
            // how to cope with time signature changes?
            if (markerTime > prevMarkerTime) {
                str << indent(col);
                writeSkip(m_composition->getTimeSignatureAt(markerTime),
                          markerTime, markerTime - prevMarkerTime, false, str);
                str << "\\mark "; 
                switch (m_exportMarkerMode) {
                case EXPORT_DEFAULT_MARKERS:
                    // Use the marker name for text
                    str << "\\default %% " << (*i_marker)->getName() << std::endl;
                    break;
                case EXPORT_TEXT_MARKERS:
                    // Raise the text above the staff as not to clash with the other stuff
                    str << "\\markup { \\hspace #0 \\raise #1.5 \"" << (*i_marker)->getName() << "\" }" << std::endl;
                    break;
                default:
                    break;
                }
                prevMarkerTime = markerTime;
            }
            ++i_marker;
        }
        str << indent(--col) << "}" << std::endl;
    }


    // open \score section
    str << "\\score {" << std::endl;

    int lastTrackIndex = -1;
    int voiceCounter = 0;
    bool firstTrack = true;
    int staffGroupCounter = 0;
    int pianoStaffCounter = 0;
    int bracket = 0;
    int prevBracket = -1;
    bool hasInstrumentNames = false;

    // Write out all segments for each Track, in track order.
    // This involves a hell of a lot of loops through all tracks
    // and segments, but the time spent doing that should still
    // be relatively small in the greater scheme.

    Track *track = 0;
    int trackPos = 0;

    for (track = lsc.useFirstTrack(); track; track = lsc.useNextTrack()) {
        trackPos = lsc.getTrackPos();

        // Allow some oportunities for user to cancel
        if (isOperationCancelled()) {
            return false;
        }
        
        int voiceIndex;
        for (voiceIndex = lsc.useFirstVoice();
                          voiceIndex != -1; voiceIndex = lsc.useNextVoice()) {

            /* timeT repeatOffset = 0; */

            Segment *seg;
            for (seg = lsc.useFirstSegment(); seg; seg = lsc.useNextSegment()) {
                if (!lsc.isVolta()) {
                    // handle the bracket(s) for the first track, and if no brackets
                    // present, open with a <<
                    prevBracket = bracket;
                    bracket = track->getStaffBracket();

                    //!!! how will all these indentions work out?  Probably not well,
                    // but maybe if users always provide sensible input, this will work
                    // out sensibly.  Maybe.  If not, we'll need some tracking gizmos to
                    // figure out the indention, or just skip the indention for these or
                    // something.  TBA.
                    if (firstTrack) {
                        // seems to be common to every case now
                        str << indent(++col) << "<< % common" << std::endl;
                    }

                    if (firstTrack && m_exportStaffGroup) {

                        if (bracket == Brackets::SquareOn) {
                            str << indent(++col) << "\\context StaffGroup = \"" << staffGroupCounter++
                                << "\" << " << std::endl; //indent+
                        } else if (bracket == Brackets::CurlyOn) {
                            str << indent(++col) << "\\context GrandStaff = \"" << pianoStaffCounter++
                                << "\" << " << std::endl; //indent+
                        } else if (bracket == Brackets::CurlySquareOn) {
                            str << indent(++col) << "\\context StaffGroup = \"" << staffGroupCounter++
                                << "\" << " << std::endl; //indent+
                            str << indent(++col) << "\\context GrandStaff = \"" << pianoStaffCounter++
                                << "\" << " << std::endl; //indent+
                        }

                        // Make chords offset colliding notes by default (only write for
                        // first track)
                        str << indent(++col) << "% Force offset of colliding notes in chords:"
                            << std::endl;
                        str << indent(col)   << "\\override Score.NoteColumn #\'force-hshift = #1.0"
                            << std::endl;
                        if (m_fingeringsInStaff) {
                            str << indent(col) << "% Allow fingerings inside the staff (configured from export options):"
                                << std::endl;
                            str << indent(col)   << "\\override Score.Fingering #\'staff-padding = #\'()"
                                << std::endl;
                        }
                    }

                    emit setValue(int(double(trackPos) /
                                    double(m_composition->getNbTracks()) * 100.0));
                                    
                    qApp->processEvents(QEventLoop::AllEvents, 100);

                    if ((int) seg->getTrack() != lastTrackIndex) {
                        if (lastTrackIndex != -1) {
                            // close the old track (Staff context)
                            str << indent(--col) << ">> % Staff ends" << std::endl; //indent-
                        }

                        // handle any necessary bracket closures with a rude
                        // hack, because bracket closures need to be handled
                        // right under staff closures, but at this point in the
                        // loop we are one track too early for closing, so we use
                        // the bracket setting for the previous track for closing
                        // purposes (I'm not quite sure why this works, but it does)            
                        if (m_exportStaffGroup) {
                            if (prevBracket == Brackets::SquareOff ||
                                prevBracket == Brackets::SquareOnOff) {
                                str << indent(--col) << ">> % StaffGroup " << staffGroupCounter
                                    << std::endl; //indent-
                            } else if (prevBracket == Brackets::CurlyOff) {
                                str << indent(--col) << ">> % GrandStaff " << pianoStaffCounter
                                    << std::endl; //indent-
                            } else if (prevBracket == Brackets::CurlySquareOff) {
                                str << indent(--col) << ">> % GrandStaff " << pianoStaffCounter
                                    << std::endl; //indent-
                                str << indent(--col) << ">> % StaffGroup " << staffGroupCounter
                                    << std::endl; //indent-
                            }
                        }
                    }

                    //
                    // Write the chord text events into a lead sheet format.
                    // The chords are placed into ChordName context above the staff,
                    // which is between the previous ending staff and next starting
                    // staff.
                    //
                    if (m_chordNamesMode) {
                        int numberOfChords = -1;

                        timeT lastTime = compositionStartTime;
                        for (Segment::iterator j = seg->begin();
                            seg->isBeforeEndMarker(j); ++j) {

                            bool isNote = (*j)->isa(Note::EventType);
                            bool isChord = false;

                            if (!isNote) {
                                if ((*j)->isa(Text::EventType)) {
                                    std::string textType;
                                    if ((*j)->get
                                        <String>(Text::TextTypePropertyName, textType) &&
                                        textType == Text::Chord) {
                                        isChord = true;
                                    }
                                }
                            }

                            if (!isNote && !isChord) continue;

                            timeT myTime = (*j)->getNotationAbsoluteTime();

                            if (isChord) {
                                std::string schord;
                                (*j)->get<String>(Text::TextPropertyName, schord);
                                QString chord(strtoqstr(schord));
                                chord.replace(QRegExp("\\s+"), "");
                                chord.replace(QRegExp("h"), "b");

                                // DEBUG: str << " %{ '" << chord.toUtf8() << "' %} ";
                                QRegExp rx("^([a-g]([ei]s)?)([:](m|dim|aug|maj|sus|\\d+|[.^]|[+-])*)?(/[+]?[a-g]([ei]s)?)?$");
                                if (rx.indexIn(chord) != -1) {
                                    // The chord duration is zero, but the chord
                                    // intervals is given with skips (see below).
                                    QRegExp rxStart("^([a-g]([ei]s)?)");
                                    chord.replace(QRegExp(rxStart), QString("\\1") + QString("4*0"));
                                } else {
                                    // Skip improper chords.
                                    str << (" %{ improper chord: '") << qStrToStrUtf8(chord) << ("' %} ");
                                    continue;
                                }

                                if (numberOfChords == -1) {
                                    str << indent(col++) << "\\new ChordNames " << "\\with {alignAboveContext=\"track " <<
                                        (trackPos + 1) << "\"}" << "\\chordmode {" << std::endl;
                                    str << indent(col) << "\\set chordNameExceptions = #chExceptions" << std::endl;
                                    str << indent(col);
                                    numberOfChords++;
                                }
                                if (numberOfChords >= 0) {
                                    // The chord intervals are specified with skips.
                                    writeSkip(m_composition->getTimeSignatureAt(myTime), lastTime, myTime - lastTime, false, str);
                                    str << qStrToStrUtf8(chord) << " ";
                                    numberOfChords++;
                                }
                                lastTime = myTime;
                            }
                        } // for
                        if (numberOfChords >= 0) {
                            writeSkip(m_composition->getTimeSignatureAt(lastTime), lastTime, compositionEndTime - lastTime, false, str);
                            if (numberOfChords == 1) str << "s8 ";
                            str << std::endl;
                            str << indent(--col) << "} % ChordNames " << std::endl;
                        }
                    } // if (m_exportChords....

                    if ((int) seg->getTrack() != lastTrackIndex) {
                        lastTrackIndex = seg->getTrack();

                        // handle any bracket start events (unless track staff
                        // brackets are being ignored, as when printing single parts
                        // out of a bigger score one by one)
                        if (!firstTrack && m_exportStaffGroup) {
                            if (bracket == Brackets::SquareOn ||
                                bracket == Brackets::SquareOnOff) {
                                str << indent(col++) << "\\context StaffGroup = \""
                                    << ++staffGroupCounter << "\" <<" << std::endl;
                            } else if (bracket == Brackets::CurlyOn) {
                                str << indent(col++) << "\\context GrandStaff = \""
                                    << ++pianoStaffCounter << "\" <<" << std::endl;
                            } else if (bracket == Brackets::CurlySquareOn) {
                                str << indent(col++) << "\\context StaffGroup = \""
                                    << ++staffGroupCounter << "\" <<" << std::endl;
                                str << indent(col++) << "\\context GrandStaff = \""
                                    << ++pianoStaffCounter << "\" <<" << std::endl;
                            }
                        } 

                        // avoid problem with <untitled> tracks yielding a
                        // .ly file that jumbles all notes together on a
                        // single staff...  every Staff context has to
                        // have a unique name, even if the
                        // Staff.instrument property is the same for
                        // multiple staffs...
                        // Added an option to merge staffs with the same, non-empty
                        // name. This option makes it possible to produce staffs
                        // with polyphonic, and polyrhytmic, music. Polyrhytmic
                        // music in a single staff is typical in piano, or
                        // guitar music. (hjj)
                        // In the case of colliding note heads, user may define
                        //  - DISPLACED_X -- for a note/chord
                        //  - INVISIBLE -- for a rest
                        const std::string staffName = protectIllegalChars(m_composition->
                                                        getTrackById(lastTrackIndex)->getLabel());

                        std::string shortStaffName = protectIllegalChars(m_composition->
                                getTrackById(lastTrackIndex)->getShortLabel());

                        /*
                        * The context name is unique to a single track.
                        */
                        str << std::endl << indent(col)
                            << "\\context Staff = \"track "
                            << (trackPos + 1) << (staffName == "" ? "" : ", ")
                            << staffName << "\" ";

                        str << "<< " << std::endl;
                        ++col;

                        if (staffName.size()) {
                            hasInstrumentNames = true;
                            // The octavation is omitted in the instrument name.
                            // HJJ: Should it be automatically added to the clef: G^8 ?
                            // What if two segments have different transpose in a track?
                            std::ostringstream staffNameWithTranspose;
                            staffNameWithTranspose << "\\markup { \\center-column { \"" << staffName << " \"";
                            if ((seg->getTranspose() % 12) != 0) {
                                staffNameWithTranspose << " \\line { ";
                                int t = seg->getTranspose();
                                t %= 12;
                                if (t < 0) t+= 12;
                                switch (t) {
                                case 1 : staffNameWithTranspose << "\"in D\" \\smaller \\flat"; break;
                                case 2 : staffNameWithTranspose << "\"in D\""; break;
                                case 3 : staffNameWithTranspose << "\"in E\" \\smaller \\flat"; break;
                                case 4 : staffNameWithTranspose << "\"in E\""; break;
                                case 5 : staffNameWithTranspose << "\"in F\""; break;
                                case 6 : staffNameWithTranspose << "\"in G\" \\smaller \\flat"; break;
                                case 7 : staffNameWithTranspose << "\"in G\""; break;
                                case 8 : staffNameWithTranspose << "\"in A\" \\smaller \\flat"; break;
                                case 9 : staffNameWithTranspose << "\"in A\""; break;
                                case 10 : staffNameWithTranspose << "\"in B\" \\smaller \\flat"; break;
                                case 11 : staffNameWithTranspose << "\"in B\""; break;
                                }
                                staffNameWithTranspose << " }";
                            }
                            staffNameWithTranspose << " } }";
                            if (m_languageLevel < LILYPOND_VERSION_2_10) {
                                str << indent(col) << "\\set Staff.instrument = " << staffNameWithTranspose.str()
                                    << std::endl;
                            } else {
                                // always write long staff name
                                str << indent(col) << "\\set Staff.instrumentName = "
                                    << staffNameWithTranspose.str() << std::endl;

                                // write short staff name if user desires, and if
                                // non-empty
                                if (m_useShortNames && shortStaffName.size()) {
                                    str << indent(col) << "\\set Staff.shortInstrumentName = \""
                                        << shortStaffName << "\"" << std::endl;
                                }
                            }
                        }

                        // Set midi instrument for the Staff when possible
                        Instrument *instr = m_studio->getInstrumentById(
                            m_composition->getTrackById(lastTrackIndex)
                                                            ->getInstrument());
                        if (instr) {
                            str << indent(col)
                                << "\\set Staff.midiInstrument = \""
                                << instr->getProgramName().c_str()
                                << "\"" << std::endl;
                        }

                        // multi measure rests are used by default
                        str << indent(col) << "\\set Score.skipBars = ##t" << std::endl;

                        // turn off the stupid accidental cancelling business,
                        // because we don't do that ourselves, and because my 11
                        // year old son pointed out to me that it "Looks really
                        // stupid.  Why is it cancelling out four flats and then
                        // adding five flats back?  That's brain damaged."
                        //
                        // New option to turn it back on, per user request.  There
                        // doesn't seem to be any way to get LilyPond's behavior to
                        // quite mimic our own, so we just offer it to them as an
                        // either/or choice.
                        if (m_cancelAccidentals) {
                            str << indent(col) << "\\set Staff.printKeyCancellation = ##t" << std::endl;
                        } else {
                            str << indent(col) << "\\set Staff.printKeyCancellation = ##f" << std::endl;
                        }
                        str << indent(col) << "\\new Voice \\global" << std::endl;
                        if (tempoCount > 0) {
                            str << indent(col) << "\\new Voice \\globalTempo" << std::endl;
                        }
                        if (m_exportMarkerMode != EXPORT_NO_MARKERS) {
                            str << indent(col) << "\\new Voice \\markers" << std::endl;
                        }

                        if (m_exportBeams) {
                            str << indent(col) << "\\set Staff.autoBeaming = ##f % turns off all autobeaming" << std::endl;
                        }
                    }
                } /// if (!lsc.isVolta())

                // Temporary storage for non-atomic events (!BOOM)
                // ex. LilyPond expects signals when a decrescendo starts
                // as well as when it ends
                eventendlist preEventsInProgress;
                eventendlist postEventsInProgress;

                // If the segment doesn't start at 0, add a "skip" to the start
                // No worries about overlapping segments, because Voices can overlap
                // voiceCounter is a hack because LilyPond does not by default make
                // them unique
                std::ostringstream voiceNumber;

                voiceNumber << "voice " << ++voiceCounter;
                if (!lsc.isVolta()) {
                    str << std::endl << indent(col++) << "\\context Voice = \"" << voiceNumber.str()
                        << "\" {"; // indent+

                    str << std::endl << indent(col) << "% Segment: " << seg->getLabel();
                    
                    str << std::endl << indent(col) << "\\override Voice.TextScript #'padding = #2.0";
                    str << std::endl << indent(col) << "\\override MultiMeasureRest #'expand-limit = 1" << std::endl;

                    // staff notation size
                    int staffSize = track->getStaffSize();
                    if (staffSize == StaffTypes::Small) str << indent(col) << "\\small" << std::endl;
                    else if (staffSize == StaffTypes::Tiny) str << indent(col) << "\\tiny" << std::endl;
                } /// if (!lsc.isVolta())
                SegmentNotationHelper helper(*seg);
                helper.setNotationProperties();

                int firstBar = m_composition->getBarNumber(seg->getStartTime());

                if (!lsc.isVolta()) {        // Don't write any skip in a volta
                    if (firstBar > 0) {
                        // Add a skip for the duration until the start of the first
                        // bar in the segment.  If the segment doesn't start on a
                        // bar line, an additional skip will be written at the start
                        // of writeBar, below.
                        //!!! This doesn't cope correctly yet with time signature changes
                        // during this skipped section.
                        // dmm - changed this to call writeSkip with false, to avoid
                        // writing actual rests, and write a skip instead, so
                        // visible rests do not appear before the start of short
                        // bars
                        str << std::endl << indent(col);
                        writeSkip(timeSignature, compositionStartTime,
                                lsc.getSegmentStartTime(), false, str);
                    }
        
                    // If segment is not starting on a bar, but is starting at barTime + offset,
                    // we have to do :
                    //     if segment is the first one : add partial (barDuration - offset)
                    //     else  add skip (offset)
                    if (seg->getStartTime() - m_composition->getBarStart(firstBar) > 0) {
                        if (seg->getStartTime() == firstSegmentStartTime) {
                            timeT partialDuration = m_composition->getBarStart(firstBar + 1)
                                                    - seg->getStartTime();
                            str << indent(col) << "\\partial ";
                            // Arbitrary partial durations are handled by the following
                            // way: split the partial duration to 64th notes: instead
                            // of "4" write "64*16". (hjj)
                            Note partialNote = Note::getNearestNote(1, MAX_DOTS);
                            writeDuration(1, str);
                            str << "*" << ((int)(partialDuration / partialNote.getDuration()))
                                << std::endl;
        
                        } else {
                            if (m_repeatMode == REPEAT_BASIC) {
                                timeT partialOffset = seg->getStartTime()
                                                    - m_composition->getBarStart(firstBar);
                                str << indent(col) << "\\skip ";
                                // Arbitrary partial durations are handled by the following
                                // way: split the partial duration to 64th notes: instead
                                // of "4" write "64*16". (hjj)
                                Note partialNote = Note::getNearestNote(1, MAX_DOTS);
                                writeDuration(1, str);
                                str << "*" << ((int)(partialOffset / partialNote.getDuration()))
                                    << std::endl;
                            }
                        }
                    }
                } /// if (!lsc.isVolta())


                std::string lilyText = "";      // text events
                std::string prevStyle = "";     // track note styles

                Rosegarden::Key key = lsc.getPreviousKey();

                bool haveRepeating = false;
                bool haveAlternates = false;

                bool haveRepeatingWithVolta = false;
                bool haveVolta = false;

                bool nextBarIsAlt1 = false;
                bool nextBarIsAlt2 = false;
                bool prevBarWasAlt2 = false;

                int MultiMeasureRestCount = 0;

                bool nextBarIsDouble = false;
                bool nextBarIsEnd = false;
                bool nextBarIsDot = false;

                for (int barNo = m_composition->getBarNumber(seg->getStartTime());
                    barNo <= m_composition->getBarNumber(seg->getEndMarkerTime());
                    ++barNo) {

                    timeT barStart = m_composition->getBarStart(barNo);
                    timeT barEnd = m_composition->getBarEnd(barNo);
                    timeT currentSegmentStartTime = seg->getStartTime();
                    timeT currentSegmentEndTime = seg->getEndMarkerTime();
                    // Check for a partial measure in the beginning of the composition
                    if (barStart < compositionStartTime) {
                        barStart = compositionStartTime;
                    }
                    // Check for a partial measure in the end of the composition
                    if (barEnd > compositionEndTime) {
                        barEnd = compositionEndTime;
                    }
                    // Check for a partial measure beginning in the middle of a
                    // theoretical bar
                    if (barStart < currentSegmentStartTime) {
                        barStart = currentSegmentStartTime;
                    }
                    // Check for a partial measure ending in the middle of a
                    // theoretical bar
                    if (barEnd > currentSegmentEndTime) {
                        barEnd = currentSegmentEndTime;
                    }

                    // Check for a time signature in the first bar of the segment
                    bool timeSigInFirstBar = false;
                    TimeSignature firstTimeSig =
                        m_composition->getTimeSignatureInBar(firstBar, timeSigInFirstBar);
                    // and write it here (to avoid multiple time signatures when
                    // a repeating segment is unfolded)
                    if (timeSigInFirstBar && (barNo == firstBar)) {
                        writeTimeSignature(firstTimeSig, col, str);
                    }

                    // open \repeat section if this is the first bar in the
                    // repeat
                    if (seg->isRepeating() && !haveRepeating) {

                        haveRepeating = true;
                        int numRepeats = 2; 

                        if (m_repeatMode == REPEAT_BASIC) {
                            // The old unfinished way
                            str << std::endl << indent(col++) << "\\repeat volta " << numRepeats << " {";
                        } else {
                            numRepeats = lsc.getNumberOfRepeats();
                            if ((m_repeatMode == REPEAT_VOLTA) && lsc.isSynchronous()) {
                                str << std::endl << indent(col++) 
                                    << "\\repeat volta " << numRepeats << " {";
                            } else {
                                // m_repeatMode == REPEAT_UNFOLD
                                str << std::endl << indent(col++) 
                                    << "\\repeat unfold " << numRepeats << " {";
                            }
                        }
                    } else if (lsc.isRepeatWithVolta() &&
                            !haveRepeatingWithVolta &&
                            !haveVolta) {
                        if (!lsc.isVolta()) {
                            str << std::endl << indent(col++); 
                            if (lsc.isAutomaticVoltaUsable()) {
                                str << "\\repeat volta "
                                    << lsc.getNumberOfRepeats() << " ";
                            }
                            // Opening of main repeating segment
                            str << "{   % Repeating stegment start here";
                            str << std::endl << indent(col)
                                << "% Segment: " << seg->getLabel();
                            haveRepeatingWithVolta = true;
                            if (!lsc.isAutomaticVoltaUsable()) {
                            str << std::endl << indent(col)
                                << "\\set Score.repeatCommands = #'(start-repeat)";
                            }
                        } else {
                            str << std::endl << indent(col) 
                                << "{   % Alternative start here";
                            str << std::endl << indent(col++) 
                                << "    % Segment: " << seg->getLabel();
                            if (!lsc.isAutomaticVoltaUsable()) {
                                str << std::endl << indent(col)
                                    << "\\set Score.repeatCommands = ";
                                if (lsc.isFirstVolta()) {   
                                    str << "#'((volta \""
                                        << lsc.getVoltaText() << "\"))";
                                } else {
                                    str << "#'((volta #f) (volta \""
                                        << lsc.getVoltaText() << "\") end-repeat)";
                                }
                            }
                            if (m_voltaBar) {
                                str << std::endl << indent(col) 
                                    << "\\bar \"|\" ";
                            }
                            haveVolta = true;
                        }
                    }

                    // open the \alternative section if this bar is alternative ending 1
                    // ending (because there was an "Alt1" flag in the
                    // previous bar to the left of where we are right now)
                    //
                    // Alt1 remains in effect until we run into Alt2, which
                    // runs to the end of the segment
                    if (nextBarIsAlt1 && haveRepeating) {
                        str << std::endl << indent(--col) << "} \% repeat close (before alternatives) ";
                        str << std::endl << indent(col++) << "\\alternative {";
                        str << std::endl << indent(col++) << "{  \% open alternative 1 ";
                        nextBarIsAlt1 = false;
                        haveAlternates = true;
                    } else if (nextBarIsAlt2 && haveRepeating) {
                        if (!prevBarWasAlt2) {
                            col--;
                            // add an extra str to the following to shut up
                            // compiler warning from --ing and ++ing it in the
                            // same statement
                            str << std::endl << indent(--col) << "} \% close alternative 1 ";
                            str << std::endl << indent(col++) << "{  \% open alternative 2";
                            col++;
                        }
                        prevBarWasAlt2 = true;
                    }

                    // should a time signature be writed in the current bar ?
                    bool noTimeSig;
                    if (timeSigInFirstBar) {
                        noTimeSig = barNo == firstBar;
                    } else {
                        noTimeSig = barNo != firstBar;
                    }

                    // write out a bar's worth of events
                    writeBar(seg, barNo, barStart, barEnd, col, key,
                            lilyText,
                            prevStyle, preEventsInProgress, postEventsInProgress, str,
                            MultiMeasureRestCount, 
                            nextBarIsAlt1, nextBarIsAlt2, nextBarIsDouble,
                            nextBarIsEnd, nextBarIsDot,
                            noTimeSig);

                }

                // close \repeat
                if (haveRepeating) {

                    // close \alternative section if present
                    if (haveAlternates) {
                        str << std::endl << indent(--col) << "} \% close alternative 2 ";
                    }

                    // close \repeat section in either case
                    str << std::endl << indent(--col) << "} \% close "
                        << (haveAlternates ? "alternatives" : "repeat");
                }

                // Open alternate parts if repeat with volta from linked segments
                if (haveRepeatingWithVolta) {
                    if (!lsc.isVolta()) {
                        str << std::endl << indent(--col) << "} \% close main repeat";
                        if (lsc.isAutomaticVoltaUsable()) {
                            str << std::endl << indent (col++) << "\\alternative  {";
                        }
                        str <<  std::endl;
                    } else {
                        // Close alternative segment
                        str << std::endl << indent(--col) << "}";
                    }
                }

                // closing bar
                if ((seg->getEndMarkerTime() == compositionEndTime) && !haveRepeating) {
                    str << std::endl << indent(col) << "\\bar \"|.\"";
                }

                if (!haveRepeatingWithVolta && !haveVolta) {
                    // close Voice context
                    str << std::endl << indent(--col) << "} % Voice" << std::endl;  // indent-
                }

                if (lsc.isVolta()) {
                    // close volta
                    if (!lsc.isAutomaticVoltaUsable() && lsc.isLastVolta()) {
                        str << std::endl << indent (col)
                            << "\\set Score.repeatCommands = ";
                        if (lsc.getVoltaRepeatCount() > 1) {
                            str << "#'((volta #f) end-repeat)";
                        } else {
                            str << "#'((volta #f))";
                        }
                        if (lsc.getVoltaRepeatCount() < 1) {
                            RG_WARNING << "BUG in LilyPondExporter : "
                                    << "lsc.getVoltaRepeatCount() = "
                                    << lsc.getVoltaRepeatCount();
                        }
                    }
                    str << std::endl << indent(--col) << "}" << std::endl;  // indent-

                    if (lsc.isLastVolta()) {
                        if (lsc.isAutomaticVoltaUsable()) {
                            // close alternative section
                            str << std::endl << indent(--col) << "}" << std::endl;  // indent-
                        }

                    // close Voice context
                        str << std::endl << indent(--col) << "} % Voice" << std::endl;  // indent-
                    }
                }

                //
                // Write accumulated lyric events to the Lyric context, if desired.
                //
                // Sync the code below with LyricEditDialog::unparse() !!
                //
                if (m_exportLyrics != EXPORT_NO_LYRICS) {
                    // To force correct ordering of verses must track when first verse is printed.
                    bool isFirstPrintedVerse = true;
                    for (long currentVerse = 0, lastVerse = 0; 
                        currentVerse <= lastVerse; 
                        currentVerse++) {
                        bool haveLyric = false;
                        bool firstNote = true;
                        QString text = "";

                        timeT lastTime = seg->getStartTime();
                        for (Segment::iterator j = seg->begin();
                            seg->isBeforeEndMarker(j); ++j) {

                            bool isNote = (*j)->isa(Note::EventType);
                            bool isLyric = false;

                            if (!isNote) {
                                if ((*j)->isa(Text::EventType)) {
                                    std::string textType;
                                    if ((*j)->get
                                        <String>(Text::TextTypePropertyName, textType) &&
                                        textType == Text::Lyric) {
                                        isLyric = true;
                                    }
                                }
                            }

                            if (!isNote && !isLyric) continue;

                            timeT myTime = (*j)->getNotationAbsoluteTime();

                            if (isNote) {
                                if ((myTime > lastTime) || firstNote) {
                                    if (!haveLyric)
                                        text += " _";
                                    lastTime = myTime;
                                    haveLyric = false;
                                    firstNote = false;
                                }
                            }

                            if (isLyric) {
                                // Very old .rg files may not have the verse property.
                                // In such a case there is only one verse which
                                // is numbered 0.
                                long verse;
                                if (! (*j)->get<Int>(Text::LyricVersePropertyName,
                                                     verse)) verse = 0;

                                if (verse == currentVerse) {
                                    std::string ssyllable;
                                    (*j)->get<String>(Text::TextPropertyName, ssyllable);
                                    text += " ";
            
                                    QString syllable(strtoqstr(ssyllable));
                                    syllable.replace(QRegExp("^\\s+"), "");
                                    syllable.replace(QRegExp("\\s+$"), "");
                                    syllable.replace(QRegExp("\""), "\\\"");
                                    text += "\"" + syllable + "\"";
                                    haveLyric = true;
                                } else if (verse > lastVerse) {
                                    lastVerse = verse;
                                }
                            }
                        }

                        text.replace(QRegExp(" _+([^ ])") , " \\1");
                        text.replace("\"_\"" , " ");

                        // Do not create empty context for lyrics.
                        // Does this save some vertical space, as was written
                        // in earlier comment?
                        QRegExp rx("\"");
                        if (rx.indexIn(text) != -1) {
        
                            if (m_languageLevel <= LILYPOND_VERSION_2_10) {
                                str << indent(col) << "\\lyricsto \"" << voiceNumber.str() << "\""
                                    << " \\new Lyrics \\lyricmode {" << std::endl;
                            } else {
                                str << indent(col)
                                    << "\\new Lyrics ";
                                // Put special alignment info for first printed verse only.
                                // Otherwise, verses print in reverse order.
                                if (isFirstPrintedVerse) {
                                    str << "\\with {alignBelowContext=\"track " << (trackPos + 1) << "\"} ";
                                    isFirstPrintedVerse = false;
                                }
                                str << "\\lyricsto \"" << voiceNumber.str() << "\"" << " \\lyricmode {" << std::endl;
                            }
                            if (m_exportLyrics == EXPORT_LYRICS_RIGHT) {
                                str << indent(++col) << "\\override LyricText #'self-alignment-X = #RIGHT"
                                    << std::endl;
                            } else if (m_exportLyrics == EXPORT_LYRICS_CENTER) {
                                str << indent(++col) << "\\override LyricText #'self-alignment-X = #CENTER"
                                    << std::endl;
                            } else {
                                str << indent(++col) << "\\override LyricText #'self-alignment-X = #LEFT"
                                    << std::endl;
                            }
                            str << indent(col) << qStrToStrUtf8("\\set ignoreMelismata = ##t") << std::endl;
                            str << indent(col) << qStrToStrUtf8(text) << " " << std::endl;
                            str << indent(col) << qStrToStrUtf8("\\unset ignoreMelismata") << std::endl;
                            str << indent(--col) << qStrToStrUtf8("} % Lyrics ") << (currentVerse+1) << std::endl;
                            // close the Lyrics context
                        } // if (rx.search(text....
                    } // for (long currentVerse = 0....
                } // if (m_exportLyrics....
                firstTrack = false;
            } // for (Composition::iterator i = m_composition->begin()....
        } // for (voiceIndex = ...
    } // for (int trackPos = 0....

    // close the last track (Staff context)
    if (voiceCounter > 0) {
        str << indent(--col) << ">> % Staff (final) ends" << std::endl;  // indent-

        // handle any necessary final bracket closures (if brackets are being
        // exported)
        if (m_exportStaffGroup) {
            if (bracket == Brackets::SquareOff ||
                bracket == Brackets::SquareOnOff) {
                str << indent(--col) << ">> % StaffGroup " << staffGroupCounter
                    << std::endl; //indent-
            } else        if (bracket == Brackets::CurlyOff) {
                str << indent(--col) << ">> % GrandStaff (final) " << pianoStaffCounter
                    << std::endl; //indent-
            } else if (bracket == Brackets::CurlySquareOff) {
                str << indent(--col) << ">> % GrandStaff (final) " << pianoStaffCounter
                    << std::endl; //indent-
                str << indent(--col) << ">> % StaffGroup (final) " << staffGroupCounter
                    << std::endl; //indent-
            }
        }
    } else {
        str << indent(--col) << "% (All staffs were muted.)" << std::endl;
    }

    // close \notes section
    str << std::endl << indent(--col) << ">> % notes" << std::endl << std::endl; // indent-
    //    str << std::endl << indent(col) << ">> % global wrapper" << std::endl;

    // write \layout block
    str << indent(col++) << "\\layout {" << std::endl;

    // indent instrument names
    if (hasInstrumentNames) {
        str << indent(col) << "indent = 3.0\\cm" << std::endl
            << indent(col) << "short-indent = 1.5\\cm" << std::endl;
    }

    if (!m_exportEmptyStaves) {
        str << indent(col) << "\\context { \\Staff \\RemoveEmptyStaves }" << std::endl;
    }
    if (m_chordNamesMode) {
        str << indent(col) << "\\context { \\GrandStaff \\accepts \"ChordNames\" }" << std::endl;
    }
    if (m_exportLyrics != EXPORT_NO_LYRICS) {
        str << indent(col) << "\\context { \\GrandStaff \\accepts \"Lyrics\" }" << std::endl;
    }
    str << indent(--col) << "}" << std::endl;

    // write initial tempo in Midi block, always, but commented out
    // makes debugging the .ly file easier because fewer "noisy" errors are
    // produced during the process of rendering MIDI...)
    int tempo = int(Composition::getTempoQpm(m_composition->getTempoAtTime(m_composition->getStartMarker())));
    // Incomplete?  Can I get away without converting tempo relative to the time
    // signature for this purpose?  we'll see...
    str << "% " << indent(col++) << "uncomment to enable generating midi file from the lilypond source" << std::endl;
    str << "% " << indent(col++) << "\\midi {" << std::endl;
    if (m_languageLevel < LILYPOND_VERSION_2_10) {
        str << "% " << indent(col) << "\\tempo 4 = " << tempo << std::endl;
    }
    str << "% " << indent(--col) << "} " << std::endl;

    // close \score section and close out the file
    str << "} % score" << std::endl;
    str.close();
    return true;
}

timeT 
LilyPondExporter::calculateDuration(Segment *s,
                                    const Segment::iterator &i,
                                    timeT barEnd,
                                    timeT &soundingDuration,
                                    const std::pair<int, int> &tupletRatio,
                                    bool &overlong)
{
    timeT duration = (*i)->getNotationDuration();
    timeT absTime = (*i)->getNotationAbsoluteTime();

    //RG_DEBUG << "calculateDuration: first duration, absTime:" << duration << "," << absTime;

    timeT durationCorrection = 0;

    if ((*i)->isa(Note::EventType) || (*i)->isa(Note::EventRestType)) {
        try {
            // tuplet compensation, etc
            Note::Type type = (*i)->get<Int>(NOTE_TYPE);
            int dots = (*i)->get<Int>(NOTE_DOTS);
            durationCorrection = Note(type, dots).getDuration() - duration;
        } catch (Exception e) { // no properties
        }
    }

    duration += durationCorrection;

    //RG_DEBUG << "calculateDuration: now duration is" << duration << "after correction of" << durationCorrection;

    soundingDuration = duration * tupletRatio.first/ tupletRatio.second;

    timeT toNext = barEnd - absTime;
    if (soundingDuration > toNext) {
        soundingDuration = toNext;
        duration = soundingDuration * tupletRatio.second/ tupletRatio.first;
        overlong = true;
    }

    //RG_DEBUG << "calculateDuration: time to barEnd is " << toNext;

    // Examine the following event, and truncate our duration
    // if we overlap it.
    Segment::iterator nextElt = s->end();
    toNext = soundingDuration;

    if ((*i)->isa(Note::EventType)) {

        Chord chord(*s, i, m_composition->getNotationQuantizer());
        Segment::iterator nextElt = chord.getFinalElement();
        ++nextElt;

        if (s->isBeforeEndMarker(nextElt)) {
            // The quantizer sometimes sticks a rest at the same time
            // as this note -- don't use that one here, and mark it as
            // not to be exported -- it's just a heavy-handed way of
            // rendering counterpoint in RG
            if ((*nextElt)->isa(Note::EventRestType) &&
                (*nextElt)->getNotationAbsoluteTime() == absTime) {
                (*nextElt)->set<Bool>(SKIP_PROPERTY, true);
                ++nextElt;
            }
        }

    } else {
        nextElt = i;
        ++nextElt;
        while (s->isBeforeEndMarker(nextElt)) {
            if ((*nextElt)->isa(Controller::EventType) ||
                (*nextElt)->isa(ProgramChange::EventType) ||
                (*nextElt)->isa(SystemExclusive::EventType) ||
                (*nextElt)->isa(ChannelPressure::EventType) ||
                (*nextElt)->isa(KeyPressure::EventType) ||
                (*nextElt)->isa(PitchBend::EventType)) {
                ++nextElt;
            } else {
                break;
            }
        }
    }

    if (s->isBeforeEndMarker(nextElt)) {
        //RG_DEBUG << "calculateDuration: inside conditional";
        toNext = (*nextElt)->getNotationAbsoluteTime() - absTime;
        // if the note was lengthened, assume it was lengthened to the left
        // when truncating to the beginning of the next note
        if (durationCorrection > 0) {
            toNext += durationCorrection;
        }
        if (soundingDuration > toNext) {
            soundingDuration = toNext;
            duration = soundingDuration * tupletRatio.second/ tupletRatio.first;
        }
    }

    //RG_DEBUG << "calculateDuration: second toNext is" << toNext;
    //RG_DEBUG << "calculateDuration: final duration, soundingDuration:" << duration << "," << soundingDuration;

    return duration;
}

static std::string lilyClefType(const std::string &clefType)
{
    if (clefType == Clef::Treble) {
        return "treble";
    } else if (clefType == Clef::French) {
        return "french";
    } else if (clefType == Clef::Soprano) {
        return "soprano";
    } else if (clefType == Clef::Mezzosoprano) {
        return "mezzosoprano";
    } else if (clefType == Clef::Alto) {
        return "alto";
    } else if (clefType == Clef::Tenor) {
        return "tenor";
    } else if (clefType == Clef::Baritone) {
        return "baritone";
    } else if (clefType == Clef::Varbaritone) {
        return "varbaritone";
    } else if (clefType == Clef::Bass) {
        return "bass";
    } else if (clefType == Clef::Subbass) {
        return "subbass";
    }
    return std::string();
}

void LilyPondExporter::handleGuitarChord(Segment::iterator i, std::ofstream &str)
{
    try {
        Guitar::Chord chord = Guitar::Chord(**i);
        const Guitar::Fingering& fingering = chord.getFingering();

        int barreStart = 0, barreEnd = 0, barreFret = 0;

        //
        // Check if there is a barre.
        //
        if (fingering.hasBarre()) {
            Guitar::Fingering::Barre barre = fingering.getBarre();
            barreStart = barre.start;
            barreEnd = barre.end;
            barreFret = barre.fret;
        }

        if (barreStart == 0) {
            str << " s4*0^\\markup \\fret-diagram #\"";
        } else {
            str << " s4*0^\\markup \\override #'(barre-type . straight) \\fret-diagram #\"";
        }
        //
        // Check each string individually.
        // Note: LilyPond numbers strings differently.
        //
        for (int stringNum = 6; stringNum >= 1; --stringNum) {
            if (barreStart == stringNum) {
                str << "c:" << barreStart << "-" << barreEnd << "-" << barreFret << ";";
            }

            if (fingering.getStringStatus(6-stringNum) == Guitar::Fingering::MUTED) {
                str << stringNum << "-x;";
            } else if (fingering.getStringStatus(6-stringNum) == Guitar::Fingering::OPEN) {
                str << stringNum << "-o;";
            } else {
                int stringStatus = fingering.getStringStatus(6-stringNum);
                if ((stringNum <= barreStart) && (stringNum >= barreEnd)) {
                    str << stringNum << "-" << barreFret << ";";
                } else {
                    str << stringNum << "-" << stringStatus << ";";
                }
            }
        }
        str << "\" ";

    } catch (Exception e) { // GuitarChord ctor failed
        RG_DEBUG << "Bad GuitarChord event in LilyPond export";
    }
}

void
LilyPondExporter::writeBar(Segment *s,
                           int barNo, timeT barStart, timeT barEnd, int col,
                           Rosegarden::Key &key,
                           std::string &lilyText,
                           std::string &prevStyle,
                           eventendlist &preEventsInProgress,
                           eventendlist &postEventsInProgress,
                           std::ofstream &str,
                           int &MultiMeasureRestCount,
                           bool &nextBarIsAlt1, bool &nextBarIsAlt2,
                           bool &nextBarIsDouble, bool &nextBarIsEnd,
                           bool &nextBarIsDot,  bool noTimeSignature)
{
    int lastStem = 0; // 0 => unset, -1 => down, 1 => up
    int isGrace = 0;

    Segment::iterator i =
        SegmentNotationHelper(*s).findNotationAbsoluteTime(barStart);
    if (!s->isBeforeEndMarker(i))
        return ;

    //RG_DEBUG << "===== Writing bar" << barNo;

    if (MultiMeasureRestCount == 0) {
        str << std::endl;

        if ((barNo + 1) % 5 == 0) {
            str << "%% " << barNo + 1 << std::endl << indent(col);
        } else {
            str << indent(col);
        }
    }

    bool isNew = false;
    TimeSignature timeSignature = m_composition->getTimeSignatureInBar(barNo, isNew);
    if (isNew && !noTimeSignature) {
        writeTimeSignature(timeSignature, col, str);
    }

    timeT absTime = (*i)->getNotationAbsoluteTime();
    timeT writtenDuration = 0;
    std::pair<int,int> barDurationRatio(timeSignature.getNumerator(),timeSignature.getDenominator());
    std::pair<int,int> durationRatioSum(0,1);
    static std::pair<int,int> durationRatio(0,1);

    if (absTime > barStart) {
        Note note(Note::getNearestNote(absTime - barStart, MAX_DOTS));
        writtenDuration += note.getDuration();
        durationRatio = writeSkip(timeSignature, 0, note.getDuration(), false, str);
        durationRatioSum = fractionSum(durationRatioSum,durationRatio);
        // str << qstrtostr(QString(" %{ %1/%2 %} ").arg(durationRatio.first).arg(durationRatio.second)); // DEBUG
    }

    timeT prevDuration = -1;
    eventstartlist preEventsToStart;
    eventstartlist postEventsToStart;

    long groupId = -1;
    std::string groupType = "";
    std::pair<int, int> tupletRatio(1, 1);

    bool overlong = false;

    bool inBeamedGroup = false;
    bool startingBeamedGroup = false;
    Event *nextBeamedNoteInGroup = 0;
    Event *nextNoteInTuplet = 0;

    while (s->isBeforeEndMarker(i)) {

        Event *event = *i;

        if (event->getNotationAbsoluteTime() >= barEnd)
            break;

        // First test whether we're entering or leaving a group,
        // before we consider how to write the event itself (at least
        // for tuplets)
        QString startTupledStr;

        const bool isNote = event->isa(Note::EventType);
        const bool isRest = event->isa(Note::EventRestType);

        if (isNote || isRest ||
            event->isa(Clef::EventType) || event->isa(Key::EventType) ||
            event->isa(Symbol::EventType)) {

            // skip everything until the next beamed note in the current group
            if (!nextBeamedNoteInGroup || event == nextBeamedNoteInGroup) {
                nextBeamedNoteInGroup = 0;

                groupType = "";
                event->get<String>(BEAMED_GROUP_TYPE, groupType); // might fail
                const bool tuplet = groupType == GROUP_TYPE_TUPLED;

                long newGroupId = -1;
                if (!groupType.empty() && (isNote || isRest)) {
                    event->get<Int>(BEAMED_GROUP_ID, newGroupId);

                    if (newGroupId != -1) {
                        if (tuplet) {
                            nextNoteInTuplet = nextNoteInGroup(s, i, groupType, barEnd);
                        }
                        nextBeamedNoteInGroup = nextNoteInGroup(s, i, GROUP_TYPE_BEAMED, barEnd);
                    }
                }

                if (newGroupId != -1 && newGroupId != groupId) {
                    // entering a new beamed group
                    groupId = newGroupId;

                    startingBeamedGroup = true;

                    //RG_DEBUG << "Entering group" << groupId << "type" << groupType;
                    if (tuplet) {
                        long numerator = 0;
                        long denominator = 0;
                        event->get<Int>(BEAMED_GROUP_TUPLED_COUNT, numerator);
                        event->get<Int>(BEAMED_GROUP_UNTUPLED_COUNT, denominator);
                        if (numerator == 0 || denominator == 0) {
                            RG_WARNING << "WARNING: LilyPondExporter::writeBar: "
                                      << "tupled event without tupled/untupled counts";
                            groupId = -1;
                            groupType = "";
                        } else {
                            startTupledStr += QString("\\times %1/%2 { ").arg(numerator).arg(denominator);
                            tupletRatio = std::pair<int, int>(numerator, denominator);
                        }
                    } else if (groupType == GROUP_TYPE_BEAMED) {
                        // there can currently be only on group type, reset tuplet ratio
                        tupletRatio = std::pair<int, int>(1,1);
                    }
                }
            }
        } else if (event->isa(Controller::EventType) &&
                   event->has(Controller::NUMBER) &&
                   event->has(Controller::VALUE)) {
            if (event->get <Int>(Controller::NUMBER) == 64) {
                postEventsToStart.insert(event);
                postEventsInProgress.insert(event);
            }
        }

        // Test whether the next note is grace note or not.
        // The start or end of beamed grouping should be put in proper places.
        if (event->has(IS_GRACE_NOTE) && event->get<Bool>(IS_GRACE_NOTE)) {
            if (isGrace == 0) { 
                isGrace = 1;

                // LilyPond export hack:  If a grace note has one or more
                // tremolo slashes, we export it as a slashed grace note instead
                // of a plain one.
                long slashes;
                slashes = event->get<Int>(NotationProperties::SLASHES, slashes);
                if (slashes > 0) str << "\\slashedGrace { ";
                else str << "\\grace { ";

                // str << "%{ grace starts %} "; // DEBUG
            }
        } else if (isGrace == 1) {
            isGrace = 0;
            // str << "%{ grace ends %} "; // DEBUG
            str << "} ";
        }
        str << qStrToStrUtf8(startTupledStr);

        timeT soundingDuration = -1;
        timeT duration = calculateDuration
            (s, i, barEnd, soundingDuration, tupletRatio, overlong);

        if (soundingDuration == -1) {
            soundingDuration = duration * tupletRatio.first / tupletRatio.second;
        }

        if (event->has(SKIP_PROPERTY)) {
            event->unset(SKIP_PROPERTY);
            ++i;
            continue;
        }

        bool needsSlashRest = false;

        // symbols have no duration, so handle these ahead of anything else
        if (event->isa(Symbol::EventType)) {

            Symbol symbol(*event);

            if (symbol.getSymbolType() == Symbol::Segno) {
                str << "\\mark \\markup { \\musicglyph #\"scripts.segno\" } ";
            } else if (symbol.getSymbolType() == Symbol::Coda) {
                str << "\\mark \\markup { \\musicglyph #\"scripts.coda\" } ";
            } else if (symbol.getSymbolType() == Symbol::Breath) {
                str << "\\breathe ";
            }

        } else if (isNote) {

            Chord chord(*s, i, m_composition->getNotationQuantizer());
            Event *e = *chord.getInitialNote();
            bool tiedForward = false;
            bool tiedUp = false;

            // Examine the following event, and truncate our duration
            // if we overlap it.

            if (e->has(DISPLACED_X)) {
                double xDisplacement = 1 + ((double) e->get
                                            <Int>(DISPLACED_X)) / 1000;
                str << "\\once \\override NoteColumn #'force-hshift = #"
                    << xDisplacement << " ";
            }

            const bool hiddenNote = e->has(INVISIBLE) && e->get<Bool>(INVISIBLE);
            if (hiddenNote) {
                str << "\\hideNotes ";
            }

            // Export stem direction - but don't change it in the middle of a beamed group
            if (!m_exportBeams || !inBeamedGroup || startingBeamedGroup) {
                if (e->has(NotationProperties::STEM_UP)) {
                    if (e->get<Bool>(NotationProperties::STEM_UP)) {
                        if (lastStem != 1) {
                            str << "\\stemUp ";
                            lastStem = 1;
                        }
                    } else {
                        if (lastStem != -1) {
                            str << "\\stemDown ";
                            lastStem = -1;
                        }
                    }
                } else {
                    if (lastStem != 0) {
                        str << "\\stemNeutral ";
                        lastStem = 0;
                    }
                }
            }

            handleEndingPreEvents(preEventsInProgress, i, str);
            handleStartingPreEvents(preEventsToStart, str);

            if (chord.size() > 1)
                str << "< ";

            Segment::iterator stylei = s->end();

            for (i = chord.getInitialElement(); s->isBeforeEndMarker(i); ++i) {

                event = *i;
                if (event->isa(Text::EventType)) {
                    if (!handleDirective(event, lilyText, nextBarIsAlt1, nextBarIsAlt2,
                                         nextBarIsDouble, nextBarIsEnd, nextBarIsDot)) {

                        handleText(event, lilyText);
                    }

                } else if (event->isa(Note::EventType)) {

                    if (m_languageLevel >= LILYPOND_VERSION_2_8) {
                        // one \tweak per each chord note
                        if (chord.size() > 1)
                            writeStyle(event, prevStyle, col, str, true);
                        else
                            writeStyle(event, prevStyle, col, str, false);
                    } else {
                        // only one override per chord, and that outside the <>
                        stylei = i;
                    }

                    writePitch(event, key, str);

                    bool noteHasCautionaryAccidental = false;
                    event->get
                        <Bool>(NotationProperties::USE_CAUTIONARY_ACCIDENTAL, noteHasCautionaryAccidental);
                    if (noteHasCautionaryAccidental)
                        str << "?";

                    // get TIED_FORWARD and TIE_IS_ABOVE for later
                    event->get<Bool>(TIED_FORWARD, tiedForward);
                    event->get<Bool>(TIE_IS_ABOVE, tiedUp);

                    str << " ";
                } else if (event->isa(Indication::EventType)) {
                    preEventsToStart.insert(event);
                    preEventsInProgress.insert(event);
                    postEventsToStart.insert(event);
                    postEventsInProgress.insert(event);
                }

                if (i == chord.getFinalElement())
                    break;
            }

            if (chord.size() > 1)
                str << "> ";

            if (duration != prevDuration) {
                durationRatio = writeDuration(duration, str);
                str << " ";
                prevDuration = duration;
            }

            if (m_languageLevel == LILYPOND_VERSION_2_6) {
                // only one override per chord, and that outside the <>
                if (stylei != s->end()) {
                    writeStyle(*stylei, prevStyle, col, str, false);
                    stylei = s->end();
                }
            }

            if (lilyText != "") {
                str << lilyText;
                lilyText = "";
            }
            writeSlashes(event, str);

            writtenDuration += soundingDuration;
            std::pair<int,int> ratio = fractionProduct(durationRatio,tupletRatio);
            durationRatioSum = fractionSum(durationRatioSum, ratio);
            // str << qstrtostr(QString(" %{ %1/%2 * %3/%4 = %5/%6 %} ").arg(durationRatio.first).arg(durationRatio.second).arg(tupletRatio.first).arg(tupletRatio.second).arg(ratio.first).arg(ratio.second)); // DEBUG

            std::vector<Mark> marks(chord.getMarksForChord());
            // problem here: stem direction unavailable (it's a view-local property)
            bool stemUp = true;
            e->get<Bool>(NotationProperties::STEM_UP, stemUp);
            for (std::vector<Mark>::iterator j = marks.begin(); j != marks.end(); ++j) {
                str << composeLilyMark(*j, stemUp);
            }
            if (!marks.empty())
                str << " ";

            handleEndingPostEvents(postEventsInProgress, i, str);
            handleStartingPostEvents(postEventsToStart, str);

            if (tiedForward) {
                if (tiedUp) {
                    str << "^~ ";
                } else {
                    str << "_~ ";
                }
            }

            if (hiddenNote) {
                str << "\\unHideNotes ";
            }

            if (m_exportBeams && startingBeamedGroup && nextBeamedNoteInGroup && canStartOrEndBeam(event)) {
                // starting a beamed group
                str << "[ ";
                startingBeamedGroup = false;
                inBeamedGroup = true;
                //RG_DEBUG << "BEGIN GROUP" << groupId;
            }
        } else if (isRest) {

            const bool hiddenRest = event->has(INVISIBLE) && event->get<Bool>(INVISIBLE);

            // If the rest has a manually repositioned Y coordinate, we try to
            // create a letter note of an appropriate height, and bind a \rest
            // to it, in order to make repositioned rests exportable.  This is
            // necessary because LilyPond's automatic rest collision avoidance
            // frequently chokes to death on our untidy machine-generated files,
            // and yields terrible results, so we have to offer some manual
            // mechanism for adjusting the rests unless we want users to have to
            // edit .ly files by hand to correct for this, which we do not.
            bool offsetRest = event->has(DISPLACED_Y);
            int restOffset  = 0;
            if (offsetRest) {
                restOffset  = event->get<Int>(DISPLACED_Y);
            }

            //if (offsetRest) {
            //    RG_DEBUG << "REST OFFSET: " << restOffset;
            //} else {
            //    RG_DEBUG << "NO REST OFFSET";
            //}

            if (MultiMeasureRestCount == 0) {
                if (hiddenRest) {
                    RG_DEBUG << "HIDDEN REST.  Using duration " << duration;
                    str << "s";
                } else if (duration == timeSignature.getBarDuration()) {
                    // Look ahead the segment in order to detect
                    // the number of measures in the multi measure rest.
                    RG_DEBUG << "INCREMENTING MULTI-MEASURE COUNTER (offset rest height will be ignored)";
                    Segment::iterator mm_i = i;
                    while (s->isBeforeEndMarker(++mm_i)) {
                        if ((*mm_i)->isa(Note::EventRestType) &&
                            (*mm_i)->getNotationDuration() == event->getNotationDuration() &&
                            timeSignature == m_composition->getTimeSignatureAt((*mm_i)->getNotationAbsoluteTime())) {
                            MultiMeasureRestCount++;
                        } else {
                            break;
                        }
                    }
                    str << "R";
                } else {
                    handleEndingPreEvents(preEventsInProgress, i, str);
                    handleStartingPreEvents(preEventsToStart, str);

                    if (offsetRest) {
                        // translate the fine tuning of steps into steps
                        int offset = -(restOffset / 500);

                        // accept only even steps to imitate Rosegarden's behaviour
                        if (offset % 2 != 0) {
                            offset += (offset > 0 ? -1 : 1);
                        }
                        // move the default position of the rest
                        int heightOnStaff = 4 + offset;

                        // find out the pitch corresponding to the rest position
                        Clef m_lastClefFound((*s).getClefAtTime(event->getAbsoluteTime()));
                        Pitch helper(heightOnStaff, m_lastClefFound, Rosegarden::Key::DefaultKey);

                        // use MIDI pitch to get a named note with octavation
                        int p = helper.getPerformancePitch();
                        std::string n = convertPitchToLilyNote(p, Accidentals::NoAccidental,
                                                               Rosegarden::Key::DefaultKey);

                        // write named note
                        str << n;

                        RG_DEBUG << "Offsetting rest: "
                                  << "offset = " << offset << ", "
                                  << "heightOnStaff = " << heightOnStaff << ", "
                                  << "pitch = " << p << ", "
                                  << "note = " << n;

                        // defer the \rest until after any duration, because it
                        // can't come before a duration
                        // necessary, which is all determined a bit further on
                        needsSlashRest = true;

                    } else {
                        str << "r";

                        // a rest CAN have a fermata, and it could have a text
                        // mark conceivably, so we need to export these
                        std::vector<Mark> marks(Marks::getMarks(*event));
                        for (std::vector<Mark>::iterator j = marks.begin(); j != marks.end(); ++j) {
                            // even at the most optimistic, I doubt we'd ever
                            // find a way to get a stemUp from a rest, so just
                            // set this true, because a fermata is the main
                            // thing we're after, and it will want to go up top
                            // more often than not
                            str << composeLilyMark(*j, true);
                        }
                        if (!marks.empty())
                            str << " ";
                    }
                }
    
                if (duration != prevDuration) {
                    durationRatio = writeDuration(duration, str);
                    if (MultiMeasureRestCount > 0) {
                        str << "*" << (1 + MultiMeasureRestCount);
                    }
                    prevDuration = duration;
                }

                // have to add \rest to a fake rest note after any required
                // duration change
                if (needsSlashRest) {
                    str << "\\rest";
                    needsSlashRest = false;
                }

                if (lilyText != "") {
                    str << lilyText;
                    lilyText = "";
                }

                str << " ";
    
                handleEndingPostEvents(postEventsInProgress, i, str);
                handleStartingPostEvents(postEventsToStart, str);
            } else {
                MultiMeasureRestCount--;
            }
            writtenDuration += soundingDuration;
            std::pair<int,int> ratio = fractionProduct(durationRatio,tupletRatio);
            durationRatioSum = fractionSum(durationRatioSum, ratio);
            // str << qstrtostr(QString(" %{ %1/%2 * %3/%4 = %5/%6 %} ").arg(durationRatio.first).arg(durationRatio.second).arg(tupletRatio.first).arg(tupletRatio.second).arg(ratio.first).arg(ratio.second)); // DEBUG
        } else if (event->isa(Clef::EventType)) {

            try {
                // Incomplete: Set which note the clef should center on  (DMM - why?)
                // To allow octavation of the clef, enclose the clefname always with quotes.
                str << "\\clef \"";

                Clef clef(*event);
                const std::string clefType = clef.getClefType();
                str << lilyClefType(clefType);

                // save clef for later use by rests that need repositioned
                m_lastClefFound = clef;
                RG_DEBUG << "clef:" << clefType << "lastClefFound:" << m_lastClefFound.getClefType();

                // Transpose the clef one or two octaves up or down, if specified.
                int octaveOffset = clef.getOctaveOffset();
                if (octaveOffset > 0) {
                    str << "^" << 1 + 7*octaveOffset;
                } else if (octaveOffset < 0) {
                    str << "_" << 1 + 7*(-octaveOffset);
                }

                str << "\"" << std::endl << indent(col);

            } catch (Exception e) {
                RG_WARNING << "Bad clef: " << e.getMessage();
            }

        } else if (event->isa(Rosegarden::Key::EventType)) {
            // don't export invisible key signatures
            const bool hiddenKey = event->has(INVISIBLE) && event->get<Bool>(INVISIBLE);

            try {
                // Remember current key to write key signature only when
                // there is a key change
                Rosegarden::Key previousKey = key;

                // grab the value of the key anyway, so we know what it was for
                // future calls to writePitch() (fixes #2039048)
                key = Rosegarden::Key(*event);
                // then we only write a \key change to the export stream if the
                // key signature was meant to be visible
                if ((key != previousKey) && !hiddenKey) {
                    str << "\\key ";

                    Accidental accidental = Accidentals::NoAccidental;

                    str << convertPitchToLilyNoteName(key.getTonicPitch(), accidental, key);

                    if (key.isMinor()) {
                        str << " \\minor";
                    } else {
                        str << " \\major";
                    }
                    str << std::endl << indent(col);
                }

            } catch (Exception e) {
                RG_WARNING << "Bad key: " << e.getMessage();
            }

        } else if (event->isa(Text::EventType)) {

            if (!handleDirective(event, lilyText, nextBarIsAlt1, nextBarIsAlt2,
                                 nextBarIsDouble, nextBarIsEnd, nextBarIsDot)) {
                handleText(event, lilyText);
            }

        } else if (event->isa(Guitar::Chord::EventType)) {
            handleGuitarChord(i, str);
        }

        if (event->isa(Indication::EventType)) {
            preEventsToStart.insert(event);
            preEventsInProgress.insert(event);
            postEventsToStart.insert(event);
            postEventsInProgress.insert(event);
        }

        if ((isNote || isRest)) {
            bool endGroup = false;
            if (!nextBeamedNoteInGroup) {
                //RG_DEBUG << "Leaving beamed group" << groupId;
                // ending a beamed group
                if (m_exportBeams && inBeamedGroup) {
                    str << "] ";
                }
                inBeamedGroup = false;
                if (groupType == GROUP_TYPE_BEAMED)
                    endGroup = true;
            }
            if (groupType == GROUP_TYPE_TUPLED && !nextNoteInTuplet) {
                //RG_DEBUG << "Leaving tuplet group" << groupId;
                str << "} ";
                tupletRatio = std::pair<int, int>(1, 1);
                endGroup = true;
            }
            if (endGroup) {
                groupId = -1;
                groupType = "";
            }
        }

        ++i;
    } // end of the gigantic while loop, I think

    if (isGrace == 1) {
        isGrace = 0;
        // str << "%{ grace ends %} "; // DEBUG
        str << "} ";
    }

    if (lastStem != 0) {
        str << "\\stemNeutral ";
    }

    if (overlong) {
        str << std::endl << indent(col) <<
            qstrtostr(QString("% %1").
                      arg(QObject::tr("warning: overlong bar truncated here")));
    }

    //
    // Pad bars whose notes do not add up to the length of the bar.
    // This may happen if the note quantization fails somehow. 
    //
    if ((barStart + writtenDuration < barEnd) &&
        fractionSmaller(durationRatioSum, barDurationRatio)) {
        str << std::endl << indent(col) <<
            qstrtostr(QString("% %1").
                      arg(QObject::tr("warning: bar too short, padding with rests")));
        str << std::endl << indent(col) <<
            qstrtostr(QString("% %1 + %2 < %3  &&  %4/%5 < %6/%7").
                      arg(barStart).
                      arg(writtenDuration).
                      arg(barEnd).
                      arg(durationRatioSum.first).
                      arg(durationRatioSum.second).
                      arg(barDurationRatio.first).
                      arg(barDurationRatio.second))
            << std::endl << indent(col);
    
        durationRatio = writeSkip(timeSignature, writtenDuration,
                                  (barEnd - barStart) - writtenDuration, true, str);
        durationRatioSum = fractionSum(durationRatioSum, durationRatio);
    }
    //
    // Export bar and bar checks.
    //
    if (nextBarIsDouble) {
        str << "\\bar \"||\" ";
        nextBarIsDouble = false;
    } else if (nextBarIsEnd) {
        str << "\\bar \"|.\" ";
        nextBarIsEnd = false;
    } else if (nextBarIsDot) {
        str << "\\bar \":\" ";
        nextBarIsDot = false;
    } else if (MultiMeasureRestCount == 0) {
        str << " |";
    }
}

void
LilyPondExporter::writeTimeSignature(TimeSignature timeSignature,
                                     int col, std::ofstream &str)
{
    if (timeSignature.isHidden()) {
        str << indent (col)
            << "\\once \\override Staff.TimeSignature #'break-visibility = #(vector #f #f #f) "
            << std::endl;
    }
    //
    // It is not possible to jump between common time signature "C"
    // and fractioned time signature "4/4", because new time signature
    // is entered only if the time signature fraction changes.
    // Maybe some tweak is needed in order to allow the jumping between
    // "C" and "4/4" ? (HJJ)
    //
    if (timeSignature.isCommon() == false) {
        // use numberedtime signature: 4/4
        str << indent (col)
            << "\\once \\override Staff.TimeSignature #'style = #'() "
            << std::endl;
    }
    str << indent (col)
        << "\\time "
        << timeSignature.getNumerator() << "/"
        << timeSignature.getDenominator()
        << std::endl << indent(col);
}

std::pair<int,int>
LilyPondExporter::writeSkip(const TimeSignature &timeSig,
                            timeT offset,
                            timeT duration,
                            bool useRests,
                            std::ofstream &str)
{
    DurationList dlist;
    timeSig.getDurationListForInterval(dlist, duration, offset);
    std::pair<int,int> durationRatioSum(0,1);
    std::pair<int,int> durationRatio(0,1);

    int t = 0, count = 0;

    for (DurationList::iterator i = dlist.begin(); ; ++i) {

        if (i == dlist.end() || (*i) != t) {

            if (count > 0) {

                if (!useRests)
                    str << "\\skip ";
                else if (t == timeSig.getBarDuration())
                    str << "R";
                else
                    str << "r";

                durationRatio = writeDuration(t, str);

                if (count > 1) {
                    str << "*" << count;
                    durationRatio = fractionProduct(durationRatio,count);
                }
                str << " ";

                durationRatioSum = fractionSum(durationRatioSum,durationRatio);
            }

            if (i != dlist.end()) {
                t = *i;
                count = 1;
            }

        } else {
            ++count;
        }

        if (i == dlist.end())
            break;
    }
    return durationRatioSum;
}

bool
LilyPondExporter::handleDirective(const Event *textEvent,
                                  std::string &lilyText,
                                  bool &nextBarIsAlt1, bool &nextBarIsAlt2,
                                  bool &nextBarIsDouble, bool &nextBarIsEnd, bool &nextBarIsDot)
{
    Text text(*textEvent);

    if (text.getTextType() == Text::LilyPondDirective) {
        std::string directive = text.getText();
        if (directive == Text::FakeSegno) {
            lilyText += "^\\markup { \\musicglyph #\"scripts.segno\" } ";
        } else if (directive == Text::FakeCoda) {
            lilyText += "^\\markup { \\musicglyph #\"scripts.coda\" } ";
        } else if (directive == Text::Alternate1) {
            nextBarIsAlt1 = true;
        } else if (directive == Text::Alternate2) {
            nextBarIsAlt1 = false;
            nextBarIsAlt2 = true;
        } else if (directive == Text::BarDouble) {
            nextBarIsDouble = true;
        } else if (directive == Text::BarEnd) {
            nextBarIsEnd = true;
        } else if (directive == Text::BarDot) {
            nextBarIsDot = true;
        } else {
            // pass along less special directives for handling as plain text,
            // so they can be attached to chords and whatlike without
            // redundancy
            return false;
        }
        return true;
    } else {
        return false;
    }
}

void
LilyPondExporter::handleText(const Event *textEvent,
                             std::string &lilyText)
{
    try {

        Text text(*textEvent);
        std::string s = text.getText();

        // only protect illegal chars if this is Text, rather than
        // LilyPondDirective
        if ((*textEvent).isa(Text::EventType))
            s = protectIllegalChars(s);

        if (text.getTextType() == Text::Tempo) {

            // print above staff, bold, large
            lilyText += "^\\markup { \\bold \\large \"" + s + "\" } ";

        } else if (text.getTextType() == Text::LocalTempo) {

            // print above staff, bold, small
            lilyText += "^\\markup { \\bold \"" + s + "\" } ";
        } else if (m_chordNamesMode == false && text.getTextType() == Text::Chord) {

            // Either (1) the chords will have an own ChordNames context
            //     or (2) they will be printed above staff, bold, small.
            lilyText += "^\\markup { \\bold \"" + s + "\" } ";

        } else if (text.getTextType() == Text::Dynamic) {

            // supported dynamics first
            if (s == "ppppp" || s == "pppp" || s == "ppp" ||
                s == "pp" || s == "p" || s == "mp" ||
                s == "mf" || s == "f" || s == "ff" ||
                s == "fff" || s == "ffff" || s == "rfz" ||
                s == "sf") {

                lilyText += "-\\" + s + " ";

            } else {
                // export as a plain markup:
                // print below staff, bold italics, small
                lilyText += "_\\markup { \\bold \\italic \"" + s + "\" } ";
            }

        } else if (text.getTextType() == Text::Direction) {

            // print above staff, large
            lilyText += "^\\markup { \\large \"" + s + "\" } ";

        } else if (text.getTextType() == Text::LocalDirection) {

            // print below staff, bold italics, small
            lilyText += "_\\markup { \\bold \\italic \"" + s + "\" } ";

            // LilyPond directives that don't require special handling across
            // barlines are handled here along with ordinary text types.  These
            // can be injected wherever they happen to occur, and should get
            // attached to the right bits in due course without extra effort.
            //
        } else if (text.getText() == Text::Gliss) {
            lilyText += "\\glissando ";
        } else if (text.getText() == Text::Arpeggio) {
            lilyText += "\\arpeggio ";
        } else if (text.getText() == Text::Tiny) {
            lilyText += "\\tiny ";
        } else if (text.getText() == Text::Small) {
            lilyText += "\\small ";
        } else if (text.getText() == Text::NormalSize) {
            lilyText += "\\normalsize ";
        } else if (text.getText() == Text::Lyric) {
            // avoid printing a misleading warning that something is amiss when
            // it really isn't
        } else {
            textEvent->get
                <String>(Text::TextTypePropertyName, s);
            RG_WARNING << "LilyPondExporter::write() - unhandled text type: " << s;
        }
    } catch (Exception e) {
        RG_WARNING << "Bad text: " << e.getMessage();
    }
}

void
LilyPondExporter::writePitch(const Event *note,
                             const Rosegarden::Key &key,
                             std::ofstream &str)
{
    // Note pitch (need name as well as octave)
    // It is also possible to have "relative" pitches,
    // but for simplicity we always use absolute pitch
    // 60 is middle C, one unit is a half-step

    long pitch = 60;
    note->get
        <Int>(PITCH, pitch);

    Accidental accidental = Accidentals::NoAccidental;
    note->get
        <String>(ACCIDENTAL, accidental);

    // format of LilyPond note is:
    // name + octave + (duration) + text markup

    // calculate note name and write note
    std::string lilyNote;

    lilyNote = convertPitchToLilyNote(pitch, accidental, key);

    // handle parallel color

    if (note->has(BaseProperties::MEMBER_OF_PARALLEL)) {

        bool memberOfParallel;

        note->get<Bool>(BaseProperties::MEMBER_OF_PARALLEL, memberOfParallel);

        if (memberOfParallel) {
            str << "\\tweak color #magenta ";
        }
    }

    //RG_DEBUG << "NOTE" << lilyNote;
    str << lilyNote;
}

void
LilyPondExporter::writeStyle(const Event *note, std::string &prevStyle,
                             int col, std::ofstream &str, bool isInChord)
{
    // some hard-coded styles in order to provide rudimentary style export support
    // note that this is technically bad practice, as style names are not supposed
    // to be fixed but deduced from the style files actually present on the system
    const std::string styleMensural = "Mensural";
    const std::string styleTriangle = "Triangle";
    const std::string styleCross = "Cross";
    const std::string styleClassical = "Classical";

    // handle various note styles before opening any chord
    // brackets
    std::string style = "";
    note->get
        <String>(NotationProperties::NOTE_STYLE, style);

    if (style != prevStyle) {

        if (style == styleClassical && prevStyle == "")
            return ;

        if (!isInChord)
            prevStyle = style;

        if (style == styleMensural) {
            style = "mensural";
        } else if (style == styleTriangle) {
            style = "triangle";
        } else if (style == styleCross) {
            style = "cross";
        } else {
            style = "default"; // failsafe default or explicit
        }

        if (!isInChord) {
            str << std::endl << indent(col) << "\\override Voice.NoteHead #'style = #'" << style << std::endl << indent(col);
        } else {
            str << "\\tweak #'style #'" << style << " ";
        }
    }
}

std::pair<int,int>
LilyPondExporter::writeDuration(timeT duration,
                                std::ofstream &str)
{
    Note note(Note::getNearestNote(duration, MAX_DOTS));
    std::pair<int,int> durationRatio(0,1);

    switch (note.getNoteType()) {

    case Note::SixtyFourthNote:
        str << "64"; durationRatio = std::pair<int,int>(1,64);
        break;

    case Note::ThirtySecondNote:
        str << "32"; durationRatio = std::pair<int,int>(1,32);
        break;

    case Note::SixteenthNote:
        str << "16"; durationRatio = std::pair<int,int>(1,16);
        break;

    case Note::EighthNote:
        str << "8"; durationRatio = std::pair<int,int>(1,8);
        break;

    case Note::QuarterNote:
        str << "4"; durationRatio = std::pair<int,int>(1,4);
        break;

    case Note::HalfNote:
        str << "2"; durationRatio = std::pair<int,int>(1,2);
        break;

    case Note::WholeNote:
        str << "1"; durationRatio = std::pair<int,int>(1,1);
        break;

    case Note::DoubleWholeNote:
        str << "\\breve"; durationRatio = std::pair<int,int>(2,1);
        break;
    }

    for (int numDots = 0; numDots < note.getDots(); numDots++) {
        str << ".";
    }
    durationRatio = fractionProduct(durationRatio,
                                    std::pair<int,int>((1<<(note.getDots()+1))-1,1<<note.getDots()));
    return durationRatio;
}

void
LilyPondExporter::writeSlashes(const Event *note, std::ofstream &str)
{
    // if a grace note has tremolo slashes, they have already been used to turn
    // the note into a slashed grace note, and need not be exported here
    if (note->has(IS_GRACE_NOTE) && note->get<Bool>(IS_GRACE_NOTE)) return;

    // write slashes after text
    // / = 8 // = 16 /// = 32, etc.
    long slashes = 0;
    note->get
        <Int>(NotationProperties::SLASHES, slashes);
    if (slashes > 0) {
        str << ":";
        int length = 4;
        for (int c = 1; c <= slashes; c++) {
            length *= 2;
        }
        str << length;
    }
}

}
