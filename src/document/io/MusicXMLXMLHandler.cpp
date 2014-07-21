/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MusicXMLXMLHandler]"

#include "MusicXMLImportHelper.h"
#include "MusicXMLXMLHandler.h"
#include "base/Event.h"
#include "base/BaseProperties.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Studio.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "gui/editors/notation/NotationProperties.h"
#include "base/StaffExportTypes.h"
#include "base/Segment.h"
#include "base/Track.h"

#include <QString>
#include <QtGlobal>

namespace Rosegarden

{

using namespace BaseProperties;


MusicXMLXMLHandler::MusicXMLXMLHandler(Composition *composition, Studio *studio):
        m_composition(composition),
        m_studio(studio),
        m_errormessage("")
{}

MusicXMLXMLHandler::~MusicXMLXMLHandler()
{
    for (PartMap::iterator p = m_parts.begin(); p != m_parts.end(); ++p)
        delete (*p).second;
}

bool
MusicXMLXMLHandler::error(const QXmlParseException & exception)
{
    m_errormessage = QString("Error on line %1, column %2: %3")
                             .arg(exception.lineNumber())
                             .arg(exception.columnNumber())
                             .arg(exception.message());
    return false;
}

bool
MusicXMLXMLHandler::fatalError(const QXmlParseException & exception)
{
    m_errormessage = QString("Fatal error on line %1, column %2: %3")
                             .arg(exception.lineNumber())
                             .arg(exception.columnNumber())
                             .arg(exception.message());
    return false;
}

bool
MusicXMLXMLHandler::warning(const QXmlParseException & exception)
{
    m_errormessage = QString("Warning on line %1, column %2: %3")
                             .arg(exception.lineNumber())
                             .arg(exception.columnNumber())
                             .arg(exception.message());
    return true;
}

QString
MusicXMLXMLHandler::errorString () const
{
    return m_errormessage;
}

bool
MusicXMLXMLHandler::startDocument()
{
    m_currentState = NoData;
    m_inDynamics = false;
    m_brace = 0;
    m_bracket = 0;
    m_ignored = "";
    m_groupId = -1;
//     m_group = "";
    m_tupletGroup = false;
    m_beamGroup = false;
    return true;
}

void
MusicXMLXMLHandler::setDocumentLocator(QXmlLocator *locator)
{
    m_locator = locator;
}

bool
MusicXMLXMLHandler::startElement(const QString& /*namespaceURI*/,
                                 const QString& /*localName*/,
                                 const QString& qName,
                                 const QXmlAttributes& atts)
{
    // If m_ignored is not an empty string it contains the name of an element
    // which will be ignored, including all it children.
    if (m_ignored != "") return true;

    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

cerrInfo(QString("startElement : \"%1\"").arg(m_currentElement));

    bool ret = true;
    switch (m_currentState) {

    case NoData:
        if (m_currentElement == "score-partwise") {
            m_currentState = ReadHeader;
            ret = getAttributeString(atts, "version", m_mxmlVersion);
        } else if (m_currentElement == "score-timewise") {
            m_currentState = ReadHeader;
            ret = getAttributeString(atts, "version", m_mxmlVersion);
        }
        if (!ret)
            ret = startHeader(qName, atts);
        break;

    case ReadHeader:
        if (m_currentElement == "part-list") {
            m_currentState = ReadPartList;
            ret = startPartList(qName, atts);
        } else
            ret = startHeader(qName, atts);
        break;

    case ReadPartList:
        ret = startPartList(qName, atts);
        break;

    case ReadMusicData:
        if (m_currentElement == "measure") {
// for debugging only
            QString m;
            ret =  getAttributeString(atts, "number", m);
            cerrInfo(QString("Entering measure %1").arg(m));
        } else if (m_currentElement == "part") {
            ret =  getAttributeString(atts, "id", m_partId);
            if (m_parts.find(m_partId) == m_parts.end()) {
                m_errormessage = QString("part id \"%1\" not defined.").arg(m_partId);
                return false;
            }
        } else if (m_currentElement == "note") {
            m_currentState = ReadNoteData;
            ret = startNoteData(qName, atts);
        } else if (m_currentElement == "backup") {
            m_currentState = ReadBackupData;
            ret = startBackupData(qName, atts);
        } else if (m_currentElement == "direction") {
            m_currentState = ReadDirectionData;
            ret = startDirectionData(qName, atts);
        } else if (m_currentElement == "attributes") {
            m_currentState = ReadAttributesData;
            ret = startAttributesData(qName, atts);
        } else if (m_currentElement == "barline") {
            m_currentState = ReadBarlineData;
            ret = startBarlineData(qName, atts);
        }
        break;

    case ReadNoteData:
        ret = startNoteData(qName, atts);
        break;

    case ReadBackupData:
        ret = startBackupData(qName, atts);
        break;

    case ReadDirectionData:
        ret = startDirectionData(qName, atts);
        break;

    case ReadAttributesData:
        return
        startAttributesData(qName, atts);
        break;
    case ReadBarlineData:
        ret = startBarlineData(qName, atts);
        break;
    }

    return ret;
}

bool
MusicXMLXMLHandler::endElement(const QString& /*namespaceURI*/,
                               const QString& /*localName*/,
                               const QString& qName)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    // Still inside an ignored section?
    if (m_ignored != "") {
        // This is the last element?
        if (m_ignored == m_currentElement) m_ignored = "";
        return true;
    }

cerrInfo(QString("endElement : \"%1\"").arg(m_currentElement));


    // Start the real work!
    bool ret = true;
    switch (m_currentState) {

    case NoData:
        break;

    case ReadHeader:
        ret = endHeader(qName);
        break;

    case ReadPartList:
        ret = endPartList(qName);
        if (m_currentElement == "part-list")
            m_currentState = ReadMusicData;
        break;

    case ReadMusicData:
        break;

    case ReadNoteData:
        ret = endNoteData(qName);
        if (m_currentElement == "note")
            m_currentState = ReadMusicData;
        break;

    case ReadBackupData:
        ret = endBackupData(qName);
        if (m_currentElement == "backup")
            m_currentState = ReadMusicData;
        break;

    case ReadDirectionData:
        ret = endDirectionData(qName);
        if (m_currentElement == "direction")
            m_currentState = ReadMusicData;
        break;

    case ReadAttributesData:
        ret = endAttributesData(qName);
        if (m_currentElement == "attributes")
            m_currentState = ReadMusicData;
        break;

    case ReadBarlineData:
        ret = endBarlineData(qName);
        if (m_currentElement == "barline")
            m_currentState = ReadMusicData;
        break;
    }

    return ret;
}

