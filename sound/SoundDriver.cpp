// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include <stdlib.h>

#include "SoundDriver.h"
#include "WAVAudioFile.h"
#include "MappedStudio.h"
#include "AudioPlayQueue.h"

#include <unistd.h>
#include <sys/time.h>
#include <pthread.h> // for mutex

//#define DEBUG_PLAYABLE_CONSTRUCTION 1
//#define DEBUG_PLAYABLE 1

namespace Rosegarden
{

// ---------- SoundDriver -----------
//


SoundDriver::SoundDriver(MappedStudio *studio, const std::string &name):
    m_name(name),
    m_driverStatus(NO_DRIVER),
    m_playStartPosition(0, 0),
    m_startPlayback(false),
    m_playing(false),
    m_midiRecordDevice(0),
    m_recordStatus(ASYNCHRONOUS_MIDI),
    m_midiRunningId(MidiInstrumentBase),
    m_audioRunningId(AudioInstrumentBase),
    m_audioQueue(0),
    m_audioMonitoringInstrument(AudioInstrumentBase),
    m_studio(studio),
    m_sequencerDataBlock(0),
    m_externalTransport(0),
    m_mmcEnabled(false),
    m_mmcMaster(false),
    m_mmcId(0),           // default MMC id of 0
    m_midiClockEnabled(false),
    m_midiClockInterval(0, 0),
    m_midiClockSendTime(RealTime::zeroTime),
    m_midiSongPositionPointer(0)
{
    m_audioQueue = new AudioPlayQueue();
}


SoundDriver::~SoundDriver()
{
    std::cout << "SoundDriver::~SoundDriver (exiting)" << std::endl;
    delete m_audioQueue;
}

MappedInstrument*
SoundDriver::getMappedInstrument(InstrumentId id)
{
    std::vector<MappedInstrument*>::const_iterator it;

    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getId() == id)
            return (*it);
    }

    return 0;
}

void
SoundDriver::initialiseAudioQueue(const std::vector<MappedEvent> &events)
{
    m_audioQueue->clear();

    for (std::vector<MappedEvent>::const_iterator i = events.begin();
	 i != events.end(); ++i) {

	// Check for existence of file - if the sequencer has died
	// and been restarted then we're not always loaded up with
	// the audio file references we should have.  In the future
	// we could make this just get the gui to reload our files
	// when (or before) this fails.
	//
	AudioFile *audioFile = getAudioFile(i->getAudioID());

	if (audioFile)
	{ 
	    MappedAudioFader *fader =
		dynamic_cast<MappedAudioFader*>
		(getMappedStudio()->getAudioFader(i->getInstrument()));

	    if (!fader) {
		std::cerr << "WARNING: SoundDriver::initialiseAudioQueue: no fader for audio instrument " << i->getInstrument() << std::endl;
		continue;
	    }

	    unsigned int channels = fader->getPropertyList(
		MappedAudioFader::Channels)[0].toInt();

//#define DEBUG_PLAYING_AUDIO
#ifdef DEBUG_PLAYING_AUDIO
	    std::cout << "Creating playable audio file: id " << audioFile->getId() << ", event time " << i->getEventTime() << ", time now " << getSequencerTime() << ", start marker " << i->getAudioStartMarker() << ", duration " << i->getDuration() << ", instrument " << i->getInstrument() << " channels " << channels <<  std::endl;
#endif

	    RealTime bufferLength = getAudioReadBufferLength();
	    int bufferFrames = RealTime::realTime2Frame
		(bufferLength, getSampleRate());

	    //!!! previously in AlsaDriver we had a test here to ensure the
	    // buffer length was at least the JACK buffer size -- otherwise
	    // we'd get into terrible trouble -- but buffer lengths are
	    // going to go from here anyway?

	    PlayableAudioFile *paf =
		new PlayableAudioFile(i->getInstrument(),
				      audioFile,
				      i->getEventTime(),
				      i->getAudioStartMarker(),
				      i->getDuration(),
				      bufferFrames,
				      getSmallFileSize() * 1024,
				      channels,
				      getSampleRate());

	    paf->setRuntimeSegmentId(i->getRuntimeSegmentId());

            if (i->isAutoFading())
            {
                paf->setAutoFade(true);
                paf->setFadeInTime(i->getFadeInTime());
                paf->setFadeOutTime(i->getFadeInTime());

#define DEBUG_AUTOFADING
#ifdef DEBUG_AUTOFADING
                 std::cout << "SoundDriver::initialiseAudioQueue - "
                           << "PlayableAudioFile is AUTOFADING - "
                           << "in = " << i->getFadeInTime()
                           << ", out = " << i->getFadeOutTime()
                           << std::endl;
#endif
            }
#ifdef DEBUG_AUTOFADING
            else
            {
                std::cout << "PlayableAudioFile has no AUTOFADE"
                          << std::endl;
            }
#endif


	    m_audioQueue->addScheduled(paf);
	}
	else
	{
	    std::cerr << "SoundDriver::initialiseAudioQueue - "
		      << "can't find audio file reference" 
		      << std::endl;

	    std::cerr << "SoundDriver::initialiseAudioQueue - "
		      << "try reloading the current Rosegarden file"
		      << std::endl;
	}
    }
}

