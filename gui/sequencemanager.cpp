// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#include "audiopluginmanager.h"
#include "ktmpstatusmsg.h"
#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "rosegardentransportdialog.h"
#include "rosegardenguiview.h"
#include "sequencemanager.h"
#include "SegmentPerformanceHelper.h"
#include "SoundDriver.h"
#include "MappedRealTime.h"
#include "studiocontrol.h"
#include "MidiDevice.h"
#include "widgets.h"

using std::cout;
using std::cerr;
using std::endl;

namespace Rosegarden
{

SequenceManager::SequenceManager(RosegardenGUIDoc *doc,
                                 RosegardenTransportDialog *transport):
    m_doc(doc),
    m_transportStatus(STOPPED),
    m_soundDriverStatus(NO_DRIVER),
    m_transport(transport),
    m_sendStop(false),
    m_lastRewoundAt(clock()),
    m_sliceFetched(true) // default to true (usually ignored)
{
}


SequenceManager::~SequenceManager()
{
} 

void SequenceManager::setDocument(RosegardenGUIDoc* doc)
{
    m_doc = doc;
}

RosegardenGUIDoc* SequenceManager::getDocument()
{
    return m_doc;
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
    {
        if (!m_sliceFetched) m_sliceFetched = true;
        return &m_mC; // return empty
    }

    timeT sliceStartElapsed =
              comp.getElapsedTimeForRealTime(m_mC.getStartTime());

    timeT sliceEndElapsed =
              comp.getElapsedTimeForRealTime(m_mC.getEndTime());

    timeT jackPlaybackLatency = comp.getElapsedTimeForRealTime(sliceStart)
        - comp.getElapsedTimeForRealTime(sliceStart - m_playbackAudioLatency);

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

        track = comp.getTrackById((*it)->getTrack());

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
            segmentStartTime -= jackPlaybackLatency;


            Rosegarden::RealTime segmentStart =
                comp.getElapsedRealTime(segmentStartTime);

            /*
            Rosegarden::RealTime segmentEnd =
                comp.getElapsedRealTime(segmentEndTime);
                */

            Rosegarden::RealTime audioStart = (*it)->getAudioStartTime();
            Rosegarden::RealTime audioDuration =
                (*it)->getAudioEndTime() - audioStart;

            Rosegarden::RealTime audioEnd = (*it)->getAudioEndTime() -
                                            (*it)->getAudioStartTime() +
                                            segmentStart;

            // I wonder how this algorithm works?  It's taken a few months
            // and a few rewrites to get it like this.  And now what?
            // 
            if (firstFetch)
            {
                // If we're starting in the middle of a Segment then
                // make an adjustment to start time and duration.
                //
                if (segmentStart < sliceStart)
                {
                    if (audioEnd < sliceStart && (*it)->isRepeating())
                    {
                        Rosegarden::RealTime repeatEnd =
                            comp.getElapsedRealTime((*it)->getRepeatEndTime());

                        // If we've stopped repeating
                        if (sliceStart > repeatEnd)
                            continue;
                    
                        while (segmentStart < sliceStart)
                            segmentStart = segmentStart + audioDuration;
    
                        if (segmentStart > sliceEnd)
                            continue;
                    }

                    // Reposition starting point within segment
                    //
                    Rosegarden::RealTime moveTime =
                        sliceStart - segmentStart;
                    segmentStart = sliceStart;
                    audioStart = audioStart + moveTime;
                    audioDuration = audioDuration - moveTime;

                }
                else if (segmentStart < sliceEnd ||
                         segmentStart == sliceStart)
                {
                    ; // play
                }
                else
                    continue;
            }
            else
            {
                if (segmentStart >= sliceStart &&
                    segmentStart < sliceEnd)
                {
                    ; // play
                }
                else if (segmentStart < sliceStart &&
                         (*it)->isRepeating())
                {
                    Rosegarden::RealTime repeatEnd =
                        comp.getElapsedRealTime((*it)->getRepeatEndTime());

                    // If we've stopped repeating
                    if (sliceStart > repeatEnd)
                        continue;
                    
                    // Otherwise see if repeat event lies within
                    // this slice
                    //
                    while (segmentStart < sliceStart)
                        segmentStart = segmentStart + audioDuration;

                    if (segmentStart > sliceEnd)
                        continue;
                }
                else
                    continue;
            }

            // Insert Audio event
            if (audioDuration > Rosegarden::RealTime(0, 0))
            {
                Rosegarden::MappedEvent *me =
                    new Rosegarden::MappedEvent(track->getInstrument(),
                                                (*it)->getAudioFileId(),
                                                segmentStart,
                                                audioDuration,
                                                audioStart);
                m_mC.insert(me);
            }

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
	Segment::iterator i0;

        // Start from the beginning if it's the first fetch
        //
        if (firstFetch)
            i0 = (*it)->begin();
        else 
            i0 = (*it)->findTime(seekStartTime);

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

            // Skip this event if it's a rest or a clef
            //
            if ((*j)->isa(Rosegarden::Note::EventRestType) ||
                (*j)->isa(Rosegarden::Clef::EventType))
                continue;


	    // Find the performance duration, i.e. taking into account
	    // any ties etc that this note may have
	    // 
	    duration = helper.getRealSoundingDuration(j);

	    // No duration and we're a note?  Probably in a tied
            // series, but not as first note
	    //
	    if (duration == Rosegarden::RealTime(0, 0) &&
                (*j)->isa(Rosegarden::Note::EventType))
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
                if (playTime < sliceStartElapsed)
                {
                    if (playTime + helper.getSoundingDuration(j)
                                                        >= sliceStartElapsed)
                    {
                        duration = duration - (sliceStart - eventTime);
                        eventTime = sliceStart;
                    }
                    else
                        continue;
                }
            }