bool
MusicXMLXMLHandler::characters(const QString& chars)
{
    // If m_ignored is not an empty string it contains the name of an element
    // which will be ignored, including all it children.
    if (m_ignored != "") return true;

    // S
    m_characters = chars.trimmed();
    return true;
}

bool
MusicXMLXMLHandler::endDocument()
{
    RG_DEBUG << "MusicXMLXMLHandler::endDocument" << endl;
    return true;
}

bool
MusicXMLXMLHandler::startHeader(const QString& qName,
                                const QXmlAttributes& /* atts */)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    return true;
}

bool
MusicXMLXMLHandler::endHeader(const QString& qName)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    return true;
}

bool
MusicXMLXMLHandler::startPartList(const QString& qName,
                                 const QXmlAttributes& atts)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    bool ret = true;
    if (m_currentElement == "part-list") {
        // no action required here
    } else if (m_currentElement == "part-group") {
        // A simple algorithme is used. Overlapping braces or overlapping brackets
        // are not supported. However braces an brackets may overlap.
        // A complication arises in case of multi staff systems because these
        // will imply a brace which can cause implicit overlapping brace. Since
        // these overlapping braces are merges a new, extended multi staff system
        // can the result.
        QString type;
        if (!getAttributeString(atts, "type", type)) return false;
        if (!getAttributeInteger(atts, "number", m_number, false, 0)) return false;
        if (type.toLower() == "start") {
            // Next group-symbol implies a start, so we can ignore the start here.
        } else if (type.toLower() == "stop") {
            if (m_bracket == -m_number) {
                m_parts[m_partId]->setBracketType(Brackets::SquareOff);
                m_bracket = 0;
            }
            if (m_brace == -m_number) {
                m_parts[m_partId]->setBracketType(Brackets::CurlyOff);
                m_brace = 0;
            }
        } else
            cerrWarning(QString("type \"%1\" ignored.").arg(type));
    } else if (m_currentElement == "group-symbol") {
        // no action required here
    } else if (m_currentElement == "score-part") {
        ret = getAttributeString(atts, "id", m_partId);
        m_parts[m_partId] = new MusicXMLImportHelper(m_composition);
        if (m_brace > 0) {
            m_parts[m_partId]->setBracketType(Brackets::CurlyOn);
            m_brace = -m_brace;
        }
        if (m_bracket > 0) {
            m_parts[m_partId]->setBracketType(Brackets::SquareOn);
            m_bracket = -m_bracket;
        }
    } else if ((m_currentElement == "score-instrument") ||
            (m_currentElement == "instrument-name")) {
        // These elements get ignored
    } else if (m_currentElement == "midi-instrument") {
        ret = getAttributeString(atts, "id", m_midiInstrument);
    } else if ((m_currentElement == "volume") ||
              (m_currentElement == "pan")) {
        //  Probably could be supported, but far easier to ignore
    } else if ((m_currentElement == "part-name") ||
            (m_currentElement == "midi-channel") ||
            (m_currentElement == "midi-program") ||
            (m_currentElement == "midi-unpitched")) {
        // No action is required here; the character contents of these are
        // processed via endElement().
    } else {
        // For debugging
        RG_DEBUG << "MusicXMLXMLHandler::startPartList: cannot process element \'"
                 << m_currentElement
                 << "\'"
                 << endl;
        // I did a test in a clean bash environment (no extra environment
        // variables)  with a clean release build, and Rosegarden crashed on
        // this assert.  I added -DQT_NO_DEBUG_OUTPUT and -dQT_NO_WARNING_OUTPUT
        // to configure.ac, did another clean release build, and Rosegarden
        // continued to crash on this assert.  According to the Qt API docs that
        // should not be, but it is.  Plainly it is not safe to use Q_ASSERT in
        // production code.
        //Q_ASSERT(0);
    }
    
    return ret;
}

bool
MusicXMLXMLHandler::endPartList(const QString& qName)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    if (m_currentElement == "part-list") {
        // no action required here
    } else if (m_currentElement == "part-group") {
        // no action required here
    } else if (m_currentElement == "group-symbol") {
        // This implies a part-group type="start".
        if (m_characters.toLower() == "brace") {
            if (m_brace == 0) {
                m_brace = m_number;
            } else {
                cerrWarning("Overlapping braces are not supported, this brace is ignored!");
            }
        } else if (m_characters.toLower() == "bracket") {
            if (m_bracket == 0) {
                m_bracket = m_number;
            } else {
                cerrWarning("Overlapping brackets are not supported, this bracket is ignored!");
            }
        } else {
            cerrWarning(QString("group-symbol \"%1\" not supported, ignored.").arg(m_characters));
        }
    } else if (m_currentElement == "score-part") {
//         Instrument *instr = m_studio->assignMidiProgramToInstrument(
//                                             m_midiProgram-1,
//                                             m_parts[m_partId]->isPercussion());
//         m_parts[m_partId]->setInstrument(instr->getId());
    } else if (m_currentElement == "score-instrument") {
        // ignored
    } else if (m_currentElement == "instrument-name") {
        // ignored
    } else if (m_currentElement == "part-name") {
        m_parts[m_partId]->setLabel(m_characters);
    } else if (m_currentElement == "midi-instrument") {
        // No action required here
    } else if (m_currentElement == "midi-channel") {
        if (!checkInteger(m_currentElement, m_midiChannel))
            return false;
    } else if (m_currentElement == "midi-program") {
        if (!checkInteger(m_currentElement, m_midiProgram))
            return false;

    } else if (m_currentElement == "midi-unpitched") {
        int pitch;
        if (!checkInteger(m_currentElement, pitch))
            return false;
        m_parts[m_partId]->addPitch(m_midiInstrument, pitch-1);
    }
    return true;
}

