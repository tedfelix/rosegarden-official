// -*- c-basic-offset: 4 -*-

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

#include <klocale.h>
#include <qbutton.h>
#include <dcopclient.h>
#include <qpushbutton.h>
#include <ktmpstatusmsg.h>

#include "rosegardenguidoc.h"
#include "rosegardentransportdialog.h"
#include "sequencemanager.h"
#include "SegmentPerformanceHelper.h"

namespace Rosegarden
{

SequenceManager::SequenceManager(RosegardenGUIDoc *doc,
                                 RosegardenTransportDialog *transport):
    m_doc(doc),
    m_playLatency(0, 100000),  // the sequencer's head start
    m_fetchLatency(0, 50000),  // how long to fetch and queue new events
    m_readAhead(0, 40000),     // how many events to fetch
    m_transportStatus(STOPPED),
    m_soundSystemStatus(Rosegarden::NO_SEQUENCE_SUBSYS),
    m_metronomeInstrument(0),
    m_metronomeTrack(0),
    m_metronomePitch(70),
    m_metronomeBarVelocity(110),
    m_metronomeBeatVelocity(70),
    m_metronomeDuration(0, 10000),
    m_transport(transport)
{
}


SequenceManager::~SequenceManager()
{
} 

void
SequenceManager::setTransportStatus(const TransportStatus &status)
{
    m_transportStatus = status;
}

// This method is called from the Sequencer when it's playing.
// It's a request to get the next slice of events for the
// Sequencer to play.
//
//
MappedComposition*
SequenceManager::getSequencerSlice(const Rosegarden::RealTime &sliceStart,
                                   const Rosegarden::RealTime &sliceEnd)
{
    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();
  
    // Reset MappedComposition
    m_mC.clear();
    m_mC.setStartTime(sliceStart);
    m_mC.setEndTime(sliceEnd);

    if (m_transportStatus == STOPPING)
      return &m_mC; // return empty

    timeT sliceStartElapsed =
              comp.getElapsedTimeForRealTime(m_mC.getStartTime());

    timeT sliceEndElapsed =
              comp.getElapsedTimeForRealTime(m_mC.getEndTime());

    // Place metronome clicks in the MappedComposition
    // if they're required
    //
    insertMetronomeClicks(sliceStartElapsed, sliceEndElapsed);

    Rosegarden::RealTime eventTime;
    Rosegarden::RealTime duration;
    Rosegarden::Instrument *instrument;
    Rosegarden::Track *track;
  
    for (Composition::iterator it = comp.begin(); it != comp.end(); it++)
    {
	timeT segmentStartTime = (*it)->getStartTime();
	timeT segmentDuration  = (*it)->getDuration();

        track = comp.getTrackByIndex((*it)->getTrack());
        instrument = studio.getInstrumentById(track->getInstrument());

        // Skip if track is muted
        if (track->isMuted())
            continue;

        // Skip the Segment if it starts too late to be of
        // interest to our slice.
        if (segmentStartTime > sliceEndElapsed)
            continue;

	// Skip the Segment if it ends too early to be of
	// interest and it's not repeating.
	if (segmentStartTime + segmentDuration <= sliceStartElapsed &&
	    !(*it)->isRepeating())
	    continue;

        if ((*it)->getType() == Rosegarden::Segment::Audio)
        {

            // An Audio event has three time parameters associated
            // with it.  The start of the Segment is when the audio
            // event should start.  The StartTime is how far into
            // the sample the playback should commence and the
            // EndTime is how far into the sample playback should
            // stop.
            //
            //
            if (segmentStartTime < sliceStartElapsed ||
                segmentStartTime > sliceEndElapsed)
                continue;

            cout << "AUDIO START = " << (*it)->getAudioStartTime() << endl;
            cout << "AUDIO END   = " << (*it)->getAudioEndTime() << endl;
            cout << "SLICE START = " << sliceStartElapsed << endl;
            cout << "SLICE END   = " << sliceEndElapsed << endl << endl;

            eventTime = comp.getElapsedRealTime(segmentStartTime);

            Rosegarden::RealTime startTime =
                       comp.getElapsedRealTime (((*it)->getAudioStartTime()));

            duration = comp.getElapsedRealTime
                            (((*it)->getAudioEndTime())) - startTime;

            assert (duration >= Rosegarden::RealTime(0,0));
            
            // Insert Audio event
            //
            Rosegarden::MappedEvent *me =
                    new Rosegarden::MappedEvent(track->getInstrument(),
                                                (*it)->getAudioFileID(),
                                                eventTime,
                                                duration,
                                                startTime);
            m_mC.insert(me);
            continue; // next Segment
        }

        SegmentPerformanceHelper helper(**it);

	// Now, we know that the segment (or its repeating trail,
	// if it is repeating) has some intersection with the slice.
	// Our procedure is: find a suitable starting time within
	// the segment (by modular arithmetic for repeating segments),
	// iterate from there playing all events whose times when
	// adjusted for repeats fall within the slice, looping around
	// to the start of the segment when we reach the end; and
	// drop out when we hit the end of the slice.

	// Segments don't repeat before they begin, only after they
	// end, so if the slice's start time is before the segment's
	// start we should use the segment's start instead.  (We
	// already know sliceEndElapsed >= segmentStartTime.)
	//
	timeT seekStartTime =
	    (sliceStartElapsed < segmentStartTime) ? segmentStartTime :
						     sliceStartElapsed;

	// Need to know how many times we've repeated to get here,
	// so we can adjust the performance times again later
	//
	int repeatNo = (seekStartTime - segmentStartTime) / segmentDuration; 

	// Locate a suitable starting iterator
	//
	seekStartTime = seekStartTime - repeatNo * segmentDuration;
	Segment::iterator i0 = (*it)->findTime(seekStartTime);

	// Now, we end at the slice's end, except where we're a
	// repeating segment followed by another segment on the same
	// track when we end at that other segment's start time
	// 
	timeT seekEndTime = sliceEndElapsed;

	if ((*it)->isRepeating())
	{
	    Composition::iterator scooter(it);
	    if (++scooter != comp.end() &&
		(*scooter)->getTrack() == (*it)->getTrack()) {
		//!!! What if the following segment starts before this one
		// ends?  Do we want to ignore it and keep going?
		seekEndTime = (*scooter)->getStartTime();
	    }
	}

	// No ending condition -- we do all that in the initial
	// conditional within the loop, and subsequent breaks
	//
        for (Segment::iterator j = i0; ; ++j)
        {
	    if (j == (*it)->end())
	    {
		// Wrap around if we're repeating, abandon otherwise
		//
		if ((*it)->isRepeating())
		{
		    j = (*it)->begin();
		    ++repeatNo; // adjust subsequent events one segment further
		}
		else
		{
		    break;
		}
	    }

            // Skip this event if it isn't a note
            //
            if (!(*j)->isa(Rosegarden::Note::EventType))
                continue;

            // Get the performance time, adjusted for repeats
	    // 
	    timeT playTime =
		helper.getSoundingAbsoluteTime(j) + repeatNo * segmentDuration;

	    // All over when we hit the end of the slice (the most
	    // usual break condition for long or repeating segments)
	    // 
	    if (playTime >= seekEndTime) break;

	    // Convert to real-time
	    // 
            eventTime = comp.getElapsedRealTime(playTime);

            // Continue if the time falls before the slice (can happen
	    // through rounding error etc)
	    // 
	    if (eventTime < m_mC.getStartTime())
		continue;

            // Escape if we're beyond the slice (again, can happen)
	    // 
            if (eventTime >= m_mC.getEndTime())
                break;

	    // Add any performance delay.  Note that simply adding
	    // comp.getElapsedRealTime((*it)->getDelay()) would fail to
	    // take tempo changes into account correctly
	    // 
	    eventTime = eventTime + comp.getRealTimeDifference
		(playTime, playTime + (*it)->getDelay());

	    // Find the performance duration, i.e. taking into account
	    // any ties etc that this note may have
	    // 
	    duration = helper.getRealSoundingDuration(j);

	    // No duration?  Probably in a tied series, but not as first note
	    //
	    if (duration == Rosegarden::RealTime(0, 0))
		continue;

	    // Make mapped event
	    // 
	    Rosegarden::MappedEvent *mE =
		new Rosegarden::MappedEvent(track->getInstrument(),
                                            **j,
                                            eventTime,
                                            duration);

	    // Add any performance transposition
	    // 
	    mE->setPitch(mE->getPitch() + ((*it)->getTranspose()));

            // Force velocity if the Instrument is set
            //
            if (instrument->sendsVelocity())
                mE->setVelocity(instrument->getVelocity());

	    // And stick it in the mapped composition
	    // 
	    m_mC.insert(mE);
        }
    }

    return &m_mC;
}


void
SequenceManager::play()
{
    Composition &comp = m_doc->getComposition();

    QByteArray data;
    QCString replyType;
    QByteArray replyData;
  
    // If already playing or recording then stop
    //
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO )
    {
        stop();
        return;
    }

