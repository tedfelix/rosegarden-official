/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Tuning.h"

#include "base/NotationTypes.h"
#include "gui/general/ResourceFinder.h"

#include <QtDebug>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QFile>

#include <stdlib.h>
#include <qfile.h>
#include <qstring.h>
#include <iostream>
#include <qtextstream.h>
#include <math.h>
#include <string>


// Set the debug level to:
//
// 1: summary printout of tunings after they've been read (default)
// 2: detail while parsing tunings file (quite verbose)
// 3: more detail on XML parser state (rather verbose)
#define TUNING_DEBUG 0

using namespace Rosegarden::Accidentals;

// Map accidental number to accidental string
const Tuning::AccMap::value_type Tuning::accMapData[] = {
    AccMap::value_type(-4, &DoubleFlat),
    AccMap::value_type(-3, &ThreeQuarterFlat),
    AccMap::value_type(-2, &Flat),
    AccMap::value_type(-1, &QuarterFlat),
    AccMap::value_type(0,  &NoAccidental),
    AccMap::value_type(1,  &QuarterSharp),
    AccMap::value_type(2,  &Sharp),
    AccMap::value_type(3,  &ThreeQuarterSharp),
    AccMap::value_type(4,  &DoubleSharp)
};
const unsigned int Tuning::accMapSize =
    sizeof(Tuning::accMapData) / sizeof(Tuning::accMapData[0]);
Tuning::AccMap Tuning::accMap(Tuning::accMapData,
                              Tuning::accMapData + Tuning::accMapSize);

std::vector<Tuning*> Tuning::m_tunings;