	    // Add any performance delay.  Note that simply adding
	    // comp.getElapsedRealTime((*it)->getDelay()) would fail to
	    // take tempo changes into account correctly
	    // 
	    eventTime = eventTime +
		comp.getRealTimeDifference(playTime,
					   playTime + (*it)->getDelay()) +
		(*it)->getRealTimeDelay();

	    Rosegarden::MappedEvent *mE;
	    // Make mapped event
	    // 
	    mE = new Rosegarden::MappedEvent(track->getInstrument(),
                                             **j,
                                             eventTime,
                                             duration);

            // If the event doesn't translate to a MappedEvent type
            // then skip it.
            //
            if (mE->getType() == Rosegarden::MappedEvent::InvalidMappedEvent)
            {
                delete mE;
                continue;
            }

            // Do some more tweaking if we've got a note
            //
            if ((*j)->isa(Rosegarden::Note::EventType))
            {
	        // Add any performance transposition
	        // 
		int pitch = mE->getPitch() + ((*it)->getTranspose());

		// Rosegarden pitches can be outside MIDI range
		// sometimes even without performance transposition
		if (pitch < 0) pitch = 0;
		if (pitch > 127) pitch = 127;

	        mE->setPitch(pitch);
            }
 
	    // And stick it in the mapped composition
	    // 
	    m_mC.insert(mE);
        }
    }

    // Use this flag to synchonise with the last fetch at the GUI side
    //
    if (!m_sliceFetched) m_sliceFetched = true;

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
    checkSoundDriverStatus();

    // Align Instrument lists and send initial program changes
    //
    preparePlayback();

    // Send audio latencies
    //
    //sendAudioLatencies();

    // make sure we toggle the play button
    // 
    m_transport->PlayButton()->setOn(true);

    // write the start position argument to the outgoing stream
    //
    QDataStream streamOut(data, IO_WriteOnly);

    if (comp.getTempo() == 0)
    {
        comp.setDefaultTempo(120.0);

        SEQMAN_DEBUG << "SequenceManager::play() - setting Tempo to Default value of 120.000\n";
    }
    else
    {
        SEQMAN_DEBUG << "SequenceManager::play() - starting to play\n";
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
        if (m_doc->getComposition().isLooping())
            m_doc->setPointerPosition(m_doc->getComposition().getLoopStart());
        else
            m_doc->setPointerPosition(m_doc->getComposition().getStartMarker());

        return;
    }

    // Disarm recording and drop back to STOPPED
    //
    if (m_transportStatus == RECORDING_ARMED)
    {
        m_transportStatus = STOPPED;
        m_transport->RecordButton()->setOn(false);
        m_transport->MetronomeButton()->
            setOn(m_doc->getComposition().usePlayMetronome());
        return;
    }

    SEQMAN_DEBUG << "SequenceManager::stopping() - preparing to stop\n";

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
    if (m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO)
    {
        m_transport->RecordButton()->setOn(false);
        m_transport->MetronomeButton()->
            setOn(m_doc->getComposition().usePlayMetronome());
    }

    // Now playback
    m_transport->PlayButton()->setOn(false);

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

    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "stop()", data))
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
        SEQMAN_DEBUG << "SequenceManager::stop() - stopped recording MIDI\n";
    }

    if (m_transportStatus == RECORDING_AUDIO)
    {
        m_doc->stopRecordingAudio();
        SEQMAN_DEBUG << "SequenceManager::stop() - stopped recording audio\n";
    }

    // always untoggle the play button at this stage
    //
    m_transport->PlayButton()->setOn(false);
    SEQMAN_DEBUG << "SequenceManager::stop() - stopped playing\n";

    // ok, we're stopped
    //
    m_transportStatus = STOPPED;
    m_sendStop = false;

    resetControllers();
}