bool
MusicXMLXMLHandler::startNoteData(const QString& qName,
                                   const QXmlAttributes& atts)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    if (m_currentElement == "note") {
        m_event = new Event(Note::EventType, 0); // A dummy note,makes sure m_event is defined.
        m_isGrace = false;
        m_hasGraceNotes = false;
        m_type = 0;
        m_dots = 0;
        m_untupletcount = 0;
        m_tupletcount = 0;
        m_duration = 0;
        m_staff = "1";
        m_voice = "";
        m_chord = false;
    } else if (m_currentElement == "grace") {
        // no action required here
    } else if (m_currentElement == "cue") {
        ignoreElement();
    } else if (m_currentElement == "chord") {
        // no action required here
    } else if (m_currentElement == "pitch") {
        m_step = 'C';
        m_accidental = Accidentals::NoAccidental;
        m_octave = 4;
    } else if (m_currentElement == "step") {
        // no action required here
    } else if (m_currentElement == "alter") {
        // no action required here
    } else if (m_currentElement == "octave") {
        // no action required here
    } else if (m_currentElement == "rest") {
        // no action required here
    } else if (m_currentElement == "unpitched") {
        // no action required here
    } else if (m_currentElement == "display-step") {
        // ignored
    } else if (m_currentElement == "display-octave") {
        // ignored
    } else if (m_currentElement == "duration") {
        // no action required here
    } else if (m_currentElement == "tie") {
        //
    } else if (m_currentElement == "instrument") {
        QString instrument;
        if (!getAttributeString(atts, "id", instrument)) return false;
        int pitch = m_parts[m_partId]->getPitch(instrument);
        if (pitch < 0) {
            cerrError(QString("id \"%1\" not defined.").arg(instrument));
            pitch = 60; // just a default!
        }
        m_event->set<Int>(PITCH, pitch);
    } else if (m_currentElement == "voice") {
        // no action required here
    } else if (m_currentElement == "type") {
        // no action required here
    } else if (m_currentElement == "dot") {
        // no action required here
    } else if (m_currentElement == "accidental") {
        // no action required here
    } else if (m_currentElement == "time-modification") {
        // m_tupletGroup is set here and not during <tuplet type="start"> to make
        // sure m_tupletGroup is set before <beam> is encountered.
        m_tupletGroup = true;
        if (m_groupId < 0) m_groupId = -m_groupId;
    } else if (m_currentElement == "actual-notes") {
        // no action required here
    } else if (m_currentElement == "normal-notes") {
        // no action required here
    } else if (m_currentElement == "normal-type") {
        m_type = 0;
        m_dots = 0;
    } else if (m_currentElement == "normal-dot") {
        // no action required here
    } else if (m_currentElement == "stem") {
        // no action required here
    } else if (m_currentElement == "notehead") {
        ignoreElement();
    } else if (m_currentElement == "staff") {
        // no action required here
    } else if (m_currentElement == "beam") {
        int tmp;
        if (!getAttributeInteger(atts, "number", tmp))
            return false;
        if ((tmp == 1) && !m_beamGroup) {
            m_beamGroup = true;
            if (m_groupId < 0) m_groupId = -m_groupId;
        }
    } else if (m_currentElement == "notations") {
        // no action required here
    } else if (m_currentElement == "tied") {
        QString type;
        if (!getAttributeString(atts, "type", type)) return false;
        if (type.toLower() == "start")
            m_event->set<Bool>(TIED_FORWARD, true);
        else if (type.toLower() == "stop")
            m_event->set<Bool>(TIED_BACKWARD, true);
        else
            cerrWarning(QString("Undefined type \"%1\", ignored.").arg(type));
    } else if (m_currentElement == "slur") {
        int number;
        if (!getAttributeInteger(atts, "number", number, false, 1))
            return false;
        QString type;
        if (!getAttributeString(atts, "type", type)) return false;
        m_parts[m_partId]->setStaff(m_staff);
        m_parts[m_partId]->setVoice(m_voice);
        if (type.toLower() == "start")
            m_parts[m_partId]->startIndication(Indication::Slur, number);
        else if (type.toLower() == "stop")
            m_parts[m_partId]->endIndication(Indication::Slur, number, m_duration);
        else
            cerrWarning(QString("Undefined type \"%1\", ignored.").arg(type));
    } else if (m_currentElement == "tuplet") {
        int number;
        if (!getAttributeInteger(atts, "number", number, false, 1))
            return false;
        if (number > 1)
            cerrWarning("Nested tuplets not supported, ignored.");
        QString type;
        if (!getAttributeString(atts, "type", type)) return false;
        if (type.toLower() == "start") {
            // start is implicit at <time-modifications>.
        } else if (type.toLower() == "stop") {
            m_tupletGroup = false;
            if (!m_tupletGroup && !m_beamGroup)
                m_groupId = -(m_groupId + 1);
        } else
            cerrWarning(QString("Undefined type \"%1\", ignored.").arg(type));
    } else if (m_currentElement == "tuplet-actual") {
        // no action required here
    } else if (m_currentElement == "tuplet-normal") {
        // no action required here
    } else if (m_currentElement == "glissando") {
        //
    } else if (m_currentElement == "slide") {
        //
    } else if (m_currentElement == "other-notation") {
        //
    } else if (m_currentElement == "ornament") {
        // no action required here
    } else if (m_currentElement == "trill-mark") {
        Marks::addMark(*m_event, Marks::Trill, true);
    } else if (m_currentElement == "turn") {
        Marks::addMark(*m_event, Marks::Turn, true);
    } else if (m_currentElement == "delayed-turn") {
        //
    } else if (m_currentElement == "inverted-turn ") {
        //
    } else if (m_currentElement == "shake") {
        //
    } else if (m_currentElement == "wavy-line") {
        //
    } else if (m_currentElement == "mordent") {
        Marks::addMark(*m_event, Marks::Mordent, true);
    } else if (m_currentElement == "inverted-mordent") {
        Marks::addMark(*m_event, Marks::MordentInverted, true);
    } else if (m_currentElement == "schleifer") {
        //
    } else if (m_currentElement == "tremolo") {
        //
    } else if (m_currentElement == "other-ornament") {
        //
    } else if (m_currentElement == "accidental-mark") {
        //
    } else if (m_currentElement == "technical") {
        // no action required here
    } else if (m_currentElement == "up-bow") {
        Marks::addMark(*m_event, Marks::UpBow, true);
    } else if (m_currentElement == "down-bow") {
        Marks::addMark(*m_event, Marks::DownBow, true);
    } else if (m_currentElement == "harmonic") {
        Marks::addMark(*m_event, Marks::Harmonic, true);
    } else if (m_currentElement == "open-string") {
        Marks::addMark(*m_event, Marks::Open, true);
    } else if (m_currentElement == "thumb-position") {
        //
    } else if (m_currentElement == "fingering") {
        //
    } else if (m_currentElement == "pluck") {
        //
    } else if (m_currentElement == "double-tongue") {
        //
    } else if (m_currentElement == "triple-tongue") {
        //
    } else if (m_currentElement == "stopped") {
        Marks::addMark(*m_event, Marks::Stopped, true);
    } else if (m_currentElement == "snap-pizzicato") {
        //
    } else if (m_currentElement == "fret") {
        //
    } else if (m_currentElement == "string") {
        //
    } else if (m_currentElement == "hammer-on") {
        //
    } else if (m_currentElement == "pull-off") {
        //
    } else if (m_currentElement == "bend") {
        //
    } else if (m_currentElement == "tap") {
        //
    } else if (m_currentElement == "heel") {
        //
    } else if (m_currentElement == "toe") {
        //
    } else if (m_currentElement == "fingernails") {
        //
    } else if (m_currentElement == "other-technical") {
        //
    } else if (m_currentElement == "articulations") {
        // no action required here
    } else if (m_currentElement == "accent") {
        Marks::addMark(*m_event, Marks::Accent, true);
    } else if (m_currentElement == "strong-accent") {
        Marks::addMark(*m_event, Marks::Marcato, true);
    } else if (m_currentElement == "staccato") {
        Marks::addMark(*m_event, Marks::Staccato, true);
    } else if (m_currentElement == "tenuto") {
        Marks::addMark(*m_event, Marks::Tenuto, true);
    } else if (m_currentElement == "detached-legato") {
        //
    } else if (m_currentElement == "staccatissimo") {
        Marks::addMark(*m_event,Marks::Staccatissimo , true);
    } else if (m_currentElement == "spiccato") {
        //
    } else if (m_currentElement == "scoop") {
        //
    } else if (m_currentElement == "plop") {
        //
    } else if (m_currentElement == "doit") {
        //
    } else if (m_currentElement == "falloff") {
        //
    } else if (m_currentElement == "breath-mark") {
        //
    } else if (m_currentElement == "caesura") {
        //
    } else if (m_currentElement == "stress") {
        //
    } else if (m_currentElement == "unstress") {
        //
    } else if (m_currentElement == "other-articulation") {
        //
    } else if (m_currentElement == "dynamics") {
        m_inDynamics = true;
    } else if (m_currentElement == "fermata") {
        //
    } else if (m_currentElement == "lyric") {
        if (!getAttributeInteger(atts, "number", m_verse, false, 1))
            return false;
    } else if (m_currentElement == "elision") {
        // ignored
    } else if (m_currentElement == "syllabic") {
        // no action required here
    } else if (m_currentElement == "text") {
        // no action required here
    } else if (m_currentElement == "extend") {
        // ignored
    } else if (m_currentElement == "laughing") {
        // ignored
    } else if (m_currentElement == "humming") {
        // ignored
    } else if (m_currentElement == "end-line") {
        // ignored
    } else if (m_currentElement == "end-paragraph") {
        // ignored
    }

    return true;
}

