
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

#ifndef RG_MUSICXMLIMPORTHELPER_H
#define RG_MUSICXMLIMPORTHELPER_H

#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/Instrument.h"
#include "base/NotationTypes.h"
#include "base/StaffExportTypes.h"
#include "base/Segment.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "base/Event.h"
#include "base/Track.h"
#include "base/NotationTypes.h"
#include "gui/editors/notation/NotationProperties.h"

#include <string>
#include <vector>
#include <queue>

#include <QString>


namespace Rosegarden
{

// class Segment;
// class Composition;

//     typedef std::map<QString, timeT> IndicationMap;
//     typedef std::map<QString, int> UnpitchedMap;

class MusicXMLImportHelper {
public:

    class IndicationStart {
    public:
        IndicationStart(const QString &staff="", const QString &voice="",
                        const std::string &name="", timeT time=0, int number=1,
                        const std::string &endName="") :
                m_staff(staff),
                m_voice(voice),
                m_name(name),
                m_time(time),
                m_number(number)
        {
            m_endName = (endName == "") ? name : endName;
        };
        QString     m_staff;
        QString     m_voice;
        std::string m_name;
        std::string m_endName;
        timeT       m_time;
        int         m_number;
    };

//     struct IndicationCMP {
//         bool operator()(const IndicationStart &a, const IndicationStart &b) {
//             return true;
//         };
//     };

    typedef std::vector<IndicationStart> IndicationVector;

    typedef std::map<QString, Track*> TrackMap;
    typedef std::map<QString, Segment *> SegmentMap;
    typedef std::map<QString, timeT> TimeMap;
    typedef std::map<QString, int> PercussionMap;
    typedef std::map<QString, QString> VoiceMap;

    MusicXMLImportHelper(Composition *composition);
    ~MusicXMLImportHelper();
    bool setStaff(const QString &staff="1");
    bool setVoice(const QString &voice="");
    bool setLabel(const QString &label);
    bool setDivisions(int divisions);
    int getDivisions() const {return m_divisions;}
    bool insertKey(const Key &key, int number=0);
    bool insertTimeSignature(const TimeSignature &ts);
    bool insertClef(const Clef &clef, int number=0);
    bool insert(Event *event);
    bool moveCurTimeBack(timeT time);
    bool startIndication(const std::string type, int number,
                         const std::string endName="");
    bool endIndication(const std::string type, int number, timeT extent);
    timeT getCurTime()  {return m_curTime;}
    void addPitch(const QString &instrument, int pitch);
    int getPitch(const QString &instrument);
    bool isPercussion() {return !m_unpitched.empty();};
    void setInstrument(InstrumentId instrument);
    void setBracketType(int bracket);

protected:
    Composition         *m_composition;
    VoiceMap            m_mainVoice;
    QString             m_staff;
    QString             m_voice;
    TrackMap            m_tracks;
    SegmentMap          m_segments;

    timeT               m_curTime;
    int                 m_divisions;
    IndicationVector    m_indications;
    PercussionMap       m_unpitched;

};

}

#endif