    // This check may throw an exception
    checkSoundSystemStatus();

    // Align Instrument lists and send initial program changes
    //
    preparePlayback();

    // make sure we toggle the play button
    // 
    if (m_transport->PlayButton->state() == QButton::Off)
        m_transport->PlayButton->toggle();

    // write the start position argument to the outgoing stream
    //
    QDataStream streamOut(data, IO_WriteOnly);

    if (comp.getTempo() == 0)
    {
        comp.setDefaultTempo(120.0);

        cout << "SequenceManager::play()"
             << " - setting Tempo to Default value of 120.000"
             << endl;
    }
    else
    {
        cout << "SequenceManager::play() - starting to play"
             <<  endl;
    }

    // set the tempo in the transport
    m_transport->setTempo(comp.getTempo());

    // The arguments for the Sequencer
    RealTime startPos = comp.getElapsedRealTime(comp.getPosition());

    // If we're looping then jump to loop start
    if (comp.isLooping())
        startPos = comp.getElapsedRealTime(comp.getLoopStart());

    // playback start position
    streamOut << startPos.sec;
    streamOut << startPos.usec;

    // playback latency
    streamOut << m_playLatency.sec;
    streamOut << m_playLatency.usec;

    // fetch latency
    streamOut << m_fetchLatency.sec;
    streamOut << m_fetchLatency.usec;

