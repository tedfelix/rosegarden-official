#include "chord.h"
#include "base/Exception.h"

#include <iostream>
#include <sstream>

namespace guitar
{
/*---------------------------------------------------------------
              Chord
  ---------------------------------------------------------------*/
Chord::Chord ()
        : m_name ( 0 ),
        m_arrangement ( new Fingering() ),
        m_written( false )
{}

Chord::Chord ( ChordName* name )
        : m_name ( name ),
        m_arrangement ( new Fingering() ),
        m_written( false )
{}

Chord::Chord ( ChordName* name, Fingering* arrange )
        : m_name ( name ),
        m_arrangement ( arrange ),
        m_written( false )
{}

Chord::Chord ( Chord const& rhs )
        : m_name ( new ChordName(*rhs.m_name) ),
        m_arrangement ( new Fingering(*rhs.m_arrangement) ),
        m_written ( rhs.m_written )
{}

Chord::~Chord ()
{
    delete m_name;
    delete m_arrangement;
}

ChordName*
Chord::getName ()
{
    return m_name;
}

Fingering*
Chord::getArrangement ()
{
    return m_arrangement;
}

void
Chord::load ( QDomNode const& node )
{
    QDomNode child_node = node.firstChild();

    while ( child_node.nodeType() == QDomNode::CommentNode )
    {
        child_node = child_node.nextSibling();
    }

    if ( ( child_node.nodeType() == QDomNode::ElementNode ) &&
            ( child_node.nodeName() == "ChordName" ) )
    {
        // Read ChordName
        m_name = new ChordName();
        m_name->load ( child_node );
    }

    child_node = child_node.nextSibling();
    while ( child_node.nodeType() == QDomNode::CommentNode )
    {
        child_node = child_node.nextSibling();
    }

    if ( ( child_node.nodeType() == QDomNode::ElementNode ) &&
            ( child_node.nodeName() == "StartFret" ) &&
            ( child_node.hasAttributes() ) )
    {
        QDomNamedNodeMap attributes = child_node.attributes();
        QDomNode numberNode = attributes.namedItem( "number" );
        if ( numberNode.isNull() )
        {
            throw Rosegarden::Exception ( "Chord::load - Input node missing required attribute (number)" );
        }

        QString const start_fret_str = numberNode.nodeValue();
        m_arrangement->setFirstFret ( start_fret_str.toInt() );
    }

    // Read Finginering
    child_node = child_node.nextSibling();
    m_arrangement->load ( child_node );
}

void Chord::save ( QDomNode& parentNode )
{
    if ( !m_written )
    {
        QDomDocument domDoc = parentNode.ownerDocument();
        QDomElement node = domDoc.createElement ( "Chord" );

        // Save ChordName
        m_name->save ( node );

        QDomElement startFretNode = domDoc.createElement ( "StartFret" );
        startFretNode.setAttribute ( "number", m_arrangement->getFirstFret() );
        node.appendChild( startFretNode );

        // Save all Bar and Note objects in Fingering
        m_arrangement->save ( node );

        parentNode.appendChild ( node );
        m_written = true;
    }
}

bool
Chord::operator== ( Chord const& rhs ) const
{
    bool result = true;

    if ( this != &rhs )
    {

        std::cout << "MATCHING:" << std::endl;
        std::cout << rhs.m_name->toString() << std::endl;
        std::cout << rhs.m_arrangement->toString() << std::endl;
        std::cout << m_name->toString() << std::endl;
        std::cout << m_arrangement->toString() << std::endl;

        result = ( ( *m_arrangement ) == ( *rhs.m_arrangement ) );
        if ( result )
        {
            std::cout << "   MATCH: yes" << std::endl;
        }
        else
        {
            std::cout << "   MATCH: no" << std::endl;
        }
        std::cout << std::endl;

    }

    return result;
}

std::string
Chord::toString ( void ) const
{
    std::stringstream output;
    output << "--------" << std::endl << "Chord" << std::endl << "--------" << std::endl;
    output << "  " << m_name->toString() << std::endl;
    output << m_arrangement->toString() << std::endl;
    return output.str();
}
}
