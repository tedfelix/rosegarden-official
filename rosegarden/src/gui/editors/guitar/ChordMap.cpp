/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "misc/Debug.h"
#include "ChordMap.h"

#include <qfile.h>
#include <qtextstream.h>

namespace Rosegarden
{

namespace Guitar
{
    
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
    
    for (chordset::const_iterator i = m_map.lower_bound(tmp); i != m_map.end(); ++i) {
        NOTATION_DEBUG << "ChordMap2::getChords : checking chord " << *i << endl;
        
        if (i->getRoot() != root)
            break;

        if (/* ext.isNull() || */ i->getExt() == ext) {
            NOTATION_DEBUG << "ChordMap2::getChords : adding chord " << *i << endl;
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
        rootNotes = QStringList::split(QString(","), "A,A#/Bb,B,C,C#/Db,D,D#/Eb,E,F,F#/Gb,G,G#/Ab");
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
    
    for(chordset::const_iterator i = m_map.lower_bound(tmp); i != m_map.end(); ++i) {
        const Chord& chord = *i;
//        NOTATION_DEBUG << "ChordMap::getExtList : chord = " << chord << endl;
         
        if (chord.getRoot() != root)
            break;
            
        if (chord.getExt() != currentExt) {
//            NOTATION_DEBUG << "ChordMap::getExtList : adding ext " << chord.getExt() << " for root " << root << endl;
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

bool ChordMap::saveDocument(const QString& filename, QString& errMsg)
{
    QFile file(filename);
    file.open(IO_WriteOnly);
   
    QTextStream outStream(&file);
    
    outStream.setEncoding(QTextStream::UnicodeUTF8);
    
    outStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    << "<!DOCTYPE rosegarden-chord-data>\n"
    << "<rosegarden-chord-data version=\"" << VERSION
    << "\" format-version-major=\"" << FILE_FORMAT_VERSION_MAJOR
    << "\" format-version-minor=\"" << FILE_FORMAT_VERSION_MINOR
    << "\" format-version-point=\"" << FILE_FORMAT_VERSION_POINT
    << "\">\n";
    
    outStream << "<chords>\n";
    
    QString currentExt, currentRoot;
    
    for(iterator i = begin(); i != end(); ++i) {
        const Chord& chord = *i;
    
        if (chord.getRoot() != currentRoot) {

            currentRoot = chord.getRoot();
            
            // close current chordset (if there was one)
            if (i != begin())
                outStream << "\n</chordset>\n";

            // open new chordset            
            outStream << "<chordset root=\"" << chord.getRoot() << "\">";
            currentExt = "NEWEXT"; // to make sure we open a new chord right after that
        }
    
        if (chord.getExt() != currentExt) {
            
            currentExt = chord.getExt();
            
            // close current chord (if there was one)
            if (i != begin())
                outStream << "</chord>\n";
            // open new chord
            
            if (!chord.getExt().isEmpty())
                outStream << "<chord ext=\"" << chord.getExt() << "\">\n";
            else
                outStream << "<chord>\n";
        }
        
        outStream << "<fingering>" << chord.getFingering().toString() << "</fingering>\n";
    }

    if (!m_map.empty())
        outStream << "</chord>\n"; // close last written chord
        
    outStream << "</chords>\n";    
    outStream << "</rosegarden-chord-data>\n";
  
    return outStream.status() == IO_Ok;
}

int ChordMap::FILE_FORMAT_VERSION_MAJOR = 1;
int ChordMap::FILE_FORMAT_VERSION_MINOR = 0;
int ChordMap::FILE_FORMAT_VERSION_POINT = 0;


void
ChordMap::debugDump() const
{
    for(chordset::const_iterator i = m_map.begin(); i != m_map.end(); ++i) {
        Chord chord = *i;
        NOTATION_DEBUG << "ChordMap2::debugDump " << chord << endl;
    } 
}

}

}
