#include "note.h"
#include "fingers.h"
#include "symbols.h"
#include "guitar.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/NotationTypes.h"
#include "base/Property.h"

#include <sstream>

using namespace Rosegarden;

namespace Guitar
{

const Rosegarden::PropertyName Note::NOTE_FRET = "note_fret";
const Rosegarden::PropertyName Note::NOTE_STRING = "note_string";

/*---------------------------------------------------------------
              Note
  ---------------------------------------------------------------*/
Note::Note ()
        : m_string ( 0 ),
	m_fret (0)
{}

Note::Note ( unsigned int str_val, unsigned int fret_val )
	: m_string (str_val),
	m_fret ( fret_val )
{}

Note::Note ( Note const& rhs )
        : m_string ( rhs.m_string ),
	m_fret ( rhs.m_fret )
{}

void Note::load ( QDomNode const& node, GuitarNeck* g_ptr )
{
    if ( ! node.hasAttributes() )
    {
        throw Rosegarden::Exception ( "Note::load - Input node missing attributes" );
    }

    QDomNamedNodeMap attributes = node.attributes();
    QDomNode attrNode = attributes.namedItem( "string" );
    if ( attrNode.isNull() )
    {
        throw Rosegarden::Exception ( "Note::load - Input node missing attribute (string)" );
    }
    QString string_str = attrNode.nodeValue();
    m_string = string_str.toUInt();

    attrNode = attributes.namedItem( "fret" );
    if ( !attrNode.isNull() )
    {
        QString fret_str = attrNode.nodeValue();
        m_fret = fret_str.toUInt();
    }

    attrNode = attributes.namedItem( "action" );
    if ( !attrNode.isNull() )
    {
        QString action_str = attrNode.nodeValue();

        if ( action_str == "open" )
        {
            g_ptr->setStringStatus ( string_str.toUInt(), GuitarString::OPEN );
        }
        else if ( action_str == "muted" )
        {
            g_ptr->setStringStatus ( string_str.toUInt(), GuitarString::MUTED );
        }
        else
        {
            g_ptr->setStringStatus ( string_str.toUInt(), GuitarString::PRESSED );
        }
    }
    else
    {
        g_ptr->setStringStatus ( string_str.toUInt(), GuitarString::PRESSED );
    }
}

void Note::save ( QDomNode& parentNode, GuitarNeck* g_ptr )
{
    QDomDocument domDoc = parentNode.ownerDocument();
    QDomElement node = domDoc.createElement ( "Note" );
    node.setAttribute ( "string", m_string );
    node.setAttribute ( "fret", m_fret );

    // Save action
    GuitarString::Action action = g_ptr->getStringStatus ( m_string );

    node.setAttribute ( "action", GuitarString::actionStringName( action ) );

    parentNode.appendChild( node );
}

void Note::setFirstFret ( int const& fret_change )
{
    m_fret += fret_change;
}

unsigned int
Note::getFret ( void ) const
{
    return m_fret;
}

void Note::setNote ( unsigned int str_val, unsigned int fret_val )
{
    m_string = str_val;
    m_fret = fret_val;
}

void Note::drawContents ( QPainter* p,
                          unsigned int start_fret,
                          unsigned int string_count,
                          unsigned int frets_displayed )
{
    unsigned int string_position = string_count - m_string;
    unsigned int fret_position = m_fret - start_fret;

    NoteSymbols ns;
    ns.drawNoteSymbol( p, string_position, fret_position, string_count, frets_displayed );
}

QString Note::noteName ( void )
{
    //Settings::noteName((appl[i] + parm->tune[i]) % 12));
    return "";
}


unsigned int
Note::getStringNumber ( void ) const
{
    return m_string;
}

QString Note::toString ( void ) const
{
    std::stringstream output;
    output << "  Note: string #" << m_string << " fret #" << m_fret << std::endl;
    return output.str();
}

/*
Rosegarden::Event*
Note::getData ( void ) const
{
    return m_data;
}
*/

bool Note::operator== ( Note const& rhs ) const
{
    return ( m_fret == rhs.m_fret ) &&
           ( m_string == rhs.m_string );
}

} /* namespace Guitar */
