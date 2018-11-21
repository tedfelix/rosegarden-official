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

#ifndef RG_MUSICXMLXMLHANDLER_H
#define RG_MUSICXMLXMLHANDLER_H

#include "MusicXMLImportHelper.h"
#include "MusicXMLLoader.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "base/Event.h"
#include "base/Track.h"
#include "base/NotationTypes.h"
#include "misc/Version.h"

#include <QCoreApplication>
#include <QString>

#include <string>
#include <vector>
#include <queue>
#include <qxml.h>


class QXmlAttributes;


namespace Rosegarden
{

class Segment;
class Composition;


class MusicXMLXMLHandler : public QXmlDefaultHandler
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MusicXMLXMLHandler)

public:
    typedef std::map<QString, Track*> TrackMap;
    typedef std::map<QString, timeT> IndicationMap;
    typedef std::map<QString, int> UnpitchedMap;
    typedef std::map<QString, Segment *> SegmentMap;
    typedef std::map<QString, timeT> TimeMap;

    typedef std::map<QString, MusicXMLImportHelper *> PartMap;

    typedef enum
    {
        NoData,
        ReadHeader,
        ReadPartList,
        ReadMusicData,
        ReadNoteData,
        ReadBackupData,
        ReadDirectionData,
        ReadAttributesData,
        ReadBarlineData,
    } ReadState;

    typedef enum
    {
        NotActive,
        Start,
        Stop
    } TypeStatus;

    MusicXMLXMLHandler(Composition *comp, Studio *studio);
    ~MusicXMLXMLHandler() override;

    /**
      * Overloaded handler functions
      */
    void setDocumentLocator(QXmlLocator * locator) override;

    bool startDocument() override;

    bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts) override;

    bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName) override;

    bool characters(const QString& ch) override;

    bool endDocument () override;
private:
    /**
     * startElement() and endElement() call the functions below for
     * processing based on m_currentState
     */
    
    bool startHeader(const QString& qName, const QXmlAttributes& atts);
    bool endHeader(const QString& qName);
    bool startPartList(const QString& qName, const QXmlAttributes& atts);
    bool endPartList(const QString& qNames);
    bool startMusicData(const QString& qName, const QXmlAttributes& atts);
    bool endMusicData(const QString& qName);
    bool startNoteData(const QString& qName, const QXmlAttributes& atts);
    bool endNoteData(const QString& qName);
    bool startBackupData(const QString& qName, const QXmlAttributes& atts);
    bool endBackupData(const QString& qName);
    bool startDirectionData(const QString& qName, const QXmlAttributes& atts);
    bool endDirectionData(const QString& qName);
    bool startAttributesData(const QString& qName, const QXmlAttributes& atts);
    bool endAttributesData(const QString& qName);
    bool startBarlineData(const QString& qName, const QXmlAttributes& atts);
    bool endBarlineData(const QString& qName);


public:
    bool error(const QXmlParseException & exception) override;
    bool fatalError(const QXmlParseException & exception) override;
    bool warning(const QXmlParseException & exception) override;
    QString errorString() const override;

private:
    void ignoreElement();
    bool checkInteger(const QString &element, int &value);
    bool checkFloat(const QString &element, float &value);
    void cerrInfo(const QString &message);
    void cerrWarning(const QString &message);
    void cerrError(const QString &message);
    void cerrElementNotSupported(const QString &element);
    bool getAttributeString(const QXmlAttributes& atts, const QString &name,
                            QString &value, bool required=true, const QString &defValue="");
    bool getAttributeInteger(const QXmlAttributes& atts, const QString &name,
                            int &value, bool required=true, int defValue=0);
    void handleNoteType();
    void handleDynamics();



protected:

    typedef struct {
        std::string type;
        timeT       time;
    } QueuedEvent;

    typedef std::map<QString, Event*> EventMap;
    typedef std::map<QString, Segment*> SegmentMap2;
    typedef std::map<QString, int> IntegerMap;
    typedef std::map<QString, QueuedEvent> EventMap2;

    Composition     *m_composition;
    Studio          *m_studio;

    QString         m_errormessage;
    QXmlLocator     *m_locator;

    PartMap         m_parts;

    int             m_number;

    bool            m_isGrace;
    bool            m_hasGraceNotes;
    bool            m_chord;
    char            m_step;
    Accidental      m_accidental;
    int             m_octave;
    int             m_type;
    int             m_dots;
    int             m_tupletcount;
    int             m_untupletcount;
    QString         m_staff;
    QString         m_voice;

    // Some running variables
    QString         m_currentElement;
    QString         m_characters;
    ReadState       m_currentState;
    QString         m_mxmlVersion;
    QString         m_partId;

    // If true, the parser is scanning for a dynamics.
    bool            m_inDynamics;
    std::string     m_dynamic;

    TypeStatus      m_directionStart;
    std::string     m_indicationStart;
    std::string     m_indicationEnd;

    // Keep track of the number of the activebrace of bracket.
    // Ovrlapping braces or overlappen brackets are not supported and ignored.
    int             m_brace;
    int             m_bracket;

    // If ignored is not an empty string, it repesents a ignored element. This
    // variable is set by the method ignoreElement and will be reset when a
    // </element> is found.
    QString         m_ignored;

    // Keep track of the used note groups. This is the next available
    // group number.
    int             m_groupId;
    std::string     m_group;
    bool            m_tupletGroup;
    bool            m_beamGroup;

    // Active event.
    Event           *m_event;

    // Duration of the current note.
    int             m_duration;


    // Some lyric related variables.
    int             m_verse;
    bool            m_multiSyllabic;

    // Time
    int             m_beats;
    int             m_beattype;
    bool            m_common;

    bool            m_isgrace;

    // Key
    int             m_fifths;
    bool            m_major;

    // Transpose
    int             m_chromatic;
    int             m_octavechange;

    // Clef
    QString         m_sign;
    int             m_line;
    int             m_clefoctavechange;

    QString         m_midiInstrument;


    // Track
    int             m_midiChannel; //! NOTE Still in use???
    int             m_midiProgram;

};


}

#endif
