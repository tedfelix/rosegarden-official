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
#include "gui/application/TransportStatus.h"
#include "gui/seqmanager/SequenceManager.h"

#include <QSettings>

#include <algorithm>  // For std::sort().

namespace Rosegarden
{


MetronomeMapper::MetronomeMapper(RosegardenDocument *doc) :
    MappedEventBuffer(doc),
    m_metronome(nullptr),
    m_channelManager(nullptr), // We will set this below after we find instrument.
    m_metronomeDuring(GeneralConfigurationPage::DuringBoth)
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
        RG_WARNING << "WARNING: ctor: No metronome for device " << metronomeDeviceId;
        m_metronome = new MidiMetronome(SystemInstrumentBase);
    }

    m_instrument = studio.getInstrumentById(m_metronome->getInstrument());
    // As we promised, set instrument
    m_channelManager.setInstrument(m_instrument);
    m_channelManager.setEternalInterval();

    Composition &composition = m_doc->getComposition();

    int depth = m_metronome->getDepth();

    // If the metronome has bars at the very least, generate the metronome
    // ticks.
    if (depth > 0) {

        // Start at a somewhat arbitrary time (-20) prior to the beginning
        // of the composition.
        timeT barStart = composition.getBarStart(-20);
        m_start = composition.getElapsedRealTime(barStart);

        // For each bar
        for (timeT barTime = barStart;
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
    //RG_DEBUG << "dtor: " << this;

    delete m_metronome;
    m_metronome = nullptr;
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
            MidiByte velocity = 0;
            MidiByte pitch = 0;

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

    m_channelManager.allocateChannelInterval(false);
    m_channelManager.setDirty();
}

int
MetronomeMapper::calculateSize()
{
    return static_cast<int>(m_ticks.size());
}

void
MetronomeMapper::doInsert(MappedInserterBase &inserter, MappedEvent &evt,
                          RealTime start, bool firstOutput)
{
    if (!m_instrument)
        return;

    m_channelManager.insertEvent(
            NO_TRACK,  // trackId
            m_instrument->getStaticControllers(),
            start,
            evt,
            firstOutput,
            inserter);
}

void
MetronomeMapper::
makeReady(MappedInserterBase &inserter, RealTime time)
{
    if (!m_instrument)
        return;

    // No sense sending out a channel setup if the metronome is muted.
    if (ControlBlock::getInstance()->isMetronomeMuted())
        return;

    m_channelManager.makeReady(
            NO_TRACK,  // trackId
            time,
            m_instrument->getStaticControllers(),
            inserter);

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    m_metronomeDuring =
            static_cast<GeneralConfigurationPage::MetronomeDuring>(
                    settings.value(
                            "enableMetronomeDuring",
                            GeneralConfigurationPage::DuringBoth).toUInt());
    settings.endGroup();
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

    TransportStatus transportStatus =
            m_doc->getSequenceManager()->getTransportStatus();

    if (transportStatus == RECORDING  ||
        transportStatus == STARTING_TO_RECORD) {
        // If we're in the count-in
        if (m_doc->getSequenceManager()->inCountIn(
                evt->getEventTime() + evt->getDuration())) {
            // If we're only supposed to play the metronome during record,
            // indicate that we shouldn't be playing the metronome.
            if (m_metronomeDuring == GeneralConfigurationPage::DuringRecord)
                return false;
        } else {  // We're recording
            // If we're only supposed to play the metronome during count-in,
            // indicate that we shouldn't be playing the metronome.
            if (m_metronomeDuring == GeneralConfigurationPage::DuringCountIn)
                return false;
        }
    }

    // It's a metronome event.  Play it if the metronome isn't muted.

    return !ControlBlock::getInstance()->isMetronomeMuted();
}


}
