
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Composition.h"
#include "Track.h"

namespace Rosegarden 
{

Composition::Composition(unsigned int nbTracks)
    : m_nbTicksPerBar(384)
{
//     cerr << "Composition:(" << nbTracks << ") : this = "
//          << this <<  " - size = "
//          << m_tracks.size() << endl;
}

Composition::~Composition()
{
    clear();
}


Composition::iterator
Composition::addTrack(Track *track)
{
    if (!track) return end();
    
    std::pair<iterator, bool> res = m_tracks.insert(track);

    return res.first;
}

void
Composition::deleteTrack(Composition::iterator i)
{
    if (i == end()) return;

    Track *p = (*i);
    delete p;
    m_tracks.erase(i);
}

bool
Composition::deleteTrack(Track *p)
{
    iterator i = find(begin(), end(), p);
    
    if (i != end()) {
        delete p;
        m_tracks.erase(i);
        return true;
    }

    return false;
}

unsigned int
Composition::getNbBars() const
{
    //!!! No, not really!

    unsigned int maxSize = 0,
        maxNbBars = 0;

    for (trackcontainer::const_iterator i = m_tracks.begin();
         i != m_tracks.end(); ++i) {

        if ((*i) && (*i)->size() > maxSize) {
            maxSize = (*i)->size();

            Track::const_iterator lastEl = (*i)->end();
            --lastEl;
            maxNbBars = ((*lastEl)->getAbsoluteTime() +
                         (*lastEl)->getDuration()) / getNbTicksPerBar();

//             cerr << "Composition::getNbBars() : last el. abs.Time : "
//                  << (*lastEl)->absoluteTime()
//                  << " - nbTicksPerBar : "
//                  << getNbTicksPerBar()
//                  << " - maxNbBars : " << maxNbBars << endl;
        }
    }
    
    return maxNbBars;
}

void
Composition::clear()
{
    for(trackcontainer::iterator i = m_tracks.begin();
        i != m_tracks.end(); ++i) {
        delete (*i);
    }
    m_tracks.erase(begin(), end());
    
}
 
}
