// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4 v0.1
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

#include "config.h"

#ifndef HAVE_ALSA

// aRts
#include <arts/artsversion.h>
#include <arts/connect.h>

#include <cstdio> // for sprintf
#include <unistd.h> // for usleep


// RG
//
#include "ArtsDriver.h"
#include "Midi.h"

using std::cerr;
using std::cout;
using std::endl;

namespace Rosegarden
{

ArtsDriver::ArtsDriver():
    SoundDriver(std::string("Arts ") + std::string(ARTS_VERSION)),
    m_artsPlayStartTime(0, 0),
    m_artsRecordStartTime(0, 0)
{
    std::cout << "Rosegarden ArtsDriver - " << m_name << std::endl;

    // Get a reference on the aRts sound server
    //

    m_soundServer = Arts::Reference("global:Arts_SoundServerV2");

    if (m_soundServer.isNull())
    {
	    cerr << "ArtsDriver - can't find aRts SoundServer - " <<
                "ensure that artsd is running!" << endl;
        m_driverStatus = NO_DRIVER;
        return;
    }

}

ArtsDriver::~ArtsDriver()
{
}

// Create a list of possible instruments for this driver
//
void
ArtsDriver::generateInstruments()
{
    std::string name("aRts MIDI");
    std::string channelName;
    char number[100];

    for (int channel = 0; channel < 16; channel++)
    {
        sprintf(number, " %d", channel);
        channelName = name + std::string(number);

        MappedInstrument *instr = new MappedInstrument(Instrument::Midi,
                                                       channel,
                                                       m_midiRunningId++,
                                                       channelName,
                                                       m_deviceRunningId);
        m_instruments.push_back(instr);
    }

    m_deviceRunningId++;
    name = "aRts Audio";

    for (int channel = 0; channel < 16; channel++)
    {
        sprintf(number, " %d", channel);
        channelName = name + std::string(number);

        MappedInstrument *instr = new MappedInstrument(Instrument::Audio,
                                                       channel,
                                                       m_audioRunningId++,
                                                       channelName,
                                                       m_deviceRunningId);
        m_instruments.push_back(instr);
    }

}

void
ArtsDriver::initialiseMidi()
{
    // Don't come in here if there's no SoundServer
    if (m_soundServer.isNull()) return;

    m_midiManager = Arts::Reference("global:Arts_MidiManager");
    if (m_midiManager.isNull())
    {
        cerr << "ArtsDriver - can't get aRts MidiManager" << endl;
        return;
    }

    m_midiRecordPort =
        Arts::DynamicCast(m_soundServer.createObject("RosegardenMidiRecord"));

    if (m_midiRecordPort.isNull())
    {
        cerr << "ArtsDriver - can't create aRts MidiRecorder"
             << endl << endl;
        cerr << "Most likely this is because you've not updated your "
             << "~/.mcopc file yet." << endl
             << "Please look in :" << endl
             << "  rosegarden/docs/howtos/artsd-mcop-notes" << endl << endl;
        exit(1);
    }

    m_midiPlayClient = m_midiManager.addClient(Arts::mcdPlay,
                                               Arts::mctApplication,
                                               "Rosegarden (play)","rosegarden");
    if (m_midiPlayClient.isNull())
    {
        cerr << "ArtsDriver - can't create aRts MidiClient" << endl;
        return;
    }

    m_midiPlayPort = m_midiPlayClient.addOutputPort();
    if (m_midiPlayPort.isNull())
    {
        cerr << "ArtsDriver - can't create aRts Midi Output Port"
             << endl;
        return;
    }

    m_midiRecordClient = m_midiManager.addClient(Arts::mcdRecord,
                                                 Arts::mctApplication,
                                                 "Rosegarden (record)",
                                                 "rosegarden");
    if (m_midiRecordClient.isNull())
    {
        cerr << "ArtsDriver - can't create aRts MidiRecordClient"
             << endl;
        return;
    }

    // Create our recording midi port
    //
    m_midiRecordClient.addInputPort(m_midiRecordPort);

    // set MIDI thru - we _have_ to set this on otherwise aRts barfs
    //
    m_midiRecordPort.setMidiThru(m_midiPlayPort);

    // Set recording on
    //
    m_midiRecordPort.record(true);


    cout << "ArtsDriver - initialised MIDI subsystem" << endl;

    if(m_driverStatus == AUDIO_OK)
        m_driverStatus = MIDI_AND_AUDIO_OK;
    else
        m_driverStatus = MIDI_OK;


    generateInstruments();

}

void
ArtsDriver::initialiseAudio()
{
    // don't come in here if there's no SoundServer
    if (m_soundServer.isNull()) return;

    m_amanPlay =
        Arts::DynamicCast(m_soundServer.createObject("Arts::Synth_AMAN_PLAY"));

    m_amanPlay.title("Rosegarden Audio Play");
    m_amanPlay.autoRestoreID("Rosegarden Play");

    if (m_amanPlay.isNull())
    {
        cerr << "ArtsDriver - can't create audio play object" << endl;
        return;
    }

    m_amanRecord =
      Arts::DynamicCast(m_soundServer.createObject("Arts::Synth_AMAN_RECORD"));

    m_amanRecord.title("Rosegarden Audio Record");
    m_amanRecord.autoRestoreID("Rosegarden Record");

    if (m_amanRecord.isNull())
    {
        cerr << "ArtsDriver - can't create audio record object" <<
             endl;
        return;
    }

    // Start the play and record audio ports
    //
    m_amanPlay.start();
    m_amanRecord.start();

    // Test play and record connections - create server objects
    //
    Arts::Synth_CAPTURE_WAV captureWav = Arts::DynamicCast(m_soundServer.
                               createObject("Arts::Synth_CAPTURE_WAV"));

    if (captureWav.isNull())
    {
        cerr << "ArtsDriver - can't create .wav record object" << endl;
        return;
    }

    Arts::Synth_PLAY_WAV playWav = Arts::DynamicCast(m_soundServer.
                               createObject("Arts::Synth_PLAY_WAV"));

    if (playWav.isNull())
    {
        cerr << "ArtsDriver - can't create .wav play object "<< endl;
        return;
    }

    // Now test connections - aRts will moan if these fail
    //
    playWav.start();
    connect(playWav, "left", m_amanPlay, "left");
    connect(playWav, "right", m_amanPlay, "right");
    disconnect(playWav, "left", m_amanPlay, "left");
    disconnect(playWav, "right", m_amanPlay, "right");
    playWav.stop();

    // Test record connection
    captureWav.start();
    connect(captureWav, "left", m_amanRecord, "left");
    connect(captureWav, "right", m_amanRecord, "right");
    disconnect(captureWav, "left", m_amanRecord, "left");
    disconnect(captureWav, "right", m_amanRecord, "right");
    captureWav.stop();

    // Seems the audio is OK, report some figures and exit
    //
    if (m_soundServer.fullDuplex())
        cout << "ArtsDriver - aRts is in full duplex audio mode"
             << endl;

    cout << "ArtsDriver - audio sampling rate = "
         << m_soundServer.samplingRate() << "Hz" << endl;

    cout << "ArtsDriver - sample resolution = "
         << m_soundServer.bits() << " bits" << endl;

    cout << "ArtsDriver - initialised audio subsystem" << endl;

    if(m_driverStatus == MIDI_OK)
        m_driverStatus = MIDI_AND_AUDIO_OK;
    else
        m_driverStatus = AUDIO_OK;

}

void
ArtsDriver::initialisePlayback(const Rosegarden::RealTime &position)
{
    cout << "ArtsDriver - initialisePlayback" << endl;
    m_startPlayback = true;
    m_artsPlayStartTime.sec = 0;
    m_artsPlayStartTime.usec = 0;
    m_playStartPosition = position;
}

void
ArtsDriver::stopPlayback()
{
    allNotesOff();
    m_playing = false;

    // Wait for a tenth of a second and then flush off every device
    //
    usleep(100000);

    // Tell all MIDI devices to stop playing
    //
    sendDeviceController(MIDI_CONTROLLER_SOUNDS_OFF, 0);
}



void
ArtsDriver::resetPlayback(const RealTime &position,
                          const RealTime &latency)
{
    //allNotesOff();
    RealTime artsPlayStartTime = RealTime(m_artsPlayStartTime.sec,
                                          m_artsPlayStartTime.usec);

    // save the modification period
    RealTime  modifyNoteOff = m_playStartPosition - artsPlayStartTime;

    m_artsPlayStartTime = deltaTime(m_midiPlayPort.time(),
                                    Arts::TimeStamp(latency.sec, latency.usec));
    artsPlayStartTime = RealTime(m_artsPlayStartTime.sec,
                                 m_artsPlayStartTime.usec);

    m_playStartPosition = position;

    // adjust modification period
    modifyNoteOff = modifyNoteOff - m_playStartPosition + artsPlayStartTime;

    for (NoteOffQueue::iterator i = m_noteOffQueue.begin();
                                i != m_noteOffQueue.end(); i++)
    {
        // if we're fast forwarding then we bring the note off closer
        if (modifyNoteOff <= RealTime(0, 0))
            (*i)->setRealTime((*i)->getRealTime() + modifyNoteOff);
        else // we're rewinding - kill the note immediately
            (*i)->setRealTime(m_playStartPosition);
    }
}

// Force all pending note offs to stop immediately.
// (Actually aRts can't do this yet, so all we can do is
// send the note off events and wait for them to clear.
// We shouldn't send NOTE OFFs with an immediate timestamp
//  as some of these notes might not yet have been played.)
//

void
ArtsDriver::allNotesOff()
{
    Arts::MidiEvent event;
    Rosegarden::RealTime noteOff;

    for (NoteOffQueue::iterator i = m_noteOffQueue.begin();
          i != m_noteOffQueue.end(); ++i)
    {
        // Rather long way around of converting these two
        //
        noteOff = (*i)->getRealTime();
        event.time = aggregateTime(m_artsPlayStartTime,
                       Arts::TimeStamp(noteOff.sec, noteOff.usec));

        event.command.data1 = (*i)->getPitch();
        event.command.data2 = 127;
        event.command.status = Arts::mcsNoteOff | (*i)->getChannel();
        m_midiPlayPort.processEvent(event);

        // Clear the event
        //
        delete(*i);
        m_noteOffQueue.erase(i);
    }

}

void
ArtsDriver::processNotesOff(const RealTime &time)
{
    Arts::MidiEvent event;
    Rosegarden::RealTime noteOffTime;

    for (NoteOffQueue::iterator i = m_noteOffQueue.begin();
          i != m_noteOffQueue.end(); ++i)

    {
        // If there's a pregnant NOTE OFF around then send it
        if ((*i)->getRealTime() <= time)
        {
            // Conversion from RealTime to Arts::TimeStamp
            noteOffTime = (*i)->getRealTime();
            event.time = aggregateTime(m_artsPlayStartTime,
                           Arts::TimeStamp(noteOffTime.sec, noteOffTime.usec));

            event.command.data1 = (*i)->getPitch();
            event.command.data2 = 127;
            event.command.status = Arts::mcsNoteOff | (*i)->getChannel();
            m_midiPlayPort.processEvent(event);

            // Delete and remove the note from the queue
            //
            delete(*i);
            m_noteOffQueue.erase(i);
        }
    }

}

void
ArtsDriver::processAudioQueue(const RealTime &playLatency)
{
    // Now check queue for events that need playing
    std::vector<PlayableAudioFile*>::iterator it;
    RealTime currentTime = getSequencerTime();

    for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); ++it)
    {
        if (currentTime >= (*it)->getStartTime() && 
            (*it)->getStatus() == PlayableAudioFile::IDLE)
        {
            /*
            (*it)->getAudioObject().start();

            Arts::connect((*it)->getAudioObject(), "left",
                          m_amanPlay, "left");
            Arts::connect((*it)->getAudioObject(), "right",
                          m_amanPlay, "right");
                          */

            (*it)->setStatus(PlayableAudioFile::PLAYING);
        }

        if (currentTime >= (*it)->getEndTime() &&
            (*it)->getStatus() == PlayableAudioFile::PLAYING)
        {
            // Disconnect properly and stop
            //
            /*
            Arts::disconnect((*it)->getAudioObject(), "left",
                             m_amanPlay, "left");
            Arts::disconnect((*it)->getAudioObject(), "right",
                             m_amanPlay, "right");

            (*it)->getAudioObject().stop();

            */
            (*it)->setStatus(PlayableAudioFile::DEFUNCT);
        }
    }

    // Finally remove all defunct objects treading carefully around
    // the iterators.
    //
    int defunctEvents;

    do
    {
        for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); ++it)
        {
            if ((*it)->getStatus() == PlayableAudioFile::DEFUNCT)
            {
                delete(*it);
                it = m_audioPlayQueue.erase(it);
                break;
            }
        }

        defunctEvents = 0;
        for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); ++it)
            if ((*it)->getStatus() == PlayableAudioFile::DEFUNCT)
               defunctEvents++;
    }
    while(defunctEvents);

}