    // read ahead slice
    streamOut << m_readAhead.sec;
    streamOut << m_readAhead.usec;

    // Send Play to the Sequencer
    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "play(long int, long int, long int, long int, long int, long int, long int, long int)",
                                  data, replyType, replyData))
    {
        // failed - pop up and disable sequencer options
        m_transportStatus = STOPPED;
        throw(i18n("Playback failed to contact Rosegarden sequencer"));
    }
    else
    {
        // ensure the return type is ok
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
  
        if (result)
        {
            // completed successfully 
            m_transportStatus = STARTING_TO_PLAY;
        }
        else
        {
            m_transportStatus = STOPPED;
            throw(i18n("Failed to start playback"));
        }
    }
}

void
SequenceManager::stop()
{
    Composition &comp = m_doc->getComposition();

    if (m_transportStatus == STOPPED)
    {
        m_doc->setPointerPosition(0);
        return;
    }

    // toggle the metronome button according to state
    m_transport->MetronomeButton->setOn(comp.usePlayMetronome());

    // if we're recording MIDI then tidy up the recording Segment
    if (m_transportStatus == RECORDING_MIDI)
        m_doc->stopRecordingMidi();


    QByteArray data;

    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "stop()", data))
    {
        // failed - pop up and disable sequencer options
        throw(i18n("Failed to contact Rosegarden sequencer"));
    }

    // always untoggle the play button at this stage
    //
    if (m_transport->PlayButton->state() == QButton::On)
        m_transport->PlayButton->toggle();

    if (m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO)
    {
        // untoggle the record button
        //
        if (m_transport->RecordButton->state() == QButton::On)
            m_transport->RecordButton->toggle();

        cout << "SequenceManager::stop() - stopped recording" << endl;
    }
    else
    {
        cout << "SequenceManager::stop() - stopped playing" << endl;
    }

    // ok, we're stopped
    //
    m_transportStatus = STOPPED;
}

