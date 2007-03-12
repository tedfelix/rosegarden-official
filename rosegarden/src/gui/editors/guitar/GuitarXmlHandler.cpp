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

#include "GuitarXmlHandler.h"
#include "base/Exception.h"

#include <iostream>

#include <qfile.h>

namespace Rosegarden
{

namespace Guitar
{
GuitarXmlHandler::GuitarXmlHandler ()
{}

ChordMap const*
GuitarXmlHandler::parse ( QString const& file )
{
    Guitar::ChordMap * chordMap_ptr = new Guitar::ChordMap();

    std::cout << "GuitarXmlHandler::parse - file: " << file << std::endl;

    QDomDocument doc ( "guitar_chords" );

    QFile input_file ( file );
    if ( !input_file.open ( IO_ReadOnly ) ) {
        QString error = "GuitarXmlHandler::parse - Problem opening input file (";
        error += file;
        error += ")";
        throw Exception ( error );
    }
    if ( !doc.setContent ( &input_file ) ) {
        // EXCEPTION: XMLException - Failed to set content
        QString error = "GuitarXmlHandler::parse - Problem reading data from input file (";
        error += file;
        error += ")";
        input_file.close();
        throw Exception ( error );
    }
    input_file.close();

    QDomElement rootNode = doc.documentElement();
    if ( ! rootNode.isNull() ) {
        if ( ( rootNode.nodeType() == QDomNode::ElementNode ) &&
                ( rootNode.nodeName() == "Chord_Group" ) ) {
            try {
                this->handleChordGroup( chordMap_ptr, rootNode );
            } catch ( Exception & re ) {
                std::cerr << "GuitarXmlHandler::parse - Parse error occurred" << std::endl
                << "  " << re.getMessage() << std::endl
                << "GuitarXmlHandler::parse - Quiting parsing " << file
                << std::endl;
            }
        }
    } else {
        std::cerr << "GuitarXmlHandler::parse - Read operation failed"
        << std::endl
        << "  Could not find " << file.ascii()
        << std::endl;
    }

    return chordMap_ptr;
}

void GuitarXmlHandler::handleChordGroup ( Guitar::ChordMap* chordMap_ptr,
        QDomNode const& chord_grp_ref )
{
    // Parse each Chord
    if ( chord_grp_ref.hasChildNodes() ) {
        QDomNodeList const chord_list = chord_grp_ref.childNodes();
        unsigned int i = 0;
        while ( i < chord_list.length() ) {
            QDomNode node = chord_list.item( i );
            while ( node.nodeType() == QDomNode::CommentNode ) {
                ++i;
                node = chord_list.item( i );
            }

            if ( ( node.nodeType() == QDomNode::ElementNode ) &&
                    ( node.nodeName() == "Chord" ) ) {
                Chord * chd_ptr = new Chord();
                chd_ptr->load ( node );
                chordMap_ptr->insert ( chd_ptr );
            }
            ++i;
        }
    }
}
} /* namespace Guitar */

}
