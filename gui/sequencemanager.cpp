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

#include <qbutton.h>
#include <dcopclient.h>
#include <qpushbutton.h>
#include <qcursor.h>
#include <qtimer.h>

#include <klocale.h>
#include <kconfig.h>

#include "ktmpstatusmsg.h"
#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "rosegardentransportdialog.h"
#include "rosegardenguiview.h"
#include "sequencemanager.h"
#include "SegmentPerformanceHelper.h"

using std::cout;
using std::cerr;
using std::endl;

namespace Rosegarden
{

SequenceManager::SequenceManager(RosegardenGUIDoc *doc,
                                 RosegardenTransportDialog *transport):
    m_doc(doc),
    m_transportStatus(STOPPED),
    m_soundSystemStatus(Rosegarden::NO_SEQUENCE_SUBSYS),
    m_transport(transport),
    m_sendStop(false)
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
                                   const Rosegarden::RealTime &sliceEnd,
                                   bool firstFetch)
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

    KConfig* config = kapp->config();
    config->setGroup("Latency Options");
    int jackSec = config->readLongNumEntry("jackplaybacklatencysec", 0);
    int jackUSec = config->readLongNumEntry("jackplaybacklatencyusec", 0);
    RealTime jackPlaybackLatency(jackSec, jackUSec);

    timeT jackPlaybackLatencyTimeT = comp.getElapsedTimeForRealTime(sliceStart)
        - comp.getElapsedTimeForRealTime(sliceStart - jackPlaybackLatency);

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
	timeT segmentEndTime = (*it)->getEndMarkerTime();
	timeT segmentDuration = segmentEndTime - segmentStartTime;

        track = comp.getTrackByIndex((*it)->getTrack());

        // check to see if track actually exists
        //
        if (track == 0)
            continue;

        instrument = studio.getInstrumentById(track->getInstrument());

        if (instrument == 0)
            continue;

        // Skip if track is muted
        if (track->isMuted())
            continue;

        // If soloing then skip until we get the solo track
        if (comp.isSolo() && track->getId() != comp.getSelectedTrack())
            continue;

        // With audio segments we can cheat and look further
        // ahead into time than this slice - this is because
        // we have the JACK/Audio latency to interpret and
        // this could very well mean we should start audio
        // events more than a single slice away from where
        // they should start.
        //
        // More mind bending timing issues follow.  Don't
        // mess with any of this unless you're really unsure
        // what you're doing.
        //
        //
        if ((*it)->getType() == Rosegarden::Segment::Audio)
        {

            // An Audio event has three time parameters associated
            // with it.  The start of the Segment is when the audio
            // event should start.  The StartTime is how far into
            // the sample the playback should commence and the
            // EndTime is how far into the sample playback should
            // stop.  We make sure that an overlapping duration (part
            // of a sample) isn't cast away.
            // 
            // The EndTime can of course be worked out from the StartTime
            // and segment duration so we certainly have something
            // redundant here.
            //

            // Adjust for JACK latency for Audio segments
            //
            segmentStartTime -= jackPlaybackLatencyTimeT;

            Rosegarden::RealTime segmentStart =
                comp.getElapsedRealTime(segmentStartTime);

            Rosegarden::RealTime segmentEnd =
                comp.getElapsedRealTime(segmentEndTime);

            Rosegarden::RealTime audioStart = (*it)->getAudioStartTime();
            Rosegarden::RealTime audioDuration =
                (*it)->getAudioEndTime() - audioStart;

            Rosegarden::RealTime audioEnd = (*it)->getAudioEndTime() -
                                            (*it)->getAudioStartTime() +
                                            segmentStart;

            // Oh this freak fuction just does my head - take a guess,
            // move a clause - see if it still works.  Bet it doesn't.
            //
            if ((segmentStartTime < sliceStartElapsed ||
                 segmentStartTime > sliceEndElapsed) &&
                 firstFetch == false)
            {
                if ((*it)->isRepeating())
                {
                    Rosegarden::RealTime repeatEnd =
                        comp.getElapsedRealTime((*it)->getRepeatEndTime());

                    // If we've stopped repeating
                    if (sliceStart > repeatEnd)
                        continue;
                    
                    // Otherwise see if repeat event lies within
                    // this slice
                    //
                    Rosegarden::RealTime segmentDuration =
                        segmentEnd - segmentStart;

                    while (segmentStart < sliceStart)
                    {
                        segmentStart = segmentStart + segmentDuration;
                    }

                    if (segmentStart > sliceEnd)
                        continue;
                }
                else
                    continue;
            }

            // If we're starting in the middle of a Segment then
            // make an adjustment to start time and duration.
            //
            if (firstFetch == true &&
                segmentStartTime <= sliceStartElapsed &&
                audioEnd >= sliceStart)
            {
                // reset audioStart and duration to shorter values
                Rosegarden::RealTime moveTime = comp.
                    getElapsedRealTime(sliceStartElapsed - segmentStartTime);

                //cout << "DURATION was   " << duration << endl;
                audioStart = audioStart + moveTime;
                audioDuration = audioDuration - moveTime;
                //cout << "DURATION is now " << duration << endl;
            }
            else
            {
                // bail out if we're first fetching and the above
                // condition doesn't hold.
                if (firstFetch)
                    continue;
            }

            // Insert Audio event
            Rosegarden::MappedEvent *me =
                    new Rosegarden::MappedEvent(track->getInstrument(),
                                                (*it)->getAudioFileId(),
                                                segmentStart,
                                                audioDuration,
                                                audioStart);
            m_mC.insert(me);
            continue; // next Segment
        }

        // Skip the Segment if it starts too late to be of
        // interest to our slice.
        if (segmentStartTime > sliceEndElapsed)
            continue;

	// Skip the Segment if it ends too early to be of
	// interest and it's not repeating.
	if (segmentEndTime <= sliceStartElapsed && !(*it)->isRepeating())
	    continue;

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
	int repeatNo;
       
        if (segmentDuration != 0)
            repeatNo = (seekStartTime - segmentStartTime) / segmentDuration;
        else
            repeatNo = 0;

	// Locate a suitable starting iterator
	//
	seekStartTime = seekStartTime - repeatNo * segmentDuration;
	Segment::iterator i0 = (*it)->findTime(seekStartTime);

        // start from the beginning if it's the first fetch
        if (firstFetch)
            i0 = (*it)->begin();

	// Now, we end at the slice's end, except where we're a
	// repeating segment followed by another segment on the same
	// track when we end at that other segment's start time
	// 
	timeT seekEndTime = sliceEndElapsed;
	if ((*it)->isRepeating()) seekEndTime = (*it)->getRepeatEndTime();

	// No ending condition -- we do all that in the initial
	// conditional within the loop, and subsequent breaks
	//
        for (Segment::iterator j = i0; ; ++j)
        {
	    if (!(*it)->isBeforeEndMarker(j))
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

            // Continue if the time falls before the slice (can happen
	    // through rounding error etc).  Use absolute times here!
            // RealTimes again give us rounding problems that can drop
            // events.
	    // 
            if (playTime < sliceStartElapsed && firstFetch == false)
                continue;

            // Escape if we're beyond the slice
	    // 
            if (playTime >= sliceEndElapsed)
                break;

	    // Find the performance duration, i.e. taking into account
	    // any ties etc that this note may have
	    // 
	    duration = helper.getRealSoundingDuration(j);

	    // No duration?  Probably in a tied series, but not as first note
	    //
	    if (duration == Rosegarden::RealTime(0, 0))
		continue;

	    // Convert to real-time
	    // 
            eventTime = comp.getElapsedRealTime(playTime);

            // If we've got a note overlapping with the start of this slice
            // but it doesn't actually start in the slice then we still
            // need to play it but with reduced duration.
            //
            if (firstFetch == true)
            {
                // The "<=" makes sure we don't forget items that start
                // exactly _on_ the slice which are now valid.  Also 
                // test that these events should actually sound by 
                // testing there finish time is greater than the start
                // of the slice.
                //
                if (playTime <= sliceStartElapsed &&
                    playTime + helper.getSoundingDuration(j)
                                                    >= sliceStartElapsed)
                {
                    duration = duration - (sliceStart - eventTime);
                    eventTime = sliceStart;
                }
                else
                    continue;
            }


	    // Add any performance delay.  Note that simply adding
	    // comp.getElapsedRealTime((*it)->getDelay()) would fail to
	    // take tempo changes into account correctly
	    // 
	    eventTime = eventTime + comp.getRealTimeDifference
		(playTime, playTime + (*it)->getDelay());


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
        stopping();
        return;
    }

    // This check may throw an exception
    checkSoundSystemStatus();

    // Align Instrument lists and send initial program changes
    //
    preparePlayback();

    // Send audio latencies
    //
    //sendAudioLatencies();

    // make sure we toggle the play button
    // 
    m_transport->PlayButton->setOn(true);

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

    KConfig* config = kapp->config();
    config->setGroup("Latency Options");

    Rosegarden::Configuration& docConfig = m_doc->getConfiguration();

    // playback start position
    streamOut << startPos.sec;
    streamOut << startPos.usec;

    // playback latency
    streamOut << config->readLongNumEntry("playbacklatencysec", 0);
    streamOut << config->readLongNumEntry("playbacklatencyusec", 100000);

    // fetch latency
    RealTime fetchLatency = docConfig.get<RealTimeT>("fetchlatency");
    streamOut << fetchLatency.sec;
    streamOut << fetchLatency.usec;

    // read ahead slice
    streamOut << config->readLongNumEntry("readaheadsec", 0);
    streamOut << config->readLongNumEntry("readaheadusec", 40000);

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
SequenceManager::stopping()
{
    // Do this here rather than in stop() to avoid any potential
    // race condition (we use setPointerPosition() during stop()).
    //
    if (m_transportStatus == STOPPED)
    {
        m_doc->setPointerPosition(0);
        return;
    }

    m_sendStop = true;
}

