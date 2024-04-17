
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.

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

#include "base/Instrument.h"
#include "base/TimeT.h"

#include <QString>

#include <string>
#include <vector>
#include <map>


namespace Rosegarden
{


class RosegardenDocument;
class Track;
class Segment;
class Event;
class Key;
class TimeSignature;
class Clef;


class MusicXMLImportHelper {
public:

    typedef std::map<QString, Track*> TrackMap;
    typedef std::map<QString, Segment *> SegmentMap;
    typedef std::map<QString, timeT> TimeMap;
    typedef std::map<QString, int> PercussionMap;
    typedef std::map<QString, QString> VoiceMap;

    MusicXMLImportHelper(RosegardenDocument *doc);
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
    bool startIndication(const std::string& name, int number,
                         const std::string& endName="");
    bool endIndication(const std::string& name, int number, timeT extent);
    timeT getCurTime() const {return m_curTime;}
    void addPitch(const QString &instrument, int pitch);
    int getPitch(const QString &instrument);
    bool isPercussion() const {return !m_unpitched.empty();};
    void setInstrument(InstrumentId instrument);
    void setBracketType(int bracket);

private:

    class IndicationStart {
    public:
        IndicationStart(const QString &staff="", const QString &voice="",
                        const std::string &name="", timeT time=0, int number=1,
                        const std::string &endName="") :
                m_staff(staff),
                m_voice(voice),
                m_name(name),
                m_endName((endName == "") ? name : endName),
                m_time(time),
                m_number(number)
        {
        };
        QString     m_staff;
        QString     m_voice;
        std::string m_name;
        std::string m_endName;
        timeT       m_time{};
        int         m_number{1};
    };
    //struct IndicationCMP {
    //    bool operator()(const IndicationStart &a, const IndicationStart &b) {
    //        return true;
    //    };
    //};

    RosegardenDocument *m_document;

    VoiceMap            m_mainVoice;
    QString             m_staff;
    QString             m_voice;
    TrackMap            m_tracks;
    SegmentMap          m_segments;

    timeT               m_curTime{0};
    int                 m_divisions{960};
    typedef std::vector<IndicationStart> IndicationVector;
    IndicationVector    m_indications;
    PercussionMap       m_unpitched;
    QString             m_label;
};


}


#endif