RealTime
ArtsDriver::getSequencerTime()
{
    if (m_playing)
    {
        Arts::TimeStamp artsTimeNow = m_midiPlayPort.time();

        Rosegarden::RealTime internalRelativeTime =
        Rosegarden::RealTime(artsTimeNow.sec, artsTimeNow.usec) -
        Rosegarden::RealTime(m_artsPlayStartTime.sec, m_artsPlayStartTime.usec);
        return (m_playStartPosition + internalRelativeTime);
    }

    return Rosegarden::RealTime(0, 0);
}

MappedComposition*
ArtsDriver::getMappedComposition(const RealTime &playLatency)
{
    // clear out our global segment
    m_recordComposition.clear();

    // Return empty set if we're currently not recording or monitoring
    
    if (m_recordStatus != RECORD_MIDI &&
        m_recordStatus != RECORD_AUDIO &&
        m_recordStatus != ASYNCHRONOUS_MIDI &&
        m_recordStatus != ASYNCHRONOUS_AUDIO)
           return &m_recordComposition;


    std::vector<Arts::MidiEvent>::iterator midiQueueIt;
    std::vector<Arts::MidiEvent> *midiQueue;
    // get the MIDI queue
    midiQueue = m_midiRecordPort.getQueue();

    // Process it into the internal Composition
    for (midiQueueIt = midiQueue->begin();
         midiQueueIt != midiQueue->end();
         midiQueueIt++)
    {
        processMidiIn(midiQueueIt->command,
                      recordTime(midiQueueIt->time),
                      playLatency);
    }

    // Free the returned std::vector from here as the aRts stub
    // has already gone ahead and allocated a new one.
    delete midiQueue;

    return &m_recordComposition;
}
    