std::vector<Tuning*> *Tuning::getTunings() {
    
    // if already have tunings, return them
    // TODO: It would be polite to check the mtime on the tunings file
    //       and to re-read it if it's been changed. For now, you need
    //       to restart Rosegarden.
    if (!m_tunings.empty())
        return &m_tunings;
    
    QString tuningsPath =
        ResourceFinder().getResourcePath("pitches", "tunings.xml");
    if (tuningsPath == "") return NULL;
    #   if (TUNING_DEBUG > 1)
    qDebug() << "Path to tunings file:" << tuningsPath;
    #   endif
    QFile infile(tuningsPath);
    
    IntervalList *intervals = new IntervalList;
    SpellingList *spellings = new SpellingList;
    
    if (infile.open(QIODevice::ReadOnly) ) {
        QXmlStreamReader stream(&infile);
        QString tuningName, intervalRatio;
        
        stream.readNextStartElement();
        if (stream.name() != "rosegarden_scales") {
            qDebug()  << "Tunings configuration file " << tuningsPath
                      << " is not a valid rosegarden scales file. "
                      << "(Root element is " << stream.name() << ")";
            return NULL;
        }
        
        enum {needTuning, needName, needInterval, needSpellings} state;
#       if (TUNING_DEBUG > 2)
        static const char * stateNames[] = {
            "expecting <tuning>", "expecting <name>", 
            "expecting <interval>", "expecting <spelling>"
        };
#       endif
        state = needTuning;
        
        while(!stream.atEnd()) {

            // Read to the next element delimiter
            do {
                stream.readNext();
            } while(!stream.isStartElement() &&
                    !stream.isEndElement() &&
                    !stream.atEnd());
            
            if (stream.atEnd()) break;
                        
            // Perform state transistions at end elements
            if (stream.isEndElement()) {
                if (stream.name() == "tuning") {
                    // Save the tuning and prepare for a new one
                    saveTuning(tuningName, intervals, spellings);
                    intervals = new IntervalList;
                    spellings = new SpellingList;       
                    state = needTuning;
                } else if (stream.name() == "name") {
                    // End of tuning name: expect intervals
                    state = needInterval;
                } else if (stream.name() == "interval") {
                    // After an </interval>, we're expecting another interval
                    state = needInterval;
                } else if (stream.name() == "rosegarden_scales") {
                    // XML's fininshed. Don't need the current tuning
                    // or spelling lists created when the last tuning ended
                    // so let's not leak memory.
                    delete intervals;
                    delete spellings;
                    // Don't bother reading any more of the file
                    break;
                }
                
#               if (TUNING_DEBUG > 2)
                qDebug() << "End of XML element " << stream.name()
                         << "New state: " << state
                         << " (" << stateNames[state] << ")";
#               endif
                continue;
            }
            
            // So it's a start element. Parse it.
            
            // If we are in the needSpellings state but hit a new interval,
            // we need to process that. So force a state-change here.
            if (state == needSpellings && stream.name() == "interval") {
                state = needInterval;
            }
            
#           if (TUNING_DEBUG > 2)
            qDebug() << "XML Element: " << stream.name()
            << " Current XML parser state: " << state
            << " (" << stateNames[state] << ")";
#           endif
            
            switch (state) {
            case needTuning:
                if (stream.name() != "tuning") {
                    qDebug() << "Reading Tunings. Expected tuning element, "
                             << "found " << stream.name();
                    stream.skipCurrentElement();
                } else {                
                    // Require a name element
                    state = needName;
                }
                break;
                
            case needName:
                if (stream.name() != "name") {
                    qDebug() << "Tuning must start with a <name> element, "
                             << "found <" << stream.name() << ">";
                    stream.skipCurrentElement();
                } else {
                    tuningName = stream.readElementText();
                    state = needInterval;
#                   if (TUNING_DEBUG > 1)
                    qDebug() << "\nNew Tuning: " << tuningName;
#                   endif
                }
                break;
                
            case needInterval:
                if (stream.name() != "interval") {
                    qDebug() << "Expecting an <interval> element, "
                             << "found <" << stream.name() << ">";
                    // Bail out
                    stream.skipCurrentElement();
                } else {
                    intervalRatio =
                        stream.attributes().value("ratio").toString();
#                   if (TUNING_DEBUG > 1)
                    qDebug() << "New Ratio: " << intervalRatio;
#                   endif
                    const double cents =
                        scalaIntervalToCents(intervalRatio,
                                             stream.lineNumber());
                    intervals->push_back(cents);
                    state = needSpellings;
                }
                break;
            
            case needSpellings:
                if (stream.name() != "spelling") {
                    qDebug() << "Intervals may contain only spellings. "
                             << "Found <" << stream.name() << ">";
                    // Keep looking for spellings
                    stream.skipCurrentElement();
                } else {
                    // Parse this spelling
                    parseSpelling(stream.readElementText(),
                                  intervals, spellings);
                }
                break;
            
            default:
                // Illegal state (can't happen: it's an enumerated type!)
                qDebug() << "Something nasty happened reading tunings. "
                            "Illegal state at line " << stream.lineNumber();
                stream.skipCurrentElement();
                break;
            } // end switch(state)
        } // end while(!stream.atEnd())
#       if (TUNING_DEBUG > 1)
        qDebug() << "Closing tunings file";
#       endif
        infile.close();
    } // end if (m_infile.open(...
    
    return &m_tunings;
}

void Tuning::parseSpelling(QString note,
                           IntervalList *intervals,
                           SpellingList *spellings)
{
    QString acc = note;
    acc.remove(0, 1);
    note.remove(1, note.length()-1); 
#   if (TUNING_DEBUG > 1)
    qDebug() << "Accidental: " << acc << "\tPitch Class: " << note;
#   endif
    if (acc.toInt() != 0) {
        const int acc_i = atoi(acc.toStdString().c_str());
        note.append(accMap[acc_i]->c_str());
    }
    //insert into spelling list
    spellings->insert(Spelling(note.toStdString().c_str(), intervals->size()-1));
#   if (TUNING_DEBUG > 1)
    qDebug() << "Translated variation:" << note << "\n";
#   endif                               
}