void
SoundDriver::clearAudioQueue()
{
    m_audioQueue->clear();
}

const AudioPlayQueue *
SoundDriver::getAudioQueue() const
{
    return m_audioQueue;
}


void
SoundDriver::setMappedInstrument(MappedInstrument *mI)
{
    std::vector<MappedInstrument*>::iterator it;

    // If we match then change existing entry
    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getId() == mI->getId())
        {
            (*it)->setChannel(mI->getChannel());
            (*it)->setType(mI->getType());
            delete mI;
            return;
        }
    }

    // else create a new one
    m_instruments.push_back(mI);

    std::cout << "SoundDriver: setMappedInstrument() : "
              << "type = " << mI->getType() << " : "
              << "channel = " << (int)(mI->getChannel()) << " : "
              << "id = " << mI->getId() << std::endl;

}

unsigned int
SoundDriver::getDevices()
{
    return m_devices.size();
}

MappedDevice
SoundDriver::getMappedDevice(DeviceId id)
{
    MappedDevice retDevice;
    std::vector<MappedInstrument*>::iterator it;

    std::vector<MappedDevice*>::iterator dIt = m_devices.begin();
    for (; dIt != m_devices.end(); dIt++)
    {
        if ((*dIt)->getId() == id) retDevice = **dIt;
    }

    // If we match then change existing entry
    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getDevice() == id)
            retDevice.push_back(*it);
    }

    std::cout << "SoundDriver::getMappedDevice(" << id << ") - "
              << "name = \"" << retDevice.getName() 
              << "\" type = " << retDevice.getType()
              << " direction = " << retDevice.getDirection()
              << " connection = \"" << retDevice.getConnection() << "\"" 
              << " recording = " << retDevice.isRecording()
              << std::endl;

    return retDevice;
}



bool
SoundDriver::addAudioFile(const std::string &fileName, unsigned int id)
{
    AudioFile *ins = new WAVAudioFile(id, fileName, fileName);
    try
    {
        ins->open();
    }
    catch(std::string s)
    {
        return false;
    }

    m_audioFiles.push_back(ins);

    std::cout << "Sequencer::addAudioFile() = \"" << fileName << "\"" << std::endl;

    return true;
}

bool
SoundDriver::removeAudioFile(unsigned int id)
{
    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
    {
        if ((*it)->getId() == id)
        {
            std::cout << "Sequencer::removeAudioFile() = \"" <<
                          (*it)->getFilename() << "\"" << std::endl;

            delete (*it);
            m_audioFiles.erase(it);
            return true;
        }
    }

    return false;
}

AudioFile*
SoundDriver::getAudioFile(unsigned int id)
{
    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
    {
        if ((*it)->getId() == id)
            return *it;
    }

    return 0;
}

void
SoundDriver::clearAudioFiles()
{
    std::cout << "SoundDriver::clearAudioFiles() - clearing down audio files"
              << std::endl;

    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
        delete(*it);

    m_audioFiles.erase(m_audioFiles.begin(), m_audioFiles.end());
}


// Close down any playing audio files - we can manipulate the
// live play stack as we're only changing state.  Switch on
// segment id first and then drop to instrument/audio file.
//
void
SoundDriver::cancelAudioFile(MappedEvent *mE)
{
    std::cout << "SoundDriver::cancelAudioFile" << std::endl;


/*!!!

    std::list<PlayableAudioFile *>::iterator it;

    pthread_mutex_lock(&_audioQueueLock);

    for (it = m_audioPlayQueue.begin();
         it != m_audioPlayQueue.end();
         it++)
    {
        if (mE->getRuntimeSegmentId() == -1)
        {
            if ((*it)->getInstrument() == mE->getInstrument() &&
		(int)(*it)->getAudioFile()->getId() == mE->getAudioID())
                (*it)->setStatus(PlayableAudioFile::DEFUNCT);
        }
        else
        {
            if ((*it)->getRuntimeSegmentId() == mE->getRuntimeSegmentId() &&
		(*it)->getStartTime() == mE->getEventTime()) {
#ifdef DEBUG_PLAYABLE
		std::cerr << "audio file match: ids "
			  << (*it)->getRuntimeSegmentId() << " vs "
			  << mE->getRuntimeSegmentId() << ", times "
			  << (*it)->getStartTime() << " vs "
			  << mE->getEventTime() << ", status "
			  << (*it)->getStatus() << std::endl;
#endif
                (*it)->setStatus(PlayableAudioFile::DEFUNCT);
	    } else {
#ifdef DEBUG_PLAYABLE
		std::cerr << "audio file mismatch: ids "
			  << (*it)->getRuntimeSegmentId() << " vs "
			  << mE->getRuntimeSegmentId() << ", times "
			  << (*it)->getStartTime() << " vs "
			  << mE->getEventTime() << std::endl;
#endif
	    }
        }
    }

    pthread_mutex_unlock(&_audioQueueLock);
*/
}

