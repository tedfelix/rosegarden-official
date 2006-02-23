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
#ifndef GUITAR_H_
#define GUITAR_H_

#include <map>

#include "note.h"
#include "guitarstring.h"


namespace guitar
{
class Guitar
{
public:
    static const unsigned int MAX_STRING = 6;
    static const unsigned int MAX_FRET = 21;

    typedef std::map<unsigned int, GuitarString*> GuitarStringMap;

    //! Constructor
    Guitar ( unsigned int str_num = MAX_STRING,
             unsigned int fret_num = MAX_FRET );

    Guitar ( Guitar const& rhs );

    virtual ~Guitar();

    //! Set the action for a given string
    void setStringStatus ( unsigned int const string_num, GuitarString::Action const state );

    //! Return the present action for a string
    GuitarString::Action const& getStringStatus ( unsigned int const& string_num ) const;

    //! Return the maximum number of strings for this instruments
    unsigned int const& getStringNumber ( void ) const;

    //! Return the maximum number of frets for this instruments
    unsigned int const& getFretNumber ( void ) const;

    //! Return the iterator for the beginning of the string list
    GuitarStringMap::const_iterator begin ( void ) const;

    //! Return the iterator for the end of the string list
    GuitarStringMap::const_iterator end ( void ) const;

    //! Clear the guitar string status
    void clear ( void );

    //! Return true if input guitar matches
    bool operator== ( Guitar const& rhs ) const;

    std::string toString ( void ) const;

private:
    //! Maximum number of strings
    unsigned int m_string_num;

    //! Maximum number of frets
    unsigned int m_fret_num;

    /**
     * ID - String index (An index is the max string number - string number). The string
     *      number is what a typical guitarist would understand (6th string is the fatest
     *      string on a guitar). The index is what is used to match how QT displays
     *      in the fingering constructor. The 0 position on the x-axis corresponds to the 6th
     *      string.
     * GuitarString pointer
     */
    GuitarStringMap m_setup;

};

} /* namespace guitar */




#endif /* GUITAR_H_ */