void
ArtsDriver::processMidiOut(const MappedComposition &mC,
                           const RealTime &playLatency,
                           bool now)
{
    Arts::MidiEvent event;

    // MidiCommandStatus
    //   mcsCommandMask
    //   mcsChannelMask
    //   mcsNoteOff
    //   mcsNoteOn
    //   mcsKeyPressure
    //   mcsParameter
    //   mcsProgram 
    //   mcsChannelPressure
    //   mcsPitchWheel

    Rosegarden::RealTime midiRelativeTime;
    Rosegarden::RealTime midiRelativeStopTime;
    Rosegarden::MappedInstrument *instrument;
    MidiByte channel;

    for (MappedComposition::iterator i = mC.begin(); i != mC.end(); ++i)
    {
        // Check that we're processing the right type
        //
        if ((*i)->getType() == MappedEvent::Audio)
            continue;

        // Sanity check when playing
        if (!now)
            assert((*i)->getEventTime() >= m_playStartPosition);

        // Relative to start of the piece including play latency
        //
        midiRelativeTime = (*i)->getEventTime() - m_playStartPosition +
                            playLatency;

        // Add the aRts play start time
        //
        event.time = aggregateTime(m_artsPlayStartTime,
                  Arts::TimeStamp(midiRelativeTime.sec, midiRelativeTime.usec));

        // Use the duration to find out when it ends
        //
        midiRelativeStopTime = midiRelativeTime + (*i)->getDuration();
    
        // Get instrument channel
        //
        instrument = getMappedInstrument((*i)->getInstrument());

        if (instrument != 0)
            channel = instrument->getChannel();
        else
            channel = 0;

        // Load the command structure
        //
        switch((*i)->getType())
        {
            case MappedEvent::MidiNote:
            case MappedEvent::MidiNoteOneShot:
                event.command.status = Arts::mcsNoteOn | channel;
                event.command.data1 = (*i)->getPitch();
                event.command.data2 = (*i)->getVelocity();
                break;

            case MappedEvent::MidiProgramChange:
                event.command.status = Arts::mcsProgram | channel;
                event.command.data1 = (*i)->getData1();
                break;

            case MappedEvent::MidiKeyPressure:
                event.command.status = Arts::mcsKeyPressure | channel;
                event.command.data1 = (*i)->getData1();
                event.command.data2 = (*i)->getData2();
                break;

            case MappedEvent::MidiChannelPressure:
                event.command.status = Arts::mcsChannelPressure | channel;
                event.command.data1 = (*i)->getData1();
                event.command.data2 = (*i)->getData2();
                break;

            case MappedEvent::MidiPitchWheel:
                event.command.status = Arts::mcsPitchWheel | channel;
                event.command.data1 = (*i)->getData1();
                event.command.data2 = (*i)->getData2();
                break;

            case MappedEvent::MidiController:
                event.command.status = Arts::mcsParameter | channel;
                event.command.data1 = (*i)->getData1();
                event.command.data2 = (*i)->getData2();
                break;

            default:
                 std::cerr << "processMidiOut() - got unrecognized event"
                            << endl;
                 break;
        }

        Arts::TimeStamp timeNow = m_midiPlayPort.time();

        if(now)
        {
            event.time.sec = timeNow.sec;
            event.time.usec = timeNow.usec;
        }
        else
        {
            // Check event timing for lag
            int secAhead = event.time.sec - timeNow.sec;
            int uSecAhead = event.time.usec - timeNow.usec;

            // Adjust for negative
            //
            if (uSecAhead < 0) 
            {
                uSecAhead += 1000000;
                secAhead++;
            }
    
            if (secAhead < 0)
            {
                std::cerr <<
                    "Sequencer::processMidiOut: MIDI processing lagging by "
                          << -secAhead
                          << "s and "
                          << uSecAhead
                          << "us" << endl;
            }
        }

        // Send the event out to the MIDI port
        //
        m_midiPlayPort.processEvent(event);

#if 0
        int secFromStart = event.time.sec - m_artsPlayStartTime.sec;
        int usecFromStart = event.time.usec - m_artsPlayStartTime.usec;

        if (usecFromStart < 0)
        {
            secFromStart--;
            usecFromStart += 1000000;
        }


        cout << "Event sent to aRts at " << secFromStart << "s & "
             << usecFromStart << "ms" << endl;
#endif

        // Only create a NOTE OFF entry for a NOTE ON
        //
        if ((*i)->getType() == MappedEvent::MidiNote)
        {
            NoteOffEvent *noteOffEvent =
                new NoteOffEvent(midiRelativeStopTime,
                         (Rosegarden::MidiByte)event.command.data1,
                         (Rosegarden::MidiByte)channel,
                         (*i)->getInstrument());

            m_noteOffQueue.insert(noteOffEvent);
        }

    }

    // If there's no midiRelativeTime set then set one - occurs if we're
    // not processing any NOTE ONs.
    //
    //
    if (midiRelativeTime.sec == 0 && midiRelativeTime.usec == 0)
    {
        Arts::TimeStamp now = m_midiPlayPort.time();
        int sec = now.sec - m_artsPlayStartTime.sec;
        int usec = now.usec - m_artsPlayStartTime.usec;
 
        if (usec < 0)
        {
            sec--;
            usec += 1000000;
        }

        Arts::TimeStamp relativeNow(sec, usec);
        midiRelativeTime =
               Rosegarden::RealTime(relativeNow.sec, relativeNow.usec);
    }

    // Process NOTE OFFs for current time
    processNotesOff(midiRelativeTime);
}