// Jump to previous bar
//
void
SequenceManager::rewind()
{
    Rosegarden::Composition &composition = m_doc->getComposition();

    timeT position = composition.getPosition();
    std::pair<timeT, timeT> barRange =
	composition.getBarRangeForTime(position - 1);

    if (m_transportStatus == PLAYING) {

	// if we're playing and we had a rewind request less than 200ms
	// ago and we're some way into the bar but less than half way
	// through it, rewind two barlines instead of one

	clock_t now = clock();
	int elapsed = (now - m_lastRewoundAt) * 1000 / CLOCKS_PER_SEC;

	SEQMAN_DEBUG << "That was " << m_lastRewoundAt << ", this is " << now << ", elapsed is " << elapsed << endl;

	if (elapsed >= 0 && elapsed <= 200) {
	    if (position > barRange.first &&
		position < barRange.second &&
		position <= (barRange.first + (barRange.second -
					       barRange.first) / 2)) {
		barRange = composition.getBarRangeForTime(barRange.first - 1);
	    }
	}

	m_lastRewoundAt = now;
    }
    
    if (barRange.first < composition.getStartMarker()) {
	m_doc->setPointerPosition(composition.getStartMarker());
    } else {
	m_doc->setPointerPosition(barRange.first);
    }
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
SequenceManager::record(bool toggled)
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Studio &studio = m_doc->getStudio();
    KConfig* config = kapp->config();
    config->setGroup("General Options");

    // Rather clumsy additional check for audio subsys when we start
    // recording - once we enforce audio subsystems then this will
    // become redundant.
    //
    if (!(m_soundDriverStatus & AUDIO_OK))
    {
        int rID = comp.getRecordTrack();
        Rosegarden::InstrumentId instrId =
            comp.getTrackById(rID)->getInstrument();
        Rosegarden::Instrument *instr = studio.getInstrumentById(instrId);

        if (!instr || instr->getType() == Rosegarden::Instrument::Audio)
        {
            m_transport->RecordButton()->setOn(false);
            throw(i18n("Audio subsystem is not available - can't record audio"));
        }
    }

    if (toggled)
    {
        if (m_transportStatus == RECORDING_ARMED)
        {
            SEQMAN_DEBUG << "SequenceManager::record - unarming record\n";
            m_transportStatus = STOPPED;

            // Toggle the buttons
            m_transport->MetronomeButton()->setOn(comp.usePlayMetronome());
            m_transport->RecordButton()->setOn(false);

            return;
        }

        if (m_transportStatus == STOPPED)
        {
            SEQMAN_DEBUG << "SequenceManager::record - armed record\n";
            m_transportStatus = RECORDING_ARMED;

            // Toggle the buttons
            m_transport->MetronomeButton()->setOn(comp.useRecordMetronome());
            m_transport->RecordButton()->setOn(true);

            return;
        }

        if (m_transportStatus == RECORDING_MIDI ||
            m_transportStatus == RECORDING_AUDIO)
        {
            SEQMAN_DEBUG << "SequenceManager::record - stop recording and keep playing\n";
            return;
        }

        if (m_transportStatus == PLAYING)
        {
            SEQMAN_DEBUG << "SequenceManager::record - punch in recording\n";
            return;
        }

    }
    else
    {
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
        Rosegarden::InstrumentId inst =
            comp.getTrackById(rID)->getInstrument();

        // If no matching record instrument
        //
        if (studio.getInstrumentById(inst) == 0)
        {
            m_transport->RecordButton()->setDown(false);
            throw(i18n("No Record instrument selected"));
        }


        // may throw an exception
        checkSoundDriverStatus();

        // toggle the Metronome button if it's in use
        m_transport->MetronomeButton()->setOn(comp.useRecordMetronome());

        // If we are looping then jump to start of loop and start recording,
        // if we're not take off the number of count-in bars and start 
        // recording.
        //
        if(comp.isLooping())
            m_doc->setPointerPosition(comp.getLoopStart());
        else
        {
            if (m_transportStatus != RECORDING_ARMED)
            {
                int startBar = comp.getBarNumber(comp.getPosition());
                startBar -= config->readUnsignedNumEntry("countinbars", 2);
                m_doc->setPointerPosition(comp.getBarRange(startBar).first);
            }
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
                SEQMAN_DEBUG << "SequenceManager::record() - starting to record MIDI\n";
                break;

            case Rosegarden::Instrument::Audio:
                recordType = STARTING_TO_RECORD_AUDIO;
                SEQMAN_DEBUG << "SequenceManager::record() - starting to record Audio\n";
                break;

            default:
                SEQMAN_DEBUG << "SequenceManager::record() - unrecognised instrument type\n";
                return;
                break;
        }

        // set the buttons
        m_transport->RecordButton()->setOn(true);
        m_transport->PlayButton()->setOn(true);

        // write the start position argument to the outgoing stream
        //
        QDataStream streamOut(data, IO_WriteOnly);

        if (comp.getTempo() == 0)
        {
            SEQMAN_DEBUG << "SequenceManager::play() - setting Tempo to Default value of 120.000\n";
            comp.setDefaultTempo(120.0);
        }
        else
        {
            SEQMAN_DEBUG << "SequenceManager::record() - starting to record\n";
        }

        // set the tempo in the transport
        //
        m_transport->setTempo(comp.getTempo());

        // The arguments for the Sequencer  - record is similar to playback,
        // we must being playing to record.
        //
        Rosegarden::RealTime startPos =
            comp.getElapsedRealTime(comp.getPosition());
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
                // Stop immediately - turn off buttons in parent
                //
                m_transportStatus = STOPPED;

                if (recordType == STARTING_TO_RECORD_AUDIO)
                {
                    throw(i18n("Couldn't start recording audio.  Ensure your audio record path is valid\nin Document Properties (Edit->Edit Document Properties->Audio"));
                }
                else
                {
                    throw(i18n("Couldn't start recording MIDI"));
                }

            }
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
    processAsynchronousMidi(mC, 0);


    // Send any recorded Events to a Segment for storage and display.
    // We have to send the transport status because this method is
    // called asynchronously from the sequencer and the calls below
    // can create a new recording SegmentItem on the canvas if we
    // don't check that recording is coming to a close (or has already
    // been stopped).
    //
    //  Filter on the way out.
    //
    //
    m_doc->insertRecordedMidi(
            applyFiltering(mC,
                           Rosegarden::MappedEvent::MappedEventType(
                               m_doc->getStudio().getMIDIRecordFilter())),
            m_transportStatus);

}