double Tuning::scalaIntervalToCents(const QString & interval,
                                    const qint64 lineNumber)
{
    double cents = -1.0;
    bool ok;
    QString intervalString(interval.trimmed());
    int dotPos = intervalString.indexOf(QChar('.'));                
    if (dotPos == -1) { // interval is a ratio          
#       if (TUNING_DEBUG > 1)
        qDebug() << "Interval is a ratio";
#       endif
        int slashPos = intervalString.indexOf(QChar('/'));
        double ratio = 1.0;
        if (slashPos == -1) { // interval is integer ratio
#           if (TUNING_DEBUG > 1)
            qDebug() << "Ratio is an integer";
#           endif
            ratio = intervalString.toInt(&ok);
            if (!ok) {
                std::cerr << "Syntax Error in tunings file, line "
                << lineNumber << std::endl;
                return -1.0;
            } else {
                //convert ratio to cents          
                QString numeratorString = intervalString;
                numeratorString.remove(slashPos,
                                       numeratorString.length() - slashPos);
#               if (TUNING_DEBUG > 1)
                qDebug() << "numerator:" << numeratorString;
#               endif
                int numerator = numeratorString.toInt( &ok );
                if (!ok) {
                    std::cerr << "Syntax Error in tunings file, "
                    "line " << lineNumber << std::endl;
                    return -1.0;
                }
                QString denominatorString = intervalString;
                denominatorString.remove( 0, slashPos+1 );
#               if (TUNING_DEBUG > 1)
                qDebug() << "denominator:" << denominatorString;
#               endif
                int denominator = denominatorString.toInt( &ok );
                if (!ok) {
                    std::cerr << "Syntax Error in tunings file, "
                                 "line " << lineNumber
                              << std::endl;
                              return -1.0;
                }      
                                       
#               if (TUNING_DEBUG > 1)
                qDebug() << "Ratio:" << numeratorString
                         << "/" << denominatorString;
#                           endif
                                       
                ratio = (double)numerator / (double)denominator;
                //calculate cents       
                cents = 1200.0 * log(ratio)/log(2.0);
#               if (TUNING_DEBUG > 1)
                qDebug() << "cents" << cents;
#               endif
            }
        }
    } else { // interval is in cents
#       if (TUNING_DEBUG > 1)
        qDebug() << "Interval is in cents";
#       endif
        cents = intervalString.toDouble(&ok);
        if (!ok) {
            std::cerr << "Syntax Error in tunings file, line "
                      << lineNumber << std::endl;
            return -1.0;
        }
#       if (TUNING_DEBUG > 1)
        qDebug() << "Cents: " << cents;
#       endif
    }
    
    return cents;
}

void Tuning::saveTuning(const QString &tuningName,
                        const IntervalList *intervals,
                        SpellingList *spellings)
{
#   if (TUNING_DEBUG > 1)
    qDebug() << "End of tuning" << tuningName;
#   endif
    std::string name = tuningName.toStdString().c_str();
    Tuning *newTuning = new Tuning(name, intervals, spellings);
    m_tunings.push_back(newTuning);
#   if (TUNING_DEBUG)
    newTuning->printTuning();
#   endif
}


Tuning::Tuning(const std::string name, 
               const IntervalList *intervals, 
               SpellingList *spellings) :
    m_name(name),
    m_rootPitch(9, 3),
    m_refPitch(9, 3),
    m_intervals(intervals),
    m_spellings(spellings)
{
#                   if (TUNING_DEBUG > 1)
                    qDebug() << "Given name:" << name.c_str() << &name;
                    qDebug() << "Stored name:" << m_name.c_str() << &m_name;
#                   endif

                    m_size = intervals->size();
                    
                    //check interval & spelling list sizes match
                    for (SpellingListIterator it = spellings->begin();
                         it != spellings->end();
                         ++it) {
                        if (it->second > m_size) {
                            qDebug() << "Spelling list does not match "
                                        "number of intervals!";
                            spellings->erase(it);
                        }
                    }
                    
                    Rosegarden::Pitch p(9, 3);
                    
                    //default A3 = 440;
                    setRootPitch(p);
                    setRefNote(p, 440);
}
                
