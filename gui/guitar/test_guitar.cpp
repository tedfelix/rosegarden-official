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
// Boost.Test
#include "guitar.h"
#include <iostream>

using namespace guitar;

//---------------------------
//         TESTS
//---------------------------
void test_constructors ( void )
{
    guitar::Guitar a;
    bool result = true;

    if ( ( a.getStringNumber() != 6 ) || ( a.getFretNumber() != 21 ) )
    {
        result = false;
    }

    for ( Guitar::GuitarStringMap::const_iterator pos = a.begin();
            pos != a.end();
            ++pos )
    {
	GuitarString* g_ptr = (*pos).second;

        if ( g_ptr->m_state != GuitarString::MUTED )
        {
            result = false;
            pos = a.end();
        }
    }

    if ( result )
    {
        std::cout << "  test_constructors: PASSED" << std::endl;
    }
    else
    {
        std::cout << "  test_constructors: FAILED" << std::endl;
    }
}

void test_stringStatus ( void )
{
    bool result = true;
    guitar::Guitar g_ref;


    // Test #1 - Change string #4 to GuitarString::PRESSED
    g_ref.setStringStatus( 4, GuitarString::PRESSED );
    if ( g_ref.getStringStatus( 4 ) != GuitarString::PRESSED )
    {
        result = false;
    }


    // Test #2 - Change string #6 to GuitarString::OPEN
    g_ref.setStringStatus( 6, GuitarString::OPEN );
    if ( g_ref.getStringStatus( 6 ) != GuitarString::OPEN )
    {
        result = false;
    }

    // Test #3 - Change string #4 to GuitarString::MUTED
    g_ref.setStringStatus( 4, GuitarString::MUTED );
    if ( g_ref.getStringStatus( 4 ) != GuitarString::MUTED )
    {
        result = false;
    }

    if ( result )
    {
        std::cout << "  test_stringStatus: PASSED" << std::endl;
    }
    else
    {
        std::cout << "  test_stringStatus: FAILED" << std::endl;
    }
}

void test_equal ( void )
{
    Guitar g_ref;
    bool result = true;

    // Test #1 - Default obj to itself
    if ( ! ( g_ref == g_ref ) ) { result = false; }

    // Test #2 - Compare obj 1 (strings 4,1,2 set to PRESSED)
    // against g_ref (default)
    Guitar obj1_ref;
    g_ref.setStringStatus ( 4, GuitarString::PRESSED );
    g_ref.setStringStatus ( 1, GuitarString::PRESSED );
    g_ref.setStringStatus ( 2, GuitarString::PRESSED );

    if ( obj1_ref == g_ref ) { result = false; }
    if ( ! ( obj1_ref == obj1_ref ) ) { result = false; }

    if ( result )
    {
        std::cout << "  test_equal: PASSED" << std::endl;
    }
    else
    {
        std::cout << "  test_equal: FAILED" << std::endl;
    }
}

void test_clear ( void )
{
    bool result = true;
    Guitar g_ref;
    Guitar obj1_ref;
    obj1_ref.setStringStatus ( 4, GuitarString::PRESSED );
    obj1_ref.setStringStatus ( 1, GuitarString::PRESSED );
    obj1_ref.setStringStatus ( 2, GuitarString::PRESSED );

    // Test #1 - Good barre added
    obj1_ref.clear();

    if ( ! ( g_ref == obj1_ref ) )
    {
        result = false;
    }

    if ( result )
    {
        std::cout << "  test_clear: PASSED" << std::endl;
    }
    else
    {
        std::cout << "  test_clear: FAILED" << std::endl;
    }
}

int main ( int, char* [] )
{
    std::cout << "Running test: Guitar" << std::endl;
    test_constructors();
    test_stringStatus ();
    test_equal ();
    test_clear ();

    return 0;
}
