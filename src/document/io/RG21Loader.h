
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_RG21LOADER_H
#define RG_RG21LOADER_H

#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include <map>
#include <string>
#include <QString>
#include <QStringList>
#include <vector>
#include "base/Event.h"


class QTextStream;
class QObject;
class Iterator;


namespace Rosegarden
{

class Studio;
class Segment;
class Event;
class Composition;


/**
 * Rosegarden 2.1 file import
 */
class RG21Loader
{
public:
    RG21Loader(Studio *);
    ~RG21Loader();
    
    /**
     * Load and parse the RG2.1 file \a fileName, and write it into the
     * given Composition (clearing the existing segment data first).
     * Return true for success.
     */
    bool load(const QString& fileName, Composition &);

protected:

    // RG21 note mods
    enum { ModSharp   = (1<<0),
           ModFlat    = (1<<1),
           ModNatural = (1<<2)
    };

    // RG21 chord mods
    enum { ModDot     = (1<<0),
           ModLegato  = (1<<1),
           ModAccent  = (1<<2),
           ModSfz     = (1<<3),
           ModRfz     = (1<<4),
           ModTrill   = (1<<5),
           ModTurn    = (1<<6),
           ModPause   = (1<<7)
    };

    // RG21 text positions
    enum { TextAboveStave = 0,
           TextAboveStaveLarge,
           TextAboveBarLine,
           TextBelowStave,
           TextBelowStaveItalic,
           TextChordName,
           TextDynamic
    };

    bool parseClef();
    bool parseKey();
    bool parseMetronome();
    bool parseChordItem();
    bool parseRest();
    bool parseText();
    bool parseGroupStart();
    bool parseIndicationStart();
    bool parseBarType();
    bool parseStaveType();

    void closeGroup();
    void closeIndication();
    void closeSegment();

    void setGroupProperties(Event *);

    long convertRG21Pitch(long rg21pitch, int noteModifier);
    timeT convertRG21Duration(QStringList::Iterator&);
    std::vector<std::string> convertRG21ChordMods(int chordMod);

    bool readNextLine();

    //--------------- Data members ---------------------------------

    QTextStream *m_stream;

    Studio *m_studio;
    Composition* m_composition;
    Segment* m_currentSegment;
    unsigned int m_currentSegmentTime;
    unsigned int m_currentSegmentNb;
    Clef m_currentClef;
    Rosegarden::Key m_currentKey;
    InstrumentId m_currentInstrumentId;

    typedef std::map<int, Event *> EventIdMap;
    EventIdMap m_indicationsExtant;

    bool m_inGroup;
    long m_groupId;
    std::string m_groupType;
    timeT m_groupStartTime;
    int m_groupTupledLength;
    int m_groupTupledCount;
    int m_groupUntupledLength;
    int m_groupUntupledCount;

    int m_tieStatus; // 0 -> none, 1 -> tie started, 2 -> seen one note

    QString m_currentLine;
    QString m_currentStaffName;

    QStringList m_tokens;

    unsigned int m_nbStaves;
};



}

#endif