// Process unexpected MIDI events at the GUI - send them to the Transport
// or to a MIDI mixer for display purposes only.  Useful feature to enable
// the musician to prove to herself quickly that the MIDI input is still
// working.
//
//
void
SequenceManager::processAsynchronousMidi(const MappedComposition &mC,
                                         Rosegarden::AudioManagerDialog
                                             *audioManagerDialog)
{
    if (m_doc == 0) return;

    if (mC.size())
    {
        Rosegarden::MappedComposition::iterator i;
        Rosegarden::Composition &comp = m_doc->getComposition();
        Rosegarden::Track *track =
                  comp.getTrackById(comp.getSelectedTrack());
        Rosegarden::InstrumentId id = track->getInstrument();

        Rosegarden::MappedComposition tempMC =
                applyFiltering(mC,
                               Rosegarden::MappedEvent::MappedEventType(
                                   m_doc->getStudio().getMIDIThruFilter()));

        Rosegarden::MappedComposition retMC;

        // send all events to the MIDI in label
        //
        for (i = tempMC.begin(); i != tempMC.end(); ++i )
        {
            m_transport->setMidiInLabel(*i);

            // Skip all audio events
            //
            if ((*i)->getType() >= Rosegarden::MappedEvent::Audio)
            {
                if ((*i)->getType() == Rosegarden::MappedEvent::AudioStopped)
                {
                    /*
                    SEQMAN_DEBUG << "AUDIO FILE ID = "
                                 << int((*i)->getData1())
                                 << " - FILE STOPPED - " 
                                 << "INSTRUMENT = "
                                 << (*i)->getInstrument()
                                 << endl;
                    */

                    if (audioManagerDialog && (*i)->getInstrument() == 
                            m_doc->getStudio().getAudioPreviewInstrument())
                    {
                        audioManagerDialog->
                            closePlayingDialog(
                                Rosegarden::AudioFileId((*i)->getData1()));
                    }
                }

                if ((*i)->getType() == Rosegarden::MappedEvent::AudioLevel)
                    sendAudioLevel(*i);

                if ((*i)->getType() == 
                        Rosegarden::MappedEvent::AudioGeneratePreview)
                {
                    m_doc->finalizeAudioFile(
                            Rosegarden::AudioFileId((*i)->getData1()));
                }

                if ((*i)->getType() ==
                        Rosegarden::MappedEvent::SystemUpdateInstruments)
                {
                    // resync Devices and Instruments
                    //
                    m_doc->syncDevices();
                }

                continue;

            } else {

		// if we aren't recording, consider invoking any
		// step-by-step clients

		if (m_transportStatus == STOPPED ||
		    m_transportStatus == RECORDING_ARMED) {

		    if ((*i)->getType() == Rosegarden::MappedEvent::MidiNote) {
			if ((*i)->getVelocity() == 0) {
			    emit insertableNoteOffReceived((*i)->getPitch());
			} else {
			    emit insertableNoteOnReceived((*i)->getPitch());
			}
		    }
		}
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
        showVisuals(retMC);

        // Filter
        //
        Rosegarden::StudioControl::sendMappedComposition(retMC);

#endif 
    }
}


void
SequenceManager::rewindToBeginning()
{
    SEQMAN_DEBUG << "SequenceManager::rewindToBeginning()\n";
    m_doc->setPointerPosition(m_doc->getComposition().getStartMarker());
}


void
SequenceManager::fastForwardToEnd()
{
    SEQMAN_DEBUG << "SequenceManager::fastForwardToEnd()\n";

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
SequenceManager::checkSoundDriverStatus()
{
    QByteArray data;
    QCString replyType;
    QByteArray replyData;

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "getSoundDriverStatus()",
                                  data, replyType, replyData))
    {
        // failed - pop up and disable sequencer options
        throw(i18n("Failed to contact Rosegarden sequencer"));
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        unsigned int result;
        streamIn >> result;
        m_soundDriverStatus = result;

        if (m_soundDriverStatus == NO_DRIVER)
            throw(i18n("MIDI and Audio subsystems have failed to initialise"));

        if (!(m_soundDriverStatus & MIDI_OK))
            throw(i18n("MIDI subsystem has failed to initialise"));

        /*
        if (!(m_soundDriverStatus & AUDIO_OK))
            throw(i18n("Audio subsystem has failed to initialise"));
        */
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

    Rosegarden::RealTime mDuration = config.get<RealTimeT>("metronomeduration");
    Rosegarden::MidiByte mBarVelocity =
        Rosegarden::MidiByte(config.get<Int>("metronomebarvelocity"));
    Rosegarden::MidiByte mBeatVelocity =
        Rosegarden::MidiByte(config.get<Int>("metronomebarvelocity"));

    if (mDuration == Rosegarden::RealTime(0, 0))
        mDuration = Rosegarden::RealTime(0, 10000);

    if (mBarVelocity == 0) mBarVelocity = 120;
    if (mBeatVelocity == 0) mBeatVelocity = 80;

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
                                mBarVelocity,
                                comp.getElapsedRealTime(barStart.first),
                                mDuration);
        m_mC.insert(me);
    }
    else if (barEnd.first >= sliceStart && barEnd.first <= sliceEnd)
    {
        MappedEvent *me =
                new MappedEvent(metronome->instrument,
                                metronome->pitch,
                                mBarVelocity,
                                comp.getElapsedRealTime(barEnd.first),
                                mDuration);
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
            MappedEvent *me = new MappedEvent(metronome->instrument,
                                              metronome->pitch,
                                              mBeatVelocity,
                                              comp.getElapsedRealTime(i),
                                              mDuration);
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
        Rosegarden::StudioControl::sendMappedInstrument(MappedInstrument(*it));

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

        }
        else if ((*it)->getType() == Instrument::Audio)
        {
            Rosegarden::StudioControl::setStudioObjectProperty(
                    (*it)->getId(), "value", (*it)->getVelocity());
        }
        else
        {
            std::cerr << "SequenceManager::preparePlayback - "
                      << "unrecognised instrument type" << std::endl;
        }


    }

    // Send the MappedComposition if it's got anything in it
    showVisuals(mC);
    Rosegarden::StudioControl::sendMappedComposition(mC);

    // Set up the audio playback latency
    //
    KConfig* config = kapp->config();
    config->setGroup("Latency Options");

    int jackSec = config->readLongNumEntry("jackplaybacklatencysec", 0);
    int jackUSec = config->readLongNumEntry("jackplaybacklatencyusec", 0);
    m_playbackAudioLatency = Rosegarden::RealTime(jackSec, jackUSec);

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
    QList<RosegardenGUIView>& viewList = m_doc->getViewList();

    for (v = viewList.first(); v != 0; v = viewList.next())
    {
        v->showVisuals(mE);
    }

}

