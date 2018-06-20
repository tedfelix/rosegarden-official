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
    m_instrument = studio.getInstrumentById(m_metronome->getInstrument());
    m_channelManager.setInstrument(m_instrument);
    m_channelManager.setEternalInterval();

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
        //     Beware that it will need to increase the buffer's capacity.
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
    //RG_DEBUG << "fillBuffer(): instrument is " << m_metronome->getInstrument();

    Q_ASSERT_X(capacity() >= static_cast<int>(m_ticks.size()),
               "MetronomeMapper::fillBuffer()",
               "Buffer capacity is too small.");

    Composition &composition = m_doc->getComposition();

    const RealTime tickDuration(0, 100000000);

    int index = 0;

    // For each tick
    for (TickContainer::iterator tick = m_ticks.begin();
         tick != m_ticks.end();
         ++tick) {

        //RG_DEBUG << "fillBuffer(): velocity = " << int(velocity);

        RealTime eventTime = composition.getElapsedRealTime(tick->first);

        MappedEvent e;

        if (tick->second == MidiTimingClockTick) {
            e = MappedEvent(0,  // Instrument ID is irrelevant
                            MappedEvent::MidiSystemMessage);
            e.setData1(MIDI_TIMING_CLOCK);
            e.setEventTime(eventTime);
        } else {
            MidiByte velocity;
            MidiByte pitch;

            switch (tick->second) {
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
                            RealTime::zeroTime);  // audioStartMarker
        }

        // Add the event to the buffer.
        getBuffer()[index] = e;

        ++index;
    }

    //RG_DEBUG << "fillBuffer(): capacity: " << capacity();
    //RG_DEBUG << "  Total events written: " << index;

    resize(index);

    m_channelManager.reallocate(false);
    m_channelManager.setDirty();
}

int
MetronomeMapper::calculateSize()
{
    return static_cast<int>(m_ticks.size());
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
    m_channelManager.doInsert(inserter, evt, start,
                              m_instrument->getStaticControllers(),
                              firstOutput, NO_TRACK);
}

void
MetronomeMapper::
makeReady(MappedInserterBase &inserter, RealTime time)
{
    m_channelManager.makeReady(inserter, time,
                               m_instrument->getStaticControllers(), NO_TRACK);
}

bool
MetronomeMapper::
shouldPlay(MappedEvent *evt, RealTime sliceStart)
{
    // If it's finished, don't play it.
    if (evt->EndedBefore(sliceStart))
        return false;

    // If it's a MIDI Timing Clock, always play it.
    if (evt->getType() == MappedEvent::MidiSystemMessage  &&
        evt->getData1() == MIDI_TIMING_CLOCK) {
        return true;
    }

    // It's a metronome event.  Play it if the metronome isn't muted.

    return !ControlBlock::getInstance()->isMetronomeMuted();
}


}
