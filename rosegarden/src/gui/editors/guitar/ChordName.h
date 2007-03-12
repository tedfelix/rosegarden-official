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

#ifndef GUITAR_CHORD_NAME_H_
#define GUITAR_CHORD_NAME_H_

#include <qlistbox.h>
#include <qstring.h>
#include <qdom.h>

#include <vector>

namespace Rosegarden
{

namespace Guitar
{
    class ChordName
    {
    public:

        //! Constructor
        ChordName ();

        //! Copy Constructor
        ChordName (ChordName const& ref);

        //! Destructor
        virtual ~ChordName ();

        //! Return list of alias names for this name
        std::vector<ChordName*> const& getAliasList () const;

        //! Create ChordName object from XML data
        void load (QDomNode const& nodePtr);

        //! Create ChordName object as XML data
        void save (QDomNode& nodePtr, bool isAlias = false);

        //! Add new alias for this name
        void addAlias (ChordName* name);

        //! Display chord name in GUI
        void write (QListBox*& major, QListBox*& modifier, QListBox*& suffix);

        //! Set data for ChordName
        void setName (QString scale, QString modifier, QString suffix, unsigned int version = 0);

        //! Convert data to text string
        std::string toString (void) const;

        //! Return the scale value for this chord name (e.g. C)
        QString const& getScale (void) const;

        //! Return the modifier value for this chord name (e.g. Major)
        QString const& getModifier (void) const;

        //! Return the suffix string for this chord (e.g. 2nd, 7(#5) )
        QString const& getSuffix (void) const;

        //! Return the version value for this chord (e.g. 1)
        unsigned int const& getVersion (void) const;

        //! Set the version value for this chord
        void setVersion (unsigned int value);

        //! Return if the chord names match
        bool operator== (ChordName const& rhs) const;

    private:

        //! Chord - (e.g. C)
        QString m_scale;

        //! Chord modifier - (e.g. Major)
        QString m_modifier;

        //! Chord suffix - (e.g. 2nd)
        QString m_suffix;

        //! Chord Version
        unsigned int m_version;

        //! List of alias names for chord
        std::vector<ChordName*> m_aliases;
    };

}

}

#endif /* GUITAR_CHORD_NAME_H_ */
