
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

#ifndef _RG_METRONOMEMMAPPER_H_
#define _RG_METRONOMEMMAPPER_H_

#include "base/MidiProgram.h"
#include "base/RealTime.h"
#include "SegmentMmapper.h"
#include <qstring.h>
#include <utility>
#include <vector>
#include "base/Event.h"




namespace Rosegarden
{

class RosegardenGUIDoc;
class MidiMetronome;


class MetronomeMmapper : public SegmentMmapper
{
    friend class SegmentMmapperFactory;

public:

    virtual ~MetronomeMmapper();

    InstrumentId getMetronomeInstrument();

    // overrides from SegmentMmapper
    virtual unsigned int getSegmentRepeatCount();

protected:
    MetronomeMmapper(RosegardenGUIDoc* doc);

    virtual size_t computeMmappedSize();

    void sortTicks();
    QString createFileName();

    // override from SegmentMmapper
    virtual void dump();

    //--------------- Data members ---------------------------------
    typedef std::pair<timeT, int> Tick;
    typedef std::vector<Tick> TickContainer;
    friend bool operator<(Tick, Tick);

    TickContainer m_ticks;
    bool m_deleteMetronome;
    const MidiMetronome* m_metronome;
    RealTime m_tickDuration;
};

//----------------------------------------


}

#endif