bool
MusicXMLXMLHandler::endNoteData(const QString& qName)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    // Childs of <dynamics> will be handled separate because it is shared
    // between <note> and <direction> and handling it separate makes the
    // the code much  simpler.
    handleDynamics();

    if (m_currentElement == "note") {
        if (m_event->isa(Rosegarden::Note::EventType)) {
            m_event->set<Int>(VELOCITY, 100);
        }
        timeT t = m_parts[m_partId]->getCurTime() - (m_chord ? m_event->getDuration() : 0);
        timeT notationDuration = m_event->getNotationDuration();
        if (m_isGrace) {
            Note note(m_type, m_dots);
            notationDuration = note.getDuration();
        } else {
            if (m_hasGraceNotes) {
                m_event->set<Bool>(HAS_GRACE_NOTES, true);
                m_hasGraceNotes = false;
            }
        }
        Event *event = new Event(*m_event,
                                 t,
                                 m_event->getDuration(),
                                 m_event->getSubOrdering(),
                                 t,
                                 notationDuration
                                );
        m_parts[m_partId]->setStaff(m_staff);
        m_parts[m_partId]->setVoice(m_voice);
        m_parts[m_partId]->insert(event);
        delete m_event;
    } else if (m_currentElement == "grace") {
        m_isGrace = true;
        m_hasGraceNotes = true;
    } else if (m_currentElement == "cue") {
        cerrWarning("Cue notes are not supported and will be replaced by normal notes.");
    } else if (m_currentElement == "chord") {
        m_chord = true;
    } else if (m_currentElement == "pitch") {
        // Create an empty note event.
        delete m_event;
        m_event = new Event(Note::EventType, 0, 0, (m_isGrace ? -1 : 0));
        if (m_isGrace)
            m_event->set<Bool>(IS_GRACE_NOTE, true);
        Pitch pitch(m_step, m_octave, Key(), m_accidental, -1);
        m_event->set<Int>(PITCH, pitch.getPerformancePitch());
        if (m_accidental != Accidentals::NoAccidental)
            m_event->set<String>(ACCIDENTAL, m_accidental);
    } else if (m_currentElement == "step") {
        m_step = m_characters.toUpper().at(0).toLatin1();
    } else if (m_currentElement == "alter") {
        // Although a floating point value is acceptable, the only
        // valid non-integer values are -1.5, -0.5, 0.5, 1.5.  So some
        // Extra sanity checking is required.
        //
        // Note: Rosegarden does not currently do anything with the
        //       .5 values other than storing them so the handling logic
        //       here just stops the import failing.
        
        float alter;
        if (!checkFloat(m_currentElement, alter))
            return false;

        int alter_as_int = int(alter * 2.0);

        if (float(alter_as_int) / 2.0 != alter) {
            m_errormessage = QString("Bad value \"%1\" for <alter>")
                                .arg(m_characters);
            return false;
        }

        // Convert to int to allow switch statement
        switch(alter_as_int) {
        case -4: // Original -2
            m_accidental = Accidentals::DoubleFlat;
            break;

        case -3: // Original -1.5
            m_accidental = Accidentals::ThreeQuarterFlat;
            break;

        case -2: // Original -1
            m_accidental = Accidentals::Flat;
            break;

        case -1: // Original -0.5
            m_accidental = Accidentals::QuarterFlat;
            break;

        case 0: // Original 0
            m_accidental = Accidentals::Natural; //NoAccidental;
            break;

        case 1: // Original 0.5
            m_accidental = Accidentals::QuarterSharp;
            break;

        case 2: // Original 1
            m_accidental = Accidentals::Sharp;
            break;

        case 3: // Original 1.5
            m_accidental = Accidentals::ThreeQuarterSharp;
            break;

        case 4: // Original 2
            m_accidental = Accidentals::DoubleSharp;
            break;

        default:
            m_errormessage = QString("Bad value \"%1\" for <alter>")
                                .arg(m_characters);
            return false;
            
        }
    } else if (m_currentElement == "octave") {
        if (!checkInteger(m_currentElement, m_octave))
            return false;
    } else if (m_currentElement == "rest") {
        // Create an empty rest event.
        delete m_event;
         m_event = new Event(Note::EventRestType, 0, 0, (m_isGrace ? -1 : 0));
        if (m_isGrace)
            m_event->set<Bool>(IS_GRACE_NOTE, true);
    } else if (m_currentElement == "unpitched") {
        // Create an empty note event.
        delete m_event;
        m_event = new Event(Note::EventType, 0, 0, (m_isGrace ? -1 : 0));
        if (m_isGrace)
            m_event->set<Bool>(IS_GRACE_NOTE, true);
    } else if (m_currentElement == "display-step") {
        // ignored
    } else if (m_currentElement == "display-octave") {
        // ignored
    } else if (m_currentElement == "duration") {
        if (!checkInteger(m_currentElement, m_duration))
            return false;
        m_duration = m_duration * 960 / m_parts[m_partId]->getDivisions();
        Event *tmp = m_event;
        m_event = new Event(*tmp, 0, m_duration);
        delete tmp;
    } else if (m_currentElement == "tie") {
        //
    } else if (m_currentElement == "instrument") {
        // no action required here
    } else if (m_currentElement == "voice") {
        int dummy;
        if (!checkInteger(m_currentElement, dummy))
            return false;
        m_voice = m_characters;
    } else if (m_currentElement == "type") {
        handleNoteType();
    } else if (m_currentElement == "dot") {
        m_dots++;
    } else if (m_currentElement == "accidental") {
//         Accidental accidental;
//         if (m_characters.toLower() == "natural")
//             accidental = Accidentals::NoAccidental;
//         else if (m_characters.toLower() == "sharp")
//             accidental = Accidentals::Sharp;
//         else if (m_characters.toLower() == "flat")
//             accidental = Accidentals::Flat;
//         else if (m_characters.toLower() == "sharp-sharp")
//             accidental = Accidentals::DoubleSharp;
//         else if (m_characters.toLower() == "flat-flat")
//             accidental = Accidentals::DoubleFlat;
//         else
//             accidental = Accidentals::NoAccidental;
//         Pitch pitch(m_pitch, m_accidental);
//         std::cerr << "<accidental> = \"" << m_characters << "\" (" << pitch.getAsString() << ")\n";
//         std::cerr << "         accidental, m_accidental, pitch = \"" << accidental << "\", \"" << m_accidental << "\", " << m_pitch << std::endl;
//         std::cerr << "         Pitch::getAccidental(key) = \""
//                   << pitch.getAccidental(getSegment()->getKeyAtTime(getTime())) << "\"\n";
//         std::cerr << "         Pitch::getDisplayAccidental(key) = \""
//                   << pitch.getDisplayAccidental(getSegment()->getKeyAtTime(getTime())) << "\"\n";
//         //! NOTE getSegment() won't work here, staff is not known yet!
//         if (m_characters.toStdString() == pitch.getAccidental(getSegment()->getKeyAtTime(getTime())))
//             m_event->set<Bool>(NotationProperties::USE_CAUTIONARY_ACCIDENTAL, true);
    } else if (m_currentElement == "time-modification") {
        Note note(m_type, m_dots);
        m_event->set<String>(BEAMED_GROUP_TYPE, GROUP_TYPE_TUPLED);
        m_event->set<Int>(BEAMED_GROUP_ID, m_groupId);
        // It seems a BEAMED_GROUP_TUPLET_BASE on grace notes results in a
        // wrong grace tuplet. Why? I don't know!
        if (!m_isGrace)
            m_event->set<Int>(BEAMED_GROUP_TUPLET_BASE, note.getDuration());
        m_event->set<Int>(BEAMED_GROUP_TUPLED_COUNT, m_tupletcount);
        m_event->set<Int>(BEAMED_GROUP_UNTUPLED_COUNT, m_untupletcount);
    } else if (m_currentElement == "actual-notes") {
        if (!checkInteger(m_currentElement, m_untupletcount))
            return false;
    } else if (m_currentElement == "normal-notes") {
        if (!checkInteger(m_currentElement, m_tupletcount))
            return false;
    } else if (m_currentElement == "normal-type") {
        handleNoteType();
    } else if (m_currentElement == "normal-dot") {
        m_dots++;
    } else if (m_currentElement == "stem") {
        if (m_characters.toLower() == "up")
            m_event->set<Bool>(NotationProperties::STEM_UP, true);
        else if (m_characters.toLower() == "down")
            m_event->set<Bool>(NotationProperties::STEM_UP, false);
        else
            cerrWarning(QString("Only \"up\" and \"down\" are supported, \"%1\" ignored.").arg(m_characters));
    } else if (m_currentElement == "notehead") {
        // no action required here
    } else if (m_currentElement == "staff") {
        int dummy;
        if (!checkInteger(m_currentElement, dummy))
            return false;
        m_staff = m_characters;
    } else if (m_currentElement == "beam") {
        if (m_beamGroup) {
            if (!m_tupletGroup) {
                m_event->set<String>(BEAMED_GROUP_TYPE, GROUP_TYPE_BEAMED);
                m_event->set<Int>(BEAMED_GROUP_ID, m_groupId);
            }
            if (m_characters.toLower().toStdString() == "end") {
                m_beamGroup = false;
                if (!m_tupletGroup && !m_beamGroup)
                    m_groupId = -(m_groupId + 1);
            }
        }
    } else if (m_currentElement == "notations") {
        // no action required here
    } else if (m_currentElement == "tied") {
        // no action required here
    } else if (m_currentElement == "slur") {
        // no action required here
    } else if (m_currentElement == "tuplet") {
        // no action required here
    } else if (m_currentElement == "tuplet-actual") {
        // not needed
    } else if (m_currentElement == "tuplet-normal") {
        // not needed
    } else if (m_currentElement == "glissando") {
        //
    } else if (m_currentElement == "slide") {
        //
    } else if (m_currentElement == "other-notation") {
        //
    } else if (m_currentElement == "ornament") {
        // no action required here
    } else if (m_currentElement == "trill-mark") {
        // no action required here
    } else if (m_currentElement == "turn") {
        // no action required here
    } else if (m_currentElement == "delayed-turn") {
        //
    } else if (m_currentElement == "inverted-turn ") {
        //
    } else if (m_currentElement == "shake") {
        //
    } else if (m_currentElement == "wavy-line") {
        //
    } else if (m_currentElement == "mordent") {
        // no action required here
    } else if (m_currentElement == "inverted-mordent") {
        // no action required here
    } else if (m_currentElement == "schleifer") {
        //
    } else if (m_currentElement == "tremolo") {
        //
    } else if (m_currentElement == "other-ornament") {
        //
    } else if (m_currentElement == "accidental-mark") {
        //
    } else if (m_currentElement == "technical") {
        // no action required here
    } else if (m_currentElement == "up-bow") {
        // no action required here
    } else if (m_currentElement == "down-bow") {
        // no action required here
    } else if (m_currentElement == "harmonic") {
        // no action required here
    } else if (m_currentElement == "open-string") {
        // no action required here
    } else if (m_currentElement == "thumb-position") {
        //
    } else if (m_currentElement == "fingering") {
        //
    } else if (m_currentElement == "pluck") {
        //
    } else if (m_currentElement == "double-tongue") {
        //
    } else if (m_currentElement == "triple-tongue") {
        //
    } else if (m_currentElement == "stopped") {
        // no action required here
    } else if (m_currentElement == "snap-pizzicato") {
        //
    } else if (m_currentElement == "fret") {
        //
    } else if (m_currentElement == "string") {
        //
    } else if (m_currentElement == "hammer-on") {
        //
    } else if (m_currentElement == "pull-off") {
        //
    } else if (m_currentElement == "bend") {
        //
    } else if (m_currentElement == "tap") {
        //
    } else if (m_currentElement == "heel") {
        //
    } else if (m_currentElement == "toe") {
        //
    } else if (m_currentElement == "fingernails") {
        //
    } else if (m_currentElement == "other-technical") {
        //
    } else if (m_currentElement == "articulations") {
        // no action required here
    } else if (m_currentElement == "accent") {
        // no action required here
    } else if (m_currentElement == "strong-accent") {
        // no action required here
    } else if (m_currentElement == "staccato") {
        // no action required here
    } else if (m_currentElement == "tenuto") {
        // no action required here
    } else if (m_currentElement == "detached-legato") {
        //
    } else if (m_currentElement == "staccatissimo") {
        // no action required here
    } else if (m_currentElement == "spiccato") {
        //
    } else if (m_currentElement == "scoop") {
        //
    } else if (m_currentElement == "plop") {
        //
    } else if (m_currentElement == "doit") {
        //
    } else if (m_currentElement == "falloff") {
        //
    } else if (m_currentElement == "breath-mark") {
        //
    } else if (m_currentElement == "caesura") {
        //
    } else if (m_currentElement == "stress") {
        //
    } else if (m_currentElement == "unstress") {
        //
    } else if (m_currentElement == "other-articulation") {
        //
    } else if (m_currentElement == "dynamics") {
        m_dynamic = "text_" + m_dynamic;
        Marks::addMark(*m_event, m_dynamic, true);
    } else if (m_currentElement == "fermata") {
        //
    } else if (m_currentElement == "lyric") {
        // no action required here
    } else if (m_currentElement == "elision") {
        // ignored
    } else if (m_currentElement == "syllabic") {
        m_multiSyllabic = (m_characters.toLower() == "begin") ||
                          (m_characters.toLower() == "middle");
    } else if (m_currentElement == "text") {
        QString str = m_characters + (m_multiSyllabic ? "-" : "");
        Text text = Text(str.toStdString() , Text::Lyric);
        text.setVerse(m_verse);
        m_parts[m_partId]->setStaff(m_staff);
        m_parts[m_partId]->setVoice(m_voice);
        m_parts[m_partId]->insert(text.getAsEvent(m_parts[m_partId]->getCurTime()));
    } else if (m_currentElement == "extend") {
        // ignored
    } else if (m_currentElement == "laughing") {
        // ignored
    } else if (m_currentElement == "humming") {
        // ignored
    } else if (m_currentElement == "end-line") {
        // ignored
    } else if (m_currentElement == "end-paragraph") {
        // ignored
    }

    return true;
}