// Jump to previous bar
//
void
SequenceManager::rewind()
{
    Rosegarden::Composition &composition = m_doc->getComposition();

    timeT position = composition.getPosition();
    
    // want to cope with bars beyond the actual end of the piece
    timeT newPosition = composition.getBarRangeForTime(position - 1).first;

    m_doc->setPointerPosition(newPosition);
}


// Jump to next bar
//
void
SequenceManager::fastforward()
{
    Rosegarden::Composition &composition = m_doc->getComposition();

    timeT position = composition.getPosition() + 1;
    timeT newPosition = composition.getBarRangeForTime(position).second;

    // we need to work out where the trackseditor finishes so we
    // don't skip beyond it.  Generally we need extra-Composition
    // non-destructive start and end markers for the piece.
    //
    m_doc->setPointerPosition(newPosition);

}


// This method is a callback from the Sequencer to update the GUI
// with state change information.  The GUI requests the Sequencer
// to start playing or to start recording and enters a pending
// state (see rosegardendcop.h for TransportStatus values).
// The Sequencer replies when ready with it's status.  If anything
// fails then we default (or try to default) to STOPPED at both
// the GUI and the Sequencer.
//
void
SequenceManager::notifySequencerStatus(TransportStatus status)
{
    // for the moment we don't do anything fancy
    m_transportStatus = status;
}


void
SequenceManager::sendSequencerJump(const Rosegarden::RealTime &time)
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);
    streamOut << time.sec;
    streamOut << time.usec;

    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "jumpTo(long int, long int)",
                                  data))
    {
      // failed - pop up and disable sequencer options
      m_transportStatus = STOPPED;
      throw(i18n("Failed to contact Rosegarden sequencer"));
    }

    return;
}



// Called when we want to start recording from the GUI.
// This method tells the sequencer to start recording and
// from then on the sequencer returns MappedCompositions
// to the GUI via the "processRecordedMidi() method -
// also called via DCOP
//
//

