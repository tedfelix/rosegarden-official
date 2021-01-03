/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ChordMap]"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "ChordMap.h"

#include "rosegarden-version.h"

#include <QFile>
#include <QTextStream>

namespace Rosegarden
{

namespace Guitar
{
    
int ChordMap::FILE_FORMAT_VERSION_MAJOR = 1;
int ChordMap::FILE_FORMAT_VERSION_MINOR = 0;
int ChordMap::FILE_FORMAT_VERSION_POINT = 0;

ChordMap::ChordMap()
    : m_needSave(false)
{
}

void ChordMap::insert(const Chord& c)
{
    m_map.insert(c);
    m_needSave = true;
}


ChordMap::chordarray
ChordMap::getChords(const QString& root, const QString& ext) const
{
    chordarray res;

    Chord tmp(root, ext);

// RG_DEBUG << "ChordMap::getChords() : chord = " /*<< tmp*/ << 
//   " - ext is empty : " << ext.isEmpty();

    for (chordset::const_iterator i = m_map.lower_bound(tmp); i != m_map.end(); ++i) {

// RG_DEBUG << "ChordMap::getChords() : checking chord "/* << *i*/;

        if (i->getRoot() != root)
            break;

        if (/* ext.isNull() || */ i->getExt() == ext) {

// RG_DEBUG << "ChordMap::getChords() : adding chord " << *i;

            res.push_back(*i);
        } else {
            break;
        }
    }

    return res;
}
    
QStringList
ChordMap::getRootList() const
{
    static QStringList rootNotes;
    
    if (rootNotes.count() == 0) {
        rootNotes = QString("A,A#/Bb,B,C,C#/Db,D,D#/Eb,E,F,F#/Gb,G,G#/Ab")
            .split(",", QString::SkipEmptyParts);
    }
    
    // extract roots from map itself - not a very good idea
    //
//    QString currentRoot;
//    
//    for(chordset::const_iterator i = m_map.begin(); i != m_map.end(); ++i) {
//        const Chord& chord = *i;
//        if (chord.getRoot() != currentRoot) {
//            rootNotes.push_back(chord.getRoot());
//            currentRoot = chord.getRoot();
//        } 
//    }

    return rootNotes;
}

QStringList
ChordMap::getExtList(const QString& root) const
{
    QStringList extList;
    QString currentExt = "ZZ";
    
    Chord tmp(root);
    
    for (chordset::const_iterator i = m_map.lower_bound(tmp); 
         i != m_map.end(); ++i) {
        const Chord& chord = *i;

// RG_DEBUG << "ChordMap::getExtList() : chord = " << chord;
         
        if (chord.getRoot() != root)
            break;
            
        if (chord.getExt() != currentExt) {

// RG_DEBUG << "ChordMap::getExtList() : adding ext " << chord.getExt() << 
//   " for root " << root;

            extList.push_back(chord.getExt());
            currentExt = chord.getExt();
        }        
    }

    return extList;
}

void
ChordMap::substitute(const Chord& oldChord, const Chord& newChord)
{
    remove(oldChord);
    insert(newChord);
}

void
ChordMap::remove(const Chord& c)
{
    m_map.erase(c);
    m_needSave = true;    
}

bool ChordMap::saveDocument(
    const QString& filename, bool userChordsOnly, QString& /*errMsg*/)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly);
   
    QTextStream outStream(&file);
    
    outStream.setCodec("UTF-8");

    
    outStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    << "<!DOCTYPE rosegarden-chord-data>\n"
    << "<rosegarden-chord-data version=\"" << VERSION
    << "\" format-version-major=\"" << FILE_FORMAT_VERSION_MAJOR
    << "\" format-version-minor=\"" << FILE_FORMAT_VERSION_MINOR
    << "\" format-version-point=\"" << FILE_FORMAT_VERSION_POINT
    << "\">\n";
    
    outStream << "<chords>\n";
    
    QString currentExt, currentRoot;
    bool inChord = false;
    bool inChordset = false;

    for(iterator i = begin(); i != end(); ++i) {
        const Chord& chord = *i;
    
        if (userChordsOnly && !chord.isUserChord())
            continue; // skip non-user chords
            
        if (chord.getRoot() != currentRoot) {

            currentRoot = chord.getRoot();

            // If we are in a <chord>, close it.
            if (inChord) {
                outStream << "  </chord>\n";
                inChord = false;
            }

            // If we are in a <chordset>, close it.
            if (inChordset) {
                outStream << " </chordset>\n";
                inChordset = false;
            }

            // open new chordset            
            outStream << " <chordset root=\"" << chord.getRoot() << "\">\n";
            inChordset = true;
            currentExt = "NEWEXT"; // make sure we open a new chord
        }
    
        if (chord.getExt() != currentExt) {
            
            currentExt = chord.getExt();
            
            // If we are in a <chord>, close it.
            if (inChord) {
                outStream << "  </chord>\n";
                inChord = false;
            }

            // open new chord            
            outStream << "  <chord";
            if (!chord.getExt().isEmpty())
                outStream << " ext=\"" << chord.getExt() << "\"";
            if (chord.isUserChord())
                outStream << " user=\"true\"";
                
            outStream << ">\n";
            inChord = true;
        }

        outStream << "   <fingering>" << chord.getFingering().toString().c_str() <<
            "</fingering>\n";
    }

    // If we are in a <chord>, close it.
    if (inChord) {
        outStream << "  </chord>\n";
        inChord = false;
    }
        
    // If we are in a <chordset>, close it.
    if (inChordset) {
        outStream << " </chordset>\n";
        inChordset = false;
    }

    outStream << "</chords>\n";    
    outStream << "</rosegarden-chord-data>\n";
  
    return outStream.status() == QTextStream::Ok;
}

void
ChordMap::debugDump() const
{
    RG_DEBUG << "ChordMap::debugDump()";

    for (chordset::const_iterator chord = m_map.begin(); 
         chord != m_map.end(); ++chord) {
        RG_DEBUG << "  " << *chord;
    } 
}

}

}
