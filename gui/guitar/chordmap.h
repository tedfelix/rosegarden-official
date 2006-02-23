// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.
 
    This program is Copyright 2000-2005
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
#ifndef GUITAR_CHORD_MAP_H_
#define GUITAR_CHORD_MAP_H_

#include <qstring.h>

#include <map>

#include "chord.h"
#include "DuplicateException.h"

namespace guitar
{

class ChordMap
{
public:
    ChordMap ()
            : m_mapSize ( 0 )
    {}

    virtual ~ChordMap();

    //! Map (Key = Version number, Data = Pointer to associated Chord)
    typedef std::map<unsigned int, Chord*> VersionMap;
    typedef std::pair<unsigned int, Chord*> VersionMapPair;

    //! Map (Key = Suffix Name, Data = Pointer to associated Version Map)
    typedef std::map<QString, VersionMap*> SuffixMap;

    //! Map (Key = Modifier Name, Data = Pointer to associated Suffix Map)
    typedef std::map<QString, SuffixMap*> ModifierMap;

    //! Map (Key = Chord Name, Data = Pointer to associated Modifier Map)
    typedef std::map<QString, ModifierMap*> ScaleMap;

    typedef std::list<Chord*>::iterator iterator;
    typedef std::list<Chord*>::const_iterator const_iterator;
    typedef std::pair <bool, Chord*> pair;

    //! Add a new Chord to the ChordMap with its given name and aliases
    // This function will make a deep copy of the Chord when it is inserted.
    // Callers must handle the reclaiming of memory for the argument pointer
    void insert ( Chord* chord_ptr ) throw (DuplicateException);

    /**
     Return pair pointing to the Chord for the given name
     (TRUE => Chord is valid, FALSE => Chord is NULL)
    */
    ChordMap::pair find ( ChordName* name_ptr );

    //! Remove a chord
    void erase ( Chord* chordPtr );

    //! Append ChordMap contents
    void append ( ChordMap::const_iterator begin, ChordMap::const_iterator end );

    //! Return the number of entries contained
    unsigned int const& size ( void ) const;

    //! Return the list of scale names
    std::list<QString> getNameList ( void ) const;

    //! Return the list of interval names for a given scale
    std::list<QString> getModifierList ( QString scale ) const;

    //! Return the list of distance names for a given scale and interval
    std::list<QString> getSuffixList ( QString scale, QString interval ) const;

    //! Return the list of version names for a given scale, interval and distance
    std::list<QString> getVersionList ( QString scale, QString interval, QString distance ) const;

    //! Save the Chord map to a given directory
    void save ( QString const& directory ) const;

    //! Output chord map to string
    QString toString ( void ) const;

    //! Return const_iterator to beginning of ChordMap
    const_iterator begin ( void ) const;

    //! Return const_iterator to end of ChordMap
    const_iterator end ( void ) const;

private:

    //! Add Chord to map. Implements function of insert (Chord*)
    //! If addChord is true then its added to the m_chordList to keep
    //! track of this new entry so that it can be easily deleted.
    void insertChord ( ChordName* namePtr, Chord* chordPtr, bool addChord = false );

    //! Add Chord to version map
    void versionAdd ( VersionMap* vMap_ptr, ChordName* namePtr, Chord* chordPtr );

    //! Create missing VersionMap and add chord to it
    VersionMap* createVersionMap ( ChordName* namePtr, Chord* chordPtr );

    //! Create missing SuffixMap and add chord to it
    SuffixMap* createSuffixMap ( ChordName* namePtr, Chord* chordPtr );

    //! Create missing ModifierMap and add chord to it
    ModifierMap* createModifierMap ( ChordName* namePtr, Chord* chordPtr );

    //! Find input fingering in chord list. While this is an O(N)
    //! operation its easier than figuring out how to compare one 
    //! fingering from another.
    ChordMap::pair find ( Fingering* input_finger_ptr );

    //! Helper function to remove ChordName from map
    void eraseChordName ( ChordName* namePtr );

    /* ----- parameters ----- */
    //! List of chord pointers. These same chord pointers are used in the
    //! ScaleMap. The point of this structure is that it is a simple
    //! way to keep track of the pointers to chord objects. That is while
    //! a chord has a main name and various aliases they all point to the
    //! same fingering. We use this to keep track of actual chord objects.
    std::list<Chord*> m_chordList;

    //! Unlike the m_chordList variable this data structure is a way to 
    //! to lookup chords either by their main name or by their alias. So
    //! each chord has N entries where N = number of aliases + 1 (main name).
    ScaleMap m_chords;

    //! Total number of chords in map
    unsigned int m_mapSize;
};
}

#endif /* GUITAR_CHORD_MAP_H_ */