// We did try to make the sequencer "suspend" while we performed
// the stop() call on it but it even got wise to this.  Now we
// set a flag here in stopping() and wait until it's clear to
// proceed (just after a getSequencerSlice() has completed).
//
// Appears to work until the next time.  Overall the DCOP contention
// mechanism appears to be non-existent.  I'm sure there's probably
// a proper way of handling this situation but I've not come across
// it yet.
//
//
void
SequenceManager::stop()
{
    // If the sendStop flag isn't set we can't stop yet
    //
    if (m_sendStop == false) return;

    // Toggle off the buttons - first record
    //
    if ((m_transportStatus == RECORDING_MIDI ||
         m_transportStatus == RECORDING_AUDIO))
    {
        if (m_transport->RecordButton->state() == QButton::On)
            m_transport->RecordButton->setOn(false);

        // Metronome
        //
        if (m_transport->MetronomeButton->state() == QButton::On)
            m_transport->MetronomeButton->setOn(false);
    }

    // Now playback
    m_transport->PlayButton->setOn(false);

    // "call" the sequencer with a stop so we get a synchronous
    // response - then we can fiddle about with the audio file
    // without worrying about the sequencer causing problems
    // with access to the same audio files.
    //
    //TransportStatus recordType;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;

    // wait cursor
    //
    QApplication::setOverrideCursor(QCursor(Qt::waitCursor));

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "stop()",
                                  data, replyType, replyData, true))
    {
        // failed - pop up and disable sequencer options
        throw(i18n("Failed to contact Rosegarden sequencer"));
    }

    // restore
    QApplication::restoreOverrideCursor();

    // if we're recording MIDI or Audio then tidy up the recording Segment
    if (m_transportStatus == RECORDING_MIDI)
    {
        m_doc->stopRecordingMidi();
        cout << "SequenceManager::stop() - stopped recording MIDI" << endl;
    }

    if (m_transportStatus == RECORDING_AUDIO)
    {
        m_doc->stopRecordingAudio();
        cout << "SequenceManager::stop() - stopped recording audio" << endl;
    }

    // always untoggle the play button at this stage
    //
    if (m_transport->PlayButton->state() == QButton::On)
    {
        cout << "SequenceManager::stop() - stopped playing" << endl;
    }

    // ok, we're stopped
    //
    m_transportStatus = STOPPED;
    m_sendStop = false;
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

    if (newPosition < composition.getStartMarker())
        newPosition = composition.getStartMarker();

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

    // Don't skip past end marker
    //
    if (newPosition > composition.getEndMarker())
        newPosition = composition.getEndMarker();

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
    KConfig* config = kapp->config();
    config->setGroup("General Options");

    // if already recording then stop
    //
    if (m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO)
    {
        stopping();
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

    // If we are looping then jump to start of loop and start recording,
    // if we're not take off the number of count-in bars and start 
    // recording.
    //
    if(comp.isLooping())
        m_doc->setPointerPosition(comp.getLoopStart());
    else
    {
        int startBar = comp.getBarNumber(comp.getPosition());
        startBar -= config->readUnsignedNumEntry("countinbars", 2);
        m_doc->setPointerPosition(comp.getBarRange(startBar).first);
    }

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

    // set the buttons
    m_transport->RecordButton->setOn(true);
    m_transport->PlayButton->setOn(true);

    // write the start position argument to the outgoing stream
    //
    QDataStream streamOut(data, IO_WriteOnly);

    if (comp.getTempo() == 0)
    {
        cout << "SequenceManager::play() - setting Tempo to Default value of 120.000"
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
    // we must being playing to record.
    //
    Rosegarden::RealTime startPos =comp.getElapsedRealTime(comp.getPosition());
    Rosegarden::Configuration &docConfig = m_doc->getConfiguration();

    // playback start position
    streamOut << startPos.sec;
    streamOut << startPos.usec;

    // playback latency
    streamOut << config->readLongNumEntry("playbacklatencysec", 0);
    streamOut << config->readLongNumEntry("playbacklatencyusec", 100000);

    // fetch latency
    RealTime fetchLatency = docConfig.get<RealTimeT>("fetchlatency");
    streamOut << fetchLatency.sec;
    streamOut << fetchLatency.usec;

    // read ahead slice
    streamOut << config->readLongNumEntry("readaheadsec", 0);
    streamOut << config->readLongNumEntry("readaheadusec", 40000);

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
// For ALSA we reflect the events straight back out to the instrument
// we're currently playing just like in processAsynchronousMidi.
// aRts does it's own MIDI thru which we don't bother we worrying
// about for the moment.
//
//
void
SequenceManager::processRecordedMidi(const MappedComposition &mC)
{
    processAsynchronousMidi(mC);

    // Send any recorded Events to a Segment for storage and display.
    // We have to send the transport status because this method is
    // called asynchronously from the sequencer and the calls below
    // can create a new recording SegmentItem on the canvas if we
    // don't check that recording is coming to a close (or has already
    // been stopped).
    //
    //
    m_doc->insertRecordedMidi(mC, m_transportStatus);

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
        Rosegarden::MappedComposition retMC;
        Rosegarden::Composition &comp = m_doc->getComposition();
        Rosegarden::Track *track =
                  comp.getTrackByIndex(comp.getSelectedTrack());
        Rosegarden::InstrumentId id = track->getInstrument();

        // send all events to the MIDI in label
        //
        for (i = mC.begin(); i != mC.end(); ++i )
        {
            m_transport->setMidiInLabel(*i);

            // Skip all audio events
            //
            if ((*i)->getType() >= Rosegarden::MappedEvent::Audio)
            {
                if ((*i)->getType() == Rosegarden::MappedEvent::AudioStopped)
                {
                    cout << "AUDIO FILE ID = "
                         << (*i)->getData1()
                         << " - FILE STOPPED - " 
                         << "INSTRUMENT = "
                         << (*i)->getInstrument()
                         << endl;
                }

                if ((*i)->getType() == Rosegarden::MappedEvent::AudioLevel)
                    sendAudioLevel(*i);

                continue;
            }
#ifdef HAVE_ALSA
            (*i)->setInstrument(id);
#endif
            retMC.insert(new Rosegarden::MappedEvent(*i));
        }

#ifdef HAVE_ALSA
        // MIDI thru implemented at this layer for ALSA for
        // the moment.  aRts automatically does MIDI through,
        // this does it to the currently selected instrument.
        //
        sendMappedComposition(retMC);
#endif 
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
        stopping();
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
                // for the moment we're fine
                //
                //throw(i18n("Audio subsystem has failed to initialise"));
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
    Studio &studio = m_doc->getStudio();
    Configuration &config = m_doc->getConfiguration();

    MidiMetronome *metronome = studio.getMetronome();

    // Create a default metronome if we haven't loaded one
    //
    if(metronome == 0)
    {
        metronome = new MidiMetronome();
        metronome->pitch = config.get<Int>("metronomepitch");

        // Default instrument is the first possible instrument
        //
        metronome->instrument = Rosegarden::SystemInstrumentBase;
    }

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
            new MappedEvent(metronome->instrument,
                            metronome->pitch,
                            config.get<Int>("metronomebarvelocity"),
                            comp.getElapsedRealTime(barStart.first),
                            config.get<RealTimeT>("metronomeduration"));
        m_mC.insert(me);
    }
    else if (barEnd.first >= sliceStart && barEnd.first <= sliceEnd)
    {
        MappedEvent *me =
            new MappedEvent(metronome->instrument,
                            metronome->pitch,
                            config.get<Int>("metronomebarvelocity"),
                            comp.getElapsedRealTime(barEnd.first),
                            config.get<RealTimeT>("metronomeduration"));
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
            MappedEvent *me =new MappedEvent(metronome->instrument,
                                             metronome->pitch,
                                             config.get<Int>("metronomebeatvelocity"),
                                             comp.getElapsedRealTime(i),
                                             config.get<RealTimeT>("metronomeduration"));
            m_mC.insert(me);
        }
    }
}

