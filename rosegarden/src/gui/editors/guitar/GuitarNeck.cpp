// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
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

#include "Fingering.h"
#include "GuitarNeck.h"
#include "GuitarXmlHandler.h"
#include "NoteSymbols.h"
#include <iostream>
#include <sstream>

namespace Rosegarden
{

namespace Guitar
{
/*---------------------------------------------------------------
              Guitar
  ---------------------------------------------------------------*/
GuitarNeck::GuitarNeck ( unsigned int str_num, unsigned int fret_num )
        : m_string_num ( str_num ),
        m_fret_num ( fret_num )
{
    for ( unsigned int i = str_num; i >= 1; --i ) {
        m_setup.insert ( std::make_pair( i, new GuitarString () ) );
    }
}

GuitarNeck::GuitarNeck ( GuitarNeck const& rhs )
        : m_string_num ( rhs.m_string_num ),
        m_fret_num ( rhs.m_fret_num )
{
    for ( GuitarStringMap::const_iterator pos = rhs.m_setup.begin();
            pos != rhs.m_setup.end();
            ++pos ) {
        GuitarString* g_ptr = ( *pos ).second;
        m_setup.insert ( std::make_pair( pos->first, new GuitarString ( *g_ptr ) ) );
    }
}

GuitarNeck::~GuitarNeck ( void )
{
    for ( GuitarStringMap::iterator pos = m_setup.begin();
            pos != m_setup.end();
            ++pos ) {
        delete ( *pos ).second;
    }
}

void
GuitarNeck::setStringStatus ( unsigned int const string_num, GuitarString::Action const action )
{
    if ( ( string_num >= 1 ) && ( string_num <= m_string_num ) ) {
        GuitarString * str_ptr = m_setup[ string_num ];
        //std::cout << "GuitarNeck::setStringStatus - set action for string #" << string_num << std::endl;
        str_ptr->m_state = action;
    } else {
        std::cerr << "GuitarNeck::setStringStatus - warning: string number given ("
        << string_num << ") is outside the range of 1 to " << m_string_num << std::endl;
    }
}

GuitarString::Action const&
GuitarNeck::getStringStatus ( unsigned int const& string_num ) const
{
    GuitarStringMap::const_iterator pos = m_setup.find( string_num );
    GuitarString * str_ptr = ( *pos ).second;
    return str_ptr->m_state;
}

unsigned int const&
GuitarNeck::getStringNumber ( void ) const
{
    return m_string_num;
}

unsigned int const& GuitarNeck::getFretNumber ( void ) const
{
    return m_fret_num;
}

GuitarNeck::GuitarStringMap::const_iterator
GuitarNeck::begin ( void ) const
{
    return m_setup.begin();
}

GuitarNeck::GuitarStringMap::const_iterator
GuitarNeck::end ( void ) const
{
    return m_setup.end();
}

void
GuitarNeck::clear ( void )
{
    for ( GuitarStringMap::iterator pos = m_setup.begin();
            pos != m_setup.end();
            ++pos ) {
        GuitarString* g_ptr = ( *pos ).second;
        g_ptr->m_state = GuitarString::MUTED;
    }
}

bool
GuitarNeck::operator== ( GuitarNeck const& rhs ) const
{
    bool result = true;

    if ( m_string_num != rhs.m_string_num ) {
        result = false;
    }
    if ( m_fret_num != rhs.m_fret_num ) {
        result = false;
    }

    GuitarStringMap::const_iterator lhs_pos = m_setup.begin();
    GuitarStringMap::const_iterator rhs_pos = rhs.m_setup.begin();
    while ( ( lhs_pos != m_setup.end() ) &&
            ( rhs_pos != rhs.m_setup.end() ) &&
            ( result ) ) {
        GuitarString * lhsStr_ptr = ( *lhs_pos ).second;
        GuitarString* rhsStr_ptr = ( *rhs_pos ).second;

        result = ( ( *lhsStr_ptr ) == ( *rhsStr_ptr ) );

        ++lhs_pos;
        ++rhs_pos;
    }
    return result;
}

std::string
GuitarNeck::toString ( void ) const
{
    std::stringstream output;
    unsigned int i = 1;

    output << "      Guitar: " << std::endl
    << "          string #: " << m_string_num << std::endl
    << "          fret #:   " << m_fret_num << std::endl;

    for ( GuitarStringMap::const_iterator pos = m_setup.begin();
            pos != m_setup.end();
            ++pos ) {
        GuitarString* g_ptr = ( *pos ).second;
        output << "       #" << i << ": "
        << GuitarString::actionStringName ( g_ptr->m_state )
        << std::endl;
        ++i;
    }

    return output.str();
}

} /* namespace Guitar */

}
