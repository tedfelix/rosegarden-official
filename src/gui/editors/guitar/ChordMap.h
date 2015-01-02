/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CHORDMAP_H
#define RG_CHORDMAP_H

#include "Chord.h"

#include <QStringList>
#include <set>

namespace Rosegarden
{

namespace Guitar
{

class ChordMap
{
    typedef std::set<Chord, Chord::ChordCmp> chordset; 

public:
    typedef std::vector<Chord> chordarray;

    typedef chordset::iterator iterator;
    typedef chordset::const_iterator const_iterator;

    static int FILE_FORMAT_VERSION_MAJOR;
    static int FILE_FORMAT_VERSION_MINOR;
    static int FILE_FORMAT_VERSION_POINT;
    
	ChordMap();
    
    void insert(const Chord&);
    void substitute(const Chord& oldChord, const Chord& newChord);
    void remove(const Chord&);
    
    chordarray getChords(const QString& root, const QString& ext) const;
    
    QStringList getRootList() const;
    QStringList getExtList(const QString& root) const;
    
    void debugDump() const;
    
    bool needSave() const { return m_needSave; }
    void clearNeedSave() { m_needSave = false; }

    bool saveDocument(const QString& filename, bool userChordsOnly, QString& errMsg);

    iterator begin() { return m_map.begin(); }
    iterator end()   { return m_map.end();   }
    const_iterator begin() const { return m_map.begin(); }
    const_iterator end()   const { return m_map.end(); }
    
protected:

    chordset m_map;
    
    bool m_needSave;
};

}

}

#endif /*RG_CHORDMAP_H*/