// Send Instrument list to Sequencer and ensure that initial program
// changes follow them.  Sending the instruments ensures that we have
// channels available on the Sequencer and then the program changes
// are sent to those specific channel (referenced by Instrument ID)
//
// 
void
SequenceManager::preparePlayback()
{
    Rosegarden::Studio &studio = m_doc->getStudio();
    Rosegarden::InstrumentList list = studio.getAllInstruments();
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
        streamOut << (*it)->getId();

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
            // send bank select always before program change
            //
            if ((*it)->sendsBankSelect())
            {
                mE = new MappedEvent((*it)->getId(),
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_BANK_MSB,
                                     (*it)->getMSB());
                mC.insert(mE);

                mE = new MappedEvent((*it)->getId(),
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_BANK_LSB,
                                     (*it)->getLSB());
                mC.insert(mE);
            }

            // send program change
            //
            if ((*it)->sendsProgramChange())
            {
                mE = new MappedEvent((*it)->getId(),
                                     Rosegarden::MappedEvent::MidiProgramChange,
                                     (*it)->getProgramChange());
                mC.insert(mE);
            }

            // send pan
            //
            if ((*it)->sendsPan())
            {
                mE = new MappedEvent((*it)->getId(),
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_PAN,
                                     (*it)->getPan());
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
    //
    for (; it != mC.end(); it++)
    {
        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        // Use new MappedEvent interface
        //
        streamOut << (*it);
        if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                      ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                      "processMappedEvent(Rosegarden::MappedEvent)",
                                      data))
        {
            throw(i18n("Failed to contact Rosegarden sequencer"));
        }

        /*
        cout << "PC TYPE = " << (*it)->getType() << endl;
        cout << "PC ID = " << (*it)->getInstrument() << endl;
        cout << "PC PC = " << (int)(*it)->getPitch() << endl << endl;
        */

        /*
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
        */
    }
}

void
SequenceManager::sendMappedEvent(Rosegarden::MappedEvent *mE)
{
    Rosegarden::MappedComposition mC;

    /*
    cout << "ID = " << mE->getInstrument() << endl;
    cout << "TYPE = " << mE->getType() << endl;
    cout << "D1 = " << (int)mE->getData1() << endl;
    cout << "D2 = " << (int)mE->getData2() << endl;
    */

    mC.insert(mE);
    sendMappedComposition(mC);

}


void
SequenceManager::processRecordedAudio(const Rosegarden::RealTime &time)
{
    m_doc->insertRecordedAudio(time, m_transportStatus);
}


void
SequenceManager::sendAudioLevel(Rosegarden::MappedEvent *mE)
{
    RosegardenGUIView *v;

    for (v = m_doc->pViewList->first(); v != 0; v = m_doc->pViewList->next())
    {
        v->showVisuals(mE);
    }

}

}