Arts::TimeStamp
ArtsDriver::aggregateTime(const Arts::TimeStamp &ts1,
                          const Arts::TimeStamp &ts2)
{
    unsigned int usec = ts1.usec + ts2.usec;
    unsigned int sec = ( (unsigned int) ( usec / 1000000 ) )
                       + ts1.sec + ts2.sec;
    usec %= 1000000;
    return(Arts::TimeStamp(sec, usec));
}

Arts::TimeStamp
ArtsDriver::deltaTime(const Arts::TimeStamp &ts1, const Arts::TimeStamp &ts2)
{
    int usec = ts1.usec - ts2.usec;
    int sec = ts1.sec - ts2.sec;

    if ( usec < 0 )
    {
        sec--;
        usec += 1000000;
    }

    assert( sec >= 0 );

    return (Arts::TimeStamp(sec, usec));
}

void
ArtsDriver::processMidiIn(const Arts::MidiCommand &midiCommand,
                          const Arts::TimeStamp &timeStamp,
                          const RealTime &playLatency)
{

    Rosegarden::MidiByte channel;
    Rosegarden::MidiByte message;
    Rosegarden::RealTime guiTimeStamp(timeStamp.sec, timeStamp.usec);
  
    // Remove the playback latency from the timing
    //
    guiTimeStamp = guiTimeStamp - playLatency;

    channel = midiCommand.status & MIDI_CHANNEL_NUM_MASK;
    message = midiCommand.status & MIDI_MESSAGE_TYPE_MASK;

    // Check for a hidden NOTE OFF (NOTE ON with zero velocity)
    //
    if ( message == MIDI_NOTE_ON && midiCommand.data2 == 0 )
    {
        message = MIDI_NOTE_OFF;
    }

    // We use a map of Notes and this is the key
    //
    unsigned int chanNoteKey = ( channel << 8 ) + midiCommand.data1;

    // scan for our event
    switch(message)
    {
        case MIDI_NOTE_ON:
            if ( m_noteOnMap[chanNoteKey] == 0 )
            {
                m_noteOnMap[chanNoteKey] = new MappedEvent;

                // Set time, pitch and velocity on the MappedEvent
                //
                m_noteOnMap[chanNoteKey]->
                       setEventTime(guiTimeStamp);

                m_noteOnMap[chanNoteKey]->setPitch(midiCommand.data1);
                m_noteOnMap[chanNoteKey]->setVelocity(midiCommand.data2);
    
            }
            break;

        case MIDI_NOTE_OFF:
            // if we match an open NOTE_ON
            //
            if ( m_noteOnMap[chanNoteKey] != 0 )
            {
                Rosegarden::RealTime duration = guiTimeStamp -
                            m_noteOnMap[chanNoteKey]->getEventTime();

                // for the moment, ensure we're positive like this
                //
                assert(duration.sec >= 0 || duration.usec >= 0 );

                // set the duration
                m_noteOnMap[chanNoteKey]->setDuration(duration);

                // insert the record
                //
                m_recordComposition.insert(m_noteOnMap[chanNoteKey]);

                // tell us about it
#ifdef MIDI_DEBUG
                cout << "INSERTED NOTE at time " 
                     << m_noteOnMap[chanNoteKey]->getAbsoluteTime()
                     << " of duration "
                     << m_noteOnMap[chanNoteKey]->getDuration() << endl;
#endif

                // reset the reference
                m_noteOnMap[chanNoteKey] = 0;
            }
            else
                cerr << "MIDI_NOTE_OFF with no matching MIDI_NOTE_ON" << endl;
            break;

        case MIDI_POLY_AFTERTOUCH:
            {
                MappedEvent *mE = new MappedEvent();
                mE->setType(MappedEvent::MidiKeyPressure);
                mE->setEventTime(guiTimeStamp);
                mE->setData1(midiCommand.data1);
                mE->setData2(midiCommand.data2);
                m_recordComposition.insert(mE);
            }
            break;

        case MIDI_CTRL_CHANGE:
            {
                MappedEvent *mE = new MappedEvent();
                mE->setType(MappedEvent::MidiController);
                mE->setEventTime(guiTimeStamp);
                mE->setData1(midiCommand.data1);
                mE->setData2(midiCommand.data2);
                m_recordComposition.insert(mE);
            }
            break;

        case MIDI_PROG_CHANGE:
            {
                MappedEvent *mE = new MappedEvent();
                mE->setType(MappedEvent::MidiProgramChange);
                mE->setEventTime(guiTimeStamp);
                mE->setData1(midiCommand.data1);
                mE->setData2(midiCommand.data2);
                m_recordComposition.insert(mE);
            }
            break;

        case MIDI_CHNL_AFTERTOUCH:
            {
                MappedEvent *mE = new MappedEvent();
                mE->setType(MappedEvent::MidiChannelPressure);
                mE->setEventTime(guiTimeStamp);
                mE->setData1(midiCommand.data1);
                mE->setData2(midiCommand.data2);
                m_recordComposition.insert(mE);
            }
            break;

        case MIDI_PITCH_BEND:
            {
                MappedEvent *mE = new MappedEvent();
                mE->setType(MappedEvent::MidiPitchWheel);
                mE->setEventTime(guiTimeStamp);
                mE->setData1(midiCommand.data1);
                mE->setData2(midiCommand.data2);
                m_recordComposition.insert(mE);
            }
            break;

        case MIDI_SYSTEM_EXCLUSIVE:
            std::cout << "ArtsDriver - SYSTEM EXCLUSIVE EVENT not supported"
                      << std::endl;
            break;

        default:
            cout << "ArtsDriver - UNSUPPORTED MIDI EVENT RECEIVED" << endl;
            break;
    }
}


