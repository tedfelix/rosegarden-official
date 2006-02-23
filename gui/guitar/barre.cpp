#include "barre.h"
#include "symbols.h"
#include "base/Exception.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Property.h"

#include <iostream>
#include <sstream>

using namespace Rosegarden;

namespace guitar
{
const PropertyName Barre::BARRE_FRET = "barre_fret";
const PropertyName Barre::BARRE_START = "barre_start";
const PropertyName Barre::BARRE_END = "barre_end";

/*---------------------------------------------------------------
              Barre
  ---------------------------------------------------------------*/
Barre::Barre ()
        : m_fret (0),
	m_start (0),
	m_end (0)
{}

Barre::Barre (unsigned int fret, unsigned int start, unsigned int end)
        : m_fret (fret),
	m_start (start),
	m_end (end)
{}

void Barre::load ( QDomNode const& node )
{
    if ( ! node.hasAttributes() )
    {
        throw Rosegarden::Exception ( "Barre::load - Input node has no attributes" );
    }

    QDomNamedNodeMap attributes = node.attributes();
    QDomNode attrNode = attributes.namedItem( "fret" );
    if ( attrNode.isNull() )
    {
        throw Rosegarden::Exception ( "Barre::load - Input node missing required attribute (fret)" );
    }
    QString fretString = attrNode.nodeValue();
    m_fret = fretString.toUInt();

    attrNode = attributes.namedItem( "start" );
    if ( attrNode.isNull() )
    {
        throw Rosegarden::Exception ( "Barre::load - Input node missing required attribute (start)" );
    }
    QString startString = attrNode.nodeValue();
    m_start = startString.toUInt();

    attrNode = attributes.namedItem( "end" );
    if ( attrNode.isNull() )
    {
        throw Rosegarden::Exception ( "Barre::load - Input node missing required attribute (end)" );
    }
    QString endString = attrNode.nodeValue();
    m_end = endString.toUInt();
}

void Barre::save ( QDomNode& parentNode )
{
    QDomDocument domDoc = parentNode.ownerDocument();
    QDomElement node = domDoc.createElement ( "Barre" );
    node.setAttribute ( "fret", m_fret );
    node.setAttribute ( "start", m_start );
    node.setAttribute ( "end", m_end );

    parentNode.appendChild( node );
}

unsigned int
Barre::getFret ( void ) const
{
    return m_fret;
}

unsigned int
Barre::getStart ( void ) const
{
    return m_start;
}

unsigned int
Barre::getEnd ( void ) const
{
    return m_end;
}

void Barre::setBarre ( unsigned int fret, unsigned int start, unsigned int end )
{
    m_fret = fret;
    m_start = start;
    m_end = end;
}

void Barre::drawContents ( QPainter* p,
                           unsigned int startFret,
                           unsigned int stringCount,
                           unsigned int fretDisplayed )
{
    NoteSymbols ns;
    ns.drawBarreSymbol( p,
                        ( m_fret - startFret ),
                        ( stringCount - m_start ),
                        ( stringCount - m_end ),
                        stringCount,
                        fretDisplayed );
}

void Barre::setFirstFret ( int const& fretChange )
{
    m_fret += fretChange;
}

QString Barre::toString ( void ) const
{
    std::stringstream output;
    output << "  Barre:  fret #" << m_fret << " start #" << m_start
	 << " end #" << m_end << std::endl;
    return output.str();
}

bool Barre::operator== ( Barre const& rhs ) const
{
    return ( m_fret == rhs.m_fret ) &&
           ( m_start == rhs.m_start ) &&
           ( m_end == rhs.m_end );
}

}