void
SequenceManager::record()
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Studio &studio = m_doc->getStudio();

    // if already recording then stop
    //
    if (m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO)
    {
        stop();
        return;
    }

    // Get the record track and check the Instrument type
    int rID = comp.getRecordTrack();
    Rosegarden::InstrumentId inst = comp.getTrackByIndex(rID)->getInstrument();

    // If no matching record instrument
    //
    if (studio.getInstrumentById(inst) == 0)
    {
        m_transport->RecordButton->setDown(false);
        throw(i18n("No Record instrument selected"));
    }


    // may throw an exception
    checkSoundSystemStatus();

    // toggle the Metronome button if it's in use
    //
    m_transport->MetronomeButton->setOn(comp.useRecordMetronome());

    // Adjust backwards by the bar count in
    //
    int startBar = comp.getBarNumber(comp.getPosition());
    startBar -= comp.getCountInBars();
    m_doc->setPointerPosition(comp.getBarRange(startBar).first);


    // Some locals
    //
    TransportStatus recordType;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;

    switch (studio.getInstrumentById(inst)->getType())
    {
        case Rosegarden::Instrument::Midi:
            recordType = STARTING_TO_RECORD_MIDI;
            cout << "SequenceManager::record() - starting to record MIDI"
                 << endl;
            break;

        case Rosegarden::Instrument::Audio:
            recordType = STARTING_TO_RECORD_AUDIO;
            cout << "SequenceManager::record() - starting to record Audio"
                 << endl;
            break;

        default:
            cout << "SequenceManager::record() - unrecognised instrument type"
                 << endl;
            return;
            break;
    }

    // make sure we toggle the record button and play button
    // 
    if (m_transport->RecordButton->state() == QButton::Off)
        m_transport->RecordButton->toggle();

    if (m_transport->PlayButton->state() == QButton::Off)
        m_transport->PlayButton->toggle();

    // write the start position argument to the outgoing stream
    //
    QDataStream streamOut(data, IO_WriteOnly);

    if (comp.getTempo() == 0)
    {
        cout <<
         "SequenceManager::play() - setting Tempo to Default value of 120.000"
          << endl;
        comp.setDefaultTempo(120.0);
    }
    else
    {
        cout << "SequenceManager::record() - starting to record" << endl;
    }

    // set the tempo in the transport
    //
    m_transport->setTempo(comp.getTempo());

    // The arguments for the Sequencer  - record is similar to playback,
    // we must being playing to record
    //

    Rosegarden::RealTime startPos =comp.getElapsedRealTime(comp.getPosition());

    // playback start position
    streamOut << startPos.sec;
    streamOut << startPos.usec;

    // playback latency
    streamOut << m_playLatency.sec;
    streamOut << m_playLatency.usec;

    // fetch latency
    streamOut << m_fetchLatency.sec;
    streamOut << m_fetchLatency.usec;

    // read ahead slice
    streamOut << m_readAhead.sec;
    streamOut << m_readAhead.usec;

    // record type
    streamOut << (int)recordType;

    // Send Play to the Sequencer
    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "record(long int, long int, long int, long int, long int, long int, long int, long int, int)",
                                  data, replyType, replyData))
    {
        // failed - pop up and disable sequencer options
        m_transportStatus = STOPPED;
        throw(i18n("Failed to contact Rosegarden sequencer"));
    }
    else
    {
        // ensure the return type is ok
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
  
        if (result)
        {
            // completed successfully 
            m_transportStatus = recordType;
        }
        else
        {
            m_transportStatus = STOPPED;
            throw(i18n("Failed to start recording"));
        }
    }

}



// This method accepts an incoming MappedComposition and goes about
// inserting it into the Composition and updating the display to show
// what has been recorded and where.
//
//
void
SequenceManager::processRecordedMidi(const MappedComposition &mC)
{
    if (mC.size() > 0)
    {
        Rosegarden::MappedComposition::iterator i;

        // send all events to the MIDI in label
        //
        for (i = mC.begin(); i != mC.end(); ++i )
            m_transport->setMidiInLabel(*i);

    }

    // send any recorded Events to a Segment for storage and display
    //
    m_doc->insertRecordedMidi(mC);

}



// Process unexpected MIDI events at the GUI - send them to the Transport
// or to a MIDI mixer for display purposes only.  Useful feature to enable
// the musician to prove to herself quickly that the MIDI input is still
// working.
//
//
void
SequenceManager::processAsynchronousMidi(const MappedComposition &mC)
{
    if (mC.size())
    {
        Rosegarden::MappedComposition::iterator i;

        // send all events to the MIDI in label
        //
        for (i = mC.begin(); i != mC.end(); ++i )
            m_transport->setMidiInLabel(*i);
    }
}


void
SequenceManager::rewindToBeginning()
{
    cout << "SequenceManager::rewindToBeginning()" << endl;
    m_doc->setPointerPosition(0);
}


void
SequenceManager::fastForwardToEnd()
{
    cout << "SequenceManager::fastForwardToEnd()" << endl;

    Composition &comp = m_doc->getComposition();
    m_doc->setPointerPosition(comp.getDuration());
}


// Called from the LoopRuler (usually a double click)
// to set position and start playing
//
void
SequenceManager::setPlayStartTime(const timeT &time)
{

    // If already playing then stop
    //
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO )
    {
        stop();
        return;
    }
    
    // otherwise off we go
    //
    m_doc->setPointerPosition(time);
    play();
}