bool
MusicXMLXMLHandler::startBackupData(const QString& qName,
                                   const QXmlAttributes& /* atts */)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    if (m_currentElement == "backup") {
        //
    } else if (m_currentElement == "duration") {
        //
    }

    return true;
}

bool
MusicXMLXMLHandler::endBackupData(const QString& qName)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    if (m_currentElement == "backup") {
        //
    } else if (m_currentElement == "duration") {
        int duration;
        if (!checkInteger(m_currentElement, duration))
            return false;
        m_parts[m_partId]->moveCurTimeBack(duration);
    }

    return true;
}

bool
MusicXMLXMLHandler::startDirectionData(const QString& qName,
                                   const QXmlAttributes& atts)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    if (m_currentElement == "direction") {
        m_staff = "1";
        m_voice = "";
        m_directionStart = NotActive;
    } else if (m_currentElement == "direction-type") {
        //
    } else if (m_currentElement == "rehearsal") {
        //
    } else if (m_currentElement == "segno") {
        //
    } else if (m_currentElement == "words") {
        //
    } else if (m_currentElement == "coda") {
        //
    } else if (m_currentElement == "wedge") {
        if (!getAttributeInteger(atts, "number", m_number, false, 1))
            return false;
        QString type;
        if (!getAttributeString(atts, "type", type)) return false;
        m_indicationEnd = "wedge";
        if (type.toLower() == "crescendo") {
            m_directionStart = Start;
            m_indicationStart = Indication::Crescendo;
        } else if (type.toLower() == "diminuendo") {
            m_directionStart = Start;
            m_indicationStart = Indication::Decrescendo;
        } else if (type.toLower() == "stop") {
            m_directionStart = Stop;
        } else {
            cerrWarning(QString("Undefined type \"%1\", ignored.").arg(type));
        }
    } else if (m_currentElement == "dynamics") {
        m_inDynamics = true;
    } else if (m_currentElement == "dashes") {
        //
    } else if (m_currentElement == "pedal") {
        //
    } else if (m_currentElement == "octave-shift") {
        if (!getAttributeInteger(atts, "number", m_number, false, 1))
            return false;

        QString size;
        if (!getAttributeString(atts, "size", size, false, "8")) return false;
        if ((size != "8") && (size != "15")) {
            cerrWarning(QString("Invalid value \"%1\" for size, element ignored.").arg(size));
            return true;
        }
        bool oneOctave = size == "8";

        QString type;
        if (!getAttributeString(atts, "type", type)) return false;
        if (type.toLower() == "up") {
            m_directionStart = Start;
            m_indicationStart = oneOctave ? Indication::OttavaUp : Indication::QuindicesimaUp;
        } else if (type.toLower() == "down") {
            m_directionStart = Stop;
            m_indicationStart = oneOctave ? Indication::OttavaDown : Indication::QuindicesimaDown;
        } else if (type.toLower() == "stop") {
            m_directionStart = Stop;
        } else {
            cerrWarning(QString("Undefined type \"%1\", ignored.").arg(type));
        }
        m_indicationEnd = m_indicationStart;
    } else if (m_currentElement == "other-direction") {
        //
    } else if (m_currentElement == "voice") {
        // no action required here
    } else if (m_currentElement == "staff") {
        // no action required here
    }

    return true;
}

