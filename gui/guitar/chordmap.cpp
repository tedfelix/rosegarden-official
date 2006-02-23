#include "chordmap.h"
#include "chordname.h"
#include "base/Exception.h"

#include <qfile.h>

#include <sstream>
#include <iostream>

namespace guitar
{

ChordMap::~ChordMap ( void )
{
    // For each Chord in ChordMap
    for ( std::list<Chord*>::const_iterator pos = m_chordList.begin();
            pos != m_chordList.end();
            ++pos )
    {
        delete *pos;
    }

    for ( ScaleMap::iterator nPos = m_chords.begin();
            nPos != m_chords.end();
            ++nPos )
    {
        ScaleMap::value_type nMapPair = ( *nPos );
        ScaleMap::mapped_type mod_ptr = nMapPair.second;

        for ( ModifierMap::iterator iPos = mod_ptr->begin();
                iPos != mod_ptr->end();
                ++iPos )
        {
            ModifierMap::value_type iMapPair = ( *iPos );
            ModifierMap::mapped_type suf_ptr = iMapPair.second;

            for ( SuffixMap::iterator dPos = suf_ptr->begin();
                    dPos != suf_ptr->end();
                    ++dPos )
            {
                SuffixMap::value_type dMapPair = ( *dPos );
                SuffixMap::mapped_type ver_ptr = dMapPair.second;
                delete ver_ptr;
            }
            delete suf_ptr;
        }
        delete mod_ptr;
    }
}

void ChordMap::insert ( Chord * chordPtr ) throw ( DuplicateException )
{
    std::vector<ChordName*> aliasList;

    ChordName * namePtr = chordPtr->getName();
    if ( namePtr == 0 )
    {
        std::cerr << "ChordMap::insert - missing a ChordName object for chord."
        << " Skipping Chord insert"
        << std::endl;
        return ;
    }

    // Search for duplicate here
    Fingering* target_ptr = chordPtr->getArrangement();

    ChordMap::pair result = this->find ( target_ptr );
    if ( ! result.first )
    {
        // No duplicate found. Insert new chord
        this->insertChord ( namePtr, chordPtr, true );
        ++m_mapSize;

        // Add Chord to map by all its alias names
        ChordName* name_ptr = chordPtr->getName();
        aliasList = name_ptr->getAliasList();

        for ( std::vector<ChordName*>::const_iterator pos = aliasList.begin();
                pos != aliasList.end();
                ++pos )
        {
            this->insertChord ( *pos, chordPtr );
        }
        /*
                std::cout << "==========================================================" << std::endl;
                std::cout << "  ChordMap::insert - dump chord map of size " << m_mapSize << std::endl;
                std::cout << this->toString() << std::endl;
                std::cout << "==========================================================" << std::endl;
        */
    }
    else
    {
        throw DuplicateException ( result.second, chordPtr );
    }
}

void ChordMap::insertChord ( ChordName * namePtr, Chord * chordPtr, bool addChord )
{
    // if true add chord to main list
    // Also add chord to fingering list
    if ( addChord )
    {
        m_chordList.push_back( chordPtr );
    }

    // Add chord to scale map
    ScaleMap::iterator scalePos = m_chords.find ( namePtr->getScale() );

    // If scalePos != end then m_chords has a Modifier map for the given scale
    if ( scalePos != m_chords.end() )
    {
        // Grab iterator to Modifier map
        ScaleMap::value_type scaleMapPair = ( *scalePos );
        ScaleMap::mapped_type mMap = scaleMapPair.second;
        ModifierMap::iterator mPos = mMap->find ( namePtr->getModifier() );

        // If intPos != end then Modifier map has a Suffix map for the given modifier
        if ( mPos != mMap->end() )
        {
            // Grab iterator to Suffix Map
            ModifierMap::value_type modMapPair = ( *mPos );
            ModifierMap::mapped_type sufMap = modMapPair.second;
            SuffixMap::iterator sPos = sufMap->find ( namePtr->getSuffix() );

            // If distPos != end then Suffix map has a Version map for the given suffix
            if ( sPos != sufMap->end() )
            {
                // Grab Version map
                SuffixMap::value_type sufMapPair = ( *sPos );
                SuffixMap::mapped_type verMap = sufMapPair.second;
                this->versionAdd ( verMap, namePtr, chordPtr );
            }
            // Else no Version map
            else
            {
                // Create Version map
                VersionMap* vMap = this->createVersionMap ( namePtr, chordPtr );
                // add Version map to intPos
                sufMap->insert ( std::make_pair( namePtr->getSuffix(), vMap ) );
            }
        }
        // Else no Suffix map
        else
        {
            // Create Suffix map
            SuffixMap* sMap = this->createSuffixMap ( namePtr, chordPtr );
            // add Suffix map to scalePos
            mMap->insert ( std::make_pair( namePtr->getModifier(), sMap ) );
        }
    }
    // Else no Modifier map
    else
    {
        // Create Modifier map
        ModifierMap* mMap = this->createModifierMap ( namePtr, chordPtr );
        // add Modifier map to m_chords
        m_chords.insert ( std::make_pair( namePtr->getScale(), mMap ) );
    }
}

ChordMap::VersionMap*
ChordMap::createVersionMap ( ChordName * namePtr, Chord * chordPtr )
{
    VersionMap * vMap = new VersionMap();
    this->versionAdd ( vMap, namePtr, chordPtr );
    return vMap;
}

ChordMap::SuffixMap*
ChordMap::createSuffixMap ( ChordName * namePtr, Chord * chordPtr )
{
    SuffixMap * sMap = new SuffixMap();
    VersionMap* vMap = this->createVersionMap ( namePtr, chordPtr );
    sMap->insert ( std::make_pair( namePtr->getSuffix(), vMap ) );
    return sMap;
}

ChordMap::ModifierMap*
ChordMap::createModifierMap ( ChordName * namePtr, Chord * chordPtr )
{
    ModifierMap * mMap = new ModifierMap();
    SuffixMap* sMap = this->createSuffixMap ( namePtr, chordPtr );
    mMap->insert ( std::make_pair( namePtr->getModifier(), sMap ) );
    return mMap;
}

void
ChordMap::append ( ChordMap::const_iterator begin, ChordMap::const_iterator end )
{
    // For all scale entries
    for ( ChordMap::const_iterator cpos = begin;
            cpos != end;
            ++cpos )
    {
        Chord* chord_ptr = ( *cpos );
        this->insert ( new Chord ( *chord_ptr ) );
    }
}

ChordMap::const_iterator
ChordMap::begin ( void ) const
{
    return m_chordList.begin();
}

ChordMap::const_iterator
ChordMap::end ( void ) const
{
    return m_chordList.end();
}


//! Add Chord to version map
void ChordMap::versionAdd ( VersionMap * vMap_ptr,
                            ChordName * name_ptr,
                            Chord * chord_ptr )
{
    // If table is emtpy
    if ( vMap_ptr->empty() )
    {
        name_ptr->setVersion ( 1 );
    }
    else
    {
        // Grab last chord in version map
        // We don't care about the chord pointer returned from findDuplicate
        VersionMap::reverse_iterator endChord = vMap_ptr->rbegin();
        VersionMap::value_type endChordPair = ( *endChord );
        VersionMap::mapped_type endChord_ptr = endChordPair.second;
        ChordName* endChordName = endChord_ptr->getName();

        // Set version of new Chord
        name_ptr->setVersion ( endChordName->getVersion() + 1 );
    }

    VersionMap::value_type newVerPair =
        std::make_pair( name_ptr->getVersion(), chord_ptr );
    vMap_ptr->insert( newVerPair );
}

unsigned int const&
ChordMap::size ( void ) const
{
    return m_mapSize;
}

ChordMap::pair
ChordMap::find ( ChordName * namePtr )
{
    ChordMap::pair result;
    result.first = false;

    // Search for scale string name in Name map
    ScaleMap::const_iterator nPos = m_chords.find ( namePtr->getScale() );
    if ( nPos != m_chords.end() )
    {
        // Found a modifier map associated with scale string name
        ScaleMap::value_type nMapPair = ( *nPos );

        // Search for modifier string name in Modifier map
        ScaleMap::mapped_type mod_ptr = nMapPair.second;
        ModifierMap::const_iterator iPos = mod_ptr->find ( namePtr->getModifier() );

        if ( iPos != mod_ptr->end() )
        {
            // Found a suffix map associated with modifier string name
            ModifierMap::value_type iMapPair = ( *iPos );

            // Search for a suffix string name in Suffix Map
            ModifierMap::mapped_type suf_ptr = iMapPair.second;
            SuffixMap::const_iterator dPos = suf_ptr->find ( namePtr->getSuffix() );

            if ( dPos != suf_ptr->end() )
            {
                // Found a version map associates with suffix name
                SuffixMap::value_type dMapPair = ( *dPos );

                // Search for version string name in Suffix map
                SuffixMap::mapped_type ver_ptr = dMapPair.second;
                VersionMap::const_iterator vPos =
                    ver_ptr->find ( namePtr->getVersion() );
                if ( vPos != ver_ptr->end() )
                {
                    VersionMap::value_type vMapPair = ( *vPos );
                    result.first = true;
                    result.second = vMapPair.second;
                }
            }
        }
    }

    return result;
}

std::list<QString>
ChordMap::getNameList ( void ) const
{
    std::list<QString> result;

    for ( ScaleMap::const_iterator pos = m_chords.begin();
            pos != m_chords.end();
            ++pos )
    {
        ScaleMap::value_type pairPos = ( *pos );
        result.push_back ( pairPos.first );
    }
    return result;
}

std::list<QString>
ChordMap::getModifierList ( QString scale ) const
{
    std::list<QString> result;

    ScaleMap::const_iterator pos = m_chords.find( scale );
    if ( pos != m_chords.end() )
    {
        ScaleMap::value_type nMapPair = ( *pos );
        ScaleMap::mapped_type mod_ptr = nMapPair.second;

        for ( ModifierMap::const_iterator iPos = mod_ptr->begin();
                iPos != mod_ptr->end();
                ++iPos )
        {
            ModifierMap::value_type iMapPair = ( *iPos );
            result.push_back ( iMapPair.first );
        }
    }
    return result;
}

std::list<QString>
ChordMap::getSuffixList ( QString scale, QString modifier ) const
{
    std::list<QString> result;

    ScaleMap::const_iterator nPos = m_chords.find( scale );
    if ( nPos != m_chords.end() )
    {
        ScaleMap::value_type nMapPair = ( *nPos );
        ScaleMap::mapped_type mod_ptr = nMapPair.second;
        ModifierMap::const_iterator iPos = mod_ptr->find ( modifier );
        if ( iPos != mod_ptr->end() )
        {
            ModifierMap::value_type iMapPair = ( *iPos );
            ModifierMap::mapped_type suf_ptr = iMapPair.second;

            for ( SuffixMap::const_iterator dPos = suf_ptr->begin();
                    dPos != suf_ptr->end();
                    ++dPos )
            {
                SuffixMap::value_type dMapPair = ( *dPos );
                result.push_back ( dMapPair.first );
            }
        }
    }
    return result;
}

std::list<QString>
ChordMap::getVersionList ( QString scale, QString modifier, QString suffix ) const
{
    std::list<QString> result;

    ScaleMap::const_iterator nPos = m_chords.find( scale );

    if ( nPos != m_chords.end() )
    {

        ScaleMap::value_type nMapPair = ( *nPos );
        ScaleMap::mapped_type mod_ptr = nMapPair.second;
        ModifierMap::const_iterator iPos = mod_ptr->find ( modifier );
        if ( iPos != mod_ptr->end() )
        {
            ModifierMap::value_type iMapPair = ( *iPos );
            ModifierMap::mapped_type suf_ptr = iMapPair.second;

            SuffixMap::const_iterator dPos = suf_ptr->find( suffix );
            if ( dPos != suf_ptr->end() )
            {
                SuffixMap::value_type dMapPair = ( *dPos );
                SuffixMap::mapped_type ver_ptr = dMapPair.second;

                for ( VersionMap::const_iterator vPos = ver_ptr->begin();
                        vPos != ver_ptr->end();
                        ++vPos )
                {
                    VersionMap::value_type vMapPair = ( *vPos );
                    QCString name_str;
                    name_str.setNum ( vMapPair.first );
                    result.push_back ( name_str );
                }
            }
        }
    }
    return result;
}

void ChordMap::save ( QString const & directory ) const
{
    // For each scale in ScaleMap
    for ( ScaleMap::const_iterator nPos = m_chords.begin();
            nPos != m_chords.end();
            ++nPos )
    {
        // Create file name
        ScaleMap::value_type nMapPair = ( *nPos );
        ScaleMap::mapped_type mod_ptr = nMapPair.second;

        std::stringstream filename;
        filename << directory << "/" << nMapPair.first << "_scale_chords.xml";

        // Create a XML file
        QDomDocument doc;
        QDomElement rootNode = doc.createElement ( "Chord_Group" );

        //   For each Chord for a given scale
        for ( ModifierMap::const_iterator iPos = mod_ptr->begin();
                iPos != mod_ptr->end();
                ++iPos )
        {
            ModifierMap::value_type iMapPair = ( *iPos );
            ModifierMap::mapped_type suf_ptr = iMapPair.second;

            for ( SuffixMap::const_iterator dPos = suf_ptr->begin();
                    dPos != suf_ptr->end();
                    ++dPos )
            {
                SuffixMap::value_type dMapPair = ( *dPos );
                SuffixMap::mapped_type ver_ptr = dMapPair.second;

                for ( VersionMap::const_iterator vPos = ver_ptr->begin();
                        vPos != ver_ptr->end();
                        ++vPos )
                {
                    VersionMap::value_type vMapPair = ( *vPos );
                    VersionMap::mapped_type chord_ptr = vMapPair.second;

                    //     write Chord to XML file
                    chord_ptr->save ( rootNode );
                }
            }
        }

        doc.appendChild ( rootNode );

        //   Close file
        QFile output_file ( filename.str() );
        if ( !output_file.open( IO_WriteOnly ) )
        {
            QString error = "Chordmap::save - Problem opening output file (";
            error += filename.str();
            error += ")";
            throw Rosegarden::Exception ( error );
        }
        QTextStream stream ( &output_file );
        stream << doc.toString();
        output_file.close();

    }
}

QString
ChordMap::toString ( void ) const
{
    std::stringstream output;
    output << "-----------------" << std::endl
    << "    Chord Map    " << std::endl
    << "-----------------" << std::endl << std::endl;

    for ( ScaleMap::const_iterator nPos = m_chords.begin();
            nPos != m_chords.end();
            ++nPos )
    {
        ScaleMap::value_type nMapPair = ( *nPos );
        ScaleMap::mapped_type mod_ptr = nMapPair.second;
        ScaleMap::key_type scale_name = nMapPair.first;

        for ( ModifierMap::const_iterator iPos = mod_ptr->begin();
                iPos != mod_ptr->end();
                ++iPos )
        {
            ModifierMap::value_type iMapPair = ( *iPos );
            ModifierMap::mapped_type suf_ptr = iMapPair.second;
            ModifierMap::key_type mod_name = iMapPair.first;

            for ( SuffixMap::const_iterator sPos = suf_ptr->begin();
                    sPos != suf_ptr->end();
                    ++sPos )
            {
                SuffixMap::value_type sMapPair = ( *sPos );
                SuffixMap::mapped_type ver_ptr = sMapPair.second;
                SuffixMap::key_type suf_name = sMapPair.first;

                output << "Chord: " << "Scale=\"" << scale_name << "\" ";
                output << "Modifier=\"" << mod_name << "\" ";
                output << "Suffix=\"" << suf_name << "\" " << std::endl;

                for ( VersionMap::const_iterator vPos = ver_ptr->begin();
                        vPos != ver_ptr->end();
                        ++vPos )
                {
                    VersionMap::value_type vMapPair = ( *vPos );
                    VersionMap::key_type ver_num = vMapPair.first;
                    VersionMap::mapped_type chord_ptr = vMapPair.second;

                    output << std::endl << "Version #" << ver_num << std::endl;
                    output << chord_ptr->toString() << std::endl;
                }
            }
        }
    }
    return output.str();
}

ChordMap::pair
ChordMap::find ( Fingering * input_finger_ptr )
{
    ChordMap::pair result;
    result.first = false;

    // For each Chord in ChordMap
    for ( std::list<Chord*>::const_iterator pos = m_chordList.begin();
            pos != m_chordList.end();
            ++pos )
    {
        Chord* exist_ptr = *pos;

        Fingering* existing_finger_ptr = exist_ptr->getArrangement();
        if ( *existing_finger_ptr == *input_finger_ptr )
        {
            result.first = true;
            result.second = exist_ptr;
            break;
        }
    }

    return result;
}

void ChordMap::erase ( Chord* chordPtr )
{
    Fingering * targetPtr = chordPtr->getArrangement();

    // Remove chord from chord list: O(N)
    for ( std::list<Chord*>::iterator pos = m_chordList.begin();
            pos != m_chordList.end();
            ++pos )
    {
        Chord* existingChordPtr = ( *pos );
        Fingering* existingPtr = existingChordPtr->getArrangement();

        if ( *existingPtr == *targetPtr )
        {
            m_chordList.erase ( pos );
            delete *pos;
            break;
        }
    }

    // Remove name from ChordMap
    ChordName* name_ptr = chordPtr->getName();
    this->eraseChordName ( name_ptr );

    std::vector<ChordName*> aliasList = name_ptr->getAliasList();

    for ( std::vector<ChordName*>::const_iterator pos = aliasList.begin();
            pos != aliasList.end();
            ++pos )
    {
        this->eraseChordName ( *pos );
    }
}

void ChordMap::eraseChordName ( ChordName* namePtr )
{
    // Search for scale string name in Name map
    ScaleMap::const_iterator nPos = m_chords.find ( namePtr->getScale() );

    // Found a modifier map associated with scale string name
    ScaleMap::value_type nMapPair = ( *nPos );

    // Search for modifier string name in Modifier map
    ScaleMap::mapped_type mod_ptr = nMapPair.second;
    ModifierMap::const_iterator iPos = mod_ptr->find ( namePtr->getModifier() );

    // Found a suffix map associated with modifier string name
    ModifierMap::value_type iMapPair = ( *iPos );

    // Search for a suffix string name in Suffix Map
    ModifierMap::mapped_type suf_ptr = iMapPair.second;
    SuffixMap::const_iterator dPos = suf_ptr->find ( namePtr->getSuffix() );

    // Found a version map associates with suffix name
    SuffixMap::value_type dMapPair = ( *dPos );

    // Search for version string name in Suffix map
    SuffixMap::mapped_type ver_ptr = dMapPair.second;
    VersionMap::iterator vPos = ver_ptr->find ( namePtr->getVersion() );

    ver_ptr->erase ( vPos );
}

}
