
#ifdef COMPILE_GUITAR_TESTS

#include "guitar.h"
#include "note.h"
#include <iostream>

//--------------------
//   Helper functions
//--------------------
QDomNode
createXMLNode ( QDomDocument& doc, unsigned int string_num, unsigned int fret_num, std::string action = "" )
{
    QDomElement node = doc.createElement ( "Note" );
    node.setAttribute ( "string", string_num );
    node.setAttribute ( "fret", fret_num );
    node.setAttribute ( "action", action );

    return node;
}

//--------------------
//      TESTS
//--------------------

void test_constructors ( void )
{
    Guitar::Note a;
    bool result = true;

    if ( a.getFret() != 0 ) {
        result = false;
    }
    if ( a.getStringNumber() != 0 ) {
        result = false;
    }
    if ( result ) {
        std::cout << "  test_constructors::PASSED" << std::endl;
    } else {
        std::cout << "  test_constructors::FAILED" << std::endl;
    }
}

void test_setNote ( void )
{
    Guitar::Note a;
    bool result = true;

    a.setNote ( 0, 0 );
    if ( a.getFret() != 0 ) {
        result = false;
    }
    if ( a.getStringNumber() != 0 ) {
        result = false;
    }

    a.setNote ( UINT_MAX, UINT_MAX );
    if ( a.getFret() != UINT_MAX ) {
        result = false;
    }
    if ( a.getStringNumber() != UINT_MAX ) {
        result = false;
    }

    if ( result ) {
        std::cout << "  test_setNote::PASSED" << std::endl;
    } else {
        std::cout << "  test_setNote::FAILED" << std::endl;
    }
}

void test_load ( void )
{
    Guitar::GuitarNeck g_ref;

    // Valid Node (string #0, fret#0, action: None)
    QDomDocument doc;
    QDomNode node = createXMLNode ( doc, 1, 1 );
    Guitar::Note a;
    bool result = true;

    a.load ( node, &g_ref );
    if ( a.getFret() != 1 ) {
        result = false;
    }
    if ( a.getStringNumber() != 1 ) {
        result = false;
    }

    // Valid Node (string #UINT_MAX, fret#UINT_MAX, action: None)
    node = createXMLNode ( doc, UINT_MAX, UINT_MAX );
    a.load ( node, &g_ref );
    if ( a.getFret() != UINT_MAX ) {
        result = false;
    }
    if ( a.getStringNumber() != UINT_MAX ) {
        result = false;
    }

    if ( result ) {
        std::cout << "  test_load::PASSED" << std::endl;
    } else {
        std::cout << "  test_load::FAILED" << std::endl;
    }
}

void test_save ( void )
{
    Guitar::GuitarNeck g_ref;
    bool result = true;

    // Setup Note
    Guitar::Note a;
    a.setNote ( 1, 1 );

    // Run save test #1
    QDomDocument doc;
    QDomElement parent_node = doc.createElement ( "Chord" );
    a.save ( parent_node, &g_ref );

    QDomNode save_child_node = parent_node.firstChild();

    if ( ! save_child_node.hasAttributes() ) {
        result = false;
    }

    QDomNamedNodeMap attributes = save_child_node.attributes();
    QDomNode attrNode = attributes.namedItem( "string" );
    if ( attrNode.isNull() ) {
        result = false;
    }

    QString string_str = attrNode.nodeValue();
    unsigned int string_num = string_str.toUInt();
    if ( string_num != 1 ) {
        result = false;
    }

    attrNode = attributes.namedItem( "fret" );
    if ( attrNode.isNull() ) {
        result = false;
    }
    QString fret_str = attrNode.nodeValue();
    unsigned int fret_num = fret_str.toUInt();
    if ( fret_num != 1 ) {
        result = false;
    }

    attrNode = attributes.namedItem( "action" );
    if ( attrNode.isNull() ) {
        result = false;
    }
    QString action_str = attrNode.nodeValue();

    if ( action_str != QString( "muted" ) ) {
        result = false;
    }

    if ( result ) {
        std::cout << "  test_save::PASSED" << std::endl;
    } else {
        std::cout << "  test_save::FAILED" << std::endl;
    }
}

void test_setFirstFret ( void )
{
    bool result = true;

    // Setup Note
    Guitar::Note a;
    a.setNote ( 1, 1 );

    a.setFirstFret ( 5 );
    if ( a.getFret() != 6 ) {
        result = false;
    }

    a.setFirstFret ( -4 );
    if ( a.getFret() != 2 ) {
        result = false;
    }

    if ( result ) {
        std::cout << "  test_setFirstFret::PASSED" << std::endl;
    } else {
        std::cout << "  test_setFirstFret::FAILED" << std::endl;
    }

}

void test_toString ( void )
{
    // Setup Note
    Guitar::Note a;
    a.setNote ( 1, 1 );
    const QString testOutput = a.toString();
    const QString expectedOutput ( "  Note: string #1 fret #1\n" );

    if ( testOutput == expectedOutput ) {
        std::cout << "  test_toString::PASSED" << std::endl;
    } else {
        std::cout << "  test_toString::FAILED" << std::endl;
    }

}

void test_equal ( void )
{
    bool result = true;
    Guitar::Note a;
    a.setNote ( 5, 3 );

    Guitar::Note b;
    b.setNote ( 5, 3 );

    if ( !( a == b ) ) {
        result = false;
    }

    Guitar::Note c;
    c.setNote ( 5, 4 );

    if ( c == a ) {
        result = false;
    }

    if ( result ) {
        std::cout << "  test_equal::PASSED" << std::endl;
    } else {
        std::cout << "  test_equal::FAILED" << std::endl;
    }

}

int main ( int, char* [] )
{
    test_constructors();
    test_setNote();
    test_load();
    test_save();
    test_setFirstFret();
    test_toString();
    test_equal();

    return 0;
}

#endif