void
SequenceManager::resetControllers()
{
    SEQMAN_DEBUG << "SequenceManager::resetControllers - resetting\n";
    Rosegarden::MappedComposition mC;

    // Should do all Midi Instrument - not just guess like this is doing
    // currently.

    for (unsigned int i = 0; i < 16; i++)
    {
        Rosegarden::MappedEvent *mE =
            new Rosegarden::MappedEvent(Rosegarden::MidiInstrumentBase + i,
                                        Rosegarden::MappedEvent::MidiController,
                                        MIDI_CONTROLLER_RESET,
                                        0);

        mC.insert(mE);
    }
    showVisuals(mC);
    Rosegarden::StudioControl::sendMappedComposition(mC);
}


void
SequenceManager::getSequencerPlugins(Rosegarden::AudioPluginManager *aPM)
{
    Rosegarden::MappedObjectId id =
        Rosegarden::StudioControl::getStudioObjectByType(
                Rosegarden::MappedObject::AudioPluginManager);

    SEQMAN_DEBUG << "getSequencerPlugins - getting plugin information" << endl;
    
    Rosegarden::MappedObjectPropertyList seqPlugins
        = Rosegarden::StudioControl::getStudioObjectProperty(
                id, Rosegarden::MappedAudioPluginManager::Plugins);

    SEQMAN_DEBUG << "getSequencerPlugins - got "
                 << seqPlugins.size() << " items" << endl;

    /*
    Rosegarden::MappedObjectPropertyList seqPluginIds
        = getSequencerPropertyList(id,
                               Rosegarden::MappedAudioPluginManager::PluginIds);
                               */

    //Rosegarden::MappedObjectPropertyList::iterator it;

    unsigned int i = 0;

    while (i < seqPlugins.size())
    {
        Rosegarden::MappedObjectId id = seqPlugins[i++].toInt();
        QString name = seqPlugins[i++];
        unsigned long uniqueId = seqPlugins[i++].toLong();
        QString label = seqPlugins[i++];
        QString author = seqPlugins[i++];
        QString copyright = seqPlugins[i++];
        unsigned int portCount = seqPlugins[i++].toInt();

        AudioPlugin *aP = aPM->addPlugin(id,
                                         name,
                                         uniqueId,
                                         label,
                                         author,
                                         copyright);

        // SEQMAN_DEBUG << "PLUGIN = \"" << name << "\"" << endl;

        for (unsigned int j = 0; j < portCount; j++)
        {
            id = seqPlugins[i++].toInt();
            name = seqPlugins[i++];
            Rosegarden::PluginPort::PortType type =
                Rosegarden::PluginPort::PortType(seqPlugins[i++].toInt());
            Rosegarden::PluginPort::PortRange range =
                Rosegarden::PluginPort::PortRange(seqPlugins[i++].toInt());
            Rosegarden::PortData lowerBound = seqPlugins[i++].toFloat();
            Rosegarden::PortData upperBound = seqPlugins[i++].toFloat();
	    Rosegarden::PortData defaultValue = seqPlugins[i++].toFloat();

	    // SEQMAN_DEBUG << "DEFAULT =  " << defaultValue << endl;
            // SEQMAN_DEBUG << "ADDED PORT = \"" << name << "\"" << endl;
            aP->addPort(id,
                        name,
                        type,
                        range,
                        lowerBound,
                        upperBound,
			defaultValue);

        }

        // SEQMAN_DEBUG << " = " << seqPlugins[i] << endl;

        /*
        Rosegarden::MappedObjectPropertyList author =
            getSequencerPropertyList(seqPluginIds[i].toInt(), "author");

        if (author.size() == 1)
            SEQMAN_DEBUG << "PLUGIN AUTHOR = \"" << author[0] << "\"" << std::endl;
            */
    }
}

