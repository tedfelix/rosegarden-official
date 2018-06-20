/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Fingering.h"

#include "misc/Debug.h"

#include <QStringList>
#include <sstream>
#include <algorithm>

namespace Rosegarden
{

namespace Guitar
{
    
Fingering::Fingering(unsigned int nbStrings) :
    m_strings(nbStrings, MUTED)
{
}

Fingering::Fingering(QString s)
{
    QString errString;
    Fingering t = parseFingering(s, errString);
    m_strings = t.m_strings;
}

unsigned int
Fingering::getStartFret() const
{
    int min = 999, max = 0;
    for(std::vector<int>::const_iterator i = m_strings.begin(); i != m_strings.end(); ++i) {
        if (*i < min && *i > 0)
            min = *i;
        if (*i > max)
            max = *i;
    }
    
    if (max < 4)
        min = 1;
    
    return min == 999 ? 1 : min;
}

bool
Fingering::hasBarre() const
{
    int lastStringStatus = m_strings[getNbStrings() - 1];
    
    return ((m_strings[0] > OPEN && m_strings[0] == lastStringStatus) ||
            (m_strings[1] > OPEN && m_strings[1] == lastStringStatus) ||
            (m_strings[2] > OPEN && m_strings[2] == lastStringStatus));
}

Fingering::Barre
Fingering::getBarre() const
{
    // ??? This routine needs review and testing.  If guitar chords in
    //     lilypond look strange, this is likely the reason.

    // Fret on the last string.  (6th string for standard guitar.)
    int lastStringStatus = m_strings[getNbStrings() - 1];

    Barre res;
    
    res.fret = lastStringStatus;

    // For each string from first (0) to third (2).
    for(unsigned int i = 0; i < 3; ++i) {
        // If this string is not open (0) and it's at the same fret
        // as the last (6th) string...
        // ??? We can drop the check for OPEN if we check for
        //     lastStringStatus == OPEN and bail before we get in here.
        if (m_strings[i] > OPEN && m_strings[i] == lastStringStatus) {
            res.start = i;

            // ??? Is this the way this is supposed to be?
            //break;
        }

        // ??? Based on indentation in previous versions, it seems like this
        //     belongs in the if above.  Reformatting this to fix a compiler
        //     warning, but don't have time to figure out a regression test.
        //     As it is, this seems wrong.  It will only check the first
        //     string.  If it's not at the same fret as the 6th string,
        //     it will indicate that the barre goes from string 0 (or
        //     perhaps garbage) to string 6 at string 6's fret.
        break;
    }

    res.end = 5;
    
    return res;        
}

Fingering
Fingering::parseFingering(const QString& ch, QString& errorString)
{
    QStringList tokens = ch.split(' ', QString::SkipEmptyParts);

    unsigned int idx = 0;
    Fingering fingering;
    
    for(QStringList::iterator i = tokens.begin(); i != tokens.end() && idx < fingering.getNbStrings(); ++i, ++idx) {
        QString t = *i;
        bool b = false;
        unsigned int fn = t.toUInt(&b);
        if (b) {
//            NOTATION_DEBUG << "Fingering::parseFingering : '" << t << "' = " << fn;  
            fingering[idx] = fn;
        } else if (t.toLower() == "x") {
//            NOTATION_DEBUG << "Fingering::parseFingering : '" << t << "' = MUTED\n";  
            fingering[idx] = MUTED;
        } else {
            errorString = tr("couldn't parse fingering '%1' in '%2'").arg(t).arg(ch);            
        }
    }

    return fingering;
}


std::string Fingering::toString() const
{
    std::stringstream s;
    
    for(std::vector<int>::const_iterator i = m_strings.begin(); i != m_strings.end(); ++i) {
        if (*i >= 0)
            s << *i << ' ';
        else
            s << "x ";
    }

    return s.str();
}

bool operator<(const Fingering& a, const Fingering& b)
{
    for(unsigned int i = 0; i < Fingering::DEFAULT_NB_STRINGS; ++i) {
        if (a.getStringStatus(i) != b.getStringStatus(i)) {
            return a.getStringStatus(i) < b.getStringStatus(i);
        }
    }
    return false;
}    

}

}
