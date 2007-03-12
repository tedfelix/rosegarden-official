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

#include "Fingering.h"
#include <qstringlist.h>
#include <sstream>
#include <algorithm>

namespace Rosegarden
{

namespace Guitar
{
    
Fingering::Fingering(unsigned int nbStrings) :
    m_strings(nbStrings)
{
}

unsigned int
Fingering::getStartFret() const
{
    int min = 999;
    for(std::vector<int>::const_iterator i = m_strings.begin(); i != m_strings.end(); ++i) {
        if (*i < min && *i > 0)
            min = *i;
    }
    
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
    int lastStringStatus = m_strings[getNbStrings() - 1];

    Barre res;
    
    res.fret = lastStringStatus;
    
    for(unsigned int i = 0; i < 3; ++i) {
        if (m_strings[i] > OPEN && m_strings[i] == lastStringStatus)
            res.start = i;
            break;
    }

    res.end = 5;
    
    return res;        
}

Fingering
Fingering::parseFingering(const QString& ch, QString& errorString)
{
    QStringList tokens = QStringList::split(' ', ch);

    unsigned int idx = 0;
    Fingering fingering;
    
    for(QStringList::iterator i = tokens.begin(); i != tokens.end() && idx < fingering.getNbStrings(); ++i, ++idx) {
        QString t = *i;
        bool b;
        unsigned int fn = t.toUInt(&b);
        if (b)
            fingering[idx] = fn;
        else if (t.lower() == 'x')
            fingering[idx] = MUTED;
        else {
            errorString = i18n("couldn't parse fingering '%1' in '%2'").arg(t).arg(ch);            
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