// Clear down all temporary (non read-only) objects and then
// add the basic audio faders only (one per instrument).
//
void
SequenceManager::reinitialiseSequencerStudio()
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    if (!kapp->dcopClient()->
            send(ROSEGARDEN_SEQUENCER_APP_NAME,
                 ROSEGARDEN_SEQUENCER_IFACE_NAME,
                 "reinitialiseStudio()",
                 data))
    {
        SEQMAN_DEBUG << "failed to reinitialise studio" << endl;
        return;
    }

    SEQMAN_DEBUG << "reinitialised studio\n";

    // Now set up the audio faders for each audio instrument
    //
    Studio &studio = m_doc->getStudio();

    Rosegarden::InstrumentList list = studio.getAllInstruments();
    Rosegarden::InstrumentList::iterator it = list.begin();
    int count = 0;

    for (; it != list.end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Instrument::Audio)
        {
            // Create a sequencer-studio fader and assign the
            // mapped object id.
            //
            Rosegarden::MappedObjectId mappedId =
                Rosegarden::StudioControl::createStudioObject(
                        Rosegarden::MappedObject::AudioFader);

            // Set the instrument id against this object
            //
            Rosegarden::StudioControl::setStudioObjectProperty(mappedId,
                Rosegarden::MappedObject::Instrument,
                Rosegarden::MappedObjectValue((*it)->getId()));

            // Set the object id against the instrument
            //
            (*it)->setMappedId(mappedId);

            count++;
        }
    }

    SEQMAN_DEBUG << "initialised " << count << " audio faders" << endl;

    // Send the MIDI recording device to the sequencer
    //
    KConfig* config = kapp->config();
    config->setGroup("Sequencer Options");

    QString recordDeviceStr = config->readEntry("midirecorddevice");

    if (recordDeviceStr)
    {
        int recordDevice = recordDeviceStr.toInt();

        if (recordDevice >= 0)
        {
            Rosegarden::MappedEvent *mE =
                new Rosegarden::MappedEvent(
                    Rosegarden::MidiInstrumentBase, // InstrumentId
                    Rosegarden::MappedEvent::SystemRecordDevice,
                    Rosegarden::MidiByte(recordDevice));

            Rosegarden::StudioControl::sendMappedEvent(mE);
            SEQMAN_DEBUG << "set MIDI record device to "
                         << recordDevice << endl;
        }
    }

    // Setup JACK audio inputs
    //
    int jackAudioInputs = config->readNumEntry("jackaudioinputs", 2);

    Rosegarden::MappedEvent *mE =
        new Rosegarden::MappedEvent(
            Rosegarden::MidiInstrumentBase, // InstrumentId
            Rosegarden::MappedEvent::SystemAudioInputs,
            Rosegarden::MidiByte(jackAudioInputs));

    Rosegarden::StudioControl::sendMappedEvent(mE);

}

