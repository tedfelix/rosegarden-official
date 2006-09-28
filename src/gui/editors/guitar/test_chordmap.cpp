//
// C++ Implementation: test_chordmap
//
// Description:
//
//
// Author: Chris Cannam <cannam@all-day-breakfast.com>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "chordmap.h"
#include "guitarxmlhandler.h"

using namespace Guitar;

void test_constructor()
{
    bool result = true;
    ChordMap a;

    if ( a.size() != 0 ) {
        result = false;
    }

    if ( result ) {
        std::cout << "  test_constructor: PASSED" << std::endl;
    } else {
        std::cout << "  test_constructor: FAILED" << std::endl;
    }
}

void test_insert()
{
    bool result = true;
    ChordMap a;

    ChordName* name = new ChordName();
    name->setName( "A", "Major", "7" );
    Chord* cd = new Chord ( name );
    a.insert( cd );
    if ( a.size() == 0 ) {
        result = false;
    }

    if ( result ) {
        std::cout << "  test_insert: PASSED" << std::endl;
    } else {
        std::cout << "  test_insert: FAILED" << std::endl;
    }


}

void test_append()
{
    bool result = true;

    // Append empty map into an empty map;
    ChordMap empty1;
    ChordMap empty2;
    empty2.append ( empty1.begin(), empty1.end() );

    // Append map with one element into an empty map
    ChordMap a;
    ChordName* name_ptr = new ChordName ();
    name_ptr->setName( "A", "Major", "7" );

    Chord* cd_ptr = new Chord ( name_ptr );
    a.insert( cd_ptr );

    ChordMap b;
    b.append ( a.begin(), a.end() );

    if ( b.size() == 0 ) {
        result = false;
    }

    ChordMap::pair data = b.find ( name_ptr );
    if ( ! data.first ) {
        result = false;
    }

    ChordMap c;
    GuitarXmlHandler g_ref;
    // Test attempting to insert a duplicate chord into a map
    a.insert ( cd_ptr );

    if ( a.size() != 1 ) {
        result = false;
    }

    ChordMap const* b_ptr = g_ref.parse( "../../chords/c_major.xml" );
    c.append ( b_ptr->begin(), b_ptr->end() );

    if ( b_ptr->size() != 3 ) {
        result = false;
    }
    if ( c.size() != 3 ) {
        result = false;
    }

    delete b_ptr;

    b_ptr = g_ref.parse( "../../chords/a_flat_major.xml" );
    c.append ( b_ptr->begin(), b_ptr->end() );

    if ( b_ptr->size() != 28 ) {
        result = false;
    }
    if ( c.size() != 31 ) {
        result = false;
    }

    delete b_ptr;

    b_ptr = g_ref.parse( "../../chords/g_sharp.xml" );
    c.append ( b_ptr->begin(), b_ptr->end() );

    if ( b_ptr->size() != 2 ) {
        result = false;
    }
    if ( c.size() != 33 ) {
        result = false;
    }

    delete b_ptr;

    if ( result ) {
        std::cout << "  test_append: PASSED" << std::endl;
    } else {
        std::cout << "  test_append: FAILED" << std::endl;
    }
}

int main ( int, char** )
{
    test_constructor();
    test_insert();
    test_append();

    return 0;
}
