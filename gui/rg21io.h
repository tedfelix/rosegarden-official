
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG21IO_H_
#define _RG21IO_H_

#include <qstringlist.h>
#include <qfile.h>
#include <qtextstream.h>

class Rosegarden::Composition;
class Rosegarden::Track;

class RG21Loader 
{
public:
    RG21Loader(const QString& fileName);
    ~RG21Loader();
    
    Rosegarden::Composition* getComposition() { return m_composition; }

protected:

    // RG21 note mods
    enum { ModSharp   = (1<<0),
           ModFlat    = (1<<1),
           ModNatural = (1<<2)
    };

    enum { TrebleClef, TenorClef, AltoClef, BassClef, InvalidClef };

    bool parse();
    bool parseClef();
    bool parseKey();
    bool parseChordItem();
    bool parseRest();
    bool parseGroupStart();
    bool parseMarkStart();

    void closeGroup();
    void closeMark();
    void closeTrackOrComposition();

    long convertRG21Pitch(long rg21pitch, int nodeModifier);
    Rosegarden::timeT convertRG21Duration(QStringList::Iterator&);

    bool readNextLine();

    QFile m_file;
    QTextStream *m_stream;

    Rosegarden::Composition* m_composition;
    Rosegarden::Track* m_currentTrack;
    unsigned int m_currentTrackTime;
    unsigned int m_currentTrackNb;
    unsigned int m_currentClef;
    bool m_inGroup;
    unsigned int m_groupId;
    std::string m_groupType;

    QString m_currentLine;
    QString m_currentStaffName;

    QStringList m_tokens;

    unsigned int m_nbStaves;
};


#endif /* _MUSIC_IO_ */