void
SoundDriver::sleep(const RealTime &rt)
{
    // The usleep man page says it's deprecated and we should use
    // nanosleep.  And that's what we did.  But it seems quite a few
    // people don't have nanosleep, so we're reverting to usleep.

    unsigned long usec = rt.sec * 1000000 + rt.usec();
    usleep(usec);
}

void
SoundDriver::rationalisePlayingAudio(const std::vector<MappedEvent> &segmentAudio,
				     const RealTime &playtime)
{
/*!!!
    pthread_mutex_lock(&_audioQueueLock);

    // The mixer already ensures that anything on the queue gets
    // played and anything not on the queue doesn't.  We just need to
    // find out what's on the queue.  Furthermore, we should only
    // enqueue new files from this method if they are actually
    // supposed to have started already, because this method is only
    // intended to trap cases like unmuting in the middle of a file;
    // any actual expected situation will be handled by the normal
    // audio event route.

    std::list<PlayableAudioFile *> &driverAudio = m_audioPlayQueue;
    MappedComposition mC;

    // Check for playing audio that shouldn't be
    for (std::list<PlayableAudioFile *>::const_iterator i = driverAudio.begin();
	 i != driverAudio.end(); ++i) {

	if ((*i)->getStatus() == PlayableAudioFile::DEFUNCT) continue;
	if ((*i)->getDuration() <= getAudioReadBufferLength()) continue;

	bool found = false;

	for (std::vector<MappedEvent>::const_iterator si = segmentAudio.begin();
	     si != segmentAudio.end(); ++si) {
	    if (si->getRuntimeSegmentId() == (*i)->getRuntimeSegmentId() &&
		si->getInstrument() == (*i)->getInstrument() &&
		si->getEventTime() == (*i)->getStartTime()) {
		found = true;
		break;
	    }
	}

	if (!found) {

            // We've found an audio segment that shouldn't be playing - stop it
            // through the normal channels.  Send a cancel event to the driver.
            //
            MappedEvent mE;
            mE.setType(MappedEvent::AudioCancel);
            mE.setRuntimeSegmentId((*i)->getRuntimeSegmentId());
	    mE.setEventTime((*i)->getStartTime());

            std::cout << "SoundDriver::rationalisePlayingAudio - " 
                      << "stopping audio segment = " << (*i)->getRuntimeSegmentId() 
                      << std::endl;

	    mC.insert(new MappedEvent(mE));
	}
    }

    // Check for audio that should be playing but isn't

    for (std::vector<MappedEvent>::const_iterator si = segmentAudio.begin();
	 si != segmentAudio.end(); ++si) {

	if (playtime < si->getEventTime()) continue;
	if (si->getDuration() <= getAudioReadBufferLength()) continue;

	bool found = false;

	for (std::list<PlayableAudioFile *>::const_iterator i = driverAudio.begin();
	     i != driverAudio.end(); ++i) {

	    if (si->getRuntimeSegmentId() == (*i)->getRuntimeSegmentId() &&
		si->getInstrument() == (*i)->getInstrument() &&
		si->getEventTime() == (*i)->getStartTime()) {
		found = true;
		break;
	    }
	}

	if (!found) {

            // There's an audio event that should be playing but isn't,
            // so start it
            //
//            MappedEvent mE(m_metaIterator->
//                    getAudioSegment(si->getRuntimeSegmentId()));

            std::cout << "SoundDriver::rationalisePlayingAudio - " 
                      << "starting audio segment = " << mE.getRuntimeSegmentId()
                      << std::endl;

	    mC.insert(new MappedEvent(*si));
	}
    }

    pthread_mutex_unlock(&_audioQueueLock);

    if (!mC.empty()) {
	processEventsOut(mC, false);
    }
*/
}
	

}

