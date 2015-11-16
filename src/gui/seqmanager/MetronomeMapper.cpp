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
    m_metronome(0),
    m_channelManager(0) // We will set this below after we find instrument.
{
    //RG_DEBUG << "ctor: " << this;

    Studio &studio = m_doc->getStudio();

    const DeviceId metronomeDeviceId = studio.getMetronomeDevice();

    const MidiMetronome *metronome =
            studio.getMetronomeFromDevice(metronomeDeviceId);

    if (metronome) {
        //RG_DEBUG << "ctor: have metronome, it's on instrument " << metronome->getInstrument();
        // Make a local copy.
        m_metronome = new MidiMetronome(*metronome);
    } else {
        RG_WARNING << "ctor: no metronome for device " << metronomeDeviceId;
        m_metronome = new MidiMetronome(SystemInstrumentBase);
    }

    // As we promised, set instrument
    m_channelManager.setInstrument(
            studio.getInstrumentById(m_metronome->getInstrument()));

    Composition &composition = m_doc->getComposition();

    int depth = m_metronome->getDepth();

    // If the metronome has bars at the very least, generate the metronome
    // ticks.
    if (depth > 0) {
        // For each bar, starting at a somewhat arbitrary time prior to the
        // beginning of the composition.
        // ??? To avoid dropping a tick when expanding the Composition, we
        //     could simply generate one bar too many of these.  Though that
        //     might cause duplicate events.  Might be better to perform the
        //     Composition expansion one bar prior to hitting the end.
        //     And add a "moreTicks(newEndTime)" function to this class.
        for (timeT barTime = composition.getBarStart(-20);
             barTime < composition.getEndMarker();
             barTime = composition.getBarEndForTime(barTime)) {

            // Add the bar tick
            m_ticks.push_back(Tick(barTime, BarTick));

            // If all they want is bars, move to the next bar.
            if (depth == 1)
                continue;

            // Handle beats and subbeats.

            TimeSignature timeSig = composition.getTimeSignatureAt(barTime);
            timeT barDuration = timeSig.getBarDuration();

            // Get the beat and subbeat divisions.
            std::vector<int> divisions;
            timeSig.getDivisions(depth - 1, divisions);

            int ticks = 1;

            // For each tick type (beat then subbeat)
            for (int i = 0; i < (int)divisions.size(); ++i) {
                ticks *= divisions[i];

                // For each tick
                for (int tick = 0; tick < ticks; ++tick) {
                    // Drop the first tick.
                    if (tick % divisions[i] == 0)
                        continue;

                    timeT tickTime = barTime + (tick * barDuration) / ticks;
                    m_ticks.push_back(Tick(tickTime, static_cast<TickType>(i + 1)));
                }
            }
        }
    }

    QSettings settings;
    settings.beginGroup(SequencerOptionsConfigGroup);
    int midiClock = settings.value("midiclock", 0).toInt();
    //int mtcMode = settings.value("mtcmode", 0).toInt() ;
    settings.endGroup();

    // Send
    if (midiClock == 1) {
        // 24 MIDI timing clocks per quarter note
        timeT midiClockTime = Note(Note::Crotchet).getDuration() / 24;

        // For each MIDI timing clock
        for (timeT t = composition.getStartMarker();
             t < composition.getEndMarker();
             t += midiClockTime) {
            m_ticks.push_back(Tick(t, MidiTimingClockTick));
        }
    }

    //if (mtcMode > 0) {
    //    // do something
    //}

    std::sort(m_ticks.begin(), m_ticks.end());

    if (m_ticks.empty()) {
        RG_WARNING << "ctor: WARNING no ticks generated";
    }

    // This eventually calls fillBuffer() which will convert (map) the
    // ticks in m_ticks to events in m_buffer.
    init();
}

MetronomeMapper::~MetronomeMapper()
{
    RG_DEBUG << "dtor: " << this;

    delete m_metronome;
    m_metronome = NULL;
}

InstrumentId MetronomeMapper::getMetronomeInstrument() const
{
    return m_metronome->getInstrument();
}

void MetronomeMapper::fillBuffer()
{
    RealTime eventTime;
    Composition& comp = m_doc->getComposition();

    RG_DEBUG << "fillBuffer(): instrument is " << m_metronome->getInstrument();

    const RealTime tickDuration(0, 100000000);

    int index = 0;

    // For each tick
    for (TickContainer::iterator i = m_ticks.begin(); i != m_ticks.end(); ++i) {

        //RG_DEBUG << "fillBuffer(): velocity = " << int(velocity);

        eventTime = comp.getElapsedRealTime(i->first);

        MappedEvent e;

        if (i->second == MidiTimingClockTick) {
            e = MappedEvent(0, MappedEvent::MidiSystemMessage);
            e.setData1(MIDI_TIMING_CLOCK);
            e.setEventTime(eventTime);
        } else {
            MidiByte velocity;
            MidiByte pitch;
            switch (i->second) {
            case BarTick:
                velocity = m_metronome->getBarVelocity();
                pitch = m_metronome->getBarPitch();
                break;
            case BeatTick:
                velocity = m_metronome->getBeatVelocity();
                pitch = m_metronome->getBeatPitch();
                break;
            case SubBeatTick:
                velocity = m_metronome->getSubBeatVelocity();
                pitch = m_metronome->getSubBeatPitch();
                break;
            case MidiTimingClockTick:
            default:
                RG_WARNING << "fillBuffer(): Unexpected tick type";
            }

            e = MappedEvent(m_metronome->getInstrument(),
                            MappedEvent::MidiNoteOneShot,
                            pitch,
                            velocity,
                            eventTime,
                            tickDuration,
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