void
SequenceManager::setLoop(const timeT &lhs, const timeT &rhs)
{
    // Let the sequencer know about the loop markers
    //
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    Rosegarden::RealTime loopStart =
            m_doc->getComposition().getElapsedRealTime(lhs);
    Rosegarden::RealTime loopEnd =
            m_doc->getComposition().getElapsedRealTime(rhs);

    streamOut << loopStart.sec;
    streamOut << loopStart.usec;
    streamOut << loopEnd.sec;
    streamOut << loopEnd.usec;
  
    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                 ROSEGARDEN_SEQUENCER_IFACE_NAME,
                 "setLoop(long int, long int, long int, long int)", data))
    {
        // failed - pop up and disable sequencer options
        throw(i18n("Failed to contact Rosegarden sequencer"));
    }
}

void
SequenceManager::checkSoundSystemStatus()
{
    QByteArray data;
    QCString replyType;
    QByteArray replyData;

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "getSoundSystemStatus()",
                                  data, replyType, replyData))
    {
        // failed - pop up and disable sequencer options
        throw(i18n("Failed to contact Rosegarden sequencer"));
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
        m_soundSystemStatus = (Rosegarden::SoundSystemStatus) result;

        switch(m_soundSystemStatus)
        {
            case Rosegarden::MIDI_AND_AUDIO_SUBSYS_OK:
                // we're fine
                break;

            case Rosegarden::MIDI_SUBSYS_OK:
                throw(i18n("Audio subsystem has failed to initialise"));
                break;

            case Rosegarden::AUDIO_SUBSYS_OK:
                throw(i18n("MIDI subsystem has failed to initialise"));
                break;
                
            case Rosegarden::NO_SEQUENCE_SUBSYS:
            default:
                throw(i18n("MIDI and Audio subsystems have failed to initialise"));
                break;
        }
    }
}


// Insert metronome clicks into the global MappedComposition that
// will be returned as part of the slice fetch from the Sequencer.
//
//
void
SequenceManager::insertMetronomeClicks(const timeT &sliceStart,
                                        const timeT &sliceEnd)
{
    Composition &comp = m_doc->getComposition();

    // If neither metronome is armed and we're not playing or recording
    // then don't sound the metronome
    //
    if(!((m_transportStatus == PLAYING || m_transportStatus == STARTING_TO_PLAY)
         && comp.usePlayMetronome()) &&
       !((m_transportStatus == RECORDING_MIDI ||
          m_transportStatus == RECORDING_AUDIO ||
          m_transportStatus == STARTING_TO_RECORD_MIDI ||
          m_transportStatus == STARTING_TO_RECORD_AUDIO)
          && comp.useRecordMetronome()))
        return;

    std::pair<timeT, timeT> barStart =
        comp.getBarRange(comp.getBarNumber(sliceStart));
    std::pair<timeT, timeT> barEnd =
        comp.getBarRange(comp.getBarNumber(sliceEnd));

    // The slice can straddle a bar boundary so check
    // in both bars for the marker
    //
    if (barStart.first >= sliceStart && barStart.first <= sliceEnd)
    {
        MappedEvent *me =
            new MappedEvent(m_metronomeInstrument,
                            m_metronomePitch,
                            m_metronomeBarVelocity,
                            comp.getElapsedRealTime(barStart.first),
                            m_metronomeDuration);
        m_mC.insert(me);
    }
    else if (barEnd.first >= sliceStart && barEnd.first <= sliceEnd)
    {
        MappedEvent *me =
            new MappedEvent(m_metronomeInstrument,
                            m_metronomePitch,
                            m_metronomeBarVelocity,
                            comp.getElapsedRealTime(barEnd.first),
                            m_metronomeDuration);
        m_mC.insert(me);
    }

    // Is this solution for the beats bulletproof?  I'm not so sure.
    //
    bool isNew;
    TimeSignature timeSig =
        comp.getTimeSignatureInBar(comp.getBarNumber(sliceStart), isNew);

    for (int i = barStart.first + timeSig.getBeatDuration();
             i < barStart.second;
             i += timeSig.getBeatDuration())
    {
        if (i >= sliceStart && i <= sliceEnd)
        {
            MappedEvent *me =new MappedEvent(m_metronomeInstrument,
                                             m_metronomePitch,
                                             m_metronomeBeatVelocity,
                                             comp.getElapsedRealTime(i),
                                             m_metronomeDuration);
            m_mC.insert(me);
        }
    }
}