bool
MusicXMLXMLHandler::endDirectionData(const QString& qName)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    // Childs of <dynamics> will be handled separate because it is shared
    // between <note> and <direction> and handling it separate makes the
    // the code much  simpler.
    handleDynamics();

    if (m_currentElement == "direction") {
        m_parts[m_partId]->setStaff(m_staff);
        m_parts[m_partId]->setVoice(m_voice);
        switch (m_directionStart) {

        case Start :
            m_parts[m_partId]->startIndication(m_indicationStart, m_number, m_indicationEnd);
            break;

        case Stop :
            m_parts[m_partId]->endIndication(m_indicationEnd, m_number, m_duration);
            break;

            // Do-nothing cases to satisfy the compiler.
        case NotActive:
        default:
            break;
        }
    } else if (m_currentElement == "direction-type") {
        //
    } else if (m_currentElement == "rehearsal") {
        //
    } else if (m_currentElement == "segno") {
        //
    } else if (m_currentElement == "words") {
        //
    } else if (m_currentElement == "coda") {
        //
    } else if (m_currentElement == "wedge") {
        // no action required here
    } else if (m_currentElement == "dynamics") {
        Text dynamics = Text(m_dynamic, Text::Dynamic);
        m_parts[m_partId]->setStaff(m_staff);
        m_parts[m_partId]->setVoice(m_voice);
        m_parts[m_partId]->insert(dynamics.getAsEvent(m_parts[m_partId]->getCurTime()));
    } else if (m_currentElement == "dashes") {
        //
    } else if (m_currentElement == "pedal") {
        //
    } else if (m_currentElement == "octave-shift") {
        //
    } else if (m_currentElement == "other-direction") {
        //
    } else if (m_currentElement == "voice") {
        int dummy;
        if (!checkInteger(m_currentElement, dummy))
            return false;
        m_voice = m_characters;
    } else if (m_currentElement == "staff") {
        int dummy;
        if (!checkInteger(m_currentElement, dummy))
            return false;
        m_staff = m_characters;
    }

    return true;
}

