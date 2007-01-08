// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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

#ifndef GUITAR_CHORD_H_
#define GUITAR_CHORD_H_

#include "ChordName.h"
#include "Fingering.h"

#include <qdom.h>

namespace Rosegarden
{

namespace Guitar
{
    class Chord
    {
    public:

        typedef QMap<ChordName,Chord*> Chord_Map;

        //! Constructor
        Chord ();

        //! Constructor used by chordConverter
        Chord (ChordName*);

        //! Constructor
        Chord (ChordName*, Fingering*);

        //! Copy Constructor
        Chord (Chord const& rhs);

        //! Destructor
        virtual ~Chord();

        //! Return the ChordName object describing this Chord
        ChordName* getName ();

        //! Return the Fingering object describing the finger positions of this Chord
        Fingering* getArrangement ();

        //! Create Chord object from XML data
        void load (QDomNode const& obj);

        //! Save Chord object as XML data
        void save (QDomNode& obj);

        bool operator== (Chord const& rhs) const;

        std::string toString (void) const;

    private:

        //! Name for this Chord
        ChordName* m_name;

        //! Fingering positions for this Chord
        Fingering* m_arrangement;

        //! Used to mark a chord when saving it to file
        bool m_written;
    };
}

}

# endif /* GUITAR_CHORD_H_ */
