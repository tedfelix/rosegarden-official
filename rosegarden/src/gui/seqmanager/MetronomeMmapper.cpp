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


#include "MetronomeMmapper.h"
#include "misc/Debug.h"
#include <kapplication.h>

#include "sound/Midi.h"
#include <kstddirs.h>
#include "document/ConfigGroups.h"
#include "base/Event.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/Studio.h"
#include "base/TriggerSegment.h"
#include "document/RosegardenGUIDoc.h"
#include "SegmentMmapper.h"
#include "sound/MappedEvent.h"
#include <kconfig.h>
#include <kglobal.h>
#include <qstring.h>


namespace Rosegarden
{

MetronomeMmapper::MetronomeMmapper(RosegardenGUIDoc* doc)
        : SegmentMmapper(doc, 0, createFileName()),
        m_metronome(0),  // no metronome to begin with
        m_tickDuration(0, 100000000)
{
    SEQMAN_DEBUG << "MetronomeMmapper ctor : " << this << endl;

    // get metronome device
    Studio &studio = m_doc->getStudio();
    int device = studio.getMetronomeDevice();

    const MidiMetronome *metronome =
        m_doc->getStudio().getMetronomeFromDevice(device);

    if (metronome) {

        SEQMAN_DEBUG << "MetronomeMmapper: have metronome, it's on instrument " << metronome->getInstrument() << endl;

        m_metronome = new MidiMetronome(*metronome);
    } else {
        m_metronome = new MidiMetronome
                      (SystemInstrumentBase);
        SEQMAN_DEBUG << "MetronomeMmapper: no metronome for device " << device << endl;
    }

    Composition& c = m_doc->getComposition();
    timeT t = c.getBarStart( -20); // somewhat arbitrary
    int depth = m_metronome->getDepth();

    if (depth > 0) {
        while (t < c.getEndMarker()) {

            TimeSignature sig = c.getTimeSignatureAt(t);
            timeT barDuration = sig.getBarDuration();
            std::vector<int> divisions;
            if (depth > 0)
                sig.getDivisions(depth - 1, divisions);
            int ticks = 1;

            for (int i = -1; i < (int)divisions.size(); ++i) {
                if (i >= 0)
                    ticks *= divisions[i];

                for (int tick = 0; tick < ticks; ++tick) {
                    if (i >= 0 && (tick % divisions[i] == 0))
                        continue;
                    timeT tickTime = t + (tick * barDuration) / ticks;
                    m_ticks.push_back(Tick(tickTime, i + 1));
                }
            }

            t = c.getBarEndForTime(t);
        }
    }

    KConfig *config = kapp->config();
    config->setGroup(SequencerOptionsConfigGroup);
    int midiClock = config->readNumEntry("midiclock", 0);
    int mtcMode = config->readNumEntry("mtcmode", 0);

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

    sortTicks();

    if (m_ticks.size() == 0) {
        SEQMAN_DEBUG << "MetronomeMmapper : WARNING no ticks generated\n";
    }

    // Done by init()

    //     m_mmappedSize = computeMmappedSize();
    //     if (m_mmappedSize > 0) {
    //         setFileSize(m_mmappedSize);
    //         doMmap();
    //         dump();
    //     }
}

MetronomeMmapper::~MetronomeMmapper()
{
    SEQMAN_DEBUG << "~MetronomeMmapper " << this << endl;
    delete m_metronome;
}

InstrumentId MetronomeMmapper::getMetronomeInstrument()
{
    return m_metronome->getInstrument();
}

QString MetronomeMmapper::createFileName()
{
    return KGlobal::dirs()->resourceDirs("tmp").last() + "/rosegarden_metronome";
}

void MetronomeMmapper::dump()
{
    RealTime eventTime;
    Composition& comp = m_doc->getComposition();

    SEQMAN_DEBUG << "MetronomeMmapper::dump: instrument is " << m_metronome->getInstrument() << endl;

    MappedEvent* bufPos = m_mmappedEventBuffer, *mE;

    for (TickContainer::iterator i = m_ticks.begin(); i != m_ticks.end(); ++i) {

        /*
        SEQMAN_DEBUG << "MetronomeMmapper::dump: velocity = "
                     << int(velocity) << endl;
                     */

        eventTime = comp.getElapsedRealTime(i->first);

        if (i->second == 3) // MIDI Clock
        {
            mE = new (bufPos) MappedEvent(0, MappedEvent::MidiSystemMessage);
            mE->setData1(MIDI_TIMING_CLOCK);
            mE->setEventTime(eventTime);
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

            new (bufPos) MappedEvent(m_metronome->getInstrument(),
                                     MappedEvent::MidiNoteOneShot,
                                     pitch,
                                     velocity,
                                     eventTime,
                                     m_tickDuration,
                                     RealTime::zeroTime);
        }

        ++bufPos;
    }

    // Store the number of events at the start of the shared memory region
    *(size_t *)m_mmappedRegion = (bufPos - m_mmappedEventBuffer);

    SEQMAN_DEBUG << "MetronomeMmapper::dump: - "
    << "Total events written = " << *(size_t *)m_mmappedRegion
    << endl;
}

void MetronomeMmapper::sortTicks()
{
    sort(m_ticks.begin(), m_ticks.end());
}

size_t MetronomeMmapper::computeMmappedSize()
{
    KConfig *config = kapp->config();
    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);
    int midiClock = config->readNumEntry("midiclock", 0);
    int mtcMode = config->readNumEntry("mtcmode", 0);

    // base size for Metronome ticks
    size_t size = m_ticks.size() * sizeof(MappedEvent);
    Composition& comp = m_doc->getComposition();

    if (midiClock == 1)
    {
        using Rosegarden::Note;

        // Allow room for MIDI clocks
        int clocks = ( 24 * ( comp.getEndMarker() - comp.getStartMarker() ) ) / 
            Note(Note::Crotchet).getDuration();

        /*
        SEQMAN_DEBUG << "MetronomeMmapper::computeMmappedSize - " 
                     << "Number of clock events catered for = " << clocks
                     << endl;
        */

        size += clocks * sizeof(MappedEvent);
    }

    if (mtcMode > 0)
    {
        // Allow room for MTC timing messages (how?)
    }

    return size;
}

unsigned int MetronomeMmapper::getSegmentRepeatCount()
{
    return 1;
}

}
