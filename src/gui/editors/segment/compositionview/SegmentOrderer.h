
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENTORDERER_H
#define RG_SEGMENTORDERER_H

#include "base/Composition.h"
#include <map>




namespace Rosegarden
{

class Segment;


class SegmentOrderer : public CompositionObserver {
public:
    SegmentOrderer() : m_currentMaxZ(0) {};
    
        unsigned int getZForSegment(const Segment*);

        void segmentClicked(const Segment *);
        
protected:

    //--------------- Data members ---------------------------------
        std::map<const Segment*, unsigned int> m_segmentZs;
        unsigned int m_currentMaxZ;
};


}

#endif