bool
MusicXMLXMLHandler::startAttributesData(const QString& qName,
                                   const QXmlAttributes& atts)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    if (m_currentElement == "attributes") {
        m_parts[m_partId]->setStaff();
        m_parts[m_partId]->setVoice();
    } else if (m_currentElement == "divisions") {
        //
    } else if (m_currentElement == "key") {
        getAttributeInteger(atts, "number", m_number, false, 0);
        if (m_number > 0) {
            cerrWarning("different key for multi staff systems not supported yet.");
        }
        m_fifths = 0;
        m_major = true;
    } else if (m_currentElement == "cancel") {
        ignoreElement();
    } else if (m_currentElement == "fifths") {
        // no action required here
    } else if (m_currentElement == "mode") {
        // no action required here
    } else if (m_currentElement == "key-octave") {
        // ignored.
    } else if (m_currentElement == "time") {
        //! The number attribute is ignored! Multiple time signatures at the same
        //! time are not supported. However this is not checked!
        QString common;
        getAttributeString(atts, "symbol", common, false, "normal");
        m_common = (common == "common") || (common == "cur");
        m_beats = 4;
        m_beattype = 4;
    } else if (m_currentElement == "beats") {
        // no action required here
    } else if (m_currentElement == "beat-type") {
        // no action required here
    } else if (m_currentElement == "staves") {
        // no action required here
    } else if (m_currentElement == "part-symbol") {
        cerrElementNotSupported(m_currentElement);
    } else if (m_currentElement == "instruments") {
        cerrElementNotSupported(m_currentElement);
    } else if (m_currentElement == "clef") {
        // The additional attribute is ignored.
        getAttributeInteger(atts, "number", m_number, false, 0);
        m_sign = "G";
        m_line = 2;
        m_clefoctavechange = 0;
    } else if (m_currentElement == "sign") {
        // no action required here
    } else if (m_currentElement == "line") {
        // no action required here
    } else if (m_currentElement == "clef-octave-change") {
        // no action required here
    } else if (m_currentElement == "staff-details") {
        cerrElementNotSupported(m_currentElement);
    } else if (m_currentElement == "transpose") {
        // diatonic is ignored, not required!
//         m_chromatic = 0;
//         m_octavechange = 0;
    } else if (m_currentElement == "diatonic") {
        // ignored, not required.
    } else if (m_currentElement == "chromatic") {
        // no action required here
    } else if (m_currentElement == "octave-change") {
        // no action required here
    } else if (m_currentElement == "double") {
        // ignored
    } else if (m_currentElement == "directive") {
        cerrElementNotSupported(m_currentElement);
    } else if (m_currentElement == "measure-style") {
        cerrElementNotSupported(m_currentElement);
    }
    return true;
}