// Clear down all playing notes and reset controllers
//
void
SequenceManager::panic()
{
    SEQMAN_DEBUG << "panic button\n";

    Studio &studio = m_doc->getStudio();

    InstrumentList list = studio.getPresentationInstruments();
    InstrumentList::iterator it;

    Rosegarden::MappedComposition mC;
    Rosegarden::MappedEvent *mE;

    int maxDevices = 0, device = 0;
    for (it = list.begin(); it != list.end(); it++)
        if ((*it)->getType() == Instrument::Midi)
            maxDevices++;

    emit setProgress(10);
    for (it = list.begin(); it != list.end(); it++)
    {
        if ((*it)->getType() == Instrument::Midi)
        {
            emit setProgress(int(70.0 * float(device)/float(maxDevices)));
            for (unsigned int i = 0; i < 128; i++)
            {
                mE = new MappedEvent((*it)->getId(),
                                     Rosegarden::MappedEvent::MidiNote,
                                     i,
                                     0,
                                     RealTime(0, 0),
                                     RealTime(0, 0),
                                     RealTime(0, 0));
                mC.insert(mE);
            }

            device++;
        }
    }

    Rosegarden::StudioControl::sendMappedComposition(mC);
    emit setProgress(90);

    resetControllers();
}

// In this case we only route MIDI events to the transport ticker
//
void
SequenceManager::showVisuals(const Rosegarden::MappedComposition &mC)
{
    RosegardenGUIView *v;
    QList<RosegardenGUIView>& viewList = m_doc->getViewList();

    MappedComposition::iterator it = mC.begin();
    for (; it != mC.end(); it++)
    {
        for (v = viewList.first(); v != 0; v = viewList.next())
        {
            //v->showVisuals(*it);
            m_transport->setMidiOutLabel(*it);
        }
    }
}


// Filter a MappedComposition by Type.
//
Rosegarden::MappedComposition
SequenceManager::applyFiltering(const Rosegarden::MappedComposition &mC,
                                Rosegarden::MappedEvent::MappedEventType filter)
{
    Rosegarden::MappedComposition retMc;
    Rosegarden::MappedComposition::iterator it = mC.begin();

    for (; it != mC.end(); it++)
    {
        if (!((*it)->getType() & filter))
            retMc.insert(new MappedEvent(*it));
    }

    return retMc;
}

