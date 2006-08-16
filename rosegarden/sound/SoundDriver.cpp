// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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
    m_recordStatus(RECORD_OFF),
    m_midiRunningId(MidiInstrumentBase),
    m_audioRunningId(AudioInstrumentBase),
//    m_audioQueueScavenger(4, 50),
    m_audioQueue(0),
    m_lowLatencyMode(true),
    m_audioRecFileFormat(RIFFAudioFile::FLOAT),
    m_studio(studio),
    m_sequencerDataBlock(0),
    m_externalTransport(0),
    m_mmcStatus(TRANSPORT_OFF),
    m_mtcStatus(TRANSPORT_OFF),
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
    AudioPlayQueue *newQueue = new AudioPlayQueue();

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

	    PlayableAudioFile *paf = 0;

	    try {
		paf = new PlayableAudioFile(i->getInstrument(),
					    audioFile,
					    i->getEventTime(),
					    i->getAudioStartMarker(),
					    i->getDuration(),
					    bufferFrames,
					    getSmallFileSize() * 1024,
					    channels,
					    getSampleRate());
	    } catch (...) {
		continue;
	    }

	    paf->setRuntimeSegmentId(i->getRuntimeSegmentId());

            if (i->isAutoFading())
            {
                paf->setAutoFade(true);
                paf->setFadeInTime(i->getFadeInTime());
                paf->setFadeOutTime(i->getFadeInTime());

//#define DEBUG_AUTOFADING
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

	    newQueue->addScheduled(paf);
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

    std::cout << "SoundDriver::initialiseAudioQueue -- new queue has "
	      << newQueue->size() << " files" 
	      << std::endl;

    if (newQueue->empty()) {
	if (m_audioQueue->empty()) {
	    delete newQueue;
	    return;
	}
    }

    AudioPlayQueue *oldQueue = m_audioQueue;
    m_audioQueue = newQueue;
    if (oldQueue) m_audioQueueScavenger.claim(oldQueue);
}

void
SoundDriver::clearAudioQueue()
{
    std::cout << "SoundDriver::clearAudioQueue" << std::endl;

    if (m_audioQueue->empty()) return;

    AudioPlayQueue *newQueue = new AudioPlayQueue();
    AudioPlayQueue *oldQueue = m_audioQueue;
    m_audioQueue = newQueue;
    if (oldQueue) m_audioQueueScavenger.claim(oldQueue);
}
void
SoundDriver::cancelAudioFile(MappedEvent *mE)
{
    std::cout << "SoundDriver::cancelAudioFile" << std::endl;

    if (!m_audioQueue) return;

    // For now we only permit cancelling unscheduled files.

    const AudioPlayQueue::FileList &files = m_audioQueue->getAllUnscheduledFiles();
    for (AudioPlayQueue::FileList::const_iterator fi = files.begin();
	 fi != files.end(); ++fi) {
	PlayableAudioFile *file = *fi;
	if (mE->getRuntimeSegmentId() == -1) {

	    // ERROR? The comparison between file->getAudioFile()->getId() of type unsigned int
	    //        and mE->getAudioID() of type int.
	    if (file->getInstrument() == mE->getInstrument() &&
		int(file->getAudioFile()->getId() == mE->getAudioID())) {
		file->cancel();
	    }
	} else {
	    if (file->getRuntimeSegmentId() == mE->getRuntimeSegmentId() &&
		file->getStartTime() == mE->getEventTime()) {
		file->cancel();
	    }
	}
    }
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

//    std::cout << "Sequencer::addAudioFile() = \"" << fileName << "\"" << std::endl;

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
//    std::cout << "SoundDriver::clearAudioFiles() - clearing down audio files"
//              << std::endl;

    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
        delete(*it);

    m_audioFiles.erase(m_audioFiles.begin(), m_audioFiles.end());
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
	

}

