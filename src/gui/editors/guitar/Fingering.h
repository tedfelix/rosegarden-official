// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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

#ifndef GUITAR_FINGERING_H_
#define GUITAR_FINGERING_H_

#include <qdom.h>

#include "GuitarNeck.h"
#include "Note.h"
#include "Barre.h"
#include "BarreList.h"
#include "base/Event.h"

namespace Rosegarden
{

namespace Guitar
{
class Fingering
{
public:
    static const std::string EventType;
    static const short EventSubOrdering;

    //! Constructor
    Fingering ();

    Fingering ( GuitarNeck* gPtr );

    //! Constructor
    virtual ~Fingering ();

    //! Constructor - create from Event
    Fingering ( Event const& e_ref );

    //! Copy Constructor
    Fingering ( Fingering const& rhs );

    //! Add Note object to fingering (deep copy)
    bool addNote ( Note* notePtr );

    //! Add Barre object to fingering (deep copy)
    void addBarre ( Barre* barrePtr );

    //! Remove Note object from fingering
    void removeNote ( unsigned int string_num );

    //! Remove Barre object to fingering
    void removeBarre ( unsigned int fret_num );

    //! Set action for Guitar string
    void setStringStatus ( unsigned int stringPos, GuitarString::Action action );

    //! Get action for Guitar string
    GuitarString::Action const& getStringStatus ( unsigned int stringPos ) const;

    //! Set the base fret for Fingering object
    void setFirstFret ( unsigned int const& fret );

    //! Return the base fret
    unsigned int const& getFirstFret ( void ) const;

    //! Retrieve a Note object for a given string
    Note* getNote ( unsigned int const& string_num );

    //! Retrieve a Barre object for a given fret
    Barre* getBarre ( unsigned int const& fret_num );

    //! Display Fingering object using QPainter object
    //  frets_displayed: The maximum number of frets to be displayed
    //  p: The QPainter object where the Notes and Barres are displayed
    void drawContents ( QPainter* p,
                        unsigned int frets_displayed ) const;

    //! Display Fingering object data as a text string
    std::string toString ( void ) const;

    //! Load Barre and Note objects from a XML file
    void load ( QDomNode const& obj );

    //! Save the Barre and Note objects to an XML file
    void save ( QDomNode& obj );

    bool operator== ( Fingering const& rhs ) const;

    Event* getAsEvent ( timeT absoluteTime );

    //! Determine if a Note object exists for a particular string
    bool hasNote ( unsigned int stringPos );

    //! Determine if a Barre object exists for a particular fret
    bool hasBarre ( unsigned int const& fret_num );

private:

    //! Create Barre object from fingering information
    void setBarre ( unsigned int fretPos, unsigned int start, unsigned int end );

    //! Handle to Guitar object upon the fingering applies
    GuitarNeck* m_guitar;

    //! Base fret number for fingering
    unsigned int m_startFret;

    //! Map of Index (Fret position number) and Data (Barre played)
    typedef std::map<unsigned int, Barre*> BarreMap;
    typedef std::pair<unsigned int, Barre*> BarreMapPair;
    BarreMap m_barreFretMap;

    //! Map of Index (String number) and Data (List of Barres that use the string)
    typedef std::map< unsigned int, BarreList* > BarreStringMap;
    typedef std::pair<unsigned int, BarreList*> BarreStringMapPair;
    BarreStringMap m_barreStringMap;

    //! Map of Index (String number) and Data (Note played)
    typedef std::map<unsigned int, Note*> NoteMap;
    typedef std::pair<unsigned int, Note*> NoteMapPair;
    NoteMap m_notes;
};

}

}

#endif /* GUITAR_FINGERING_H_ */