void
SequenceManager::setSequencerSliceSize(const RealTime &time)
{
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO )
    {
        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        KConfig* config = kapp->config();
        config->setGroup("Latency Options");

        if (time == RealTime(0, 0)) // reset to default values
        {
            streamOut << config->readLongNumEntry("readaheadsec", 0);
            streamOut << config->readLongNumEntry("readaheadusec", 40000);
        }
        else
        {
            streamOut << time.sec;
            streamOut << time.usec;
        }

        if (!kapp->dcopClient()->
                send(ROSEGARDEN_SEQUENCER_APP_NAME,
                     ROSEGARDEN_SEQUENCER_IFACE_NAME,
                     "setSliceSize(long int, long int)",
                     data))
        {
            SEQMAN_DEBUG << "couldn't set sequencer slice" << endl;
            return;
        }

        // Ok, set this token and wait for the sequencer to fetch a
        // new slice.
        //
        m_sliceFetched = false;

        int msecs = (time.sec * 1000) + (time.usec / 1000);
        SEQMAN_DEBUG << "set sequencer slice = " << msecs
                     << "ms" << endl;

        // Spin until the sequencer has got the next slice - we only
        // do this if we're at the top loop level (otherwise DCOP
        // events don't get processed)
        //
        if (kapp->loopLevel() > 1)
            SEQMAN_DEBUG << "can't wait for slice fetch" << endl;
        else
            while (m_sliceFetched == false) kapp->processEvents();

    }
}

// Set a temporary slice size.  Wait until this slice is fetched
// before continuing.  The sequencer will then revert slice size 
// to original value for subsequent fetches.
//
void
SequenceManager::setTemporarySequencerSliceSize(const RealTime &time)
{
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO )
    {
        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        KConfig* config = kapp->config();
        config->setGroup("Latency Options");

        if (time == RealTime(0, 0)) // reset to default values
        {
            streamOut << config->readLongNumEntry("readaheadsec", 0);
            streamOut << config->readLongNumEntry("readaheadusec", 40000);
        }
        else
        {
            streamOut << time.sec;
            streamOut << time.usec;
        }


        if (!kapp->dcopClient()->
                send(ROSEGARDEN_SEQUENCER_APP_NAME,
                     ROSEGARDEN_SEQUENCER_IFACE_NAME,
                     "setTemporarySliceSize(long int, long int)",
                     data))
        {
            SEQMAN_DEBUG << "couldn't set temporary sequencer slice" << endl;
            return;
        }

        // Ok, set this token and wait for the sequencer to fetch a
        // new slice.
        //
        m_sliceFetched = false;

        int msecs = (time.sec * 1000) + (time.usec / 1000);
        SEQMAN_DEBUG << "set temporary sequencer slice = " << msecs
                     << "ms" << endl;

        // Spin until the sequencer has got the next slice - only do
        // this if we're being called from the top loop level.
        //
        if (kapp->loopLevel() > 1)
            SEQMAN_DEBUG << "can't wait for slice fetch" << endl;
        else
            while (m_sliceFetched == false) kapp->processEvents();
    }
}

void
SequenceManager::sendTransportControlStatuses()
{
    KConfig* config = kapp->config();
    config->setGroup("Sequencer Options");

    // Get the config values
    //
    bool jackTransport = config->readBoolEntry("jacktransport", false);
    bool jackMaster = config->readBoolEntry("jackmaster", false);

    bool mmcTransport = config->readBoolEntry("mmctransport", false);
    bool mmcMaster = config->readBoolEntry("mmcmaster", false);

    bool midiClock = config->readBoolEntry("midiclock", false);

    // Send JACK transport
    //
    int jackValue = 0;
    if (jackTransport && jackMaster)
        jackValue = 2;
    else 
    {
        if (jackTransport)
            jackValue = 1;
        else
            jackValue = 0;
    }

    Rosegarden::MappedEvent *mE =
        new Rosegarden::MappedEvent(
            Rosegarden::MidiInstrumentBase, // InstrumentId
            Rosegarden::MappedEvent::SystemJackTransport,
            Rosegarden::MidiByte(jackValue));
    Rosegarden::StudioControl::sendMappedEvent(mE);


    // Send MMC transport
    //
    int mmcValue = 0;
    if (mmcTransport && mmcMaster)
        mmcValue = 2;
    else
    {
        if (mmcTransport)
            mmcValue = 1;
        else
            mmcValue = 0;
    }

    mE = new Rosegarden::MappedEvent(
                Rosegarden::MidiInstrumentBase, // InstrumentId
                Rosegarden::MappedEvent::SystemMMCTransport,
                Rosegarden::MidiByte(mmcValue));

    Rosegarden::StudioControl::sendMappedEvent(mE);


    // Send MIDI Clock
    //
    mE = new Rosegarden::MappedEvent(
                Rosegarden::MidiInstrumentBase, // InstrumentId
                Rosegarden::MappedEvent::SystemMIDIClock,
                Rosegarden::MidiByte(midiClock));

    Rosegarden::StudioControl::sendMappedEvent(mE);



}

}