void
ArtsDriver::processEventsOut(const MappedComposition &mC,
                             const RealTime &playLatency,
                             bool now)
{
    // Start playback if it isn't already
    //
    if (m_startPlayback)
    {
        m_artsPlayStartTime = m_midiPlayPort.time();
        m_startPlayback = false;
        m_playing = true;
    }

    // Queue up any audio events that are pending and connect them
    // to the relevant audio out object
    //
    for (MappedComposition::iterator i = mC.begin(); i != mC.end(); ++i)
    {
        if ((*i)->getType() == MappedEvent::Audio)
        {
            PlayableAudioFile *audioFile =
                    new PlayableAudioFile(getAudioFile((*i)->getAudioID()),
                                          (*i)->getEventTime(),
                                          (*i)->getAudioStartMarker(),
                                          (*i)->getDuration());

            queueAudio(audioFile);
        }
    }

    // do MIDI events
    processMidiOut(mC, playLatency, now);

    // do any audio events
    processAudioQueue(playLatency);
}

void
ArtsDriver::record(const RecordStatus& recordStatus)
{
    if (recordStatus == RECORD_MIDI)
    {
        // Set status and the record start position
        //
        m_recordStatus = RECORD_MIDI;
        m_artsRecordStartTime = m_midiRecordPort.time();
    }
    else if (recordStatus == RECORD_AUDIO)
    {
        std::cerr << "ArtsDriver - record() - AUDIO RECORDING not yet supported"
                  << std::endl;
    }
    else if (recordStatus == ASYNCHRONOUS_MIDI)
    {
        m_recordStatus = ASYNCHRONOUS_MIDI;
    }
    else if (recordStatus == ASYNCHRONOUS_AUDIO)
    {
        m_recordStatus = ASYNCHRONOUS_AUDIO;
    }
    else
    {
        std::cerr << "ArtsDriver  - record() - Unsupported recording mode"
                  << std::endl;
    }
}

void
ArtsDriver::processPending(const RealTime &playLatency)
{
    if (m_playing)
    {
        Arts::TimeStamp artsTime = aggregateTime(m_artsPlayStartTime,
                                                 m_midiPlayPort.time());
        processNotesOff(Rosegarden::RealTime(artsTime.sec, artsTime.usec) +
                        playLatency);
    }
}


// Send out a controller to all channels on the MIDI device
void
ArtsDriver::sendDeviceController(MidiByte controller,
                                 MidiByte value)
{
    Arts::MidiEvent event;
    Arts::TimeStamp timeNow = m_midiPlayPort.time();

    event.time.sec = timeNow.sec;
    event.time.usec = timeNow.usec;

    for (int channel = 0; channel < 16; channel++)
    {
        event.command.status = Arts::mcsParameter | channel;
        event.command.data1 = controller;
        event.command.data2 = value;

        // send it out
        //
        m_midiPlayPort.processEvent(event);
    }


}

float
ArtsDriver::getLastRecordedAudioLevel() 
{
    return 0.0;
}

}


#endif // HAVE_ALSA