Tuning::Tuning(const Tuning *tuning) :
    m_name(tuning->getName()),
    m_rootPitch(tuning->getRootPitch()),
    m_refPitch(tuning->getRefPitch()),
    m_size(m_intervals->size()),
    m_intervals(tuning->getIntervalList()),
    m_spellings(tuning->getSpellingList())
{
#   if (TUNING_DEBUG > 1)
    qDebug() << "Given name:" << tuning->getName().c_str();
    qDebug() << "Stored name: " << m_name.c_str() << &m_name;
#   endif
    
    //default A3 = 440;
    Rosegarden::Pitch p=tuning->getRefPitch();
    Rosegarden::Pitch p2=tuning->getRootPitch();
    
    setRootPitch(tuning->getRootPitch());
    setRefNote(p, tuning->getRefFreq());
    
    Rosegarden::Key keyofc; // use key of C to obtain unbiased accidental
    
    std::cout << "Ref note " << p.getNoteName(keyofc)
    << p.getDisplayAccidental( keyofc )
    << " " << m_refFreq << std::endl;
    
    std::cout << "Ref note " << m_refPitch.getNoteName(keyofc)
    << m_refPitch.getDisplayAccidental( keyofc )
    << " " << m_refFreq << std::endl;
    
    std::cout << "Ref freq for C " << m_cRefFreq << std::endl;
    
    std::cout << "Root note " <<  p2.getNoteName(keyofc)
    << p2.getDisplayAccidental(keyofc)
    << std::endl;
    std::cout << "Root note " <<  m_rootPitch.getNoteName(keyofc)
    << m_rootPitch.getDisplayAccidental(keyofc)
    << std::endl;
}




void Tuning::setRootPitch(Rosegarden::Pitch pitch){
    
    m_rootPitch = pitch;
    
    std::string spelling = getSpelling( pitch );;
    const SpellingListIterator sit = m_spellings->find(spelling);
    if (sit == m_spellings->end()){
        std::cerr << "\nFatal: Tuning::setRootPitch root pitch "
                     "not found in tuning!!" << std::endl;
        return;
    }
#   if (TUNING_DEBUG > 1)    
    qDebug() << "Root position" << m_rootPosition;
#   endif
    m_rootPosition = sit->second;
}


std::string Tuning::getSpelling(Rosegarden::Pitch &pitch) const {
    
    
    const Rosegarden::Key key;
    
    QChar qc(pitch.getNoteName(key));
    QString spelling(qc);
    
    Rosegarden::Accidental acc = pitch.getDisplayAccidental(key);
    if (acc != Rosegarden::Accidentals::NoAccidental && 
        acc != Rosegarden::Accidentals::Natural) {
        spelling.append(acc.c_str());
    }
    
    return spelling.toStdString().c_str();
}


void Tuning::setRefNote(Rosegarden::Pitch pitch, double freq) {
    
    m_refPitch = pitch;
    m_refFreq = freq;
    m_refOctave = pitch.getOctave();
    std::string spelling = getSpelling(pitch);
    
    // position in chromatic scale
    SpellingListIterator it = m_spellings->find(spelling);
    if (it == m_spellings->end()) {
        std::cerr << "Tuning::setRefNote Spelling " << spelling 
                  << " not found in " << m_name
                  << " tuning!" << std::endl;
        return;
    }
    int refPosition = it->second;
    
    // calculate frequency for C in reference octave 
    // this makes calculation of frequencies easier
    
    it = m_spellings->find("C");
    if (it == m_spellings->end()){
        std::cerr << "Tuning::setRefNote 'C' not found in "
                  << m_name << " tuning!\n";
        return;
    }
    
    m_cPosition = it->second;
    
    // find position of C relative to root note 
    int cInterval = m_cPosition - m_rootPosition;
    if (cInterval < 0) cInterval += m_size;
    
    // cents above root note for C
    double cents = (*m_intervals)[cInterval];
    
    // cents above root note for reference note
    int refInterval = refPosition - m_rootPosition;
    if( refInterval < 0 ) refInterval += m_size;
    
    double refCents = (*m_intervals)[refInterval];
    
    // relative cents from reference note to target note
    double relativeCents = cents - refCents;
    if (relativeCents > 0) relativeCents -= 1200;
    
    //frequency ratio between reference and target notes
    double ratio = pow( 2, relativeCents/1200 );
    
    m_cRefFreq = m_refFreq * ratio;
    
#   if (TUNING_DEBUG)
    qDebug() << "c Position" << m_cPosition
             << "\nc interval" << cInterval
             << "\nc Cents" << cents
             << "\nref position" << refPosition
             << "\nref interval" << refInterval
             << "\nref Cents" << refCents
             << "\nc freq" << m_cRefFreq
             << "\nref octave" << m_refOctave;
#   endif
}