// Send Instrument list to Sequener and ensure that initial program
// changes follow them.  Sending the instruments ensures that we have
// channels available on the Sequencer and then the program changes
// are sent to those specific channel (referenced by Instrument ID)
//
// 
void
SequenceManager::preparePlayback()
{
    Rosegarden::Studio &studio = m_doc->getStudio();
    Rosegarden::InstrumentList list = studio.getInstruments();
    Rosegarden::MappedComposition mC;
    Rosegarden::MappedEvent *mE;

    // Send the MappedInstruments (minimal Instrument information
    // required for Performance) to the Sequencer
    //
    InstrumentList::iterator it = list.begin();
    for (; it != list.end(); it++)
    {
        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        streamOut << (*it)->getType();
        streamOut << (*it)->getMidiChannel();
        streamOut << (*it)->getID();

        if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                      ROSEGARDEN_SEQUENCER_IFACE_NAME,
                 "setMappedInstrument(int, unsigned char, unsigned int)", data))
        {
            throw(i18n("Failed to contact Rosegarden sequencer"));
        }

        // Send program changes for MIDI Instruments
        //
        if ((*it)->getType() == Instrument::Midi)
        {
            if ((*it)->sendsBankSelect())
            {
                mE = new MappedEvent((*it)->getID(),
                                     Rosegarden::MappedEvent::MidiController,
                                     (MidiByte)0,
                                     (*it)->getMSB());
                mC.insert(mE);

                mE = new MappedEvent((*it)->getID(),
                                     Rosegarden::MappedEvent::MidiController,
                                     (MidiByte)32,
                                     (*it)->getLSB());
                mC.insert(mE);
            }

            if ((*it)->sendsProgramChange())
            {
                mE = new MappedEvent((*it)->getID(),
                                     Rosegarden::MappedEvent::MidiProgramChange,
                                     (*it)->getProgramChange());
                mC.insert(mE);
            }
        }
    }

    // Send the MappedComposition if it's got anything in it
    sendMappedComposition(mC);
}

void
SequenceManager::sendMappedComposition(const Rosegarden::MappedComposition &mC)
{
    if (mC.size() == 0)
        return;

    QCString replyType;
    QByteArray replyData;

    MappedComposition::iterator it = mC.begin();

    // scan output MappedComposition
    for (; it != mC.end(); it++)
    {
        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        /*
        cout << "PC TYPE = " << (*it)->getType() << endl;
        cout << "PC ID = " << (*it)->getInstrument() << endl;
        cout << "PC PC = " << (int)(*it)->getPitch() << endl << endl;
        */

        streamOut << (*it)->getInstrument();
        streamOut << (*it)->getType();
        streamOut << (*it)->getData1();
        streamOut << (*it)->getData2();
        streamOut << (*it)->getEventTime().sec;
        streamOut << (*it)->getEventTime().usec;
        streamOut << (*it)->getDuration().sec;
        streamOut << (*it)->getDuration().usec;
        streamOut << (*it)->getAudioStartMarker().sec;
        streamOut << (*it)->getAudioStartMarker().usec;

        if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                      ROSEGARDEN_SEQUENCER_IFACE_NAME,
          "processMappedEvent(unsigned int, int, unsigned char, unsigned char, long int, long int, long int, long int, long int, long int)", data))
        {
            throw(i18n("Failed to contact Rosegarden sequencer"));
        }
    }
}

void
SequenceManager::sendMappedEvent(Rosegarden::MappedEvent *mE)
{
    Rosegarden::MappedComposition mC;

    mC.insert(mE);
    sendMappedComposition(mC);
}





}

