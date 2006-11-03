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

#include <iostream>

#include "Barre.h"

using namespace Guitar;

//--------------------
//   Helper functions
//--------------------
QDomNode
createXMLNode (QDomDocument& doc,
               unsigned int fret_num,
               unsigned int start_num,
               unsigned int end_num)
{
    QDomElement node = doc.createElement ("Barre");
    node.setAttribute ("fret", fret_num);
    node.setAttribute ("start", start_num);
    node.setAttribute ("end", end_num);

    return node;
}

//--------------------
//    TESTS
//--------------------
void test_constructors ()
{
    Guitar::Barre a;
    bool result = true;

    if (a.getFret() != 0) {
        result = false;
    }
    if (a.getStart() != 0) {
        result = false;
    }
    if (a.getEnd() != 0) {
        result = false;
    }
    if ( result ) {
        std::cout << "  test_constructors::PASSED" << std::endl;
    } else {
        std::cout << "  test_constructors::FAILED" << std::endl;
    }
}

void test_load (void)
{
    // Valid Barre (fret#1, start: 2, end: 1)
    QDomDocument doc;
    QDomNode node = createXMLNode (doc, 1, 2, 1);
    Guitar::Barre a;
    bool result = true;

    a.load (node);
    if (a.getFret() != 1) {
        result = false;
    }
    if (a.getStart() != 2) {
        result = false;
    }
    if (a.getEnd() != 1) {
        result = false;
    }

    // Valid Node (string #UINT_MAX, fret#UINT_MAX, action: None)
    node = createXMLNode (doc, UINT_MAX, UINT_MAX, UINT_MAX - 1);
    a.load (node);
    if (a.getFret() != UINT_MAX) {
        result = false;
    }
    if (a.getStart() != UINT_MAX) {
        result = false;
    }
    if (a.getEnd() != (UINT_MAX - 1)) {
        result = false;
    }
    if ( result ) {
        std::cout << "  test_load::PASSED" << std::endl;
    } else {
        std::cout << "  test_load::FAILED" << std::endl;
    }
}

void
test_save (void)
{
    Guitar::Barre a;
    bool result = true;

    a.setBarre (1, 2, 1);

    QDomDocument doc;
    QDomElement parentNode = doc.createElement ("Chord");

    a.save (parentNode);

    QDomNode saveChildNode = parentNode.firstChild();

    if (! saveChildNode.hasAttributes()) {
        result = false;
    }

    QDomNamedNodeMap attributes = saveChildNode.attributes();
    QDomNode attrNode = attributes.namedItem("fret");
    if (attrNode.isNull()) {
        result = false;
    }
    QString fret_str = attrNode.nodeValue();
    unsigned int fret_num = fret_str.toUInt();
    if (fret_num != 1) {
        result = false;
    }

    attrNode = attributes.namedItem("start");
    if (attrNode.isNull()) {
        result = false;
    }
    QString start_str = attrNode.nodeValue();
    unsigned int start_num = start_str.toUInt();
    if (start_num != 2) {
        result = false;
    }

    attrNode = attributes.namedItem("end");
    if (attrNode.isNull()) {
        result = false;
    }
    QString end_str = attrNode.nodeValue();
    unsigned int end_num = end_str.toUInt();
    if (end_num != 1) {
        result = false;
    }

    if ( result ) {
        std::cout << "  test_save::PASSED" << std::endl;
    } else {
        std::cout << "  test_save::FAILED" << std::endl;
    }
}

void test_setFirstFret (void)
{
    Guitar::Barre a;
    bool result = true;

    a.setBarre (1, 2, 1);

    a.setFirstFret (5);
    if (a.getFret() != 6) {
        result = false;
    }

    a.setFirstFret ( -4);
    if (a.getFret() != 2) {
        result = false;
    }

    if ( result ) {
        std::cout << "  test_setFirstFret::PASSED" << std::endl;
    } else {
        std::cout << "  test_setFirstFret::FAILED" << std::endl;
    }
}

void test_toString (void)
{
    // Setup Note
    Guitar::Barre a;
    a.setBarre (1, 2, 1);
    const QString testOutput = a.toString();
    const QString expectedOutput ("  Barre:  fret #1 start #2 end #1\n");

    if (testOutput == expectedOutput ) {
        std::cout << "  test_toString::PASSED" << std::endl;
    } else {
        std::cout << "  test_toString::FAILED" << std::endl;
    }

}

void test_equal ( void )
{
    bool result = true;
    Guitar::Barre a;
    if (!( a == a) ) {
        result = false;
    }

    Guitar::Barre b;
    b.setBarre (1, 2, 1);

    if ( a == b ) {
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
    test_load();
    test_save();
    test_setFirstFret();
    test_toString();
    test_equal();
    return 0;
}

#endif