/**
* Returns the frequency of the given pitch in the current tuning.
*/
double Tuning::getFrequency(Rosegarden::Pitch p) const {
    
    // make note spelling
    std::string spelling = getSpelling(p);
    
    int octave = p.getOctave();
    
    // position in chromatic scale
    const SpellingListIterator it = m_spellings->find(spelling);
    if (it == m_spellings->end()) {
        std::cerr << "Tuning::getFreq  Spelling '" << spelling 
                  << "' not found in " << m_name << " tuning!\n";
        return 0;
    }
    int position = it->second;
    
    // find position relative to root note 
    int relativePosition = position - m_rootPosition;
    if (relativePosition < 0) relativePosition += m_size;
    
    // cents above root note for target note
    double cents = (*m_intervals)[relativePosition];
    
    // cents above root note for reference note ( C )
    int refInterval = m_cPosition - m_rootPosition;
    if (refInterval < 0) refInterval += m_size;
    double refCents = (*m_intervals)[refInterval];
    
    // relative cents from reference note to target note
    double relativeCents = cents - refCents;
    if (relativeCents < 0) relativeCents += 1200;
    
    //frequency ratio between reference and target notes
    double ratio = pow(2, relativeCents/1200);
    
    /*
    B#  occurs in the same octave as the C immediatley above them.
    In 12ET this is true, but not in all tunings. 
    Solution: When B# and C are not equivalent spellings,
    decrement the octave of every B#.
    */
    if (spelling == "Bsharp" && position != m_cPosition) {
        octave--;
    }
    
    const int octaveDifference = octave - m_refOctave;
    
    const double octaveRatio = pow( 2, octaveDifference );
    
    ratio *= octaveRatio;
    
    const double freq = m_cRefFreq * ratio;
    
#   if (TUNING_DEBUG)
    qDebug() << "Spelling " << spelling.c_str() 
             << "\ntarget position " << position
             << "\nroot position " << m_rootPosition
             << "\ntarget interval " << relativePosition
             << "\ncents above root note " << cents
             << "\ninterval for C " << refInterval
             << "\ncents from reference note " << refCents
             << "\ncents from reference to target" << relativeCents
             << "\nrefOctave " << m_refOctave
             << "\noctave " << octave
             << "\noctave ratio " << octaveRatio
             << "\nratio " << ratio
             << "\nref freq " << m_refFreq
             << "\nfreq " << freq;
#   endif
    
    return freq;
}

/**
* Prints to std out for debugging
*/
void Tuning::printTuning() const {
    
    std::cout << "Tuning::printTuning()\n";
    std::cout << "Name: '" << m_name << "'\n";
    
    Rosegarden::Key keyofc; // use key of C to obtain unbiased accidental
    
    std::cout << "Ref note " << m_refPitch.getNoteName(keyofc)
              << m_refPitch.getDisplayAccidental( keyofc )
              << " " << m_refFreq << std::endl;
    
    std::cout << "Ref freq for C " << m_cRefFreq << std::endl;
    
    std::cout << "Root note " <<  m_rootPitch.getNoteName(keyofc)
              << m_rootPitch.getDisplayAccidental( keyofc )
              << std::endl;
    
    for (SpellingListIterator sit = m_spellings->begin();
    sit != m_spellings->end();
    ++sit) {
        
        std::cout << "Spelling '" << sit->first
                  << "'\tinterval " << sit->second
                  << std::endl;
        
    }
    
    for(unsigned int i=0; i < m_intervals->size(); i++) { 
        std::cout << "Interval '" << i
                  << "'\tinterval " << m_intervals->at(i)
                  << std::endl;
    }
    
    std::cout << "Done." << std::endl;
    
}


Rosegarden::Pitch Tuning::getRootPitch() const { return m_rootPitch; }
Rosegarden::Pitch Tuning::getRefPitch() const { return m_refPitch; }
double Tuning::getRefFreq() const{ return m_refFreq; }
const std::string Tuning::getName() const { return m_name; }
SpellingList *Tuning::getSpellingList() const{ return m_spellings; }
const IntervalList *Tuning::getIntervalList() const{ return m_intervals; }

