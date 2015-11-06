/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MetronomeMapper]"

#include "MetronomeMapper.h"

#include "misc/Debug.h"
#include "misc/ConfigGroups.h"
#include "base/MidiProgram.h"  // For InstrumentId
#include "base/RealTime.h"
#include "base/Studio.h"
#include "base/TimeT.h"
#include "document/RosegardenDocument.h"
#include "gui/seqmanager/MappedEventBuffer.h"
#include "sound/ControlBlock.h"
#include "sound/MappedEvent.h"
#include "sound/Midi.h"

#include <QSettings>

#include <algorithm>  // For std::sort().

namespace Rosegarden
{


MetronomeMapper::MetronomeMapper(RosegardenDocument *doc) :
    MappedEventBuffer(doc),
    m_metronome(0),  // no metronome to begin with
    m_tickDuration(0, 100000000),
    m_channelManager(0) // We will set this below after we find instrument.
{
    RG_DEBUG << "ctor: " << this;

    // get metronome device
    Studio &studio = m_doc->getStudio();
    int device = studio.getMetronomeDevice();

    const MidiMetronome *metronome =
        m_doc->getStudio().getMetronomeFromDevice(device);

    if (metronome) {
        RG_DEBUG << "ctor: have metronome, it's on instrument " << metronome->getInstrument();
        m_metronome = new MidiMetronome(*metronome);
    } else {
        m_metronome = new MidiMetronome(SystemInstrumentBase);
        RG_DEBUG << "ctor: no metronome for device " << device;
    }
    {
        // As we promised, set instrument
        InstrumentId id = m_metronome->getInstrument();
        m_channelManager.setInstrument(doc->getStudio().getInstrumentById(id));
    }
        
    Composition& c = m_doc->getComposition();
    timeT t = c.getBarStart( -20); // somewhat arbitrary
    int depth = m_metronome->getDepth();

    if (depth > 0) {
        while (t < c.getEndMarker()) {

            TimeSignature sig = c.getTimeSignatureAt(t);
            timeT barDuration = sig.getBarDuration();
            std::vector<int> divisions;
            if (depth > 0) sig.getDivisions(depth - 1, divisions);
            int ticks = 1;

            for (int i = -1; i < (int)divisions.size(); ++i) {
                if (i >= 0) ticks *= divisions[i];

                for (int tick = 0; tick < ticks; ++tick) {
                    if (i >= 0 && (tick % divisions[i] == 0)) continue;
                    timeT tickTime = t + (tick * barDuration) / ticks;
                    m_ticks.push_back(Tick(tickTime, i + 1));
                }
            }

            t = c.getBarEndForTime(t);
        }
    }

    QSettings settings;
    settings.beginGroup(SequencerOptionsConfigGroup);

    int midiClock = settings.value("midiclock", 0).toInt() ;
    int mtcMode = settings.value("mtcmode", 0).toInt() ;

    if (midiClock == 1) {
        timeT quarterNote = Note(Note::Crotchet).getDuration();

        // Insert 24 clocks per quarter note
        //
        for (timeT insertTime = c.getStartMarker();
             insertTime < c.getEndMarker();
             insertTime += quarterNote / 24) {
            m_ticks.push_back(Tick(insertTime, 3));
        }
    }

    if (mtcMode > 0) {
        // do something
    }

    std::sort(m_ticks.begin(), m_ticks.end());

    if (m_ticks.empty()) {
        RG_DEBUG << "ctor: WARNING no ticks generated";
    }

    settings.endGroup();
}

MetronomeMapper::~MetronomeMapper()
{
    RG_DEBUG << "dtor: " << this;
    delete m_metronome;
}

InstrumentId MetronomeMapper::getMetronomeInstrument()
{
    return m_metronome->getInstrument();
}

void MetronomeMapper::fillBuffer()
{
    RealTime eventTime;
    Composition& comp = m_doc->getComposition();

    RG_DEBUG << "fillBuffer(): instrument is " << m_metronome->getInstrument();

    int index = 0;

    for (TickContainer::iterator i = m_ticks.begin(); i != m_ticks.end(); ++i) {

        //RG_DEBUG << "fillBuffer(): velocity = " << int(velocity);

        eventTime = comp.getElapsedRealTime(i->first);

        MappedEvent e;

        if (i->second == 3) { // MIDI Clock
            e = MappedEvent(0, MappedEvent::MidiSystemMessage);
            e.setData1(MIDI_TIMING_CLOCK);
            e.setEventTime(eventTime);
        } else {
            MidiByte velocity;
            MidiByte pitch;
            switch (i->second) {
            case 0:
                velocity = m_metronome->getBarVelocity();
                pitch = m_metronome->getBarPitch();
                break;
            case 1:
                velocity = m_metronome->getBeatVelocity();
                pitch = m_metronome->getBeatPitch();
                break;
            default:
                velocity = m_metronome->getSubBeatVelocity();
                pitch = m_metronome->getSubBeatPitch();
                break;
            }

            e = MappedEvent(m_metronome->getInstrument(),
                            MappedEvent::MidiNoteOneShot,
                            pitch,
                            velocity,
                            eventTime,
                            m_tickDuration,
                            RealTime::zeroTime);
        }

        getBuffer()[index] = e;
        ++index;
    }

    resize(index);
    m_channelManager.reallocateEternalChannel();
    m_channelManager.setDirty();

    RG_DEBUG << "fillBuffer(): - Total events written = " << index;
}

int
MetronomeMapper::calculateSize()
{
    QSettings settings;
    settings.beginGroup(Rosegarden::SequencerOptionsConfigGroup);

    int midiClock = settings.value("midiclock", 0).toInt() ;
    int mtcMode = settings.value("mtcmode", 0).toInt() ;

    // base size for Metronome ticks
    size_t size = m_ticks.size();
    Composition& comp = m_doc->getComposition();

    if (midiClock == 1) {

        using Rosegarden::Note;

        // Allow room for MIDI clocks
        int clocks = ( 24 * ( comp.getEndMarker() - comp.getStartMarker() ) ) / 
            Note(Note::Crotchet).getDuration();

        size += clocks;
    }

    if (mtcMode > 0) {
        // Allow room for MTC timing messages (how?)
    }

    settings.endGroup();

    return int(size);
}

int
MetronomeMapper::getSegmentRepeatCount()
{
    return 1;
}

void
MetronomeMapper::doInsert(MappedInserterBase &inserter, MappedEvent &evt,
                         RealTime start, bool firstOutput)
{
    ChannelManager::SimpleCallbacks callbacks;
    m_channelManager.doInsert(inserter, evt, start, &callbacks,
                              firstOutput, NO_TRACK);
}

void
MetronomeMapper::
makeReady(MappedInserterBase &inserter, RealTime time)
{
    ChannelManager::SimpleCallbacks callbacks;
    // m_channelManager.setInstrument(m_doc->getStudio().getInstrumentById(getMetronomeInstrument()));
    m_channelManager.makeReady(inserter, time, &callbacks,
                               NO_TRACK);
}

bool
MetronomeMapper::
mutedEtc(MappedEvent *evt)
{
    // Apparently some clock events need to escape muting?
    if (evt->getType() == MappedEvent::MidiSystemMessage &&
        evt->getData1() == MIDI_TIMING_CLOCK) {
        //RG_DEBUG << "mutedEtc() - found clock";
        return false;
    }

    return (ControlBlock::getInstance()->isMetronomeMuted());
}
    
bool
MetronomeMapper::
shouldPlay(MappedEvent *evt, RealTime sliceStart)
{
    if (mutedEtc(evt))
        return false;

    // Otherwise it should play if it's not already all done sounding.
    // The timeslice logic will have already excluded events that
    // start too late.
    return !evt->EndedBefore(sliceStart);
}
    
}
