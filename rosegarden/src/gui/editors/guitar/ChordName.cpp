#include "ChordName.h"
#include "base/Exception.h"

#include <sstream>

namespace Rosegarden
{

namespace Guitar
{
/*---------------------------------------------------------------
              ChordName
  ---------------------------------------------------------------*/

ChordName::ChordName ()
        : m_scale ( "C" ),
        m_modifier ( "Major" ),
        m_suffix ( "None" ),
        m_version ( 0 )
{}

ChordName::ChordName ( ChordName const& rhs )
        : m_scale ( rhs.m_scale ),
        m_modifier ( rhs.m_modifier ),
        m_suffix ( rhs.m_suffix ),
        m_version ( rhs.m_version )
{
    for ( std::vector<ChordName*>::const_iterator pos = rhs.m_aliases.begin();
            pos != rhs.m_aliases.end();
            ++pos ) {
        ChordName* alias_ptr = (*pos);
        m_aliases.push_back ( new ChordName(*alias_ptr) );
    }
}

ChordName::~ChordName ()
{
    for ( std::vector<ChordName*>::iterator pos = m_aliases.begin();
            pos != m_aliases.end();
            ++pos ) {
        delete *pos;
    }
}

void ChordName::addAlias ( ChordName* name )
{
    std::vector<ChordName*>::iterator pos =
        find ( m_aliases.begin(), m_aliases.end(), name );

    if ( pos == m_aliases.end() ) {
        m_aliases.push_back ( new ChordName( *name ) );
    }
}

std::vector<ChordName*> const&
ChordName::getAliasList () const
{
    return m_aliases;
}

void ChordName::load ( QDomNode const& node )
{
    QDomNamedNodeMap attributes = node.attributes();
    QDomNode attrNode = attributes.namedItem( "scale" );
    if ( attrNode.isNull() ) {
        throw Exception ( "ChordName::load - Input node missing attribute (scale)" );
    }
    QString name = attrNode.nodeValue();

    if ( !name.isEmpty() ) {
        m_scale = name;
    }

    attrNode = attributes.namedItem( "modifier" );
    if ( !attrNode.isNull() ) {
        QString modifier = attrNode.nodeValue();
        if ( !modifier.isEmpty() ) {
            m_modifier = modifier;
        }
    }

    attrNode = attributes.namedItem( "suffix" );
    if ( !attrNode.isNull() ) {
        QString suffix = attrNode.nodeValue();

        if ( !suffix.isEmpty() ) {
            m_suffix = suffix;
        }
    }

    // Parse each alias
    QDomNodeList const alias_list = node.childNodes();
    for ( unsigned int i = 0; i < alias_list.length(); ++i ) {
        QDomNode aliasNode = alias_list.item( i );

        if ( ( aliasNode.nodeType() == QDomNode::ElementNode ) &&
                ( aliasNode.nodeName() == "Alias" ) ) {
            ChordName * alias = new ChordName();
            alias->load ( aliasNode );
            this->addAlias ( alias );
            delete alias;
        }
    }
}

void ChordName::save ( QDomNode& parentNode, bool isAlias )
{
    // Need a way to detect
    // If we are a parent chord with zero or more children
    // or we are an alias of a parent chord
    QDomDocument domDoc = parentNode.ownerDocument();

    // If m_aliases has content set name to "ChordName"
    // Else set name to "Alias"
    QString name = "ChordName";
    if ( ( m_aliases.empty() ) && ( isAlias ) ) {
        name = "Alias";
    } else {
        isAlias = true;
    }
    QDomElement node = domDoc.createElement ( name );

    // Save name
    node.setAttribute ( "suffix", m_suffix );
    node.setAttribute ( "modifier", m_modifier );
    node.setAttribute ( "scale", m_scale );

    for ( std::vector<ChordName*>::const_iterator pos = m_aliases.begin();
            pos != m_aliases.end();
            ++pos ) {
        ( *pos ) ->save( node, isAlias );
    }

    parentNode.appendChild ( node );
}

void ChordName::write ( QListBox*& major, QListBox*& modifier, QListBox*& suffix )
{
    //std::cout << "ChordName::write - name: " << m_scale << "   modifier: " << m_modifier
    //<< "    suffix: " << m_suffix << std::endl;

    if ( major->findItem( m_scale ) == 0 ) {
        major->insertItem ( m_scale );
    }

    if ( modifier->findItem ( m_modifier ) == 0 ) {
        modifier->insertItem ( m_modifier );
    }

    if ( suffix->findItem ( m_suffix ) == 0 ) {
        suffix->insertItem ( m_suffix );
    }
}

void ChordName::setName ( QString scale, QString modifier, QString suffix, unsigned int version )
{
    m_scale = scale;
    m_modifier = modifier;
    m_suffix = suffix;
    m_version = version;
}

std::string
ChordName::toString ( void ) const
{
    std::stringstream output;
    output << m_scale << ", " << m_modifier << ", " << m_suffix << std::endl;
    return output.str();
}

QString const&
ChordName::getScale ( void ) const
{
    return m_scale;
}

QString const&
ChordName::getModifier ( void ) const
{
    return m_modifier;
}

QString const&
ChordName::getSuffix ( void ) const
{
    return m_suffix;
}

unsigned int const& ChordName::getVersion ( void ) const
{
    return m_version;
}

void ChordName::setVersion ( unsigned int value )
{
    m_version = value;
}

bool
ChordName::operator== ( ChordName const& rhs ) const
{
    bool result = true;

    if ( this != &rhs ) {
        if ( ( m_scale != rhs.m_scale ) &&
                ( m_modifier != rhs.m_modifier ) &&
                ( m_suffix != rhs.m_suffix ) &&
                ( m_version != rhs.m_version ) ) {
            result = false;
        }

        std::vector<ChordName*>::const_iterator rhs_pos = rhs.m_aliases.begin();
        std::vector<ChordName*>::const_iterator this_pos = m_aliases.begin();

        while ( ( rhs_pos != rhs.m_aliases.end() ) && ( this_pos != m_aliases.end() ) && ( result ) ) {
            if ( ( *rhs_pos ) != ( *this_pos ) ) {
                result = false;
            }
        }
    }

    return result;
}

}

}
