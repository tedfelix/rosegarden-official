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

#ifdef COMPILE_GUITAR_TESTS

#include "GuitarNeck.h"
#include "Fingering.h"
#include "FingeringConstructor.h"
#include <iostream>

using namespace Rosegarden;
using namespace Rosegarden::Guitar;

//---------------------------
//         TESTS
//---------------------------
void test_constructors ( void )
{
    GuitarNeck g_ref;
    FingeringConstructor a ( &g_ref );
    Fingering* f_ptr = a.getFingering();

    if ( f_ptr->getFirstFret() == 1 ) {
        std::cout << "  test_constructors: PASSED" << std::endl;
    } else {
        std::cout << "  test_constructors: FAILED" << std::endl;
    }
}

/*
void test_addNote ( void )
{
    bool result = true;
    GuitarNeck g_ref;
    FingeringConstructor a ( &g_ref );
 
    // Test #1 - Good note added
 
    Note* n_ptr = new Note ( 1, 2 );
    if ( ! a.addNote ( n_ptr ) )
    {
        delete n_ptr;
    }
 
    Note* savedNote = a.getNote ( 1 );
 
    if ( !( *n_ptr == ( *savedNote ) ) )
    {
        std::cout << "  test_addNote (Good note): FAILED" << std::endl;
        result = false;
    }
 
    // Test #2 - Bad note added
    Note* p_ptr = new Note ( 2000, 2 );
    if ( ! a.addNote ( p_ptr ) )
    {
        delete p_ptr;
    }
 
    try
    {
        savedNote = a.getNote ( 2000 );
        if ( savedNote != 0 )
        {
            std::cout << "  test_addNote (Bad note): FAILED" << std::endl;
            result = false;
        }
    }
    catch ( Exception & )
    {}
 
    if ( result )
    {
        std::cout << "  test_addNote: PASSED" << std::endl;
    }
}
 
void test_addBarre ( void )
{
    GuitarNeck g_ref;
    Fingering a ( &g_ref );
 
    // Test #1 - Good barre added
    Barre* n_ptr = new Barre ( 1, 2, 1 );
    a.addBarre ( n_ptr );
 
    Barre* savedBarre = a.getBarre ( 1 );
 
    if ( !( ( *n_ptr ) == ( *savedBarre ) ) )
    {
        std::cout << "  test_addBarre: FAILED - Barre n is not equal to savedBarre"
        << std::endl;
    }
 
    // Test #2 - Bad barre added
    Barre* bad_ptr = new Barre ( 2000, 2, 1 );
    a.addBarre ( bad_ptr );
    try
    {
        savedBarre = a.getBarre ( 2000 );
        if ( savedBarre != 0 )
        {
            std::cout << "  test_addBarre: FAILED - savedBarre is not equal to NULL"
            << std::endl;
        }
    }
    catch ( Exception & re )
    {}
 
    std::cout << "  test_addBarre: PASSED" << std::endl;
}
 
void test_equal ( void )
{
    GuitarNeck g_ref;
    Fingering a ( &g_ref );
    bool result = true;
 
    // Test #1 - Empty Fingering (self)
    if ( ! ( a == a ) )
    {
        result = false;
    }
 
    Note* n1_ptr = new Note ( 2, 1 );
    Note* n2_ptr = new Note ( 4, 2 );
    a.addNote ( n1_ptr );
    a.addNote ( n2_ptr );
 
    Fingering b ( &g_ref );
    Note* n3_ptr = new Note ( 2, 1 );
    Note* n4_ptr = new Note ( 4, 2 );
    b.addNote ( n3_ptr );
    b.addNote ( n4_ptr );
 
    if ( ! ( a == b ) )
    {
        result = false;
    }
 
    Barre* b1_ptr = new Barre ( 1, 2, 1 );
    a.addBarre ( b1_ptr );
 
    Barre* b2_ptr = new Barre ( 1, 2, 1 );
    b.addBarre ( b2_ptr );
 
    if ( ! ( a == b ) ) { result = false; }
 
    Note* n5_ptr = new Note ( 5, 3 );
    b.addNote ( n5_ptr );
 
    if ( a == b ) { result = false; }
 
    if ( result )
    {
        std::cout << "  test_equal: PASSED" << std::endl;
    }
    else
    {
        std::cout << "  test_equal: FAILED" << std::endl;
    }
}
 
void test_setFirstFret ( void )
{
    GuitarNeck g_ref;
    Fingering finger_ref ( &g_ref );
 
    bool result = true;
 
    // Test #1 - No contents
    finger_ref.setFirstFret ( 2 );
    if ( finger_ref.getFirstFret() != 2 ) { result = false; }
 
    // Test #2 - One Note - set fret to 5 from 2
    Note* n1_ptr = new Note ( 1, 2 );
 
    finger_ref.addNote ( n1_ptr );
 
    unsigned int fret = 5;
    finger_ref.setFirstFret ( fret );
 
    Note* savedNote = finger_ref.getNote ( 1 );
    if ( savedNote->getFret() != 5 ) { result = false; }
 
    // Test #3 - One Note - set fret to 3 from 5
    fret = 3;
    finger_ref.setFirstFret ( fret );
 
    savedNote = finger_ref.getNote ( 1 );
    if ( savedNote->getFret() != 3 ) { result = false; }
 
    // Test #4 - One Note, One Barre - set fret to 16 from 3
    Barre* b1_ptr = new Barre ( 3, 2, 1 );
    finger_ref.addBarre ( b1_ptr );
 
    fret = 16;
    finger_ref.setFirstFret( fret );
    savedNote = finger_ref.getNote ( 1 );
 
    if ( savedNote->getFret() != 16 ) { result = false; }
 
    Barre* savedBarre = finger_ref.getBarre ( 16 );
    if ( savedBarre == 0 )
    {
        result = false;
    }
    if ( savedBarre->getFret() != 16 ) { result = false; }
 
    if ( result )
    {
        std::cout << "  test_setFirstFret: PASSED" << std::endl;
    }
    else
    {
        std::cout << "  test_setFirstFret: FAILED" << std::endl;
    }
}
 
void test_deleteBarre ( void )
{
    GuitarNeck g_ref;
    Fingering a ( &g_ref );
    Barre b_1;
    b_1.setBarre ( 3, 2, 1 );
    a.addBarre ( &b_1 );
 
    Barre* savedBarre = a.getBarre ( 3 );
    BOOST_CHECK_EQUAL ( savedBarre->getStart(), static_cast<unsigned int>( 2 ) );
 
    a.deleteBarre ( &b_1 );
    savedBarre = a.getBarre ( 3 );
    if ( savedBarre != 0 )
    {
        BOOST_ERROR( "Failed to delete Barre" );
    }
}
 
void test_load ( void )
{
    QDomDocument doc;
    QDomElement fingering = doc.createElement ( "Fingering" );
 
    // Create Note element
    QDomElement note = doc.createElement ( "Note" );
    note.setAttribute ( "string", 1 );
    note.setAttribute ( "fret", 2 );
    note.setAttribute ( "action", "pressed" );
    fingering.appendChild ( note );
 
    // Create Barre element
    QDomElement barre = doc.createElement ( "Barre" );
    barre.setAttribute ( "fret", 1 );
    barre.setAttribute ( "start", 5 );
    barre.setAttribute ( "end", 1 );
    fingering.appendChild ( barre );
 
    // call Fingering::load
    GuitarNeck g_ref;
    Fingering a ( &g_ref );
    a.load ( fingering.firstChild() );
 
    // Check Note
    Note* savedNote = a.getNote ( 1 );
    BOOST_CHECK_EQUAL ( savedNote->getFret(), static_cast<unsigned int>( 2 ) );
 
    // Check Barre
    Barre* savedBarre = a.getBarre ( 1 );
    BOOST_CHECK_EQUAL ( savedBarre->getFret(), static_cast<unsigned int>( 1 ) );
    BOOST_CHECK_EQUAL ( savedBarre->getStart(), static_cast<unsigned int>( 5 ) );
    BOOST_CHECK_EQUAL ( savedBarre->getEnd(), static_cast<unsigned int>( 1 ) );
}
 
void test_save ( void )
{
    GuitarNeck g_ref;
    Fingering a ( &g_ref );
 
    Note n_1;
    n_1.setNote ( 1, 2 );
    a.addNote ( &n_1 );
 
    Barre b_1;
    b_1.setBarre ( 3, 2, 1 );
    a.addBarre ( &b_1 );
 
    QDomDocument doc;
    QDomElement fingering = doc.createElement ( "Fingering" );
 
    a.save( fingering );
 
    if ( ! fingering.hasChildNodes() )
    {
        BOOST_ERROR ( "Fingering XML element does not contain either the Note or Barre XML element" );
    }
 
    QDomNodeList children = fingering.childNodes();
    for ( unsigned int i = 0; i < children.length(); ++i )
    {
        QDomNode node = children.item( i );
        QDomNamedNodeMap attributes = node.attributes ();
 
        if ( node.nodeName() == "Barre" )
        {
            // Check 'fret' value
            QDomNode attrNode = attributes.namedItem( "fret" );
            if ( attrNode.isNull() )
            {
                BOOST_ERROR ( "Barre element missing 'fret' attribute" );
            }
            QString fret_str = attrNode.nodeValue();
            unsigned int fret_num = fret_str.toUInt();
            BOOST_CHECK_EQUAL ( fret_num, static_cast<unsigned int>( 3 ) );
 
            // Check 'start' value
            attrNode = attributes.namedItem( "start" );
            if ( attrNode.isNull() )
            {
                BOOST_ERROR ( "Barre element missing 'start' attribute" );
            }
            QString start_str = attrNode.nodeValue();
            unsigned int start_num = start_str.toUInt();
            BOOST_CHECK_EQUAL ( start_num, static_cast<unsigned int>( 2 ) );
 
            // Check 'end' value
            attrNode = attributes.namedItem( "end" );
            if ( attrNode.isNull() )
            {
                BOOST_ERROR ( "Barre element missing 'end' attribute" );
            }
            QString end_str = attrNode.nodeValue();
            unsigned int end_num = end_str.toUInt();
            BOOST_CHECK_EQUAL ( end_num, static_cast<unsigned int>( 1 ) );
        }
        else
        {
            // Chekc 'fret' value
            QDomNode attrNode = attributes.namedItem( "fret" );
            if ( attrNode.isNull() )
            {
                BOOST_ERROR ( "Note element missing 'fret' attribute" );
            }
            QString fret_str = attrNode.nodeValue();
            unsigned int fret_num = fret_str.toUInt();
            BOOST_CHECK_EQUAL ( fret_num, static_cast<unsigned int>( 2 ) );
 
            // Check 'string' value
            attrNode = attributes.namedItem( "string" );
            if ( attrNode.isNull() )
            {
                BOOST_ERROR ( "Barre element missing 'string' attribute" );
            }
            QString string_str = attrNode.nodeValue();
            unsigned int string_num = string_str.toUInt();
            BOOST_CHECK_EQUAL ( string_num, static_cast<unsigned int>( 1 ) );
        }
    }
}
*/

int main ( int, char * [] )
{
    std::cout << "Running test: FingeringConstructor" << std::endl;
    test_constructors();
    /*
    test_addNote ();
    test_addBarre ();
    test_equal ();
    test_setFirstFret();
        test->add ( BOOST_TEST_CASE( &test_deleteBarre ) );
        test->add ( BOOST_TEST_CASE( &test_load ) );
        test->add ( BOOST_TEST_CASE( &test_save ) );
    */ 
    return 0;
}

#endif
