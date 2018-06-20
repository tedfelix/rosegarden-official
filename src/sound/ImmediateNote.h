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

#ifndef RG_IMMEDIATENOTE_H
#define RG_IMMEDIATENOTE_H

#include "gui/seqmanager/ChannelManager.h"
#include "base/Instrument.h"
#include "sound/MappedEventList.h"
#include "base/RealTime.h"

namespace Rosegarden
{

class MappedEventList;

class ImmediateNote
{
public:
    ImmediateNote() :
        // There's no sensible value to set for instrument.  We'll get an
        // Instrument in each call.
        m_channelManager(0)
    {
        m_channelManager.setEternalInterval();
    }

    /// Fill mappedEventList with a note and its setup events.
    void fillWithNote(MappedEventList &mappedEventList, Instrument *instrument,
                      int pitch, int velocity,
                      RealTime duration, bool oneshot);

private:
    ChannelManager m_channelManager;
};


}

#endif