bool
MusicXMLXMLHandler::endAttributesData(const QString& qName)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    if (m_currentElement == "divisions") {
        int divisions;
        if (!checkInteger(m_currentElement, divisions))
            return false;
        m_parts[m_partId]->setDivisions(divisions);
    } else if (m_currentElement == "key") {
        Key key = Key(m_fifths<0?-m_fifths:m_fifths, m_fifths>0, !m_major);
        m_parts[m_partId]->insertKey(key, m_number);
    } else if (m_currentElement == "cancel") {
        // ignored
    } else if (m_currentElement == "fifths") {
        m_fifths = 0;
        if (!checkInteger(m_currentElement, m_fifths))
            return false;
    } else if (m_currentElement == "mode") {
        QString mode = m_characters.toLower();
        if ((mode != "major") && (mode != "minor")) {
            m_errormessage = "Unsupported mode, supperted are major, or minor.";
            return false;
        }
        m_major = mode == "major";
    } else if (m_currentElement == "key-octave") {
        // ignored
    } else if (m_currentElement == "time") {
        TimeSignature ts = TimeSignature(m_beats, m_beattype, m_common);
        m_parts[m_partId]->insertTimeSignature(ts);
    } else if (m_currentElement == "beats") {
        if (!checkInteger(m_currentElement, m_beats))
            return false;
    } else if (m_currentElement == "beat-type") {
        if (!checkInteger(m_currentElement, m_beattype))
            return false;
    } else if (m_currentElement == "staves") {
        int staves = 0;
        if (!checkInteger(m_currentElement, staves))
            return false;
        for (int s = 2; s <= staves; s++) {
            QString staff;
            staff.setNum(s);
            m_parts[m_partId]->setStaff(staff);
        }
        m_parts[m_partId]->setStaff("1");
    } else if (m_currentElement == "part-symbol") {
        // no action required here
    } else if (m_currentElement == "instruments") {
        // no action required here
    } else if (m_currentElement == "clef") {
        Clef clef;
        if      ((m_sign == "G") && (m_line == 2)) clef = Clef(Clef::Treble, m_clefoctavechange);
        else if ((m_sign == "G") && (m_line == 1)) clef = Clef(Clef::French, m_clefoctavechange);
        else if ((m_sign == "C") && (m_line == 1)) clef = Clef(Clef::Soprano, m_clefoctavechange);
        else if ((m_sign == "C") && (m_line == 2)) clef = Clef(Clef::Mezzosoprano, m_clefoctavechange);
        else if ((m_sign == "C") && (m_line == 3)) clef = Clef(Clef::Alto, m_clefoctavechange);
        else if ((m_sign == "C") && (m_line == 4)) clef = Clef(Clef::Tenor, m_clefoctavechange);
        else if ((m_sign == "C") && (m_line == 5)) clef = Clef(Clef::Baritone, m_clefoctavechange);
        else if ((m_sign == "F") && (m_line == 3)) clef = Clef(Clef::Varbaritone, m_clefoctavechange);
        else if ((m_sign == "F") && (m_line == 4)) clef = Clef(Clef::Bass, m_clefoctavechange);
        else if ((m_sign == "F") && (m_line == 5)) clef = Clef(Clef::Subbass, m_clefoctavechange);
        else if  (m_sign == "PERCUSSION")          clef = Clef(Clef::Treble, m_clefoctavechange);
        else {
            m_errormessage = "Unknown clef.";
            return false;
        }
        m_parts[m_partId]->insertClef(clef, m_number);
    } else if (m_currentElement == "sign") {
        m_sign = m_characters.toUpper();
    } else if (m_currentElement == "line") {
         if (!checkInteger(m_currentElement, m_line))
            return false;
    } else if (m_currentElement == "clef-octave-change") {
        if (!checkInteger(m_currentElement, m_clefoctavechange))
            return false;
    } else if (m_currentElement == "staff-details") {
        // no action required here
    } else if (m_currentElement == "transpose") {
//         QString num;
//         for (unsigned int i = 1; i <= m_currentContext->segments.size(); i++) {
//             Segment *segment = m_currentContext->segments["1/"+num.setNum(i)];
//             segment->setTranspose(m_octavechange*12+m_chromatic);
//         }
    } else if (m_currentElement == "diatonic") {
        // ignored, not required.
    } else if (m_currentElement == "chromatic") {
        if (!checkInteger(m_currentElement, m_chromatic))
            return false;
    } else if (m_currentElement == "octave-change") {
        if (!checkInteger(m_currentElement, m_octavechange))
            return false;
    } else if (m_currentElement == "double") {
        // ignored
    } else if (m_currentElement == "directive") {
        // no action required here
    } else if (m_currentElement == "measure-style") {
        // no action required here
    }
    return true;
}

bool
MusicXMLXMLHandler::startBarlineData(const QString& qName,
                                   const QXmlAttributes& /* atts */)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    return true;
}

bool
MusicXMLXMLHandler::endBarlineData(const QString& qName)
{
    // Handle all elements lowercase.
    m_currentElement = qName.toLower();

    return true;
}

void
MusicXMLXMLHandler::ignoreElement()
{
    cerrWarning(QString("Element \"%1\" is not supported and is ignored, including all children.")
                       .arg(m_currentElement));
    m_ignored = m_currentElement;
}

bool
MusicXMLXMLHandler::checkInteger(const QString &element, int &value)
{
    bool ok = false;
    value = m_characters.toInt(&ok);
    if (!ok) {
        m_errormessage = element + " is not an integer.";
        return false;
    }
    return true;
}

bool
MusicXMLXMLHandler::checkFloat(const QString &element, float &value)
{
    bool ok = false;
    value = m_characters.toFloat(&ok);
    if (!ok) {
        m_errormessage = element + " is not a number.";
        return false;
    }
    return true;
}

void
MusicXMLXMLHandler::cerrInfo(const QString &message)
{
    std::cerr << "**** At line " << m_locator->lineNumber() << "/"
              << m_locator->columnNumber() << " *** : " << message << std::endl;
}

void
MusicXMLXMLHandler::cerrWarning(const QString &message)
{
    std::cerr << "Warning at line " << m_locator->lineNumber() << "/"
              << m_locator->columnNumber() << " : " << message << std::endl;
}

void
MusicXMLXMLHandler::cerrError(const QString &message)
{
    std::cerr << "Error at line " << m_locator->lineNumber() << "/"
              << m_locator->columnNumber() << " : " << message << std::endl;
}

void
MusicXMLXMLHandler::cerrElementNotSupported(const QString &element)
{
    std::cerr << "Warning at line " << m_locator->lineNumber() << "/"
              << m_locator->columnNumber() << " : Element \"" << element
              << "\" not supported, ignored." << std::endl;
}

bool
MusicXMLXMLHandler::getAttributeString(const QXmlAttributes& atts, const QString &name,
                    QString &value, bool required, const QString &defValue)
{
    if (atts.index(name) < 0) {
        if(required) {
            m_errormessage = QString("Required attribute \"%1\" missing.").arg(name);
            return false;
        } else
            value = defValue;
    } else
        value = atts.value(name);
    return true;
}

bool
MusicXMLXMLHandler::getAttributeInteger(const QXmlAttributes& atts, const QString &name,
                    int &value, bool required, int defValue)
{
    if (atts.index(name) < 0) {
        if(required) {
            m_errormessage = QString("Required attribute \"%1\" missing.").arg(name);
            return false;
        } else
            value = defValue;
    } else {
        bool ok;
        value = atts.value(name).toInt(&ok);
        if (!ok) {
            m_errormessage = QString("Value of attribute \"%1\" should be an integer.").arg(name);
            return false;
        }
    }
    return true;
}

void
MusicXMLXMLHandler::handleNoteType()
{
    static const char *noteNames[] = {
        "32nd", "16th", "eighth", "quarter", "half", "whole", "breve"
    };
    m_type = 0;
    int n = int(sizeof(noteNames) / sizeof(noteNames[0]));
    while ((m_type < n) && (noteNames[m_type] != m_characters.toLower())) m_type++;
    if (m_type >= n) {
        cerrWarning(QString("Note type \"%1\" not supported, replaced by a quarter note.")
                            .arg(m_characters));
        m_type = 3;
    }
    m_type++;
}

void
MusicXMLXMLHandler::handleDynamics()
{
    if (! m_inDynamics) return;
    // This method is supposed to be called as a part of an endElement method.
    // The dynamic is a child of <dynamics> (m_currentElement!) or the text of
    // <other-dynamics> (m_characters if it is called as part of an endElement).
    if (m_currentElement == "dynamics") {
        m_inDynamics = false;
    } else if (m_currentElement == "other-dynamics") {
        m_dynamic = m_characters.toStdString();
    } else {
        m_dynamic = m_currentElement.toStdString();
    }
}

}
