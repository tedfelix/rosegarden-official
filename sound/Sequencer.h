// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4 v0.2
  A sequencer and musical notation editor.

  This program is Copyright 2000-2002
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


// From this class we control our sound drivers - audio
// and MIDI are initialised, playback and recording handles
// are available to the higher levels for sending and 
// retreiving MIDI and audio.  When the Rosegarden sequencer
// (sequencer/) initialises it creates a Rosegarden::Sequencer
// which prepares itself for playback and recording.
//
// At this level we accept MappedCompositions (single point
// representation - NOTE ONs with durations) and turn them
// into MIDI events (generate and segment NOTE OFFs).
//
// Recording wise we take incoming driver events and turn them
// into a MappedComposition before sending it up to the gui.
// Timing is normalised by the GUI and returned as
// Rosegarden::RealTime timestamps that can be easily
// converted into the relevant absolute positions.
//
// We don't have any measure of tempo or resolution at
// this level - all we see are Arts::TimeStamps and
// Rosegarden::RealTimes.
//
//

#ifndef _ROSEGARDEN_SEQUENCER_H_
#define _ROSEGARDEN_SEQUENCER_H_

#include "MappedComposition.h"
#include "Midi.h"
#include "MidiEvent.h"
#include "AudioFile.h"
#include "SoundDriver.h"
#include "MappedDevice.h"


namespace Rosegarden
{

class MappedInstrument;
class MappedStudio;

class Sequencer
{
public:
    Sequencer(MappedStudio *studio);
    ~Sequencer();

    // Control playback - initialisePlayback starts us playing
    //
    void initialisePlayback(const Rosegarden::RealTime &startTime)
        { m_soundDriver->initialisePlayback(startTime); }

    void stopPlayback() { m_soundDriver->stopPlayback(); }

    // Reset internal states while playing (like when looping
    // and jumping to a new time)
    //
    void resetPlayback(const Rosegarden::RealTime &position,
                       const Rosegarden::RealTime &playLatency)
        { m_soundDriver->resetPlayback(position, playLatency); }

    // Control recording (input) state
    //
    void record(const RecordStatus& recordStatus)
        { m_soundDriver->record(recordStatus); }

    // While recording returns a wrapped MappedComposition of
    // the latest MappedEvents
    //
    MappedComposition* getMappedComposition(const Rosegarden::RealTime &pL)
        { return m_soundDriver->getMappedComposition(pL); }

    // Process MappedComposition into MIDI and audio events at
    // the driver level and queue for output.
    //
    void processEventsOut(const Rosegarden::MappedComposition &mC,
                          const Rosegarden::RealTime &playLatency,
                          bool now) // send everything immediately
        { m_soundDriver->processEventsOut(mC, playLatency, now); }

    // Return the sequencer time
    //
    Rosegarden::RealTime getSequencerTime()
        { return m_soundDriver->getSequencerTime(); }

    // Are we playing?
    //
    bool isPlaying()
        { return m_soundDriver->isPlaying(); }

    // Note off processing control
    //
    void allNotesOff()
        { m_soundDriver->allNotesOff(); }
    void processNotesOff(const Rosegarden::RealTime &time)
        { m_soundDriver->processNotesOff(time); }

    // Get playback start position
    //
    RealTime getStartPosition()
        { return m_soundDriver->getStartPosition(); }

    // Return the status of the initialised sound driver
    //
    unsigned int getSoundDriverStatus() const
        { return m_soundDriver->getStatus(); }

    // Audio files
    //
    void clearAudioFiles() { m_soundDriver->clearAudioFiles(); }
    bool addAudioFile(const std::string &fileName, const unsigned int &id)
        { return m_soundDriver->addAudioFile(fileName, id); }
    bool removeAudioFile(const unsigned int &id)
        { return m_soundDriver->removeAudioFile(id); }

    // Queue up an audio sample for playing
    //
    bool queueAudio(InstrumentId instrumentId,
                    AudioFileId audioFileId,
                    const RealTime &absoluteTime,
                    const RealTime &audioStartMarker,
                    const RealTime &duration,
                    const RealTime &playLatency)
    {
        return m_soundDriver->queueAudio(instrumentId,
                                         audioFileId,
                                         absoluteTime,
                                         audioStartMarker,
                                         duration,
                                         playLatency);
    }

    // Set a MappedInstrument at the Sequencer level
    //
    void setMappedInstrument(MappedInstrument *mI)
        { m_soundDriver->setMappedInstrument(mI); }

    // Get a MappedInstrument for querying
    //
    MappedInstrument* getMappedInstrument(InstrumentId id)
        { return m_soundDriver->getMappedInstrument(id); }

    // Return a list of MappedInstruments as a DCOP friendly MappedDevice.
    //
    //
    MappedDevice getMappedDevice(DeviceId id)
        { return m_soundDriver->getMappedDevice(id); }

    // Get total number of devices
    //
    unsigned int getDevices()
    {
        if (m_soundDriver->getStatus() == NO_DRIVER)
            return 0;

        return m_soundDriver->getDevices();
    }


    // Process anything that needs to go on in the background 
    // (NoteOffs etc).
    //
    void processPending(const RealTime &playLatency)
        { m_soundDriver->processPending(playLatency); }

    // set the file we're using for audio recording - we only currently
    // support recording of a single track at a time
    //
    void setRecordingFilename(const std::string &file)
        { m_soundDriver->setRecordingFilename(file); }

    RecordStatus getRecordStatus() const
        { return m_soundDriver->getRecordStatus(); }

    // Audio monitoring Instrument
    //
    void setAudioMonitoringInstrument(Rosegarden::InstrumentId id)
        { m_soundDriver->setAudioMonitoringInstrument(id); }
    Rosegarden::InstrumentId getAudioMonitoringInstrument()
        { return m_soundDriver->getAudioMonitoringInstrument(); }
    

    // Audio latenices from audio driver
    //
    RealTime getAudioPlayLateny()
        { return m_soundDriver->getAudioPlayLatency(); }

    RealTime getAudioRecordLateny()
        { return m_soundDriver->getAudioRecordLatency(); }

private:

    SoundDriver                                *m_soundDriver;
    std::vector<AudioFile*>                     m_audioFiles;

};


}
 

#endif // _ROSEGARDEN_SEQUENCER_H_
