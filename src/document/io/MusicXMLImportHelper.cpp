/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MusicXMLImportHelper]"
#define RG_NO_DEBUG_PRINT

#include "MusicXMLImportHelper.h"

#include "base/Event.h"
#include "base/BaseProperties.h"
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/Instrument.h"
#include "base/NotationTypes.h"  // for Clef
#include "base/StaffExportTypes.h"  // for Brackets
#include "base/Segment.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "document/RosegardenDocument.h"

#include <QString>


namespace Rosegarden
{


MusicXMLImportHelper::MusicXMLImportHelper(RosegardenDocument *doc) :
    m_document(doc)
{
    setStaff("1");
}

MusicXMLImportHelper::~MusicXMLImportHelper()
{
    //
}

bool
MusicXMLImportHelper::setStaff(const QString &staff)
{
    RG_DEBUG << "setStaff(" << staff << ")";

    Composition &composition = m_document->getComposition();

    if (m_tracks.find(staff) == m_tracks.end()) {
        // No such track, create a new one.
        TrackId id = composition.getNewTrackId();
        int pos = id;
        if (!m_tracks.empty()) {
            pos = m_tracks["1"]->getPosition() + m_tracks.size();
            Composition::TrackMap tracks = composition.getTracks();
            for (Composition::TrackMap::iterator t = tracks.begin(); t != tracks.end(); t++) {
                if (((*t).second)->getPosition() >= pos) {
                    ((*t).second)->setPosition(((*t).second)->getPosition()+1);
                }
            }
        }

        // ??? I think this is the right approach.  In all cases, this will
        //     just find the first MIDI instrument in the autoload.  The
        //     problem occurs during merge.  If there are no MIDI devices
        //     in the target (current) document, the merge process doesn't
        //     fall back on the first soft synth.  We end up with garbage
        //     which is easy for the user to fix.
        const Studio *studio = &m_document->getStudio();

        // Go with the first MIDI instrument we can find.  Otherwise we
        // might return channel 10 (drums) on a GM device.  Leave it up to
        // the user to reassign instruments as needed.
        const InstrumentId instrumentID = studio->getFirstMIDIInstrument();
        Track *track = new Track(id, instrumentID, pos);
        composition.addTrack(track);

        m_tracks[staff] = track;
    }
    m_staff = staff;

    if (m_mainVoice.find(m_staff) == m_mainVoice.end()) {
        m_mainVoice[m_staff] = "";
    }

    // When a multi staff system is created, place CurlyOn and CurlyOff on the
    // first and last track of the part.
    if (m_tracks.size() > 1) {
        TrackMap::iterator it = m_tracks.begin();
        ((*it).second)->setStaffBracket(Brackets::CurlyOn);
        while (++it != m_tracks.end()) {
            ((*it).second)->setStaffBracket(Brackets::None);
        }
        --it;
        ((*it).second)->setStaffBracket(Brackets::CurlyOff);
    }

    setVoice(m_mainVoice[m_staff]);
    return true;
}

bool
MusicXMLImportHelper::setVoice(const QString &voice)
{
    if ((voice != "") && (m_mainVoice[m_staff] == "")) {
        m_mainVoice[m_staff] = voice;
        SegmentMap::iterator s = m_segments.find(m_staff+"/");
        if (s != m_segments.end()) {
            m_segments[m_staff+"/"+m_mainVoice[m_staff]] = (*s).second;
            (*s).second->setLabel(m_label.toStdString());
            m_segments.erase(s);
        }
        m_voice = m_mainVoice[m_staff];
    } else {
        QString tmpVoice = voice;
        if (voice == "") tmpVoice = m_mainVoice[m_staff];
        bool createSegment = false;
        if (m_segments.find(m_staff+"/"+tmpVoice) == m_segments.end()) {
            createSegment = true;
        } else {
            if ((tmpVoice != m_mainVoice[m_staff]) && (m_segments[m_staff+"/"+tmpVoice]->getEndTime() < m_curTime)) {
                createSegment = true;
            }
        }
        if (createSegment) {
            Segment *segment = new Segment(Segment::Internal, m_curTime);
            segment->setLabel(m_label.toStdString());
            m_document->getComposition().addSegment(segment);
            segment->setTrack(m_tracks[m_staff]->getId());
            m_segments[m_staff+"/"+tmpVoice] = segment;
        }
        m_voice = tmpVoice;
    }
    return true;
}

bool
MusicXMLImportHelper::setLabel(const QString &label)
{
    m_label = label;
    for (TrackMap::iterator i = m_tracks.begin(); i != m_tracks.end(); ++i) {
        ((*i).second)->setLabel(label.toStdString());
    }
    return true;
}

bool
MusicXMLImportHelper::setDivisions(int divisions)
{
    m_divisions = divisions;
    return true;
}

bool
MusicXMLImportHelper::insertKey(const Key &key, int number)
{
    if (number > 0) {
        RG_WARNING << "Different keys on multistaff systems not supported yet.";
    } else {
        for (TrackMap::iterator i = m_tracks.begin(); i != m_tracks.end(); ++i) {
            m_segments[(*i).first+"/"+m_mainVoice[m_staff]]->insert(key.getAsEvent(m_curTime));
        }
    }
    return true;
}

bool
MusicXMLImportHelper::insertTimeSignature(const TimeSignature &ts)
{
    m_document->getComposition().addTimeSignature(m_curTime, ts);
    return true;
}

bool
MusicXMLImportHelper::insertClef(const Clef &clef, int number)
{
    if (number > 0) {
        QString staff;
        staff.setNum(number);
        setStaff(staff);
        m_segments[m_staff+"/"+m_voice]->insert(clef.getAsEvent(m_curTime));
    } else {
        for (TrackMap::iterator i = m_tracks.begin(); i != m_tracks.end(); ++i) {
            m_segments[(*i).first+"/"+m_mainVoice[m_staff]]->insert(clef.getAsEvent(m_curTime));
        }
    }
    return true;
}

bool
MusicXMLImportHelper::insert(Event *event)
{
    if (event->has(BaseProperties::IS_GRACE_NOTE)  &&
        event->get<Bool>(BaseProperties::IS_GRACE_NOTE)) {
        Segment *segment = m_segments[m_staff+"/"+m_voice];
        Segment::iterator start, end;
        segment->getTimeSlice(m_curTime, start, end);
        std::vector<Event *> toErase;
        for (Segment::iterator e = start; e != end; ++e) {
            if ( ! (*e)->isa(Rosegarden::Note::EventType) &&
                    ! (*e)->isa(Rosegarden::Note::EventRestType)) continue;
            Event *tmp = new Event(*(*e),
                            (*e)->getAbsoluteTime(),
                            (*e)->getDuration(),
                            (*e)->getSubOrdering()-1,
                            (*e)->getNotationAbsoluteTime(),
                            (*e)->getNotationDuration());
            segment->insert(tmp);
            toErase.push_back(*e);
        }
        for (std::vector<Event *>::iterator e = toErase.begin(); e != toErase.end(); ++e)
            segment->erase(segment->findSingle((*e)));
    }

    m_segments[m_staff+"/"+m_voice]->insert(event);
    if ( event->isa(Rosegarden::Note::EventType) || event->isa(Rosegarden::Note::EventRestType)) {
        m_curTime = event->getAbsoluteTime() + event->getDuration();
    }
    return true;
}

bool
MusicXMLImportHelper::moveCurTimeBack(timeT time)
{
    m_curTime -= time;
    return true;
}

bool
MusicXMLImportHelper::startIndication(const std::string& name, int number,
                                      const std::string& endName)
{
    m_indications.push_back(IndicationStart(m_staff, m_voice, name, m_curTime, number, endName));
    RG_DEBUG << m_curTime << " : startIndication(" << m_staff << ", " << m_voice << ", " << name << ", " << number
          << ", " << endName << ")";
    return true;
}

bool
MusicXMLImportHelper::endIndication(const std::string& name, int number, timeT extent)
{
    // Look in m_indications for an entry with a matching staff/voice/name/number.
    // However, it is possible an entry is created before the main voice for staff
    // was set in which case the voice is an empty string.
    // So, if an entry has an empty voice, the voice should be equal to the
    // mainVoice of the staff to match!
    bool found = false;
    IndicationVector::iterator i = m_indications.begin();
    while (!found && i != m_indications.end()) {
        if (((*i).m_staff == m_staff) &&
            ((((*i).m_voice != "") && ((*i).m_voice == m_voice)) ||
             (((*i).m_voice == "") && (m_voice == m_mainVoice[m_staff]))) &&
            ((*i).m_endName == name) &&
            ((*i).m_number = number)) {
            found = true;
        } else {
            ++i;
        }
    }
    RG_DEBUG << m_curTime << " : endIndication(" << m_staff << ", " << m_voice << ", " << name << ", " << number
          << ", " << extent << ") -> " << found;
    if (found) {
        Indication indication((*i).m_name, m_curTime - (*i).m_time + extent);
        m_segments[m_staff+"/"+m_voice]->insert(indication.getAsEvent((*i).m_time));
        m_indications.erase(i);
    }
    return true;
}

void
MusicXMLImportHelper::addPitch(const QString &instrument, int pitch)
{
    m_unpitched[instrument] = pitch;
}

int
MusicXMLImportHelper::getPitch(const QString &instrument)
{
    if (m_unpitched.find(instrument) == m_unpitched.end()) {
        return -1;
    } else {
        return m_unpitched[instrument];
    }
}

void
MusicXMLImportHelper::setInstrument(InstrumentId instrument)
{
    for (TrackMap::iterator t = m_tracks.begin(); t != m_tracks.end(); ++t) {
        ((*t).second)->setInstrument(instrument);
    }
}

void
MusicXMLImportHelper::setBracketType(int bracket)
{
    if (m_tracks.empty()) return;

    if ((bracket == Brackets::CurlyOff) || (bracket == Brackets::SquareOff)) {
        TrackMap::iterator it = m_tracks.end();
        --it;
        Track *track = (*it).second;
        if (bracket == Brackets::CurlyOff) {
            if (track->getStaffBracket() == Brackets::SquareOff) {
                track->setStaffBracket(Brackets::CurlySquareOff);
            } else {
                track->setStaffBracket(Brackets::CurlyOff);
            }
        } else {
            if (track->getStaffBracket() == Brackets::CurlyOff) {
                track->setStaffBracket(Brackets::CurlySquareOff);
            } else if (track->getStaffBracket() == Brackets::SquareOn) {
                track->setStaffBracket(Brackets::SquareOnOff);
            } else {
                track->setStaffBracket(Brackets::SquareOff);
            }
        }
    } else if ((bracket == Brackets::CurlyOn) || (bracket == Brackets::SquareOn)) {
        TrackMap::iterator it = m_tracks.begin();
        Track *track = (*it).second;
        if (bracket == Brackets::CurlyOn) {
            if (track->getStaffBracket() == Brackets::SquareOn) {
                track->setStaffBracket(Brackets::CurlySquareOn);
            } else {
                track->setStaffBracket(Brackets::CurlyOn);
            }
        } else {
            if (track->getStaffBracket() == Brackets::SquareOn) {
                track->setStaffBracket(Brackets::CurlySquareOn);
            } else {
                track->setStaffBracket(Brackets::SquareOn);
            }
        }
    }
}


}
